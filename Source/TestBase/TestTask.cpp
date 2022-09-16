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

#include "BlueshiftEngine.h"
#include "TestTask.h"

void TaskFunc1(void *data) {
    for (int i = 0; i < 10; i++) {
        BE_LOG("TID: %i - TaskFunc1\n", BE1::PlatformThread::GetCurrentThreadId());
    }
}

void TaskFunc2(void *data) {
    for (int i = 0; i < 10; i++) {
        BE_LOG("TID: %i - TaskFunc2\n", BE1::PlatformThread::GetCurrentThreadId());
    }
}

void TaskFunc3(void *data) {
    for (int i = 0; i < 10; i++) {
        BE_LOG("TID: %i - TaskFunc3\n", BE1::PlatformThread::GetCurrentThreadId());
    }
}

void TestTask() {
    BE1::TaskManager taskManager(1024);
    taskManager.AddTask(TaskFunc1, nullptr);
    taskManager.AddTask(TaskFunc2, nullptr);
    taskManager.AddTask(TaskFunc3, nullptr);

    taskManager.WaitFinish();
}
