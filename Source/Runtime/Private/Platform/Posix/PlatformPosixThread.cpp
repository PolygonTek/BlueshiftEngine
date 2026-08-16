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
#include <pthread.h>
#include <sched.h>
#include <sys/timeb.h>

BE_NAMESPACE_BEGIN

static int Posix_TranslateThreadPriority(ThreadPriority::Enum priority) {
    // 0 is the lowest, 31 is the highest possible priority for pthread
    switch (priority) {
    case ThreadPriority::Enum::Highest: return 30;
    case ThreadPriority::Enum::AboveNormal: return 25;
    case ThreadPriority::Enum::Normal: return 15;
    case ThreadPriority::Enum::BelowNormal: return 5;
    case ThreadPriority::Enum::Lowest: return 1;
    case ThreadPriority::Enum::SlightlyBelowNormal: return 14;
    default: BE_ERRLOG("Unknown priority passed to Posix_TranslateThreadPriority()"); return 15;
    }
}

static void Posix_SetThreadPriority(pthread_t tid, ThreadPriority::Enum priority) {
    struct sched_param sched;
    memset(&sched, 0, sizeof(sched_param));
    int policy = SCHED_RR;

    // Read the current policy
    pthread_getschedparam(tid, &policy, &sched);

    // set the priority appropriately
    sched.sched_priority = Posix_TranslateThreadPriority(priority);
    pthread_setschedparam(tid, policy, &sched);
}

struct ThreadStartupData {
    threadFunc_t startProc;
    void *param;
};

static void *ThreadStartup(ThreadStartupData *parg) {
    ThreadStartupData arg = *parg; 
    delete parg;
    parg = nullptr;

    arg.startProc(arg.param);

    return nullptr;
}

PlatformPosixThread *PlatformPosixThread::Start(threadFunc_t startProc, void *param, size_t stackSize, ThreadPriority::Enum priority) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stackSize > 0) {
        pthread_attr_setstacksize(&attr, stackSize);
    }

    pthread_t tid;
    ThreadStartupData *startup = BE_NEW ThreadStartupData;
    startup->startProc = startProc;
    startup->param = param;

    int err = pthread_create(&tid, &attr, (void *(*)(void *))ThreadStartup, startup);
    if (err != 0) {
        BE_FATALERROR("Failed to create pthread - %s", strerror(err));
    }

    Posix_SetThreadPriority(tid, priority);
    
    PlatformPosixThread *posixThread = BE_NEW PlatformPosixThread;
    posixThread->thread = tid;
    return posixThread;
}

void PlatformPosixThread::Terminate(PlatformPosixThread *posixThread) {
    assert(posixThread);
    pthread_cancel(posixThread->thread);
    delete posixThread;
}

uint64_t PlatformPosixThread::GetCurrentThreadId() {
    BE_FATALERROR("PlatformPosixThread::GetCurrentThreadId not implmeneted on this platform");
    return 0;
}

void PlatformPosixThread::SetCurrentThreadName(const char *name) {
    BE_FATALERROR("PlatformPosixThread::SetCurrentThreadName not implmeneted on this platform");
}

void PlatformPosixThread::SetCurrentThreadPriority(ThreadPriority::Enum priority) {
    Posix_SetThreadPriority(pthread_self(), priority);
}

void PlatformPosixThread::Yield() {
    pthread_yield();
}

void PlatformPosixThread::Detach(PlatformPosixThread *posixThread) {
    pthread_detach(posixThread->thread);
}

void PlatformPosixThread::Join(PlatformPosixThread *posixThread) {
    assert(posixThread);

    const int error = pthread_join(posixThread->thread, nullptr);
    if (error != 0) {
        BE_FATALERROR("Failed to join pthread - %s", strerror(error));
        return;
    }

    delete posixThread;
}

