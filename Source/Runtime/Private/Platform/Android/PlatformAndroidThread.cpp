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
#include <unistd.h>

BE_NAMESPACE_BEGIN

struct ThreadStartupData {
    threadFunc_t startProc;
    void *param;
};

static void *ThreadStartup(void *param) {
    ThreadStartupData *startup = static_cast<ThreadStartupData *>(param);
    ThreadStartupData arg = *startup;
    delete startup;

    arg.startProc(arg.param);

    return nullptr;
}

uint64_t PlatformAndroidThread::GetCurrentThreadId() {
    return static_cast<uint64_t>(gettid());
}

static int TranslateThreadPriority(ThreadPriority::Enum priority) {
    // 0 is the lowest, 31 is the highest possible priority for pthread
    switch (priority) {
    case ThreadPriority::Highest:
        return 30;
    case ThreadPriority::AboveNormal:
        return 25;
    case ThreadPriority::Normal:
        return 15;
    case ThreadPriority::SlightlyBelowNormal:
        return 15;
    case ThreadPriority::BelowNormal:
        return 5;
    case ThreadPriority::Lowest:
        return 1;
    default:
        return 15;
    }
}

static void Android_SetThreadPriority(const pthread_t *pthread, ThreadPriority::Enum priority) {
    struct sched_param sched;
    memset(&sched, 0, sizeof(sched));
    int32_t policy = SCHED_RR;

    // Read the current policy.
    pthread_getschedparam(*pthread, &policy, &sched);

    // Set the priority appropriately.
    sched.sched_priority = TranslateThreadPriority(priority);
    pthread_setschedparam(*pthread, policy, &sched);
}

PlatformAndroidThread *PlatformAndroidThread::Start(threadFunc_t startProc, void *param, size_t stackSize, ThreadPriority::Enum priority) {
    pthread_attr_t attr;
    int err = pthread_attr_init(&attr);
    if (err != 0) {
        BE_FATALERROR("Failed to initialize pthread attributes - %s", strerror(err));
        return nullptr;
    }

    if (stackSize > 0) {
        err = pthread_attr_setstacksize(&attr, stackSize);
        if (err != 0) {
            pthread_attr_destroy(&attr);
            BE_FATALERROR("Failed to set pthread stack size - %s", strerror(err));
            return nullptr;
        }
    }

    pthread_t tid;
    ThreadStartupData *startup = BE_NEW ThreadStartupData;
    startup->startProc = startProc;
    startup->param = param;

    err = pthread_create(&tid, &attr, ThreadStartup, startup);
    pthread_attr_destroy(&attr);

    if (err != 0) {
        delete startup;
        BE_FATALERROR("Failed to create pthread - %s", strerror(err));
        return nullptr;
    }

    Android_SetThreadPriority(&tid, priority);
    
    PlatformAndroidThread *androidThread = BE_NEW PlatformAndroidThread;
    androidThread->thread = tid;
    return androidThread;
}

void PlatformAndroidThread::Terminate(PlatformAndroidThread *androidThread) {
    assert(androidThread);
    pthread_kill(androidThread->thread, SIGUSR1);
    delete androidThread;
}

void PlatformAndroidThread::SetCurrentThreadName(const char *name) {
    pthread_setname_np(pthread_self(), name);
}

void PlatformAndroidThread::Yield() {
    sched_yield();
}

void PlatformAndroidThread::Detach(PlatformAndroidThread *androidThread) {
    pthread_detach(androidThread->thread);
}

void PlatformAndroidThread::Join(PlatformAndroidThread *androidThread) {
    assert(androidThread);

    const int error = pthread_join(androidThread->thread, nullptr);
    if (error != 0) {
        BE_FATALERROR("Failed to join pthread - %s", strerror(error));
        return;
    }

    delete androidThread;
}

