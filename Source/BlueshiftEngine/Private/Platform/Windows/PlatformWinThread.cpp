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
#include "Platform/Windows/PlatformWinThread.h"

BE_NAMESPACE_BEGIN

// Set the affinity of a given thread
static void SetAffinity(HANDLE thread, int affinity) {
    if (affinity < 0) {
        return;
    }

#if (_WIN32_WINNT >= 0x0601)
    int groups = GetActiveProcessorGroupCount();
    int totalProcessors = 0;
    int group = 0;
    int	number = 0;

    for (int i = 0; i < groups; i++) {
        int processors = GetActiveProcessorCount(i);
        if (totalProcessors + processors > affinity) {
            group = i;
            number = affinity - totalProcessors;
            break;
        }
        totalProcessors += processors;
    }

    GROUP_AFFINITY groupAffinity;
    memset(&groupAffinity, 0, sizeof(groupAffinity));
    groupAffinity.Group = (WORD)group;
    groupAffinity.Mask = (KAFFINITY)(uint64_t(1) << number);
    if (!SetThreadGroupAffinity(thread, &groupAffinity, nullptr)) {
        BE_FATALERROR(L"cannot set thread group affinity");
    }

    PROCESSOR_NUMBER processorNumber;
    processorNumber.Group = group;
    processorNumber.Number = number;
    processorNumber.Reserved = 0;
    if (!SetThreadIdealProcessorEx(thread, &processorNumber, nullptr)) {
        BE_FATALERROR(L"cannot set thread ideal processor");
    }
#else
    if (!SetThreadAffinityMask(thread, DWORD_PTR(uint64(1) << affinity))) {
        BE_FATALERROR(L"cannot set thread affinity mask");
    }
    if (SetThreadIdealProcessor(thread, (DWORD)affinity) == (DWORD)-1) {
        BE_FATALERROR(L"cannot set thread ideal processor");
    }
#endif
}

// Creates a hardware thread running on specific core
PlatformWinThread *PlatformWinThread::Create(threadFunc_t startProc, void *param, size_t stackSize, int affinity) {
    HANDLE threadHandle = CreateThread(nullptr, stackSize, (LPTHREAD_START_ROUTINE)startProc, param, 0, nullptr);
    if (threadHandle == nullptr) {
        BE_FATALERROR(L"cannot create thread");
    }

    BE1::SetAffinity(threadHandle, affinity);
 
    PlatformWinThread *thread = new PlatformWinThread;
    thread->threadHandle = threadHandle;
    return thread;
}

void PlatformWinThread::Delete(PlatformWinThread *thread) {
    assert(thread);
    PlatformWinThread *winThread = static_cast<PlatformWinThread *>(thread);
    TerminateThread(winThread->threadHandle, 0);
    CloseHandle(winThread->threadHandle);
    delete winThread;
}

void PlatformWinThread::SetAffinity(int affinity) {
    BE1::SetAffinity(GetCurrentThread(), affinity);
}

void PlatformWinThread::Wait(PlatformWinThread *thread) {
    const PlatformWinThread *winThread = static_cast<PlatformWinThread *>(thread);
    WaitForSingleObject(winThread->threadHandle, INFINITE);
    CloseHandle(winThread->threadHandle);
    delete winThread;
}

void PlatformWinThread::WaitAll(int numThreads, PlatformWinThread *threads) {
    HANDLE *threadHandles = (HANDLE *)_alloca(sizeof(HANDLE) * numThreads);
    for (int i = 0; i < numThreads; i++) {
        const PlatformWinThread *winThread = static_cast<PlatformWinThread *>(&threads[i]);
        threadHandles[i] = winThread->threadHandle;
    }
    
    WaitForMultipleObjects(numThreads, threadHandles, TRUE, INFINITE);
    
    for (int i = 0; i < numThreads; i++) {
        PlatformWinThread *winThread = static_cast<PlatformWinThread *>(&threads[i]);
        CloseHandle(winThread->threadHandle);
        delete winThread;
    }
}

PlatformWinMutex *PlatformWinMutex::Create() {
    PlatformWinMutex *mutex = new PlatformWinMutex;
    mutex->cs = new CRITICAL_SECTION;
    InitializeCriticalSection(mutex->cs);
    return mutex;
}

void PlatformWinMutex::Delete(PlatformWinMutex *mutex) {
    assert(0);
    DeleteCriticalSection(mutex->cs);
    delete mutex->cs;
    delete mutex;
}

void PlatformWinMutex::Lock(const PlatformWinMutex *mutex) {
    EnterCriticalSection(mutex->cs);
}

bool PlatformWinMutex::TryLock(const PlatformWinMutex *mutex) {
    return TryEnterCriticalSection(mutex->cs) ? true : false;
}

void PlatformWinMutex::Unlock(const PlatformWinMutex *mutex) {
    LeaveCriticalSection(mutex->cs);
}

PlatformWinCondition *PlatformWinCondition::Create() {
    PlatformWinCondition *condition = new PlatformWinCondition;
    condition->condVar = new CONDITION_VARIABLE;
    InitializeConditionVariable(condition->condVar);
    return condition;
}

void PlatformWinCondition::Delete(PlatformWinCondition *condition) {
    assert(condition);
    delete condition->condVar;
    delete condition;
}

void PlatformWinCondition::Wait(const PlatformWinCondition *condition, const PlatformWinMutex *mutex) {
    SleepConditionVariableCS(condition->condVar, mutex->cs, INFINITE);
}

bool PlatformWinCondition::TimedWait(const PlatformWinCondition *condition, const PlatformWinMutex *mutex, int ms) {
    return SleepConditionVariableCS(condition->condVar, mutex->cs, (DWORD)ms) ? true : false;
}

void PlatformWinCondition::Signal(const PlatformWinCondition *condition) {
    WakeConditionVariable(condition->condVar);
}

void PlatformWinCondition::Broadcast(const PlatformWinCondition *condition) {
    WakeAllConditionVariable(condition->condVar);
}

BE_NAMESPACE_END