void PlatformPosixThread::JoinAll(int numThreads, PlatformPosixThread *posixThreads[]) {
    for (int i = 0; i < numThreads; ++i) {
        PlatformPosixThread *posixThread = posixThreads[i];
        assert(posixThread);

        const int error = pthread_join(posixThread->thread, nullptr);
        if (error != 0) {
            BE_FATALERROR("Failed to join pthread - %s", strerror(error));
            return;
        }

        delete posixThread;
    }
}

PlatformPosixMutex *PlatformPosixMutex::Create(int spinCount) {
    PlatformPosixMutex *posixMutex = BE_NEW PlatformPosixMutex;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&posixMutex->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    posixMutex->spinCount = spinCount;
    return posixMutex;
}

void PlatformPosixMutex::Destroy(PlatformPosixMutex *posixMutex) {
    assert(posixMutex);
    pthread_mutex_destroy(&posixMutex->mutex);
    delete posixMutex;
}

void PlatformPosixMutex::Lock(PlatformPosixMutex *posixMutex) {
    for (int i = 0; i < posixMutex->spinCount; ++i) {
        if (pthread_mutex_trylock(&posixMutex->mutex) == 0) {
            return;
        }
        std::this_thread::yield();
    }
    pthread_mutex_lock(&posixMutex->mutex);
}

bool PlatformPosixMutex::TryLock(PlatformPosixMutex *posixMutex) {
    return pthread_mutex_trylock(&posixMutex->mutex) == 0;
}

void PlatformPosixMutex::Unlock(PlatformPosixMutex *posixMutex) {
    pthread_mutex_unlock(&posixMutex->mutex);
}

PlatformPosixSRWLock *PlatformPosixSRWLock::Create() {
    PlatformPosixSRWLock *posixLock = BE_NEW PlatformPosixSRWLock;
    pthread_rwlock_init(&posixLock->srwLock, nullptr);
    return posixLock;
}

void PlatformPosixSRWLock::Destroy(PlatformPosixSRWLock *posixLock) {
    assert(posixLock);
    pthread_rwlock_destroy(&posixLock->srwLock);
    delete posixLock;
}

void PlatformPosixSRWLock::AquireReadLock(PlatformPosixSRWLock *posixLock) {
    pthread_rwlock_rdlock(&posixLock->srwLock);
}

bool PlatformPosixSRWLock::TryAquireReadLock(PlatformPosixSRWLock *posixLock) {
    return pthread_rwlock_tryrdlock(&posixLock->srwLock) == 0;
}

void PlatformPosixSRWLock::ReleaseReadLock(PlatformPosixSRWLock *posixLock) {
    pthread_rwlock_unlock(&posixLock->srwLock);
}

void PlatformPosixSRWLock::AquireWriteLock(PlatformPosixSRWLock *posixLock) {
    pthread_rwlock_wrlock(&posixLock->srwLock);
}

bool PlatformPosixSRWLock::TryAquireWriteLock(PlatformPosixSRWLock *posixLock) {
    return pthread_rwlock_trywrlock(&posixLock->srwLock) == 0;
}

void PlatformPosixSRWLock::ReleaseWriteLock(PlatformPosixSRWLock *posixLock) {
    pthread_rwlock_unlock(&posixLock->srwLock);
}

PlatformPosixCondition *PlatformPosixCondition::Create() {
    PlatformPosixCondition *posixCondition = BE_NEW PlatformPosixCondition;
    pthread_cond_init(&posixCondition->cond, nullptr);
    pthread_mutex_init(&posixCondition->waitMutex, nullptr);
    return posixCondition;
}

void PlatformPosixCondition::Destroy(PlatformPosixCondition *posixCondition) {
    assert(posixCondition);
    pthread_cond_destroy(&posixCondition->cond);
    pthread_mutex_destroy(&posixCondition->waitMutex);
    delete posixCondition;
}

