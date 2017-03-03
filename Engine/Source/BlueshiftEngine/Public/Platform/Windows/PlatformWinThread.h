#pragma once

BE_NAMESPACE_BEGIN

class BE_API PlatformWinThread : public PlatformBaseThread {
public:
    static PlatformWinThread *  Create(threadFunc_t startProc, void *param, size_t stackSize = 0, int affinity = -1);
    static void                 Delete(PlatformWinThread *thread);

    static void                 SetAffinity(int affinity);

    static void                 Wait(PlatformWinThread *thread);
    static void                 WaitAll(int numThreads, PlatformWinThread *threads);
    
private:
    HANDLE                      threadHandle;
};

class BE_API PlatformWinMutex : public PlatformBaseMutex {
    friend class PlatformWinCondition;

public:
    static PlatformWinMutex *   Create();
    static void                 Delete(PlatformWinMutex *mutex);

    static void                 Lock(const PlatformWinMutex *mutex);
    static bool                 TryLock(const PlatformWinMutex *mutex);
    static void                 Unlock(const PlatformWinMutex *mutex);
    
private:
    PCRITICAL_SECTION           cs;
};

class BE_API PlatformWinCondition : public PlatformBaseCondition {
public:
    static PlatformWinCondition *Create();
    static void                 Delete(PlatformWinCondition *condition);
    
                                /// Release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(const PlatformWinCondition *condition, const PlatformWinMutex *mutex);
    static bool                 TimedWait(const PlatformWinCondition *condition, const PlatformWinMutex *mutex, int ms);
    
                                /// If any threads are waiting on condition, wake up one of them. Caller must hold lock, which must be the same as the lock used in the wait call.
    static void                 Signal(const PlatformWinCondition *condition);
    
                                /// Same as signal, except wake up all waiting threads
    static void                 Broadcast(const PlatformWinCondition *condition);
    
private:
    PCONDITION_VARIABLE         condVar;
};

typedef PlatformWinThread       PlatformThread;
typedef PlatformWinMutex        PlatformMutex;
typedef PlatformWinCondition    PlatformCondition;

BE_NAMESPACE_END
