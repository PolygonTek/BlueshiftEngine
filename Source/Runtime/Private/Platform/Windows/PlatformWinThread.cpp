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
#include "Platform/Windows/PlatformWinThread.h"
#include <process.h>

BE_NAMESPACE_BEGIN

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

// Creates a hardware thread.
PlatformWinThread *PlatformWinThread::Start(threadFunc_t startProc, void *param, size_t stackSize, ThreadPriority::Enum priority) {
    HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, stackSize, startProc, param, 0, nullptr);
    //HANDLE threadHandle = CreateThread(nullptr, stackSize, (LPTHREAD_START_ROUTINE)startProc, param, 0, nullptr);
    if (threadHandle == nullptr) {
        BE_FATALERROR("Failed to create Win32 thread");
        return nullptr;
    }

    Win_SetThreadPriority(threadHandle, priority);

    PlatformWinThread *winThread = BE_NEW PlatformWinThread;
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
    if (numThreads <= 0) {
        return;
    }

    static constexpr int MaxWaitHandles = MAXIMUM_WAIT_OBJECTS;
    HANDLE threadHandles[MaxWaitHandles];

    for (int baseIndex = 0; baseIndex < numThreads; baseIndex += MaxWaitHandles) {
        const int batchCount = Min(MaxWaitHandles, numThreads - baseIndex);

        for (int batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
            threadHandles[batchIndex] = winThreads[baseIndex + batchIndex]->threadHandle;
        }

        const DWORD waitResult = WaitForMultipleObjects(batchCount, threadHandles, TRUE, INFINITE);
        if (waitResult == WAIT_FAILED) {
            BE_FATALERROR("PlatformWinThread::JoinAll: WaitForMultipleObjects failed (%u)", static_cast<unsigned int>(GetLastError()));
        }
    }
    
    for (int i = 0; i < numThreads; i++) {
        PlatformWinThread *winThread = winThreads[i];
        CloseHandle(winThread->threadHandle);
        delete winThread;
    }
}

PlatformWinMutex *PlatformWinMutex::Create(int spinCount) {
    PlatformWinMutex *winMutex = BE_NEW PlatformWinMutex;
    InitializeCriticalSectionAndSpinCount(&winMutex->cs, spinCount);
    return winMutex;
}

void PlatformWinMutex::Destroy(PlatformWinMutex *winMutex) {
    DeleteCriticalSection(&winMutex->cs);
    delete winMutex;
}

void PlatformWinMutex::Lock(PlatformWinMutex *winMutex) {
    // Spin first before entering critical section, causing ring-0 transition and context switch.
    if (TryEnterCriticalSection(&winMutex->cs) == 0) {
        EnterCriticalSection(&winMutex->cs);
    }
}

bool PlatformWinMutex::TryLock(PlatformWinMutex *winMutex) {
    return (bool)TryEnterCriticalSection(&winMutex->cs);
}

void PlatformWinMutex::Unlock(PlatformWinMutex *winMutex) {
    LeaveCriticalSection(&winMutex->cs);
}

PlatformWinSRWLock *PlatformWinSRWLock::Create() {
    PlatformWinSRWLock *winLock = BE_NEW PlatformWinSRWLock;
    InitializeSRWLock(&winLock->srwLock);
    return winLock;
}

void PlatformWinSRWLock::Destroy(PlatformWinSRWLock *winLock) {
    delete winLock;
}

void PlatformWinSRWLock::AquireReadLock(PlatformWinSRWLock *lock) {
    AcquireSRWLockShared(&lock->srwLock);
}

bool PlatformWinSRWLock::TryAquireReadLock(PlatformWinSRWLock *lock) {
    return (bool)TryAcquireSRWLockShared(&lock->srwLock);
}

void PlatformWinSRWLock::ReleaseReadLock(PlatformWinSRWLock *lock) {
    ReleaseSRWLockShared(&lock->srwLock);
}

void PlatformWinSRWLock::AquireWriteLock(PlatformWinSRWLock *lock) {
    AcquireSRWLockExclusive(&lock->srwLock);
}

bool PlatformWinSRWLock::TryAquireWriteLock(PlatformWinSRWLock *lock) {
    return (bool)TryAcquireSRWLockExclusive(&lock->srwLock);
}

void PlatformWinSRWLock::ReleaseWriteLock(PlatformWinSRWLock *lock) {
    ReleaseSRWLockExclusive(&lock->srwLock);
}

PlatformWinCondition *PlatformWinCondition::Create() {
    PlatformWinCondition *winCondition = BE_NEW PlatformWinCondition;
    InitializeConditionVariable(&winCondition->condVar);
    return winCondition;
}

void PlatformWinCondition::Destroy(PlatformWinCondition *winCondition) {
    assert(winCondition);
    delete winCondition;
}

void PlatformWinCondition::Wait(PlatformWinCondition *winCondition, PlatformWinMutex *winMutex) {
    SleepConditionVariableCS(&winCondition->condVar, &winMutex->cs, INFINITE);
}

bool PlatformWinCondition::TimedWait(PlatformWinCondition *winCondition, PlatformWinMutex *winMutex, int ms) {
    return SleepConditionVariableCS(&winCondition->condVar, &winMutex->cs, (DWORD)ms) ? true : false;
}

void PlatformWinCondition::Wait(PlatformWinCondition *winCondition, PlatformWinSRWLock *winLock, bool isWriteLock) {
    SleepConditionVariableSRW(&winCondition->condVar, &winLock->srwLock, INFINITE, isWriteLock ? 0 : CONDITION_VARIABLE_LOCKMODE_SHARED);
}

bool PlatformWinCondition::TimedWait(PlatformWinCondition *winCondition, PlatformWinSRWLock *winLock, bool isWriteLock, int ms) {
    return SleepConditionVariableSRW(&winCondition->condVar, &winLock->srwLock, (DWORD)ms, isWriteLock ? 0 : CONDITION_VARIABLE_LOCKMODE_SHARED) ? true : false;
}

void PlatformWinCondition::Signal(PlatformWinCondition *winCondition) {
    WakeConditionVariable(&winCondition->condVar);
}

void PlatformWinCondition::Broadcast(PlatformWinCondition *winCondition) {
    WakeAllConditionVariable(&winCondition->condVar);
}

BE_NAMESPACE_END
