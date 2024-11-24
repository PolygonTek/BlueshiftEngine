// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Platform/PlatformSystem.h"
#include "Platform/cpuid.h"
#include "SIMD/SIMD.h"
#include "Core/ScopeLock.h"
#include "Core/Task.h"

BE_NAMESPACE_BEGIN

unsigned int TaskThreadProc(void *param);

TaskManager::TaskManager(int maxTasks) {
    taskRingBuffer.SetCount(maxTasks);

    taskMutex = PlatformMutex::Create();
    taskCondition = PlatformCondition::Create();
    finishCondition = PlatformCondition::Create();
}

TaskManager::~TaskManager() {
    Stop();

    PlatformCondition::Destroy(taskCondition);
    PlatformCondition::Destroy(finishCondition);
    PlatformMutex::Destroy(taskMutex);
}

void TaskManager::Start(int numThreads) {
    // Return if the threads already started.
    if (!threads.IsEmpty()) {
        return;
    }

    if (numThreads <= 0) {
        // Get thread count as number of logical processors.
        numThreads = PlatformSystem::NumCPUCoresIncludingHyperthreads();
    }

    tailTaskIndex = 0;
    headTaskIndex = 0;
    numActiveTasks = 0;

    threads.Reserve(numThreads);

    for (int i = 0; i < numThreads; i++) {
        threads.Append(PlatformThread::Start(TaskThreadProc, (void *)this, 0));
    }
}

void TaskManager::Stop() {
    // Set the stopping and wake all the task threads.
    {
        ScopeLock scopeLock(taskMutex);
        stopping = true;
        PlatformCondition::Broadcast(taskCondition);
    }

    // Wait until finishing all the task threads.
    PlatformThread::JoinAll(threads.Count(), threads.Ptr());

    threads.Clear();

    stopping = false;
}

bool TaskManager::AddTask(TaskFunc taskFunction, void *data) {
    // Lock to add the task.
    ScopeLock scopeLock(taskMutex);

    int nextTaskIndex = (tailTaskIndex + 1) % taskRingBuffer.Count();
    if (nextTaskIndex == headTaskIndex) {
        BE_ERRLOG("TaskManager::AddTask: Task queue is full, task rejected\n");
        return false;
    }

    taskRingBuffer[tailTaskIndex] = {taskFunction, data};
    tailTaskIndex = nextTaskIndex;

    ++numActiveTasks;
    PlatformCondition::Signal(taskCondition);

    return true;
}

void TaskManager::WaitFinish() {
    ScopeLock scopeLock(taskMutex);

    // Check if all tasks are already finished
    if (IsTaskEmpty() && numActiveTasks <= 0) {
        return; // No tasks to wait for, return immediately
    }

    PlatformCondition::Wait(finishCondition, taskMutex, [this]{ return IsTaskEmpty() && numActiveTasks <= 0; });
}

// Return false if a timeout occurs.
bool TaskManager::TimedWaitFinish(int ms) {
    ScopeLock scopeLock(taskMutex);

    // Check if all tasks are already finished
    if (IsTaskEmpty() && numActiveTasks <= 0) {
        return true; // No tasks to wait for, return immediately
    }

    return PlatformCondition::TimedWait(finishCondition, taskMutex, ms, [this]{ return IsTaskEmpty() && numActiveTasks <= 0; });
}

BE1::Task TaskManager::GetTaskInternal() {
    Task task = taskRingBuffer[headTaskIndex];
    headTaskIndex = (headTaskIndex + 1) % taskRingBuffer.Count();

    return task;
}

unsigned int TaskThreadProc(void *param) {
    SIMD::SetDenormalFlushMode(true);

    TaskManager *taskManager = reinterpret_cast<TaskManager *>(param);
    Task task;

    while (1) {
        // Lock to get the task.
        {
            ScopeLock scopeLock(taskManager->taskMutex);

            // Wait for task condition variable.
            PlatformCondition::Wait(taskManager->taskCondition, taskManager->taskMutex, [taskManager]{ return !taskManager->IsTaskEmpty() || taskManager->stopping; });

            if (taskManager->stopping) {
                // Exit loop when stopping condition is met.
                break;
            }

            // Get the task from the ring buffer.
            task = taskManager->GetTaskInternal();
        }

        // Do the task.
        task.function(task.data);

        // Handle task completion
        {
            ScopeLock scopeLock(taskManager->taskMutex);

            // Decrease active task count after finishing a task function.
            --taskManager->numActiveTasks;

            // Wake finish condition variable when there is no active tasks.
            if (taskManager->numActiveTasks == 0 && taskManager->IsTaskEmpty()) {
            
                PlatformCondition::Signal(taskManager->finishCondition);
            }
        }
    }
    return 0;
}

BE_NAMESPACE_END
