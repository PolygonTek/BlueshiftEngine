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
#include <process.h>

BE_NAMESPACE_BEGIN

// Set the processor affinity index of a given thread.
static void Win_SetThreadAffinity(HANDLE handle, int affinityIndex) {
    if (affinityIndex < 0) {
        return;
    }

#if (_WIN32_WINNT >= 0x0601)
    int groupCount = GetActiveProcessorGroupCount();
    int totalProcessorCount = 0;
    int affinityGroupIndex = 0;
    int	affinityProcessorIndex = 0;

    for (int groupIndex = 0; groupIndex < groupCount; groupIndex++) {
        int currentGroupProcessorCount = GetActiveProcessorCount(groupIndex);
        if (affinityIndex < currentGroupProcessorCount + totalProcessorCount) {
            affinityGroupIndex = groupIndex;
            affinityProcessorIndex = affinityIndex - totalProcessorCount;
            break;
        }
        totalProcessorCount += currentGroupProcessorCount;
    }

    GROUP_AFFINITY groupAffinity;
    memset(&groupAffinity, 0, sizeof(groupAffinity));
    groupAffinity.Group = (WORD)affinityGroupIndex;
    groupAffinity.Mask = (KAFFINITY)(uint64_t(1) << affinityProcessorIndex);
    if (!SetThreadGroupAffinity(handle, &groupAffinity, nullptr)) {
        BE_FATALERROR("cannot set thread group affinity");
    }

    PROCESSOR_NUMBER processorNumber;
    processorNumber.Group = affinityGroupIndex;
    processorNumber.Number = affinityProcessorIndex;
    processorNumber.Reserved = 0;
    if (!SetThreadIdealProcessorEx(handle, &processorNumber, nullptr)) {
        BE_FATALERROR("cannot set thread ideal processor");
    }
#else
    if (!SetThreadAffinityMask(handle, DWORD_PTR(uint64(1) << affinityIndex))) {
        BE_FATALERROR("cannot set thread affinity mask");
    }
    if (SetThreadIdealProcessor(handle, (DWORD)affinityIndex) == (DWORD)-1) {
        BE_FATALERROR("cannot set thread ideal processor");
    }
#endif
}

static int Win_TranslateThreadPriority(ThreadPriority::Enum priority) {
    switch (priority) {
    case ThreadPriority::Enum::AboveNormal: return THREAD_PRIORITY_ABOVE_NORMAL;
    case ThreadPriority::Enum::Normal: return THREAD_PRIORITY_NORMAL;
    case ThreadPriority::Enum::BelowNormal: return THREAD_PRIORITY_BELOW_NORMAL;
    case ThreadPriority::Enum::Highest: return THREAD_PRIORITY_HIGHEST;
    case ThreadPriority::Enum::Lowest: return THREAD_PRIORITY_LOWEST;
    case ThreadPriority::Enum::SlightlyBelowNormal: return THREAD_PRIORITY_NORMAL - 1;
    default: BE_ERRLOG("Unknown priority passed to Win_TranslateThreadPriority()"); return THREAD_PRIORITY_NORMAL;
    }
}

static void Win_SetThreadPriority(HANDLE handle, ThreadPriority::Enum priority) {
    ::SetThreadPriority(handle, Win_TranslateThreadPriority(priority));
}

// Creates a hardware thread running on specific core
PlatformWinThread *PlatformWinThread::Start(threadFunc_t startProc, void *param, size_t stackSize, ThreadPriority::Enum priority, uint64_t affinityMask) {
    HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, stackSize, startProc, param, 0, nullptr);
    //HANDLE threadHandle = CreateThread(nullptr, stackSize, (LPTHREAD_START_ROUTINE)startProc, param, 0, nullptr);
    if (threadHandle == nullptr) {
        BE_FATALERROR("Failed to create Win32 thread");
        return nullptr;
    }

    if (affinityMask != 0xFFFFFFFFFFFFFFFF) {
        ::SetThreadAffinityMask(threadHandle, affinityMask);
    }
    Win_SetThreadPriority(threadHandle, priority);

    PlatformWinThread *winThread = new PlatformWinThread;
    winThread->threadHandle = threadHandle;
    return winThread;
}

