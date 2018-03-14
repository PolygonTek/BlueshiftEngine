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

class BE_API PlatformAndroidThread : public PlatformBaseThread {
public:
    static PlatformAndroidThread *Create(threadFunc_t startProc, void *param, size_t stackSize = 0, int affinity = -1);
    static void                 Delete(PlatformAndroidThread *thread);
    
    static void                 SetAffinity(int affinity);
    
    static void                 Wait(PlatformAndroidThread *thread);
    static void                 WaitAll(int numThreads, PlatformAndroidThread *threads[]);
    
private:
    pthread_t *                 thread;
};

class BE_API PlatformAndroidMutex : public PlatformBaseMutex {
    friend class PlatformAndroidCondition;
    
public:
    static PlatformAndroidMutex * Create();
    static void                 Delete(PlatformAndroidMutex *mutex);
    
    static void                 Lock(const PlatformAndroidMutex *mutex);
    static bool                 TryLock(const PlatformAndroidMutex *mutex);
    static void                 Unlock(const PlatformAndroidMutex *mutex);

private:
    pthread_mutex_t *           mutex;
};

class BE_API PlatformAndroidCondition : public PlatformBaseCondition {
public:
    static PlatformAndroidCondition *Create();
    static void                 Delete(PlatformAndroidCondition *condition);
    
    // release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(const PlatformAndroidCondition *condition, const PlatformAndroidMutex *mutex);
    static bool                 TimedWait(const PlatformAndroidCondition *condition, const PlatformAndroidMutex *mutex, int ms);
    
    // if any threads are waiting on condition, wake up one of them. Caller must hold lock, which must be the same as the lock used in the wait call.
    static void                 Signal(const PlatformAndroidCondition *condition);
    
    // same as signal, except wake up all waiting threads
    static void                 Broadcast(const PlatformAndroidCondition *condition);
    
private:
    pthread_cond_t *            cond;
};

typedef PlatformAndroidThread   PlatformThread;
typedef PlatformAndroidMutex    PlatformMutex;
typedef PlatformAndroidCondition PlatformCondition;

BE_NAMESPACE_END