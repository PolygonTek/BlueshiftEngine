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

using TaskFunc = void (*)(void *data);

struct Task {
    TaskFunc                func;
    void *                  data;
};

class BE_API TaskManager {
public:
    explicit TaskManager(int maxTasks, int numThreads = -1);
    ~TaskManager();

                            /// Returns number of threads.
    size_t                  NumThreads() const { return taskThreads.Count(); }

                            /// Is task list empty ?
    bool                    IsEmpty() const { return firstTaskIndex == lastTaskIndex; }

                            /// Is stopping now ?
    bool                    IsStopping() const { return isStopping; }

                            /// Returns number of active tasks.
    int64_t                 NumActiveTasks() const { return numActiveTasks; }

                            /// Adds a task with the given task function and data parameters.
    template <typename T>
    bool                    AddTask(void (*func)(T *), T *data);

                            /// Starts task threads.
    void                    Start();

                            /// Stops all the tasks.
    void                    Stop();
    
                            /// Waits until finishing all tasks.
    void                    WaitFinish();

                            /// Waits given time (milliseconds) for finishing all tasks.
                            /// Returns true if it finished in given time.
    bool                    TimedWaitFinish(int msec);

private:
    bool                    AddTaskInternal(TaskFunc func, void *data);

    Task *                  taskRingBuffer;     ///< Ring buffer of task list.
    int                     maxTasks;
    int                     firstTaskIndex;
    int                     lastTaskIndex;

    std::atomic<int>        numActiveTasks;     ///< Number of tasks in active state.
    std::atomic<bool>       isStopping;

    Array<PlatformThread *> taskThreads;

    PlatformMutex *         taskMutex;          ///< Mutex for accessing task list and execution.
    PlatformCondition *     taskCondition;      ///< Condition variable for task execution.
                            
    PlatformMutex *         finishMutex;        ///< Mutex for finishing task list.
    PlatformCondition *     finishCondition;    ///< Condition variable for finishing task list.

    static int              TaskThreadProc(void *param);
};

template <typename T>
bool TaskManager::AddTask(void (*func)(T *), T *data) {
    AddTaskInternal(reinterpret_cast<TaskFunc>(func), data);
}

BE_NAMESPACE_END