void PlatformPosixCondition::Wait(PlatformPosixCondition *posixCondition, PlatformPosixMutex *posixMutex) {
    pthread_mutex_lock(&posixCondition->waitMutex);
    PlatformPosixMutex::Unlock(posixMutex);
    pthread_cond_wait(&posixCondition->cond, &posixCondition->waitMutex);
    pthread_mutex_unlock(&posixCondition->waitMutex);
    PlatformPosixMutex::Lock(posixMutex);
}

struct timespec *MillisecondsFromNow(struct timespec *time, int millisecs) {
    const int64_t NANOSEC_PER_MILLISEC = 1000000;
    const int64_t NANOSEC_PER_SEC = 1000000000;

    // get current system time and add millisecs
    timeb currSysTime;
    ftime(&currSysTime);

    int64_t secs = currSysTime.time;
    int64_t nanosecs = ((int64_t)currSysTime.millitm + millisecs) * NANOSEC_PER_MILLISEC;
    secs += nanosecs / NANOSEC_PER_SEC;
    nanosecs %= NANOSEC_PER_SEC;

    time->tv_nsec = (long)nanosecs;
    time->tv_sec = (long)secs;

    return time;
}

bool PlatformPosixCondition::TimedWait(PlatformPosixCondition *posixCondition, PlatformPosixMutex *posixMutex, int ms) {
    timespec ts;
    MillisecondsFromNow(&ts, ms);

    pthread_mutex_lock(&posixCondition->waitMutex);
    PlatformPosixMutex::Unlock(posixMutex);
    bool signaled = pthread_cond_timedwait(&posixCondition->cond, &posixCondition->waitMutex, &ts) == 0;
    pthread_mutex_unlock(&posixCondition->waitMutex);
    PlatformPosixMutex::Lock(posixMutex);
    return signaled;
}

void PlatformPosixCondition::Wait(PlatformPosixCondition *posixCondition, PlatformPosixSRWLock *posixLock, bool isWriteLock) {
    pthread_mutex_lock(&posixCondition->waitMutex);

    if (isWriteLock) {
        PlatformPosixSRWLock::ReleaseWriteLock(posixLock);
    } else {
        PlatformPosixSRWLock::ReleaseReadLock(posixLock);
    }

    pthread_cond_wait(&posixCondition->cond, &posixCondition->waitMutex);
    pthread_mutex_unlock(&posixCondition->waitMutex);

    if (isWriteLock) {
        PlatformPosixSRWLock::AquireWriteLock(posixLock);
    } else {
        PlatformPosixSRWLock::AquireReadLock(posixLock);
    }
}

bool PlatformPosixCondition::TimedWait(PlatformPosixCondition *posixCondition, PlatformPosixSRWLock *posixLock, bool isWriteLock, int ms) {
    timespec ts;
    MillisecondsFromNow(&ts, ms);

    pthread_mutex_lock(&posixCondition->waitMutex);

    if (isWriteLock) {
        PlatformPosixSRWLock::ReleaseWriteLock(posixLock);
    } else {
        PlatformPosixSRWLock::ReleaseReadLock(posixLock);
    }

    bool signaled = pthread_cond_timedwait(&posixCondition->cond, &posixCondition->waitMutex, &ts) == 0;
    pthread_mutex_unlock(&posixCondition->waitMutex);

    if (isWriteLock) {
        PlatformPosixSRWLock::AquireWriteLock(posixLock);
    } else {
        PlatformPosixSRWLock::AquireReadLock(posixLock);
    }
    return signaled;
}

void PlatformPosixCondition::Signal(PlatformPosixCondition *posixCondition) {
    pthread_mutex_lock(&posixCondition->waitMutex);
    pthread_cond_signal(&posixCondition->cond);
    pthread_mutex_unlock(&posixCondition->waitMutex);
}

void PlatformPosixCondition::Broadcast(PlatformPosixCondition *posixCondition) {
    pthread_mutex_lock(&posixCondition->waitMutex);
    pthread_cond_broadcast(&posixCondition->cond);
    pthread_mutex_unlock(&posixCondition->waitMutex);
}

BE_NAMESPACE_END