void PlatformAndroidThread::JoinAll(int numThreads, PlatformAndroidThread *androidThreads[]) {
    for (int i = 0; i < numThreads; ++i) {
        PlatformAndroidThread *androidThread = androidThreads[i];
        assert(androidThread);

        const int error = pthread_join(androidThread->thread, nullptr);
        if (error != 0) {
            BE_FATALERROR("Failed to join pthread - %s", strerror(error));
            return;
        }

        delete androidThread;
    }
}

PlatformAndroidMutex *PlatformAndroidMutex::Create(int spinCount) {
    PlatformAndroidMutex *androidMutex = BE_NEW PlatformAndroidMutex;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&androidMutex->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    androidMutex->spinCount = spinCount;
    return androidMutex;
}

void PlatformAndroidMutex::Destroy(PlatformAndroidMutex *androidMutex) {
    assert(androidMutex);
    pthread_mutex_destroy(&androidMutex->mutex);
    delete androidMutex;
}

void PlatformAndroidMutex::Lock(PlatformAndroidMutex *androidMutex) {
    for (int i = 0; i < androidMutex->spinCount; ++i) {
        if (pthread_mutex_trylock(&androidMutex->mutex) == 0) {
            return;
        }
        std::this_thread::yield();
    }
    pthread_mutex_lock(&androidMutex->mutex);
}

bool PlatformAndroidMutex::TryLock(PlatformAndroidMutex *androidMutex) {
    return pthread_mutex_trylock(&androidMutex->mutex) == 0;
}

void PlatformAndroidMutex::Unlock(PlatformAndroidMutex *androidMutex) {
    pthread_mutex_unlock(&androidMutex->mutex);
}

PlatformAndroidSRWLock *PlatformAndroidSRWLock::Create() {
    PlatformAndroidSRWLock *androidLock = BE_NEW PlatformAndroidSRWLock;
    pthread_rwlock_init(&androidLock->srwLock, nullptr);
    return androidLock;
}

void PlatformAndroidSRWLock::Destroy(PlatformAndroidSRWLock *androidLock) {
    assert(androidLock);
    pthread_rwlock_destroy(&androidLock->srwLock);
    delete androidLock;
}

void PlatformAndroidSRWLock::AquireReadLock(PlatformAndroidSRWLock *androidLock) {
    pthread_rwlock_rdlock(&androidLock->srwLock);
}

bool PlatformAndroidSRWLock::TryAquireReadLock(PlatformAndroidSRWLock *androidLock) {
    return pthread_rwlock_tryrdlock(&androidLock->srwLock) == 0;
}

void PlatformAndroidSRWLock::ReleaseReadLock(PlatformAndroidSRWLock *androidLock) {
    pthread_rwlock_unlock(&androidLock->srwLock);
}

void PlatformAndroidSRWLock::AquireWriteLock(PlatformAndroidSRWLock *androidLock) {
    pthread_rwlock_wrlock(&androidLock->srwLock);
}

bool PlatformAndroidSRWLock::TryAquireWriteLock(PlatformAndroidSRWLock *androidLock) {
    return pthread_rwlock_trywrlock(&androidLock->srwLock) == 0;
}

void PlatformAndroidSRWLock::ReleaseWriteLock(PlatformAndroidSRWLock *androidLock) {
    pthread_rwlock_unlock(&androidLock->srwLock);
}

PlatformAndroidCondition *PlatformAndroidCondition::Create() {
    PlatformAndroidCondition *androidCondition = BE_NEW PlatformAndroidCondition;
    pthread_cond_init(&androidCondition->cond, nullptr);
    pthread_mutex_init(&androidCondition->waitMutex, nullptr);
    return androidCondition;
}

void PlatformAndroidCondition::Destroy(PlatformAndroidCondition *androidCondition) {
    assert(androidCondition);
    pthread_cond_destroy(&androidCondition->cond);
    pthread_mutex_destroy(&androidCondition->waitMutex);
    delete androidCondition;
}

