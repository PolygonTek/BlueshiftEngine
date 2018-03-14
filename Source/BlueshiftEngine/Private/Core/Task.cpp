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
#include "Platform/PlatformProcess.h"
#include "Core/Task.h"

BE_NAMESPACE_BEGIN

void TaskScheduler_ThreadProc(void *param);

TaskScheduler::TaskScheduler(int numThreads) {
    numActiveTasks = 0;
    terminate = false;
    
    taskMutex = PlatformMutex::Create();
    taskCondition = PlatformCondition::Create();

    finishMutex = PlatformMutex::Create();
    finishCondition = PlatformCondition::Create();

#ifdef __WIN32__
    if (numThreads < 0) {
        // Get thread count as number of logical processors
        numThreads = PlatformProcess::NumberOfLogicalProcessors();
    }
#endif

    for (int i = 0; i < numThreads; i++) {
        PlatformThread *thread = PlatformThread::Create(TaskScheduler_ThreadProc, (void *)this, 0);
        threads.Append(thread);
    }
}

TaskScheduler::~TaskScheduler() {
    if (threads.Count() == 0) {
        return;
    }

    // task 중단 flag 를 켜고 모든 task thread 들을 깨운다.
    PlatformMutex::Lock(taskMutex);
    terminate = true;
    PlatformCondition::Broadcast(taskCondition);
    PlatformMutex::Unlock(taskMutex);

    // thread 가 모두 종료될 때까지 대기
    for (int i = 0; i < threads.Count(); i++) {
        PlatformThread::Wait(threads[i]);
    }

    PlatformCondition::Delete(taskCondition);
    PlatformMutex::Delete(taskMutex);

    PlatformCondition::Delete(finishCondition);
    PlatformMutex::Delete(finishMutex);
}

void TaskScheduler::LockTask() {
    PlatformMutex::Lock(taskMutex);
}

void TaskScheduler::UnlockTask() {
    PlatformMutex::Unlock(taskMutex);
}

void TaskScheduler::AddTask(taskFunction_t function, void *data) {
    // task 추가를 위해 lock
    PlatformMutex::Lock(taskMutex);

    Task task;
    task.function = function;
    task.data = data;
    if (taskList.size() >= MaxTasks) {
        BE_FATALERROR(L"too many tasks generated");
    }
    taskList.push_back(task);
    numActiveTasks++;

    // task 추가를 기다리는 thread 들에게 모두 신호한 후 unlock (?모두?)
    PlatformCondition::Broadcast(taskCondition);
    PlatformMutex::Unlock(taskMutex);
}

void TaskScheduler::WaitFinish() {
    // finish 신호를 기다린다.
    PlatformMutex::Lock(finishMutex);
    // 신호를 받자마자 다시 task 가 추가될 수도 있으므로 loop 를 돈다.
    while (numActiveTasks > 0) {
        PlatformCondition::Wait(finishCondition, finishMutex);
    }
    PlatformMutex::Unlock(finishMutex);
}

bool TaskScheduler::TimedWaitFinish(int ms) {
    bool ret = true;
    // finish 신호를 일정 시간동안 기다린다.
    PlatformMutex::Lock(finishMutex);
    // 신호를 받자마자 다시 task 가 추가될 수도 있으므로 loop 를 돈다.
    while (numActiveTasks > 0 && ret == true) {
        ret = PlatformCondition::TimedWait(finishCondition, finishMutex, ms);
    }
    PlatformMutex::Unlock(finishMutex);
    return ret;
}

void TaskScheduler_ThreadProc(void *param) {
    /*int cpuid = GetCpuInfo()->cpuid;
    if (cpuid & CPUID_FTZ) {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    }

    if (cpuid & CPUID_DAZ) {
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    }*/

    TaskScheduler *ts = (TaskScheduler *)param;

    while (1) {
        PlatformMutex::Lock(ts->taskMutex);
        // 처리할 task 가 생길 때까지 wait
        while (ts->taskList.empty() && !ts->terminate) { // ?if?
            PlatformCondition::Wait(ts->taskCondition, ts->taskMutex);
        }

        // 종료 중이라면 exit
        if (ts->terminate) {
            PlatformMutex::Unlock(ts->taskMutex);
            break;
        }

        // task 를 앞에서부터 받아온다
        Task task = ts->taskList.front();
        ts->taskList.pop_front();
        PlatformMutex::Unlock(ts->taskMutex);

        task.function(task.data);

        // task function 이 끝나면 numActiveTask 를 줄인다. (다른 쓰레드와 공유된 변수이므로 atomic 연산)
        atomic_add(&ts->numActiveTasks, -1);
        // 실행 중인 task 가 한개도 없으면 finish 신호를 보낸다.
        if (ts->numActiveTasks == 0) {
            PlatformMutex::Lock(ts->finishMutex);
            PlatformCondition::Broadcast(ts->finishCondition);
            PlatformMutex::Unlock(ts->finishMutex);
        }
    }
}

BE_NAMESPACE_END
