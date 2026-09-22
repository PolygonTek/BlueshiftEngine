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

#pragma once

#include "Containers/Array.h"
#include "Containers/RingBuffer.h"
#include "Containers/HashTable.h"
#include "Threading/Threading.h"
#include "Platform/PlatformThread.h"

BE_NAMESPACE_BEGIN

class BE_API TaskWorker {
public:
    virtual ~TaskWorker() = default;

    virtual void                DoWork() = 0;
};

using TaskFunc = void (*)(void *data);

struct Task {
    TaskFunc                    function;
    void *                      data;
    int32_t                     groupId = -1;
};

class BE_API TaskManager {
public:
    explicit TaskManager(int maxTasks);
    TaskManager(const TaskManager &) = delete;
    TaskManager &operator=(const TaskManager &) = delete;
    ~TaskManager();

                                /// Returns number of threads.
    uint32_t                    NumThreads() const { return threads.Count(); }

                                /// Starts task threads.
    void                        Start(int numThreads = 0);

                                /// Finishes all queued tasks and stops the task threads.
    void                        Stop();

                                /// Creates a new group ID.
    int32_t                     CreateGroupId();

                                /// Adds a task with the given task worker object.
                                /// Ownership transfers to the task manager only on successful submission.
                                /// Returns true on success, or false if the task was rejected.
    [[nodiscard]] bool          AddTask(TaskWorker *taskWorker, int32_t groupId = -1, bool withWake = true);

                                /// Adds a task with the given task function.
                                /// Returns true on success, or false if the task was rejected.
    [[nodiscard]] bool          AddTask(TaskFunc function, void *data, int32_t groupId = -1, bool withWake = true);

                                /// Adds a task with a lambda function.
                                /// Returns true on success, or false if the task was rejected.
    template <typename Lambda>
    [[nodiscard]] bool          AddTask(Lambda &&lambda, int32_t groupId = -1, bool withWake = true);

                                /// Waits until finishing all tasks in the given group.
    void                        WaitFinish(int32_t groupId, bool withWake = false);
    
                                /// Waits until finishing all tasks.
    void                        WaitFinishAll(bool withWake = false);

                                /// Waits given time (milliseconds) for finishing all tasks in the given group.
                                /// Returns true if it finished in given time.
                                /// Returns false if a timeout occurs.
    bool                        TimedWaitFinish(int msec, int32_t groupId, bool withWake = false);

                                /// Waits given time (milliseconds) for finishing all tasks.
                                /// Returns true if it finished in given time.
                                /// Returns false if a timeout occurs.
    bool                        TimedWaitFinishAll(int msec, bool withWake = false);

private:
    bool                        IsTaskEmpty() const { return taskRingBuffer.IsEmpty(); }
    Task                        GetTaskInternal();
    void                        Run();

    static unsigned int         ThreadProc(void *param);

    RingBuffer<Task>            taskRingBuffer;                 ///< Fixed-capacity task queue; submissions never grow it.

    int32_t                     activeTaskCount = 0;            ///< Number of tasks in active state.
    HashTable<int32_t, int32_t> groupActiveTaskCount;           ///< Tracks how many tasks are active in each group.
    int32_t                     nextGroupId = 0;
    bool                        stopping = false;

    Array<PlatformThread *>     threads;

    Mutex                       taskMutex;                      ///< Mutex for accessing task list and execution.
    Condition                   taskCondition;                  ///< Condition variable for task execution.
    Condition                   finishCondition;                ///< Condition variable for finishing task list.
    Condition                   groupFinishCondition;           ///< Condition variable for group finishing task list.
};

template <typename Lambda>
bool BE1::TaskManager::AddTask(Lambda &&lambda, int32_t groupId, bool withWake) {
    if constexpr (std::is_convertible_v<Lambda, TaskFunc>) {
        return AddTask(static_cast<TaskFunc>(lambda), nullptr, groupId, withWake);
    }
    struct LambdaWrapper {
        static void Call(void *data) {
            Lambda* lambda = reinterpret_cast<Lambda *>(data);
            (*lambda)();
            // Delete lambda after the task is executed.
            delete lambda;
        }
    };

    // Allocate memory for lambda and store it in a unique_ptr for automatic cleanup.
    std::unique_ptr<Lambda> lambdaPtr(BE_NEW Lambda(std::forward<Lambda>(lambda)));
    void *data = lambdaPtr.get();

    bool submitted = AddTask(&LambdaWrapper::Call, data, groupId, withWake);

    if (submitted) {
        // Transfer ownership only after the task manager accepts the task.
        lambdaPtr.release();
    }

    return submitted;
}

BE_NAMESPACE_END
