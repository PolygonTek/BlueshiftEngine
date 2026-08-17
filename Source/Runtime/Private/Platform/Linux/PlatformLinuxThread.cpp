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

#include "Precompiled.h"
#include "Platform/PlatformThread.h"
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

BE_NAMESPACE_BEGIN

PlatformLinuxThread *PlatformLinuxThread::Start(threadFunc_t startProc, void *param, size_t stackSize, ThreadPriority::Enum priority) {
    PlatformLinuxThread *linuxThread = BE_NEW PlatformLinuxThread;
    if (!StartInternal(linuxThread, startProc, param, stackSize, priority)) {
        delete linuxThread;
        return nullptr;
    }
    return linuxThread;
}

void PlatformLinuxThread::JoinAll(int numThreads, PlatformLinuxThread *linuxThreads[]) {
    for (int i = 0; i < numThreads; ++i) {
        PlatformLinuxThread *linuxThread = linuxThreads[i];
        if (!JoinInternal(linuxThread)) {
            return;
        }
        delete linuxThread;
    }
}

uint64_t PlatformLinuxThread::GetCurrentThreadId() {
    return static_cast<uint64_t>(syscall(SYS_gettid));
}

void PlatformLinuxThread::SetCurrentThreadName(const char *name) {
    pthread_setname_np(pthread_self(), name);
}

BE_NAMESPACE_END
