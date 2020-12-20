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

#include "Precompiled.h"
#include "Platform/PlatformThread.h"
#include <pthread.h>
#include <sched.h>
#include <sys/timeb.h>

BE_NAMESPACE_BEGIN

#if defined(__APPLE__)

#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#include <mach/mach_init.h>

static void SetAffinity(int affinity) {
    if (affinity >= 0) {
        thread_affinity_policy ap;
        ap.affinity_tag = affinity;
        if (thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY, (integer_t*)&ap, THREAD_AFFINITY_POLICY_COUNT) != KERN_SUCCESS) {
            std::cerr << "Thread: cannot set affinity" << std::endl;
        }
    }
}

#else

static void SetAffinity(int affinity) {
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(affinity, &cset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cset), &cset) != 0) {
        std::cerr << "Thread: cannot set affinity" << std::endl;
    }
}

#endif

struct ThreadStartupData {
    int affinity;
    threadFunc_t startProc;
    void *param;
};

static void *ThreadStartup(ThreadStartupData *parg) {
    ThreadStartupData arg = *parg; 
    delete parg;
    parg = nullptr;

    SetAffinity(arg.affinity);
    arg.startProc(arg.param);

    return nullptr;
}

PlatformBaseThread *PlatformPosixThread::Create(threadFunc_t startProc, void *param, size_t stackSize, int affinity) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stackSize > 0) {
        pthread_attr_setstacksize(&attr, stackSize);
    }

    pthread_t *tid = new pthread_t;
    ThreadStartupData *startup = new ThreadStartupData;
    startup->startProc = startProc;
    startup->param = param;
    startup->affinity = affinity;

    int err = pthread_create(tid, &attr, (void *(*)(void *))ThreadStartup, startup);
    if (err != 0) {
        BE_FATALERROR("Failed to create pthread - %s", strerror(err));
    }
    
    PlatformPosixThread *posixThread = new PlatformPosixThread;
    posixThread->thread = tid;
    return posixThread;
}

void PlatformPosixThread::Destroy(PlatformBaseThread *thread) {
    assert(thread);
    PlatformPosixThread *posixThread = static_cast<PlatformPosixThread *>(thread);
    pthread_join(*posixThread->thread, nullptr);
    delete posixThread->thread;
    delete posixThread;
}

void PlatformPosixThread::Cancel(PlatformBaseThread *thread) {
    assert(thread);
    PlatformPosixThread *posixThread = static_cast<PlatformPosixThread *>(thread);
    pthread_cancel(*posixThread->thread);
    delete posixThread->thread;
    delete posixThread;
}

static int TranslateThreadPriority(PlatformBaseThread::Priority::Enum priority) {
    // 0 is the lowest, 31 is the highest possible priority for pthread
    switch (priority) {
    case PlatformBaseThread::Priority::TimeCritical:
    case PlatformBaseThread::Priority::Highest:
        return 30;
    case PlatformBaseThread::Priority::AboveNormal:
        return 25;
    case PlatformBaseThread::Priority::Normal:
        return 15;
    case PlatformBaseThread::Priority::BelowNormal:
        return 5;
    case PlatformBaseThread::Priority::Lowest:
        return 1;
    default:
        return 15;
    }
}

void PlatformPosixThread::SetPriority(PlatformBaseThread *thread, PlatformBaseThread::Priority::Enum priority) {
    PlatformPosixThread *posixThread = static_cast<PlatformPosixThread *>(thread);

    struct sched_param sched;
    memset(&sched, 0, sizeof(sched));
    int32_t policy = SCHED_RR;

    // Read the current policy.
    pthread_getschedparam(*posixThread->thread, &policy, &sched);

    // Set the priority appropriately.
    sched.sched_priority = TranslateThreadPriority(priority);
    pthread_setschedparam(*posixThread->thread, policy, &sched);
}

void PlatformPosixThread::SetAffinity(int affinity) {
    BE1::SetAffinity(affinity);
}

void PlatformPosixThread::Yield() {
    pthread_yield();
}

void PlatformPosixThread::Join(PlatformBaseThread *thread) {
    PlatformPosixThread *posixThread = static_cast<PlatformPosixThread *>(thread);
    int err = pthread_join(*posixThread->thread, nullptr);
    if (err != 0) {
        BE_FATALERROR("Failed to joint pthread - %s", strerror(err));
    }
    delete posixThread->thread;
    delete posixThread;
}

