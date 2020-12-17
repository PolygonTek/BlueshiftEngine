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

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
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
        BE_FATALERROR("cannot set thread group affinity");
    }

    PROCESSOR_NUMBER processorNumber;
    processorNumber.Group = group;
    processorNumber.Number = number;
    processorNumber.Reserved = 0;
    if (!SetThreadIdealProcessorEx(thread, &processorNumber, nullptr)) {
        BE_FATALERROR("cannot set thread ideal processor");
    }
#else
    if (!SetThreadAffinityMask(thread, DWORD_PTR(uint64(1) << affinity))) {
        BE_FATALERROR("cannot set thread affinity mask");
    }
    if (SetThreadIdealProcessor(thread, (DWORD)affinity) == (DWORD)-1) {
        BE_FATALERROR("cannot set thread ideal processor");
    }
#endif
}

uint64_t PlatformWinThread::GetCurrentThreadId() {
    return ::GetCurrentThreadId();
}

// Creates a hardware thread running on specific core
PlatformBaseThread *PlatformWinThread::Create(threadFunc_t startProc, void *param, size_t stackSize, int affinity) {
    HANDLE threadHandle = CreateThread(nullptr, stackSize, (LPTHREAD_START_ROUTINE)startProc, param, 0, nullptr);
    if (threadHandle == nullptr) {
        BE_FATALERROR("cannot create thread");
    }

    BE1::SetAffinity(threadHandle, affinity);
 
    PlatformWinThread *thread = new PlatformWinThread;
    thread->threadHandle = threadHandle;
    return thread;
}

void PlatformWinThread::Destroy(PlatformBaseThread *thread) {
    assert(thread);
    PlatformWinThread *winThread = static_cast<PlatformWinThread *>(thread);
    TerminateThread(winThread->threadHandle, 0);
    CloseHandle(winThread->threadHandle);
    delete winThread;
}

void PlatformWinThread::SetName(const char *name) {
    /**
    * Code setting the thread name for use in the debugger.
    *
    * http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
    */
    const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
    struct THREADNAME_INFO {
        DWORD dwType;       // Must be 0x1000.
        LPCSTR szName;      // Pointer to name (in user addr space).
        DWORD dwThreadID;   // Thread ID (-1=caller thread).
        DWORD dwFlags;      // Reserved for future use, must be zero.
    };
#pragma pack(pop)
    THREADNAME_INFO threadNameInfo;
    threadNameInfo.dwType = 0x1000;
    threadNameInfo.szName = name;
    threadNameInfo.dwThreadID = ::GetCurrentThreadId();
    threadNameInfo.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(threadNameInfo) / sizeof(ULONG_PTR), (ULONG_PTR *)&threadNameInfo);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#pragma warning(pop)
}

void PlatformWinThread::SetAffinity(int affinity) {
    BE1::SetAffinity(GetCurrentThread(), affinity);
}

void PlatformWinThread::Join(PlatformBaseThread *thread) {
    const PlatformWinThread *winThread = static_cast<PlatformWinThread *>(thread);
    WaitForSingleObject(winThread->threadHandle, INFINITE);
    CloseHandle(winThread->threadHandle);
    delete winThread;
}

void PlatformWinThread::JoinAll(int numThreads, PlatformBaseThread *threads[]) {
    HANDLE *threadHandles = (HANDLE *)_alloca(sizeof(HANDLE) * numThreads);
    for (int i = 0; i < numThreads; i++) {
        const PlatformWinThread *winThread = static_cast<PlatformWinThread *>(threads[i]);
        threadHandles[i] = winThread->threadHandle;
    }
    
    WaitForMultipleObjects(numThreads, threadHandles, TRUE, INFINITE);
    
    for (int i = 0; i < numThreads; i++) {
        PlatformWinThread *winThread = static_cast<PlatformWinThread *>(threads[i]);
        CloseHandle(winThread->threadHandle);
        delete winThread;
    }
}

PlatformBaseMutex *PlatformWinMutex::Create() {
    PlatformWinMutex *mutex = new PlatformWinMutex;
    mutex->cs = new CRITICAL_SECTION;
    InitializeCriticalSectionAndSpinCount(mutex->cs, 4000);
    return mutex;
}

void PlatformWinMutex::Destroy(PlatformBaseMutex *mutex) {
    PlatformWinMutex *winMutex = static_cast<PlatformWinMutex *>(mutex);
    DeleteCriticalSection(winMutex->cs);
    delete winMutex->cs;
    delete winMutex;
}

void PlatformWinMutex::Lock(const PlatformBaseMutex *mutex) {
    const PlatformWinMutex *winMutex = static_cast<const PlatformWinMutex *>(mutex);
    // Spin first before entering critical section, causing ring-0 transition and context switch.
    if (TryEnterCriticalSection(winMutex->cs) == 0) {
        EnterCriticalSection(winMutex->cs);
    }
}

bool PlatformWinMutex::TryLock(const PlatformBaseMutex *mutex) {
    const PlatformWinMutex *winMutex = static_cast<const PlatformWinMutex *>(mutex);
    return TryEnterCriticalSection(winMutex->cs) ? true : false;
}

void PlatformWinMutex::Unlock(const PlatformBaseMutex *mutex) {
    const PlatformWinMutex *winMutex = static_cast<const PlatformWinMutex *>(mutex);
    LeaveCriticalSection(winMutex->cs);
}

PlatformBaseCondition *PlatformWinCondition::Create() {
    PlatformWinCondition *condition = new PlatformWinCondition;
    condition->condVar = new CONDITION_VARIABLE;
    InitializeConditionVariable(condition->condVar);
    return condition;
}

void PlatformWinCondition::Destroy(PlatformBaseCondition *condition) {
    PlatformWinCondition *winCondition = static_cast<PlatformWinCondition *>(condition);

    assert(winCondition);
    delete winCondition->condVar;
    delete winCondition;
}

void PlatformWinCondition::Wait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex) {
    const PlatformWinCondition *winCondition = static_cast<const PlatformWinCondition *>(condition);
    const PlatformWinMutex *winMutex = static_cast<const PlatformWinMutex *>(mutex);

    SleepConditionVariableCS(winCondition->condVar, winMutex->cs, INFINITE);
}

bool PlatformWinCondition::TimedWait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex, int ms) {
    const PlatformWinCondition *winCondition = static_cast<const PlatformWinCondition *>(condition);
    const PlatformWinMutex *winMutex = static_cast<const PlatformWinMutex *>(mutex);

    return SleepConditionVariableCS(winCondition->condVar, winMutex->cs, (DWORD)ms) ? true : false;
}

void PlatformWinCondition::Signal(const PlatformBaseCondition *condition) {
    const PlatformWinCondition *winCondition = static_cast<const PlatformWinCondition *>(condition);

    WakeConditionVariable(winCondition->condVar);
}

void PlatformWinCondition::Broadcast(const PlatformBaseCondition *condition) {
    const PlatformWinCondition *winCondition = static_cast<const PlatformWinCondition *>(condition);

    WakeAllConditionVariable(winCondition->condVar);
}

BE_NAMESPACE_END
