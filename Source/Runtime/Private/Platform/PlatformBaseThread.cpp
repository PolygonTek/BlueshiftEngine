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

uint64_t PlatformBaseThread::GetCurrentThreadId() {
    return 0;
}

PlatformBaseThread *PlatformBaseThread::Create(threadFunc_t startProc, void *param, size_t stackSize, int affinity) {
    BE_FATALERROR("PlatformThread::Create not implmeneted on this platform");
    return nullptr;
}

void PlatformBaseThread::Destroy(PlatformBaseThread *thread) {
    BE_FATALERROR("PlatformThread::Destroy not implmeneted on this platform");
}

void PlatformBaseThread::SetName(const char *name) {
    BE_FATALERROR("PlatformThread::SetName not implmeneted on this platform");
}

void PlatformBaseThread::SetAffinity(int affinity) {
    BE_FATALERROR("PlatformThread::SetAffinity not implmeneted on this platform");
}

void PlatformBaseThread::Join(PlatformBaseThread *thread) {
    BE_FATALERROR("PlatformThread::Join not implmeneted on this platform");
}

void PlatformBaseThread::JoinAll(int numThreads, PlatformBaseThread *threads[]) {
    BE_FATALERROR("PlatformThread::JoinAll not implmeneted on this platform");
}

PlatformBaseMutex *PlatformBaseMutex::Create() {
    BE_FATALERROR("PlatformMutex::Create not implmeneted on this platform");
    return nullptr;
}

void PlatformBaseMutex::Destroy(PlatformBaseMutex *mutex) {
    BE_FATALERROR("PlatformMutex::Destroy not implmeneted on this platform");
}

void PlatformBaseMutex::Lock(const PlatformBaseMutex *mutex) {
    BE_FATALERROR("PlatformMutex::Lock not implmeneted on this platform");
}

bool PlatformBaseMutex::TryLock(const PlatformBaseMutex *mutex) {
    BE_FATALERROR("PlatformMutex::TryLock not implmeneted on this platform");
    return false;
}

void PlatformBaseMutex::Unlock(const PlatformBaseMutex *mutex) {
    BE_FATALERROR("PlatformMutex::Unlock not implmeneted on this platform");
}

PlatformBaseCondition *PlatformBaseCondition::Create() {
    BE_FATALERROR("PlatformCondition::Create not implmeneted on this platform");
    return nullptr;
}

void PlatformBaseCondition::Destroy(PlatformBaseCondition *cond) {
    BE_FATALERROR("PlatformCondition::Destroy not implmeneted on this platform");
}

void PlatformBaseCondition::Wait(const PlatformBaseCondition *cond, const PlatformBaseMutex *mutex) {
    BE_FATALERROR("PlatformCondition::Wait not implmeneted on this platform");
}

bool PlatformBaseCondition::TimedWait(const PlatformBaseCondition *cond, const PlatformBaseMutex *mutex, int ms) {
    BE_FATALERROR("PlatformCondition::TimedWait not implmeneted on this platform");
    return false;
}

void PlatformBaseCondition::Broadcast(const PlatformBaseCondition *cond) {
    BE_FATALERROR("PlatformCondition::Broadcast not implmeneted on this platform");
}

BE_NAMESPACE_END
