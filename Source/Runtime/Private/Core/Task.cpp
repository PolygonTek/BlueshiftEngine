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

TaskManager::TaskManager(int maxTasks, int numThreads) {
    this->maxTasks = maxTasks;
    this->taskRingBuffer = new Task[maxTasks];

    this->headTaskIndex = 0;
    this->tailTaskIndex = 0;

    this->numActiveTasks = 0;
    this->stopping = false;

    // Create synchronization objects.
    this->taskMutex = PlatformMutex::Create();
    this->taskCondition = PlatformCondition::Create();
    this->finishCondition = PlatformCondition::Create();

    if (numThreads < 0) {
        // Get thread count as number of logical processors.
        numThreads = PlatformSystem::NumCPUCoresIncludingHyperthreads();
    }

    // Start running threads.
    threads.Reserve(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads.Append(PlatformThread::Start(TaskThreadProc, (void *)this, 0));
    }
}

TaskManager::~TaskManager() {
    Stop();

    PlatformCondition::Destroy(taskCondition);
    PlatformCondition::Destroy(finishCondition);
    PlatformMutex::Destroy(taskMutex);

    delete[] taskRingBuffer;
}

bool TaskManager::AddTask(TaskFunc taskFunction, void *data) {
    Task task;
    task.function = taskFunction;
    task.data = data;

    // Lock to add the task.
    {
        ScopeLock scopeLock(taskMutex);

        int nextTaskIndex = (tailTaskIndex + 1) % maxTasks;
        if (nextTaskIndex == headTaskIndex) {
            BE_ERRLOG("TaskManager::AddTask: Too many tasks");
            return false;
        }

        taskRingBuffer[tailTaskIndex] = task;
        tailTaskIndex = nextTaskIndex;

        numActiveTasks++;
    }
    return true;
}

void TaskManager::Start() {
    PlatformCondition::Broadcast(taskCondition);
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
}

void TaskManager::WaitFinish() {
    ScopeLock scopeLock(taskMutex);
    PlatformCondition::Wait(finishCondition, taskMutex, [this]{ return IsTaskEmpty() && numActiveTasks <= 0; });
}

// Return false if a timeout occurs.
bool TaskManager::TimedWaitFinish(int ms) {
    ScopeLock scopeLock(taskMutex);
    bool ret = PlatformCondition::TimedWait(finishCondition, taskMutex, ms, [this]{ return IsTaskEmpty() && numActiveTasks <= 0; });
    return ret;
}

BE1::Task TaskManager::GetTaskInternal() {
    Task task = taskRingBuffer[headTaskIndex];
    headTaskIndex = (headTaskIndex + 1) % maxTasks;

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
                return 0;
            }

            // Get the task from the ring buffer.
            task = taskManager->GetTaskInternal();
        }

        // Do the task.
        task.function(task.data);

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
