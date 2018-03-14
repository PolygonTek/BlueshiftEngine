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

typedef void (*threadFunc_t)(void *);

class BE_API PlatformBaseThread {
public:
    static PlatformBaseThread * Create(threadFunc_t startProc, void *param, size_t stackSize = 0, int affinity = -1);
    static void                 Delete(PlatformBaseThread *thread);

    static void                 SetAffinity(int affinity);

    static void                 Wait(PlatformBaseThread *thread);
    static void                 WaitAll(int numThreads, PlatformBaseThread *threads[]);
};

class BE_API PlatformBaseMutex {
public:
    static PlatformBaseMutex *  Create();
    static void                 Delete(PlatformBaseMutex *mutex);
    
    static void                 Lock(const PlatformBaseMutex *mutex);
    static bool                 TryLock(const PlatformBaseMutex *mutex);
    static void                 Unlock(const PlatformBaseMutex *mutex);
};

class BE_API PlatformBaseCondition {
public:
    static PlatformBaseCondition *Create();
    static void                 Delete(PlatformBaseCondition *condition);
    
                                /// Release lock, put thread to sleep until condition is signaled; when thread wakes up again, re-acquire lock before returning.
    static void                 Wait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex);
    static bool                 TimedWait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex, int ms);
    
                                /// If any threads are waiting on condition, wake up one of them. Caller must hold lock, which must be the same as the lock used in the wait call.
    static void                 Signal(const PlatformBaseCondition *condition);
    
                                /// Same as signal, except wake up all waiting threads
    static void                 Broadcast(const PlatformBaseCondition *condition);
};

BE_NAMESPACE_END

#if defined(__ANDROID__)
#include "Android/PlatformAndroidThread.h"
#elif defined(__UNIX__) 
#include "Posix/PlatformPosixThread.h"
#elif defined(__WIN32__)
#include "Windows/PlatformWinThread.h"
#endif
