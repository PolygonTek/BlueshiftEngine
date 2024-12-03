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
#include "Platform/PlatformThread.h"

BE_NAMESPACE_BEGIN

class BE_API TaskWorker {
public:
    virtual void            DoWork() = 0;
};

enum class TaskState : uint8_t {
    Ready,
    Running,
    Waiting
};

using TaskFunc = void (*)(void *data);

struct Task {
    TaskFunc                function;
    void *                  data;
    uint32_t                id;
    TaskState               state;
};

class BE_API TaskManager {
public:
    explicit TaskManager(int maxTasks);
    TaskManager(const TaskManager &) = delete;
    TaskManager &operator=(const TaskManager &) = delete;
    ~TaskManager();

                            /// Returns number of threads.
    int                     NumThreads() const { return threads.Count(); }

                            /// Is task list empty ?
    bool                    IsTaskEmpty() const { return headTaskIndex == tailTaskIndex; }

                            /// Is stopping now ?
    bool                    IsStopping() const { return (bool)stopping; }

                            /// Returns number of active tasks.
    int64_t                 NumActiveTasks() const { return numActiveTasks; }

                            /// Starts task threads.
    void                    Start(int numThreads = 0, bool useAffinity = false);

                            /// Stops all the tasks.
    void                    Stop();

                            /// Adds a task with the given task worker object.
    int32_t                 AddTask(TaskWorker *taskWorker, bool withWake = true);

                            /// Adds a task with the given task function.
    int32_t                 AddTask(TaskFunc function, void *data, bool withWake = true);

                            /// Adds a task with a lambda function.
    template <typename Lambda>
    int32_t                 AddTask(Lambda &&lambda, bool withWake = true);
    
                            /// Waits until finishing all tasks.
    void                    WaitFinish(bool withWake = false);

                            /// Waits given time (milliseconds) for finishing all tasks.
                            /// Returns true if it finished in given time.
    bool                    TimedWaitFinish(int msec, bool withWake = false);

                            /// Check if task is running with the given task id.
    bool                    IsTaskRunning(int32_t taskId) const;
    bool                    IsTaskRunning(const Array<int32_t> &taskIds) const;

private:
    Task *                  GetTaskInternal();

    Array<Task>             taskRingBuffer;             ///< Ring buffer of task list.
    int                     headTaskIndex = 0;
    int                     tailTaskIndex = 0;

    int                     numActiveTasks = 0;         ///< Number of tasks in active state.
    bool                    stopping = false;
    int32_t                 nextTaskId = 0;

    Array<PlatformThread *> threads;

    PlatformMutex *         taskMutex = nullptr;        ///< Mutex for accessing task list and execution.
    PlatformCondition *     taskCondition = nullptr;    ///< Condition variable for task execution.
    PlatformCondition *     finishCondition = nullptr;  ///< Condition variable for finishing task list.

    friend unsigned int     TaskThreadProc(void *param);
};

template <typename Lambda>
int32_t BE1::TaskManager::AddTask(Lambda &&lambda, bool withWake) {
    if constexpr (std::is_convertible_v<Lambda, TaskFunc>) {
        return AddTask(static_cast<TaskFunc>(lambda), nullptr, withWake);
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
    std::unique_ptr<Lambda> lambdaPtr(new Lambda(std::forward<Lambda>(lambda)));
    void *data = lambdaPtr.get();

    int32_t taskId = AddTask(&LambdaWrapper::Call, data, withWake);

    // Release ownership as the task manager is now responsible for cleaning up.
    lambdaPtr.release();

    return taskId;
}

BE_NAMESPACE_END
