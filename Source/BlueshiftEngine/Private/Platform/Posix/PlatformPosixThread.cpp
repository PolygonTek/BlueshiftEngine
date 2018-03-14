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
    parg = NULL;

    SetAffinity(arg.affinity);
    arg.startProc(arg.param);

    return NULL;
}

PlatformPosixThread *PlatformPosixThread::Create(threadFunc_t startProc, void *param, size_t stackSize, int affinity) {
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

    if (pthread_create(tid, &attr, (void *(*)(void *))ThreadStartup, startup) != 0) {
        BE_FATALERROR(L"pthread_create");
    }
    
    PlatformPosixThread *posixThread = new PlatformPosixThread;
    posixThread->thread = tid;
    return posixThread;
}

void PlatformPosixThread::Delete(PlatformPosixThread *posixThread) {
    assert(posixThread);
    pthread_cancel(*posixThread->thread);
    delete posixThread->thread;
    delete posixThread;
}

void PlatformPosixThread::SetAffinity(int affinity) {
    BE1::SetAffinity(affinity);
}

void PlatformPosixThread::Wait(PlatformPosixThread *posixThread) {
    if (pthread_join(*posixThread->thread, NULL) != 0) {
        BE_FATALERROR(L"pthread_join");
    }
    delete posixThread->thread;
    delete posixThread;
}

void PlatformPosixThread::WaitAll(int numThreads, PlatformPosixThread *posixThreads[]) {
    for (int i = 0; i < numThreads; i++) {
        if (pthread_join(*posixThreads[i]->thread, NULL) != 0) {
            BE_FATALERROR(L"pthread_join");
        }

        delete posixThreads[i]->thread;
        delete posixThreads[i];
    } 
}

PlatformPosixMutex *PlatformPosixMutex::Create() {
    PlatformPosixMutex *posixMutex = new PlatformPosixMutex;
    posixMutex->mutex = new pthread_mutex_t;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(posixMutex->mutex, &attr);
    return posixMutex;
}

void PlatformPosixMutex::Delete(PlatformPosixMutex *posixMutex) {
    assert(posixMutex);
    pthread_mutex_destroy(posixMutex->mutex);
    delete posixMutex->mutex;
    delete posixMutex;
}

void PlatformPosixMutex::Lock(const PlatformPosixMutex *posixMutex) {
    pthread_mutex_lock(posixMutex->mutex);
}

bool PlatformPosixMutex::TryLock(const PlatformPosixMutex *posixMutex) {
    return pthread_mutex_trylock(posixMutex->mutex) == 0;
}

void PlatformPosixMutex::Unlock(const PlatformPosixMutex *posixMutex) {
    pthread_mutex_unlock(posixMutex->mutex);
}

PlatformPosixCondition *PlatformPosixCondition::Create() {
    PlatformPosixCondition *posixCondition = new PlatformPosixCondition;
    posixCondition->cond = new pthread_cond_t;
    pthread_cond_init(posixCondition->cond, NULL);
    return posixCondition;
}

void PlatformPosixCondition::Delete(PlatformPosixCondition *posixCondition) {
    assert(posixCondition);
    delete posixCondition->cond;
}

void PlatformPosixCondition::Wait(const PlatformPosixCondition *posixCondition, const PlatformPosixMutex *posixMutex) {
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

bool PlatformPosixCondition::TimedWait(const PlatformPosixCondition *posixCondition, const PlatformPosixMutex *posixMutex, int ms) {
    timespec ts;
    MillisecondsFromNow(&ts, ms);
    
    int ret = pthread_cond_timedwait(posixCondition->cond, posixMutex->mutex, &ts);
    if (ret == ETIMEDOUT || ret == EINVAL) {
        return false;
    }
    return true;
}

void PlatformPosixCondition::Broadcast(const PlatformPosixCondition *posixCondition) {
    pthread_cond_broadcast(posixCondition->cond);
}

BE_NAMESPACE_END
