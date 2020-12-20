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
    static uint64_t             GetCurrentThreadId();

    static PlatformBaseThread *  Create(threadFunc_t startProc, void *param, size_t stackSize = 0, int affinity = -1);
    static void                 Destroy(PlatformBaseThread *thread);

    static void                 SetName(const char *name);
    static void                 SetAffinity(int affinity);

    static void                 SetPriority(PlatformBaseThread *thread, Priority::Enum priority);

    static void                 Join(PlatformBaseThread *thread);
    static void                 JoinAll(int numThreads, PlatformBaseThread *threads[]);
    
private:
    HANDLE                      threadHandle;
};

class BE_API PlatformWinMutex : public PlatformBaseMutex {
    friend class PlatformWinCondition;

public:
    static PlatformBaseMutex *   Create();
    static void                 Destroy(PlatformBaseMutex *mutex);

    static void                 Lock(const PlatformBaseMutex *mutex);
    static bool                 TryLock(const PlatformBaseMutex *mutex);
    static void                 Unlock(const PlatformBaseMutex *mutex);
    
private:
    PCRITICAL_SECTION           cs;
};

class BE_API PlatformWinCondition : public PlatformBaseCondition {
public:
    static PlatformBaseCondition *Create();
    static void                 Destroy(PlatformBaseCondition *condition);
    
                                /// Release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex);
                                /// Returns false when the time-out interval elapses.
    static bool                 TimedWait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex, int ms);
    
                                /// If any threads are waiting on condition, wake up one of them. Caller must hold lock, which must be the same as the lock used in the wait call.
    static void                 Signal(const PlatformBaseCondition *condition);
    
                                /// Same as signal, except wake up all waiting threads.
    static void                 Broadcast(const PlatformBaseCondition *condition);
    
private:
    PCONDITION_VARIABLE         condVar;
};

typedef PlatformWinThread       PlatformThread;
typedef PlatformWinMutex        PlatformMutex;
typedef PlatformWinCondition    PlatformCondition;

BE_NAMESPACE_END
