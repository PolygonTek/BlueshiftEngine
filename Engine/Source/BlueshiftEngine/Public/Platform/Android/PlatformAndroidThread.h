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

class BE_API PlatformUnixThread : public PlatformBaseThread {
public:
    static PlatformUnixThread *Create(threadFunc_t startProc, void *param, size_t stackSize = 0, int affinity = -1);
    static void                 Delete(PlatformUnixThread *thread);
    
    static void                 SetAffinity(int affinity);
    
    static void                 Wait(PlatformUnixThread *thread);
    static void                 WaitAll(int numThreads, PlatformUnixThread *threads[]);
    
private:
    pthread_t *                 thread;
};

class BE_API PlatformUnixMutex : public PlatformBaseMutex {
    friend class PlatformUnixCondition;
    
public:
    static PlatformUnixMutex * Create();
    static void                 Delete(PlatformUnixMutex *mutex);
    
    static void                 Lock(const PlatformUnixMutex *mutex);
    static bool                 TryLock(const PlatformUnixMutex *mutex);
    static void                 Unlock(const PlatformUnixMutex *mutex);

private:
    pthread_mutex_t *           mutex;
};

class BE_API PlatformUnixCondition : public PlatformBaseCondition {
public:
    static PlatformUnixCondition *Create();
    static void                 Delete(PlatformUnixCondition *condition);
    
    // release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(const PlatformUnixCondition *condition, const PlatformUnixMutex *mutex);
    static bool                 TimedWait(const PlatformUnixCondition *condition, const PlatformUnixMutex *mutex, int ms);
    
    // if any threads are waiting on condition, wake up one of them. Caller must hold lock, which must be the same as the lock used in the wait call.
    static void                 Signal(const PlatformUnixCondition *condition);
    
    // same as signal, except wake up all waiting threads
    static void                 Broadcast(const PlatformUnixCondition *condition);
    
private:
    pthread_cond_t *            cond;
};

#ifndef USE_BASE_PLATFORM_POSIX_THREAD
typedef PlatformUnixThread     PlatformThread;
typedef PlatformUnixMutex      PlatformMutex;
typedef PlatformUnixCondition  PlatformCondition;
#endif

BE_NAMESPACE_END