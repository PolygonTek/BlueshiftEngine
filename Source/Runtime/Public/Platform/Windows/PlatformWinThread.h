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

#pragma once

BE_NAMESPACE_BEGIN

class BE_API PlatformWinThread : public PlatformBaseThread {
public:
    static PlatformWinThread *  Start(threadFunc_t startProc, void *param, size_t stackSize = 0,
        ThreadPriority::Enum priority = ThreadPriority::Enum::Normal, uint64_t affinityMask = 0xFFFFFFFFFFFFFFFF);
    static void                 Terminate(PlatformWinThread *thread);

    static void                 Detach(PlatformWinThread *thread);

    static void                 Join(PlatformWinThread *thread);
    static void                 JoinAll(int numThreads, PlatformWinThread *threads[]);

    static uint64_t             GetCurrentThreadId();
    static void                 SetCurrentThreadName(const char *name);
    static void                 SetCurrentThreadAffinityMask(uint64_t affinityMask);
    static void                 SetCurrentThreadPriority(ThreadPriority::Enum priority);
    static void                 Yield();
    
private:
    HANDLE                      threadHandle;
};

class BE_API PlatformWinMutex : public PlatformBaseMutex {
    friend class PlatformWinCondition;

public:
    static PlatformWinMutex *   Create(int spinCount = 4000);
    static void                 Destroy(PlatformWinMutex *mutex);

    static void                 Lock(PlatformWinMutex *mutex);
    static bool                 TryLock(PlatformWinMutex *mutex);
    static void                 Unlock(PlatformWinMutex *mutex);

private:
    CRITICAL_SECTION            cs;
};

class BE_API PlatformWinSRWLock : public PlatformBaseSRWLock {
    friend class PlatformWinCondition;

public:
    static PlatformWinSRWLock * Create();
    static void                 Destroy(PlatformWinSRWLock *lock);

    static void                 AquireReadLock(PlatformWinSRWLock *lock);
    static bool                 TryAquireReadLock(PlatformWinSRWLock *lock);
    static void                 ReleaseReadLock(PlatformWinSRWLock *lock);

    static void                 AquireWriteLock(PlatformWinSRWLock *lock);
    static bool                 TryAquireWriteLock(PlatformWinSRWLock *lock);
    static void                 ReleaseWriteLock(PlatformWinSRWLock *lock);

private:
    SRWLOCK                     srwLock;
};

class BE_API PlatformWinCondition : public PlatformBaseCondition {
public:
    static PlatformWinCondition *Create();
    static void                 Destroy(PlatformWinCondition *condition);

                                /// Release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(PlatformWinCondition *condition, PlatformWinMutex *mutex);
    static bool                 TimedWait(PlatformWinCondition *condition, PlatformWinMutex *mutex, int ms);

    template <typename Predicate>
    static void                 Wait(PlatformWinCondition *condition, PlatformWinMutex *mutex, Predicate &&waitFinishCondition);
    template <typename Predicate>
    static bool                 TimedWait(PlatformWinCondition *condition, PlatformWinMutex *mutex, int ms, Predicate &&waitFinishCondition);

                                /// Release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(PlatformWinCondition *condition, PlatformWinSRWLock *lock);
    static bool                 TimedWait(PlatformWinCondition *condition, PlatformWinSRWLock *lock, int ms);

    template <typename Predicate>
    static void                 Wait(PlatformWinCondition *condition, PlatformWinSRWLock *lock, Predicate &&waitFinishCondition);
    template <typename Predicate>
    static bool                 TimedWait(PlatformWinCondition *condition, PlatformWinSRWLock *lock, int ms, Predicate &&waitFinishCondition);

                                /// If any threads are waiting on condition, wake up one of them. Caller must hold lock, which must be the same as the lock used in the wait call.
    static void                 Signal(PlatformWinCondition *condition);

                                /// Same as signal, except wake up all waiting threads.
    static void                 Broadcast(PlatformWinCondition *condition);
    
private:
    CONDITION_VARIABLE          condVar;
};

template <typename Predicate>
BE_INLINE void PlatformWinCondition::Wait(PlatformWinCondition *condition, PlatformWinMutex *mutex, Predicate &&waitFinishCondition) {
    while (!waitFinishCondition()) {
        PlatformWinCondition::Wait(condition, mutex);
    }
}

template <typename Predicate>
BE_INLINE bool PlatformWinCondition::TimedWait(PlatformWinCondition *condition, PlatformWinMutex *mutex, int ms, Predicate &&waitFinishCondition) {
    while (!waitFinishCondition()) {
        if (PlatformWinCondition::TimedWait(condition, mutex, ms) == false) { // time-out
            return false;
        }
    }
    return true;
}

template <typename Predicate>
BE_INLINE void PlatformWinCondition::Wait(PlatformWinCondition *condition, PlatformWinSRWLock *lock, Predicate &&waitFinishCondition) {
    while (!waitFinishCondition()) {
        PlatformWinCondition::Wait(condition, lock);
    }
}

template <typename Predicate>
BE_INLINE bool PlatformWinCondition::TimedWait(PlatformWinCondition *condition, PlatformWinSRWLock *lock, int ms, Predicate &&waitFinishCondition) {
    while (!waitFinishCondition()) {
        if (PlatformWinCondition::TimedWait(condition, lock, ms) == false) { // time-out
            return false;
        }
    }
    return true;
}

typedef PlatformWinThread       PlatformThread;
typedef PlatformWinMutex        PlatformMutex;
typedef PlatformWinCondition    PlatformCondition;

BE_NAMESPACE_END