void PlatformWinThread::Terminate(PlatformWinThread *winThread) {
    assert(winThread);
    TerminateThread(winThread->threadHandle, 0);
    CloseHandle(winThread->threadHandle);
    delete winThread;
}

uint64_t PlatformWinThread::GetCurrentThreadId() {
    return ::GetCurrentThreadId();
}

void PlatformWinThread::SetCurrentThreadName(const char *name) {
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

void PlatformWinThread::SetCurrentThreadAffinityMask(uint64_t affinityMask) {
    ::SetThreadAffinityMask(GetCurrentThread(), affinityMask);
}

void PlatformWinThread::SetCurrentThreadPriority(ThreadPriority::Enum priority) {
    Win_SetThreadPriority(GetCurrentThread(), priority);
}

void PlatformWinThread::Yield() {
    SwitchToThread();
}

void PlatformWinThread::Detach(PlatformWinThread *winThread) {
    CloseHandle(winThread->threadHandle);
}

void PlatformWinThread::Join(PlatformWinThread *winThread) {
    WaitForSingleObject(winThread->threadHandle, INFINITE);
    CloseHandle(winThread->threadHandle);
    delete winThread;
}

void PlatformWinThread::JoinAll(int numThreads, PlatformWinThread *winThreads[]) {
    HANDLE *threadHandles = (HANDLE *)_alloca(sizeof(HANDLE) * numThreads);
    for (int i = 0; i < numThreads; i++) {
        threadHandles[i] = winThreads[i]->threadHandle;
    }    
    WaitForMultipleObjects(numThreads, threadHandles, TRUE, INFINITE);
    
    for (int i = 0; i < numThreads; i++) {
        PlatformWinThread *winThread = winThreads[i];
        CloseHandle(winThread->threadHandle);
        delete winThread;
    }
}

PlatformWinMutex *PlatformWinMutex::Create() {
    PlatformWinMutex *winMutex = new PlatformWinMutex;
    winMutex->cs = new CRITICAL_SECTION;
    InitializeCriticalSectionAndSpinCount(winMutex->cs, 4000);
    return winMutex;
}

void PlatformWinMutex::Destroy(PlatformWinMutex *winMutex) {
    DeleteCriticalSection(winMutex->cs);
    delete winMutex->cs;
    delete winMutex;
}

void PlatformWinMutex::Lock(const PlatformWinMutex *winMutex) {
    // Spin first before entering critical section, causing ring-0 transition and context switch.
    if (TryEnterCriticalSection(winMutex->cs) == 0) {
        EnterCriticalSection(winMutex->cs);
    }
}

bool PlatformWinMutex::TryLock(const PlatformWinMutex *winMutex) {
    return TryEnterCriticalSection(winMutex->cs) ? true : false;
}

void PlatformWinMutex::Unlock(const PlatformWinMutex *winMutex) {
    LeaveCriticalSection(winMutex->cs);
}

PlatformWinCondition *PlatformWinCondition::Create() {
    PlatformWinCondition *winCondition = new PlatformWinCondition;
    winCondition->condVar = new CONDITION_VARIABLE;
    InitializeConditionVariable(winCondition->condVar);
    return winCondition;
}

void PlatformWinCondition::Destroy(PlatformWinCondition *winCondition) {
    assert(winCondition);
    delete winCondition->condVar;
    delete winCondition;
}

void PlatformWinCondition::Wait(const PlatformWinCondition *winCondition, const PlatformWinMutex *winMutex) {
    SleepConditionVariableCS(winCondition->condVar, winMutex->cs, INFINITE);
}

bool PlatformWinCondition::TimedWait(const PlatformWinCondition *winCondition, const PlatformWinMutex *winMutex, int ms) {
    return SleepConditionVariableCS(winCondition->condVar, winMutex->cs, (DWORD)ms) ? true : false;
}

void PlatformWinCondition::Signal(const PlatformWinCondition *winCondition) {
    WakeConditionVariable(winCondition->condVar);
}

void PlatformWinCondition::Broadcast(const PlatformWinCondition *winCondition) {
    WakeAllConditionVariable(winCondition->condVar);
}

BE_NAMESPACE_END
