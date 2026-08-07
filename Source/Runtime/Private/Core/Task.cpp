// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
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
#include "Core/ScopedLock.h"
#include "Core/Task.h"

BE_NAMESPACE_BEGIN

TaskManager::TaskManager(int maxTasks) {
    taskRingBuffer.SetCount(maxTasks);

    taskMutex = PlatformMutex::Create();
    taskCondition = PlatformCondition::Create();
    finishCondition = PlatformCondition::Create();
    groupFinishCondition = PlatformCondition::Create();
}

TaskManager::~TaskManager() {
    Stop();

    PlatformCondition::Destroy(taskCondition);
    PlatformCondition::Destroy(finishCondition);
    PlatformCondition::Destroy(groupFinishCondition);
    PlatformMutex::Destroy(taskMutex);
}

void TaskManager::Start(int numThreads, bool useAffinity) {
    ScopedLock lock(taskMutex);

    // Return if the threads already started.
    if (!threads.IsEmpty()) {
        return;
    }

    int numLogicalProcessors = PlatformSystem::NumCPUCoresIncludingHyperthreads();
    if (numThreads <= 0) {
        // Get thread count as number of logical processors.
        numThreads = numLogicalProcessors;
    }

    tailTaskIndex = 0;
    headTaskIndex = 0;
    activeTaskCount = 0;
    groupActiveTaskCount.Clear();
    stopping = false;

    threads.Reserve(numThreads);

    for (int i = 0; i < numThreads; i++) {
        uint64_t affinityMask = useAffinity ? (1ULL << (i % numLogicalProcessors)) : 0xFFFFFFFFFFFFFFFF;

        threads.Append(PlatformThread::Start(TaskManager::ThreadProc, (void *)this, 0, ThreadPriority::Normal, affinityMask));
    }
}

void TaskManager::Stop() {
    // Set the stopping and wake all the task threads.
    {
        ScopedLock lock(taskMutex);

        // Return if the threads already stopped.
        if (threads.IsEmpty()) {
            return;
        }

        stopping = true;
        PlatformCondition::Broadcast(taskCondition);
    }

    // Wait until finishing all the task threads.
    PlatformThread::JoinAll(threads.Count(), threads.Ptr());

    {
        ScopedLock lock(taskMutex);

        assert(IsTaskEmpty());
        assert(activeTaskCount == 0);

        threads.Clear();

        headTaskIndex = 0;
        tailTaskIndex = 0;
        activeTaskCount = 0;
        groupActiveTaskCount.Clear();
        stopping = false;

        // Wake any waiters after resetting the task lifecycle state.
        PlatformCondition::Broadcast(finishCondition);
        PlatformCondition::Broadcast(groupFinishCondition);
    }
}

int32_t TaskManager::CreateGroupId() {
    ScopedLock lock(taskMutex);
    return nextGroupId++;
}

bool TaskManager::AddTask(TaskFunc taskFunction, void *data, int32_t groupId, bool withWake) {
    // Lock to add the task.
    ScopedLock lock(taskMutex);

    if (stopping || threads.IsEmpty()) {
        BE_ERRLOG("TaskManager::AddTask: Task manager is not running, task rejected\n");
        return false;
    }

    int nextTaskIndex = (tailTaskIndex + 1) % taskRingBuffer.Count();
    if (nextTaskIndex == headTaskIndex) {
        BE_ERRLOG("TaskManager::AddTask: Task queue is full, task rejected\n");
        return false;
    }

    Task &task = taskRingBuffer[tailTaskIndex];
    task.groupId = groupId;
    task.function = taskFunction;
    task.data = data;

    tailTaskIndex = nextTaskIndex;

    ++activeTaskCount;

    // If valid groupId, increment groupActiveCount
    if (groupId >= 0) {
        groupActiveTaskCount[groupId]++;
    }

    if (withWake) {
        PlatformCondition::Signal(taskCondition);
    }

    return true;
}

bool TaskManager::AddTask(TaskWorker *taskWorker, int32_t groupId, bool withWake) {
    // Lock to add the task.
    ScopedLock lock(taskMutex);

    if (stopping || threads.IsEmpty()) {
        BE_ERRLOG("TaskManager::AddTask: Task manager is not running, task rejected\n");
        return false;
    }

    int nextTaskIndex = (tailTaskIndex + 1) % taskRingBuffer.Count();
    if (nextTaskIndex == headTaskIndex) {
        BE_ERRLOG("TaskManager::AddTask: Task queue is full, task rejected\n");
        return false;
    }

    Task &task = taskRingBuffer[tailTaskIndex];
    task.groupId = groupId;
    task.function = nullptr;
    task.data = taskWorker;

    tailTaskIndex = nextTaskIndex;

    ++activeTaskCount;

    // If valid groupId, increment groupActiveCount
    if (groupId >= 0) {
        groupActiveTaskCount[groupId]++;
    }

    if (withWake) {
        PlatformCondition::Signal(taskCondition);
    }

    return true;
}

