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

TaskManager::TaskManager(int maxTasks, int numThreads) {
    this->maxTasks = maxTasks;
    this->taskBuffer = new Task[maxTasks];

    this->headTaskIndex = 0;
    this->tailTaskIndex = 0;

    this->numActiveTasks = 0;
    this->stopping = 0;

    // Create synchronization objects.
    this->taskMutex = (PlatformMutex *)PlatformMutex::Create();
    this->taskCondition = (PlatformCondition *)PlatformCondition::Create();

    this->finishMutex = (PlatformMutex *)PlatformMutex::Create();
    this->finishCondition = (PlatformCondition *)PlatformCondition::Create();

    if (numThreads < 0) {
        // Get thread count as number of logical processors
        numThreads = PlatformSystem::NumCPUCoresIncludingHyperthreads();
    }

    // Create task threads.
    for (int i = 0; i < numThreads; i++) {
        PlatformThread *thread = (PlatformThread *)PlatformThread::Create(TaskThreadProc, (void *)this, 0);
        this->taskThreads.Append(thread);
    }
}

TaskManager::~TaskManager() {
    Stop();

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

    delete [] taskBuffer;
}

bool TaskManager::AddTask(TaskFunc function, void *data) {
    Task task;
    task.function = function;
    task.data = data;

    // Lock for task addition
    PlatformMutex::Lock(taskMutex);

    int nextTaskIndex = (tailTaskIndex + 1) % maxTasks;

    if (nextTaskIndex == headTaskIndex) {
        PlatformMutex::Unlock(taskMutex);
        BE_ERRLOG("Too many tasks");
        return false;
    }

    taskBuffer[tailTaskIndex] = task;

    tailTaskIndex = nextTaskIndex;

    // Unlock for task addition
    PlatformMutex::Unlock(taskMutex);

    numActiveTasks.fetch_add(1);

    return true;
}

void TaskManager::Start() {
    PlatformCondition::Broadcast(taskCondition);
}

void TaskManager::Stop() {
    // Set the stopping and wake all the task threads.
    stopping = 1;

    PlatformMutex::Lock(taskMutex);
    PlatformCondition::Broadcast(taskCondition);
    PlatformMutex::Unlock(taskMutex);

    // Wait until finishing all the task threads.
    PlatformThread::JoinAll(taskThreads.Count(), (PlatformBaseThread **)taskThreads.Ptr());
}

void TaskManager::WaitFinish() {
    PlatformMutex::Lock(finishMutex);

    while (numActiveTasks > 0) {
        PlatformCondition::Wait(finishCondition, finishMutex);
    }

    PlatformMutex::Unlock(finishMutex);
}

bool TaskManager::TimedWaitFinish(int ms) {
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

void TaskThreadProc(void *param) {
    InitCPU();

    TaskManager *tm = (TaskManager *)param;

    while (1) {
        PlatformMutex::Lock(tm->taskMutex);

        // Wait for task condition variable.
        while (tm->IsEmpty() && !tm->stopping) {
            PlatformCondition::Wait(tm->taskCondition, tm->taskMutex);
        }

        if (tm->stopping) {
            PlatformMutex::Unlock(tm->taskMutex);
            break;
        }

        // Get the task from the ring buffer.
        Task task = tm->taskBuffer[tm->headTaskIndex];

        tm->headTaskIndex = (tm->headTaskIndex + 1) % tm->maxTasks;

        PlatformMutex::Unlock(tm->taskMutex);

        // Do the task.
        task.function(task.data);

        // Decrease active task count after finishing a task function.
        tm->numActiveTasks.fetch_sub(1);

        // Wake finish condition variable when there is no active tasks.
        if (tm->numActiveTasks == 0) {
            PlatformMutex::Lock(tm->finishMutex);
            PlatformCondition::Signal(tm->finishCondition);
            PlatformMutex::Unlock(tm->finishMutex);
        }
    }
}

BE_NAMESPACE_END
