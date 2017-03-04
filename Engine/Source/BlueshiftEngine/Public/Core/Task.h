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

#pragma once

#include "Containers/Array.h"
#include "Platform/Intrinsics.h"
#include "Platform/PlatformThread.h"

BE_NAMESPACE_BEGIN

typedef void (*taskFunction_t)(void *data);

struct Task {
    taskFunction_t          function;
    void *                  data;
};

class BE_API TaskScheduler {
public:
    enum {
        MaxTasks            = 1000000
    };

    explicit TaskScheduler(int numThreads = -1);
    ~TaskScheduler();

                            /// Returns number of active threads.
    size_t                  NumActiveThread() const { return threads.Count(); }

                            /// Returns number of active tasks.
    int64_t                 NumActiveTasks() const { return numActiveTasks; }

                            /// task 추가와 관련된 Lock 
    void                    LockTask();

                            /// task 추가와 관련된 Unlock
    void                    UnlockTask();

                            /// Adds a task with the given task function.
    void                    AddTask(taskFunction_t function, void *data);

                            /// Waits until finished all tasks.
    void                    WaitFinish();

                            /// Waits given time (milliseconds) for finishing all tasks.
                            /// Returns true if it finished in given time.
    bool                    TimedWaitFinish(int msec);

private:
    std::list<Task>         taskList;           ///< Number of tasks to be run
    atomic_t                numActiveTasks;     ///< Number of tasks in active state
                            
    bool                    terminate;          ///< terminate flag
                            
    PlatformMutex *         taskMutex;          ///< task 함수 실행을 위한 동기화 객체
    PlatformCondition *     taskCondition;      ///< task 함수 실행을 위한 condition
                            
    PlatformMutex *         finishMutex;        ///< task list 를 모두 마쳤을 때 사용할 동기화 객체
    PlatformCondition *     finishCondition;    ///< task list 를 모두 마쳤을 때 사용할 condition

    Array<PlatformThread *> threads;

    friend void             TaskScheduler_ThreadProc(void *param);
};

BE_NAMESPACE_END