void PlatformPosixThread::JoinAll(int numThreads, PlatformBaseThread *threads[]) {
    for (int i = 0; i < numThreads; i++) {
        PlatformPosixThread *posixThread = static_cast<PlatformPosixThread *>(threads[i]);
        int err = pthread_join(*posixThread->thread, nullptr);
        if (err != 0) {
            BE_FATALERROR("Failed to joint pthread - %s", strerror(err));
        }

        delete posixThread->thread;
        delete posixThread;
    } 
}

PlatformBaseMutex *PlatformPosixMutex::Create() {
    PlatformPosixMutex *posixMutex = new PlatformPosixMutex;
    posixMutex->mutex = new pthread_mutex_t;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(posixMutex->mutex, &attr);
    return posixMutex;
}

void PlatformPosixMutex::Destroy(PlatformBaseMutex *mutex) {
    assert(mutex);
    PlatformPosixMutex *posixMutex = static_cast<PlatformPosixMutex *>(mutex);
    pthread_mutex_destroy(posixMutex->mutex);
    delete posixMutex->mutex;
    delete posixMutex;
}

void PlatformPosixMutex::Lock(const PlatformBaseMutex *mutex) {
    const PlatformPosixMutex *posixMutex = static_cast<const PlatformPosixMutex *>(mutex);
    pthread_mutex_lock(posixMutex->mutex);
}

bool PlatformPosixMutex::TryLock(const PlatformBaseMutex *mutex) {
    const PlatformPosixMutex *posixMutex = static_cast<const PlatformPosixMutex *>(mutex);
    return pthread_mutex_trylock(posixMutex->mutex) == 0;
}

void PlatformPosixMutex::Unlock(const PlatformBaseMutex *mutex) {
    const PlatformPosixMutex *posixMutex = static_cast<const PlatformPosixMutex *>(mutex);
    pthread_mutex_unlock(posixMutex->mutex);
}

PlatformBaseCondition *PlatformPosixCondition::Create() {
    PlatformPosixCondition *posixCondition = new PlatformPosixCondition;
    posixCondition->cond = new pthread_cond_t;
    pthread_cond_init(posixCondition->cond, nullptr);
    return posixCondition;
}

void PlatformPosixCondition::Destroy(PlatformBaseCondition *condition) {
    const PlatformPosixCondition *posixCondition = static_cast<const PlatformPosixCondition *>(condition);
    assert(posixCondition);
    delete posixCondition->cond;
}

void PlatformPosixCondition::Wait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex) {
    const PlatformPosixCondition *posixCondition = static_cast<const PlatformPosixCondition *>(condition);
    const PlatformPosixMutex *posixMutex = static_cast<const PlatformPosixMutex *>(mutex);

    pthread_cond_wait(posixCondition->cond, posixMutex->mutex);
}

struct timespec *MillisecondsFromNow(struct timespec *time, int millisecs) {
    const int64_t NANOSEC_PER_MILLISEC = 1000000;
    const int64_t NANOSEC_PER_SEC = 1000000000;

    // get current system time and add millisecs
    timeb currSysTime;
    ftime(&currSysTime);

    int64_t secs = currSysTime.time;
    int64_t nanosecs = ((int64_t)(millisecs + currSysTime.millitm)) * NANOSEC_PER_MILLISEC;
    if (nanosecs >= NANOSEC_PER_SEC) {
        secs = secs + 1;
        nanosecs %= NANOSEC_PER_SEC;
    }

    time->tv_nsec = (long)nanosecs;
    time->tv_sec = (long)secs;

    return time;
}

bool PlatformPosixCondition::TimedWait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex, int ms) {
    timespec ts;
    MillisecondsFromNow(&ts, ms);

    const PlatformPosixCondition *posixCondition = static_cast<const PlatformPosixCondition *>(condition);
    const PlatformPosixMutex *posixMutex = static_cast<const PlatformPosixMutex *>(mutex);
    
    int ret = pthread_cond_timedwait(posixCondition->cond, posixMutex->mutex, &ts);
    if (ret == ETIMEDOUT || ret == EINVAL) {
        return false;
    }
    return true;
}

void PlatformPosixCondition::Signal(const PlatformBaseCondition *condition) {
    const PlatformPosixCondition *posixCondition = static_cast<const PlatformPosixCondition *>(condition);

    pthread_cond_signal(posixCondition->cond);
}

void PlatformPosixCondition::Broadcast(const PlatformBaseCondition *condition) {
    const PlatformPosixCondition *posixCondition = static_cast<const PlatformPosixCondition *>(condition);

    pthread_cond_broadcast(posixCondition->cond);
}

BE_NAMESPACE_END