void PlatformAndroidCondition::Wait(PlatformAndroidCondition *androidCondition, PlatformAndroidMutex *androidMutex) {
    pthread_mutex_lock(&androidCondition->waitMutex);
    PlatformAndroidMutex::Unlock(androidMutex);
    pthread_cond_wait(&androidCondition->cond, &androidCondition->waitMutex);
    pthread_mutex_unlock(&androidCondition->waitMutex);
    PlatformAndroidMutex::Lock(androidMutex);
}

struct timespec *MillisecondsFromNow(struct timespec *time, int millisecs) {
    const int64_t NANOSEC_PER_MILLISEC = 1000000;
    const int64_t NANOSEC_PER_SEC = 1000000000;

    struct timeval tvstruct;
    gettimeofday(&tvstruct, nullptr);

    int64_t secs = tvstruct.tv_sec;
    int64_t nanosecs = (int64_t)tvstruct.tv_usec * 1000 + (int64_t)millisecs * NANOSEC_PER_MILLISEC;
    secs += nanosecs / NANOSEC_PER_SEC;
    nanosecs %= NANOSEC_PER_SEC;

    time->tv_nsec = (long)nanosecs;
    time->tv_sec = (long)secs;

    return time;
}

bool PlatformAndroidCondition::TimedWait(PlatformAndroidCondition *androidCondition, PlatformAndroidMutex *androidMutex, int ms) {
    timespec ts;
    MillisecondsFromNow(&ts, ms);

    pthread_mutex_lock(&androidCondition->waitMutex);
    PlatformAndroidMutex::Unlock(androidMutex);
    bool signaled = pthread_cond_timedwait(&androidCondition->cond, &androidCondition->waitMutex, &ts) == 0;
    pthread_mutex_unlock(&androidCondition->waitMutex);
    PlatformAndroidMutex::Lock(androidMutex);
    return signaled;
}

void PlatformAndroidCondition::Wait(PlatformAndroidCondition *androidCondition, PlatformAndroidSRWLock *androidLock, bool isWriteLock) {
    pthread_mutex_lock(&androidCondition->waitMutex);

    if (isWriteLock) {
        PlatformAndroidSRWLock::ReleaseWriteLock(androidLock);
    } else {
        PlatformAndroidSRWLock::ReleaseReadLock(androidLock);
    }

    pthread_cond_wait(&androidCondition->cond, &androidCondition->waitMutex);
    pthread_mutex_unlock(&androidCondition->waitMutex);

    if (isWriteLock) {
        PlatformAndroidSRWLock::AquireWriteLock(androidLock);
    } else {
        PlatformAndroidSRWLock::AquireReadLock(androidLock);
    }
}

bool PlatformAndroidCondition::TimedWait(PlatformAndroidCondition *androidCondition, PlatformAndroidSRWLock *androidLock, bool isWriteLock, int ms) {
    timespec ts;
    MillisecondsFromNow(&ts, ms);

    pthread_mutex_lock(&androidCondition->waitMutex);

    if (isWriteLock) {
        PlatformAndroidSRWLock::ReleaseWriteLock(androidLock);
    } else {
        PlatformAndroidSRWLock::ReleaseReadLock(androidLock);
    }

    bool signaled = pthread_cond_timedwait(&androidCondition->cond, &androidCondition->waitMutex, &ts) == 0;
    pthread_mutex_unlock(&androidCondition->waitMutex);

    if (isWriteLock) {
        PlatformAndroidSRWLock::AquireWriteLock(androidLock);
    } else {
        PlatformAndroidSRWLock::AquireReadLock(androidLock);
    }
    return signaled;
}

void PlatformAndroidCondition::Signal(PlatformAndroidCondition *androidCondition) {
    pthread_mutex_lock(&androidCondition->waitMutex);
    pthread_cond_signal(&androidCondition->cond);
    pthread_mutex_unlock(&androidCondition->waitMutex);
}

void PlatformAndroidCondition::Broadcast(PlatformAndroidCondition *androidCondition) {
    pthread_mutex_lock(&androidCondition->waitMutex);
    pthread_cond_broadcast(&androidCondition->cond);
    pthread_mutex_unlock(&androidCondition->waitMutex);
}

BE_NAMESPACE_END
