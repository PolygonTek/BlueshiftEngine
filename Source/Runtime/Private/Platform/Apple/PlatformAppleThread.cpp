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

PlatformAppleThread *PlatformAppleThread::Start(threadFunc_t startProc, void *param, size_t stackSize, ThreadPriority::Enum priority, uint64_t affinityMask) {
    return PlatformPosixThread::Start(startProc, param, stackSize, priority, affinityMask);
}

uint64_t PlatformAppleThread::GetCurrentThreadId() {
    return (uint64_t)pthread_mach_thread_np(pthread_self());
}

void PlatformAppleThread::SetCurrentThreadName(const char *name) {
    // According to http://stackoverflow.com/a/7989973
    // this needs to be called in the thread to be named !
    pthread_setname_np(name);
}

void PlatformAppleThread::Yield() {
    sched_yield();
}

BE_NAMESPACE_END
