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
#include "Core/Task.h"

BE_NAMESPACE_BEGIN

void TaskThreadProc(void *param);

TaskScheduler::TaskScheduler(int numThreads) {
    numActiveTasks = 0;
    terminate = false;

    // Create synchronization objects.
    taskMutex = PlatformMutex::Create();
    taskCondition = PlatformCondition::Create();

    finishMutex = PlatformMutex::Create();
    finishCondition = PlatformCondition::Create();

    if (numThreads < 0) {
        // Get thread count as number of logical processors
        numThreads = PlatformSystem::NumCPUCoresIncludingHyperthreads();
    }

    // Create task threads.
    for (int i = 0; i < numThreads; i++) {
        PlatformThread *thread = PlatformThread::Create(TaskThreadProc, (void *)this, 0);
        taskThreads.Append(thread);
    }
}

TaskScheduler::~TaskScheduler() {
    Terminate();

    // Destroy all the synchronization objects.
    PlatformCondition::Destroy(taskCondition);
    PlatformMutex::Destroy(taskMutex);

    PlatformCondition::Destroy(finishCondition);
    PlatformMutex::Destroy(finishMutex);

    // Destroy all the task threads.
    for (int i = 0; i < taskThreads.Count(); i++) {
        PlatformThread::Destroy(taskThreads[i]);
    }
    taskThreads.Clear();
}

void TaskScheduler::Terminate() {
    PlatformMutex::Lock(taskMutex);

    // Set the terminate and wake all the task threads.
    terminate = true;
    PlatformCondition::Broadcast(taskCondition);

    PlatformMutex::Unlock(taskMutex);

    // Wait until finishing all the task threads.
    PlatformThread::JoinAll(taskThreads.Count(), taskThreads.Ptr());
}

void TaskScheduler::AddTask(taskFunction_t function, void *data) {
    Task task;
    task.function = function;
    task.data = data;

    // Lock for task addition
    PlatformMutex::Lock(taskMutex);

    if (taskList.size() >= MaxTasks) {
        PlatformMutex::Unlock(taskMutex);
        BE_FATALERROR("Too many tasks are added");
    }
    taskList.push(task);

    numActiveTasks += 1;

    // Unlock for task addition
    PlatformMutex::Unlock(taskMutex);

    PlatformCondition::Broadcast(taskCondition);
}

void TaskScheduler::WaitFinish() {
    PlatformMutex::Lock(finishMutex);

    while (numActiveTasks > 0) {
        PlatformCondition::Wait(finishCondition, finishMutex);
    }

    PlatformMutex::Unlock(finishMutex);
}

bool TaskScheduler::TimedWaitFinish(int ms) {
    bool ret = true;

    PlatformMutex::Lock(finishMutex);

    while (numActiveTasks > 0 && ret == true) {
        ret = PlatformCondition::TimedWait(finishCondition, finishMutex, ms);
    }

    PlatformMutex::Unlock(finishMutex);
    return ret;
}

static void InitCPU() {
#ifdef __WIN32__
    int cpuid = GetCpuInfo()->cpuid;
    if (cpuid & CPUID_FTZ) {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    }
    if (cpuid & CPUID_DAZ) {
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    }
#endif
}

static void TaskThreadProc(void *param) {
    InitCPU();

    TaskScheduler *ts = (TaskScheduler *)param;

    while (1) {
        PlatformMutex::Lock(ts->taskMutex);

        // Wait for task condition variable.
        while (ts->taskList.empty() && !ts->terminate) {
            PlatformCondition::Wait(ts->taskCondition, ts->taskMutex);
        }

        // Exit thread when terminating.
        if (ts->terminate) {
            PlatformMutex::Unlock(ts->taskMutex);
            break;
        }

        // Get the task from the queue.
        Task task = ts->taskList.front();
        ts->taskList.pop();

        PlatformMutex::Unlock(ts->taskMutex);

        // Do the task.
        task.function(task.data);

        // Decrease active task count after finishing task function.
        ts->numActiveTasks -= 1;

        // Wake finish condition variable when there is no active tasks.
        if (ts->numActiveTasks == 0) {
            PlatformMutex::Lock(ts->finishMutex);
            PlatformCondition::Signal(ts->finishCondition);
            PlatformMutex::Unlock(ts->finishMutex);
        }
    }
}

BE_NAMESPACE_END
