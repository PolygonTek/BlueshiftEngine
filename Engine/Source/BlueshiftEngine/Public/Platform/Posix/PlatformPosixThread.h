#pragma once

BE_NAMESPACE_BEGIN

#include <pthread.h>

class BE_API PlatformPosixThread : public PlatformBaseThread {
public:
    static PlatformPosixThread *Create(threadFunc_t startProc, void *param, size_t stackSize = 0, int affinity = -1);
    static void                 Delete(PlatformPosixThread *thread);
    
    static void                 SetAffinity(int affinity);
    
    static void                 Wait(PlatformPosixThread *thread);
    static void                 WaitAll(int numThreads, PlatformPosixThread *threads[]);
    
private:
    pthread_t *                 thread;
};

class BE_API PlatformPosixMutex : public PlatformBaseMutex {
    friend class PlatformPosixCondition;
    
public:
    static PlatformPosixMutex * Create();
    static void                 Delete(PlatformPosixMutex *mutex);
    
    static void                 Lock(const PlatformPosixMutex *mutex);
    static bool                 TryLock(const PlatformPosixMutex *mutex);
    static void                 Unlock(const PlatformPosixMutex *mutex);

private:
    pthread_mutex_t *           mutex;
};

class BE_API PlatformPosixCondition : public PlatformBaseCondition {
public:
    static PlatformPosixCondition *Create();
    static void                 Delete(PlatformPosixCondition *condition);
    
    // release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(const PlatformPosixCondition *condition, const PlatformPosixMutex *mutex);
    static bool                 TimedWait(const PlatformPosixCondition *condition, const PlatformPosixMutex *mutex, int ms);
    
    // if any threads are waiting on condition, wake up one of them. Caller must hold lock, which must be the same as the lock used in the wait call.
    static void                 Signal(const PlatformPosixCondition *condition);
    
    // same as signal, except wake up all waiting threads
    static void                 Broadcast(const PlatformPosixCondition *condition);
    
private:
    pthread_cond_t *            cond;
};

#ifndef USE_BASE_PLATFORM_POSIX_THREAD
typedef PlatformPosixThread     PlatformThread;
typedef PlatformPosixMutex      PlatformMutex;
typedef PlatformPosixCondition  PlatformCondition;
#endif

BE_NAMESPACE_END