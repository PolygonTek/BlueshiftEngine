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
#include "Platform/PlatformAtomic.h"

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

    void                    Terminate();

                            /// Returns number of active threads.
    size_t                  NumActiveThread() const { return taskThreads.Count(); }

                            /// Returns number of active tasks.
    int64_t                 NumActiveTasks() const { return numActiveTasks; }

                            /// Adds a task with the given task function.
    void                    AddTask(taskFunction_t function, void *data);

                            /// Waits until finishing all tasks.
    void                    WaitFinish();

                            /// Waits given time (milliseconds) for finishing all tasks.
                            /// Returns true if it finished in given time.
    bool                    TimedWaitFinish(int msec);

private:
    bool                    terminate;          ///< terminate flag
    std::queue<Task>        taskList;           ///< task queue
    PlatformAtomic<int>     numActiveTasks;     ///< Number of tasks in active state

    PlatformMutex *         taskMutex;          ///< task 함수 실행을 위한 mutex
    PlatformCondition *     taskCondition;      ///< task 함수 실행을 위한 condition variable
                            
    PlatformMutex *         finishMutex;        ///< task list 를 모두 마쳤을 때 사용할 mutex
    PlatformCondition *     finishCondition;    ///< task list 를 모두 마쳤을 때 사용할 condition variable

    Array<PlatformThread *> taskThreads;

    friend void             TaskThreadProc(void *param);
};

BE_NAMESPACE_END
