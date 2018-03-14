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
#include <unistd.h>
#include <sys/syscall.h>

BE_NAMESPACE_BEGIN

static void SetAffinity(int affinity) {
    pid_t pid = gettid();
    long syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(affinity), &affinity);
    if (syscallres) {
        int err = errno;
        std::cerr << "Thread: cannot set affinity" << std::endl;
    }
}

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

PlatformAndroidThread *PlatformAndroidThread::Create(threadFunc_t startProc, void *param, size_t stackSize, int affinity) {
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
    
    PlatformAndroidThread *androidThread = new PlatformAndroidThread;
    androidThread->thread = tid;
    return androidThread;
}

void PlatformAndroidThread::Delete(PlatformAndroidThread *androidThread) {
    assert(androidThread);
    //pthread_cancel(*androidThread->thread);
    delete androidThread->thread;
    delete androidThread;
}

void PlatformAndroidThread::SetAffinity(int affinity) {
    BE1::SetAffinity(affinity);
}

void PlatformAndroidThread::Wait(PlatformAndroidThread *androidThread) {
    if (pthread_join(*androidThread->thread, NULL) != 0) {
        BE_FATALERROR(L"pthread_join");
    }
    delete androidThread->thread;
    delete androidThread;
}

void PlatformAndroidThread::WaitAll(int numThreads, PlatformAndroidThread *androidThreads[]) {
    for (int i = 0; i < numThreads; i++) {
        if (pthread_join(*androidThreads[i]->thread, NULL) != 0) {
            BE_FATALERROR(L"pthread_join");
        }

        delete androidThreads[i]->thread;
        delete androidThreads[i];
    } 
}

PlatformAndroidMutex *PlatformAndroidMutex::Create() {
    PlatformAndroidMutex *androidMutex = new PlatformAndroidMutex;
    androidMutex->mutex = new pthread_mutex_t;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(androidMutex->mutex, &attr);
    return androidMutex;
}

void PlatformAndroidMutex::Delete(PlatformAndroidMutex *androidMutex) {
    assert(androidMutex);
    pthread_mutex_destroy(androidMutex->mutex);
    delete androidMutex->mutex;
    delete androidMutex;
}

void PlatformAndroidMutex::Lock(const PlatformAndroidMutex *androidMutex) {
    pthread_mutex_lock(androidMutex->mutex);
}

bool PlatformAndroidMutex::TryLock(const PlatformAndroidMutex *androidMutex) {
    return pthread_mutex_trylock(androidMutex->mutex) == 0;
}

void PlatformAndroidMutex::Unlock(const PlatformAndroidMutex *androidMutex) {
    pthread_mutex_unlock(androidMutex->mutex);
}

PlatformAndroidCondition *PlatformAndroidCondition::Create() {
    PlatformAndroidCondition *androidCondition = new PlatformAndroidCondition;
    androidCondition->cond = new pthread_cond_t;
    pthread_cond_init(androidCondition->cond, NULL);
    return androidCondition;
}

void PlatformAndroidCondition::Delete(PlatformAndroidCondition *androidCondition) {
    assert(androidCondition);
    delete androidCondition->cond;
}

void PlatformAndroidCondition::Wait(const PlatformAndroidCondition *androidCondition, const PlatformAndroidMutex *androidMutex) {
    pthread_cond_wait(androidCondition->cond, androidMutex->mutex);
}

struct timespec *MillisecondsFromNow(struct timespec *time, int millisecs) {
    const int64_t NANOSEC_PER_MILLISEC = 1000000;
    const int64_t NANOSEC_PER_SEC = 1000000000;

    struct timeval tvstruct;
    gettimeofday(&tvstruct, NULL);

    int64_t secs = tvstruct.tv_sec;
    int64_t nanosecs = ((int64_t)(millisecs + tvstruct.tv_usec * 1000)) * NANOSEC_PER_MILLISEC;
    if (nanosecs >= NANOSEC_PER_SEC) {
        secs = secs + 1;
        nanosecs %= NANOSEC_PER_SEC;
    }

    time->tv_nsec = (long)nanosecs;
    time->tv_sec = (long)secs;

    return time;
}

bool PlatformAndroidCondition::TimedWait(const PlatformAndroidCondition *androidCondition, const PlatformAndroidMutex *androidMutex, int ms) {
    timespec ts;
    MillisecondsFromNow(&ts, ms);
    
    int ret = pthread_cond_timedwait(androidCondition->cond, androidMutex->mutex, &ts);
    if (ret == ETIMEDOUT || ret == EINVAL) {
        return false;
    }
    return true;
}

void PlatformAndroidCondition::Broadcast(const PlatformAndroidCondition *androidCondition) {
    pthread_cond_broadcast(androidCondition->cond);
}

BE_NAMESPACE_END
