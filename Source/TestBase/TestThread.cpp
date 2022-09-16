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

static int count = 0;
static BE1::PlatformMutex *count_mutex = nullptr;

static unsigned int CounterFunc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName((const char *)param);

    for (int i = 0; i < 10; i++) {
        BE1::PlatformMutex::Lock(count_mutex);
        count = count + 1;
        BE_LOG("TID: %i - counting %i\n", BE1::PlatformThread::GetCurrentThreadId(), count);
        BE1::PlatformMutex::Unlock(count_mutex);

        BE1::PlatformProcess::Sleep(0.001f);
    }
    return 0;
}

void TestThread() {
    count_mutex = BE1::PlatformMutex::Create();

    BE1::Array<BE1::PlatformThread *> threads;
    threads.Append(BE1::PlatformThread::Start(CounterFunc, "Counter Thread 1"));
    threads.Append(BE1::PlatformThread::Start(CounterFunc, "Counter Thread 2"));

    BE1::PlatformThread::Join(threads[0]);
    BE1::PlatformThread::Join(threads[1]);
}
