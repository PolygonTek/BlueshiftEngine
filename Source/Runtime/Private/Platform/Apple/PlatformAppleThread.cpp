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
#include "Platform/PlatformThread.h"
#include <pthread.h>

BE_NAMESPACE_BEGIN

uint64_t PlatformAppleThread::GetCurrentThreadId() {
    return (uint64_t)pthread_mach_thread_np(pthread_self());
}

void PlatformAppleThread::SetName(const char *name) {
    pthread_setname_np(name);
}

void PlatformAppleThread::Yield() {
    sched_yield();
}

BE_NAMESPACE_END