void TaskManager::WaitFinish(int32_t groupId, bool withWake) {
    ScopedLock lock(taskMutex);

    // Check If group not exist or count <= 0
    int32_t *groupTaskCountPtr;
    if (!groupActiveTaskCount.Get(groupId, &groupTaskCountPtr) || *groupTaskCountPtr <= 0) {
        return; // No tasks to wait for, return immediately
    }

    if (withWake) {
        PlatformCondition::Broadcast(taskCondition);
    }

    PlatformCondition::Wait(groupFinishCondition, taskMutex, [this, groupId] {
        int32_t *groupTaskCountPtr;
        return !groupActiveTaskCount.Get(groupId, &groupTaskCountPtr) || *groupTaskCountPtr <= 0;
    });
}

void TaskManager::WaitFinishAll(bool withWake) {
    ScopedLock lock(taskMutex);

    // Check if all tasks are already finished
    if (IsTaskEmpty() && activeTaskCount <= 0) {
        return; // No tasks to wait for, return immediately
    }

    if (withWake) {
        PlatformCondition::Broadcast(taskCondition);
    }

    PlatformCondition::Wait(finishCondition, taskMutex, [this] {
        return IsTaskEmpty() && activeTaskCount <= 0;
    });
}

bool TaskManager::TimedWaitFinish(int ms, int32_t groupId, bool withWake) {
    ScopedLock lock(taskMutex);

    // Check If group not exist or count <= 0
    int32_t *groupTaskCountPtr;
    if (!groupActiveTaskCount.Get(groupId, &groupTaskCountPtr) || *groupTaskCountPtr <= 0) {
        return true; // No tasks to wait for, return immediately
    }

    if (withWake) {
        PlatformCondition::Broadcast(taskCondition);
    }

    return PlatformCondition::TimedWait(groupFinishCondition, taskMutex, ms, [this, groupId] {
        int32_t *groupTaskCountPtr;
        return !groupActiveTaskCount.Get(groupId, &groupTaskCountPtr) || *groupTaskCountPtr <= 0;
    });
}

bool TaskManager::TimedWaitFinishAll(int ms, bool withWake) {
    ScopedLock lock(taskMutex);

    // Check if all tasks are already finished
    if (IsTaskEmpty() && activeTaskCount <= 0) {
        return true; // No tasks to wait for, return immediately
    }

    if (withWake) {
        PlatformCondition::Broadcast(taskCondition);
    }

    return PlatformCondition::TimedWait(finishCondition, taskMutex, ms, [this] {
        return IsTaskEmpty() && activeTaskCount <= 0;
    });
}

Task TaskManager::GetTaskInternal() {
    Task task = taskRingBuffer[headTaskIndex];
    headTaskIndex = (headTaskIndex + 1) % taskRingBuffer.Count();

    return task;
}

void TaskManager::Run() {
    Task task;

    while (1) {
        // Lock to get the task.
        {
            ScopedLock lock(taskMutex);

            // Wait for task condition variable.
            PlatformCondition::Wait(taskCondition, taskMutex, [this] {
                return !IsTaskEmpty() || stopping;
            });

            if (stopping && IsTaskEmpty()) {
                // Exit after all queued tasks have finished when stopping.
                break;
            }

            // Get the task from the ring buffer.
            task = GetTaskInternal();
        }

        // Do the task.
        if (task.function) {
            task.function(task.data);
        } else {
            TaskWorker *taskWorker = reinterpret_cast<TaskWorker *>(task.data);
            taskWorker->DoWork();
            delete taskWorker;
        }

        // Handle task completion
        {
            ScopedLock lock(taskMutex);

            // Decrease active task count after finishing a task function.
            --activeTaskCount;

            // Decrease active task count in the given group.
            if (task.groupId >= 0) {
                int32_t *groupTaskCountPtr;
                if (groupActiveTaskCount.Get(task.groupId, &groupTaskCountPtr)) {
                    --(*groupTaskCountPtr);

                    if (*groupTaskCountPtr <= 0) {
                        // Wake the group finish condition variable when there are no active tasks left in this group.
                        PlatformCondition::Broadcast(groupFinishCondition);
                    }
                }
            }

            // Wake the finish condition variable when there are no active tasks remaining.
            if (activeTaskCount == 0 && IsTaskEmpty()) {
                PlatformCondition::Signal(finishCondition);
            }
        }
    }
}

unsigned int TaskManager::ThreadProc(void *param) {
    PlatformThread::SetCurrentThreadName("TaskManager::ThreadProc");

    SIMD::SetDenormalFlushMode(true);

    TaskManager *taskManager = reinterpret_cast<TaskManager *>(param);
    taskManager->Run();
    return 0;
}

BE_NAMESPACE_END
