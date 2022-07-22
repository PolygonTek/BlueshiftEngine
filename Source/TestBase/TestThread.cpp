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
#include "TestThread.h"

static thread_local int counter = 0;

unsigned int threadFunc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName((const char *)param);
    //BE1::PlatformThread::SetCurrentThreadAffinityMask(1);

    for (int i = 0; i < 500; i++) {
        BE_LOG("TID: %i - counting %i\n", BE1::PlatformThread::GetCurrentThreadId(), counter);
        counter++;
    }
    return 0;
}

void TestThread() {
    BE1::Array<BE1::PlatformThread *> threads;
    threads.Append(BE1::PlatformThread::Start(threadFunc, "Test Thread 1"));
    threads.Append(BE1::PlatformThread::Start(threadFunc, "Test Thread 2"));

    BE1::PlatformThread::JoinAll(2, threads.Ptr());

    threads.Clear();
}
