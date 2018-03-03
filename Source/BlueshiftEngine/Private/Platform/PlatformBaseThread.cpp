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

BE_NAMESPACE_BEGIN

PlatformBaseThread *PlatformBaseThread::Create(threadFunc_t startProc, void *param, size_t stackSize, int affinity) {
    BE_FATALERROR(L"Thread::Create not implmeneted on this platform");
    return nullptr;
}

void PlatformBaseThread::Delete(PlatformBaseThread *PlatformBaseThread) {
    BE_FATALERROR(L"Thread::Delete not implmeneted on this platform");
}

void PlatformBaseThread::SetAffinity(int affinity) {
    BE_FATALERROR(L"Thread::SetAffinity not implmeneted on this platform");
}

void PlatformBaseThread::Wait(PlatformBaseThread *PlatformBaseThread) {
    BE_FATALERROR(L"Thread::Wait not implmeneted on this platform");
}

void PlatformBaseThread::WaitAll(int numThreads, PlatformBaseThread *ThreadBases[]) {
    BE_FATALERROR(L"Thread::WaitAll not implmeneted on this platform");
}

PlatformBaseMutex *PlatformBaseMutex::Create() {
    BE_FATALERROR(L"Mutex::Create not implmeneted on this platform");
    return nullptr;
}

void PlatformBaseMutex::Delete(PlatformBaseMutex *PlatformBaseMutex) {
    BE_FATALERROR(L"Mutex::Delete not implmeneted on this platform");
}

void PlatformBaseMutex::Lock(const PlatformBaseMutex *PlatformBaseMutex) {
    BE_FATALERROR(L"Mutex::Lock not implmeneted on this platform");
}

bool PlatformBaseMutex::TryLock(const PlatformBaseMutex *PlatformBaseMutex) {
    BE_FATALERROR(L"Mutex::TryLock not implmeneted on this platform");
    return false;
}

void PlatformBaseMutex::Unlock(const PlatformBaseMutex *PlatformBaseMutex) {
    BE_FATALERROR(L"Mutex::Unlock not implmeneted on this platform");
}

PlatformBaseCondition *PlatformBaseCondition::Create() {
    BE_FATALERROR(L"Condition::Create not implmeneted on this platform");
    return nullptr;
}

void PlatformBaseCondition::Delete(PlatformBaseCondition *PlatformBaseCondition) {
    BE_FATALERROR(L"Condition::Delete not implmeneted on this platform");
}

void PlatformBaseCondition::Wait(const PlatformBaseCondition *PlatformBaseCondition, const PlatformBaseMutex *PlatformBaseMutex) {
    BE_FATALERROR(L"Condition::Wait not implmeneted on this platform");
}

bool PlatformBaseCondition::TimedWait(const PlatformBaseCondition *PlatformBaseCondition, const PlatformBaseMutex *PlatformBaseMutex, int ms) {
    BE_FATALERROR(L"Condition::TimedWait not implmeneted on this platform");
    return false;
}

void PlatformBaseCondition::Broadcast(const PlatformBaseCondition *PlatformBaseCondition) {
    BE_FATALERROR(L"Condition::Broadcast not implmeneted on this platform");
}

BE_NAMESPACE_END
