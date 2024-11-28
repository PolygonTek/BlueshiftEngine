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

#include <pthread.h>

class BE_API PlatformPosixThread : public PlatformBaseThread {
public:
    static PlatformPosixThread *Start(threadFunc_t startProc, void *param, size_t stackSize = 0,
        ThreadPriority::Enum priority = ThreadPriority::Enum::Normal, uint64_t affinityMask = 0xFFFFFFFFFFFFFFFF);
    static void                 Terminate(PlatformPosixThread *thread);

    static void                 Detach(PlatformPosixThread *thread);

    static void                 Join(PlatformPosixThread *thread);
    static void                 JoinAll(int numThreads, PlatformPosixThread *threads[]);

    static uint64_t             GetCurrentThreadId();
    static void                 SetCurrentThreadName(const char *name);
    static void                 SetCurrentThreadAffinityMask(uint64_t affinityMask);
    static void                 SetCurrentThreadPriority(ThreadPriority::Enum priority);
    static void                 Yield();

private:
    pthread_t                   thread;
};

class BE_API PlatformPosixMutex : public PlatformBaseMutex {
    friend class PlatformPosixCondition;
    
public:
    static PlatformPosixMutex * Create();
    static void                 Destroy(PlatformPosixMutex *mutex);
    
    static void                 Lock(PlatformPosixMutex *mutex);
    static bool                 TryLock(PlatformPosixMutex *mutex);
    static void                 Unlock(PlatformPosixMutex *mutex);

private:
    pthread_mutex_t             mutex;
};

class BE_API PlatformPosixSRWLock : public PlatformBaseSRWLock {
    friend class PlatformPosixCondition;

public:
    static PlatformPosixSRWLock *Create();
    static void                 Destroy(PlatformPosixSRWLock *lock);

    static void                 AquireReadLock(PlatformPosixSRWLock *lock);
    static bool                 TryAquireReadLock(PlatformPosixSRWLock *lock);
    static void                 ReleaseReadLock(PlatformPosixSRWLock *lock);

    static void                 AquireWriteLock(PlatformPosixSRWLock *lock);
    static bool                 TryAquireWriteLock(PlatformPosixSRWLock *lock);
    static void                 ReleaseWriteLock(PlatformPosixSRWLock *lock);

private:
    pthread_rwlock_t            lock;
    pthread_mutex_t             mutex;
};

class BE_API PlatformPosixCondition : public PlatformBaseCondition {
public:
    static PlatformPosixCondition *Create();
    static void                 Destroy(PlatformPosixCondition *condition);
    
                                /// Release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(PlatformPosixCondition *condition, PlatformPosixMutex *mutex);
    static bool                 TimedWait(PlatformPosixCondition *condition, PlatformPosixMutex *mutex, int ms);

    template <typename Predicate>
    static void                 Wait(PlatformPosixCondition *condition, PlatformPosixMutex *mutex, Predicate &&waitFinishCondition);
    template <typename Predicate>
    static bool                 TimedWait(PlatformPosixCondition *condition, PlatformPosixMutex *mutex, int ms, Predicate &&waitFinishCondition);

                                /// Release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(PlatformPosixCondition *condition, PlatformPosixSRWLock *lock);
    static bool                 TimedWait(PlatformPosixCondition *condition, PlatformPosixSRWLock *lock, int ms);

    template <typename Predicate>
    static void                 Wait(PlatformPosixCondition *condition, PlatformPosixSRWLock *lock, Predicate &&waitFinishCondition);
    template <typename Predicate>
    static bool                 TimedWait(PlatformPosixCondition *condition, PlatformPosixSRWLock *lock, int ms, Predicate &&waitFinishCondition);
    
                                /// If any threads are waiting on condition, wake up one of them. Caller must hold lock, which must be the same as the lock used in the wait call.
    static void                 Signal(PlatformPosixCondition *condition);
    
                                /// Same as signal, except wake up all waiting threads.
    static void                 Broadcast(PlatformPosixCondition *condition);
    
private:
    pthread_cond_t              cond;
};

template <typename Predicate>
BE_INLINE void PlatformPosixCondition::Wait(PlatformPosixCondition *condition, PlatformPosixMutex *mutex, Predicate &&waitFinishCondition) {
    while (!waitFinishCondition()) {
        PlatformPosixCondition::Wait(condition, mutex);
    }
}

template <typename Predicate>
BE_INLINE bool PlatformPosixCondition::TimedWait(PlatformPosixCondition *condition, PlatformPosixMutex *mutex, int ms, Predicate &&waitFinishCondition) {
    while (!waitFinishCondition()) {
        if (PlatformPosixCondition::TimedWait(condition, mutex, ms) == false) { // time-out
            return false;
        }
    }
    return true;
}

template <typename Predicate>
BE_INLINE void PlatformPosixCondition::Wait(PlatformPosixCondition *condition, PlatformPosixSRWLock *lock, Predicate &&waitFinishCondition) {
    while (!waitFinishCondition()) {
        PlatformPosixCondition::Wait(condition, lock);
    }
}

template <typename Predicate>
BE_INLINE bool PlatformPosixCondition::TimedWait(PlatformPosixCondition *condition, PlatformPosixSRWLock *lock, int ms, Predicate &&waitFinishCondition) {
    while (!waitFinishCondition()) {
        if (PlatformPosixCondition::TimedWait(condition, lock, ms) == false) { // time-out
            return false;
        }
    }
    return true;
}

#ifndef USE_BASE_PLATFORM_POSIX_THREAD
typedef PlatformPosixThread     PlatformThread;
typedef PlatformPosixMutex      PlatformMutex;
typedef PlatformPosixCondition  PlatformCondition;
#endif

BE_NAMESPACE_END
