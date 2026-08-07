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

#include "BlueshiftEngine.h"
#include "Test.h"

void TaskFunc1(void *data) {
    BE_LOG("TID: %i - Start TaskFunc1\n", BE1::PlatformThread::GetCurrentThreadId());
    BE1::PlatformProcess::Sleep(0.1f);
    BE_LOG("TID: %i - End TaskFunc1\n", BE1::PlatformThread::GetCurrentThreadId());
}

void TaskFunc2(void *data) {
    BE_LOG("TID: %i - Start TaskFunc2\n", BE1::PlatformThread::GetCurrentThreadId());
    BE1::PlatformProcess::Sleep(0.1f);
    BE_LOG("TID: %i - End TaskFunc2\n", BE1::PlatformThread::GetCurrentThreadId());
}

void TaskFunc3(void *data) {
    BE_LOG("TID: %i - Start TaskFunc3\n", BE1::PlatformThread::GetCurrentThreadId());
    BE1::PlatformProcess::Sleep(0.1f);
    BE_LOG("TID: %i - End TaskFunc3\n", BE1::PlatformThread::GetCurrentThreadId());
}

void Test::TestTask() {
    int32_t groupId = BE1::Engine::taskManager->CreateGroupId();

    BE_ASSERT(BE1::Engine::taskManager->AddTask(TaskFunc1, nullptr, groupId));
    BE_ASSERT(BE1::Engine::taskManager->AddTask(TaskFunc2, nullptr, groupId));
    BE_ASSERT(BE1::Engine::taskManager->AddTask(TaskFunc3, nullptr, groupId));
    BE1::Engine::taskManager->WaitFinish(groupId);

    BE_ASSERT(BE1::Engine::taskManager->AddTask([]() {
        TaskFunc1(nullptr);
    }, groupId));
    BE_ASSERT(BE1::Engine::taskManager->AddTask([]() {
        TaskFunc2(nullptr);
    }, groupId));
    BE_ASSERT(BE1::Engine::taskManager->AddTask([]() {
        TaskFunc3(nullptr);
    }, groupId));
    BE1::Engine::taskManager->WaitFinish(groupId);

    const uint32_t numThreads = BE1::Engine::taskManager->NumThreads();
    std::atomic<int32_t> drainedTaskCount = 0;

    for (int i = 0; i < 3; ++i) {
        BE_ASSERT(BE1::Engine::taskManager->AddTask([&drainedTaskCount]() {
            drainedTaskCount.fetch_add(1, std::memory_order_relaxed);
        }, groupId, false));
    }

    BE1::Engine::taskManager->Stop();
    BE_ASSERT(drainedTaskCount.load(std::memory_order_relaxed) == 3);

    BE1::Engine::taskManager->Start(numThreads);

    const int32_t newGroupId = BE1::Engine::taskManager->CreateGroupId();
    BE_ASSERT(newGroupId != groupId);

    BE_ASSERT(BE1::Engine::taskManager->AddTask(TaskFunc1, nullptr, groupId));
    BE1::Engine::taskManager->WaitFinish(groupId);
}
