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
    parg = nullptr;

    SetAffinity(arg.affinity);
    arg.startProc(arg.param);

    return nullptr;
}

uint64_t PlatformAndroidThread::GetCurrentThreadId() {
    return static_cast<uint64_t>(gettid());
}

PlatformBaseThread *PlatformAndroidThread::Start(threadFunc_t startProc, void *param, size_t stackSize, int affinity) {
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
    
    PlatformAndroidThread *androidThread = new PlatformAndroidThread;
    androidThread->thread = tid;
    return androidThread;
}

void PlatformAndroidThread::Terminate(PlatformBaseThread *thread) {
    assert(thread);
    PlatformAndroidThread *androidThread = static_cast<PlatformAndroidThread *>(thread);
    //pthread_cancel(*androidThread->thread);
    delete androidThread->thread;
    delete androidThread;
}

void PlatformAndroidThread::SetAffinity(int affinity) {
    BE1::SetAffinity(affinity);
}

void PlatformAndroidThread::SetName(const char *name) {
    pthread_setname_np(pthread_self(), name);
}

void PlatformAndroidThread::Join(PlatformBaseThread *thread) {
    PlatformAndroidThread *androidThread = static_cast<PlatformAndroidThread *>(thread);
    int err = pthread_join(*androidThread->thread, nullptr);
    if (err != 0) {
        BE_FATALERROR("Failed to joint pthread - %s", strerror(err));
    }
    delete androidThread->thread;
    delete androidThread;
}

void PlatformAndroidThread::JoinAll(int numThreads, PlatformBaseThread *threads[]) {
    for (int i = 0; i < numThreads; i++) {
        PlatformAndroidThread *androidThread = static_cast<PlatformAndroidThread *>(threads[i]);
        int err = pthread_join(*androidThread->thread, nullptr);
        if (err != 0) {
            BE_FATALERROR("Failed to joint pthread - %s", strerror(err));
        }

        delete androidThread->thread;
        delete androidThread;
    } 
}

PlatformBaseMutex *PlatformAndroidMutex::Create() {
    PlatformAndroidMutex *androidMutex = new PlatformAndroidMutex;
    androidMutex->mutex = new pthread_mutex_t;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(androidMutex->mutex, &attr);
    return androidMutex;
}

void PlatformAndroidMutex::Destroy(PlatformBaseMutex *mutex) {
    assert(mutex);
    PlatformAndroidMutex *androidMutex = static_cast<PlatformAndroidMutex *>(mutex);
    pthread_mutex_destroy(androidMutex->mutex);
    delete androidMutex->mutex;
    delete androidMutex;
}

void PlatformAndroidMutex::Lock(const PlatformBaseMutex *mutex) {
    const PlatformAndroidMutex *androidMutex = static_cast<const PlatformAndroidMutex *>(mutex);

    pthread_mutex_lock(androidMutex->mutex);
}

bool PlatformAndroidMutex::TryLock(const PlatformBaseMutex *mutex) {
    const PlatformAndroidMutex *androidMutex = static_cast<const PlatformAndroidMutex *>(mutex);

    return pthread_mutex_trylock(androidMutex->mutex) == 0;
}

void PlatformAndroidMutex::Unlock(const PlatformBaseMutex *mutex) {
    const PlatformAndroidMutex *androidMutex = static_cast<const PlatformAndroidMutex *>(mutex);

    pthread_mutex_unlock(androidMutex->mutex);
}

PlatformBaseCondition *PlatformAndroidCondition::Create() {
    PlatformAndroidCondition *androidCondition = new PlatformAndroidCondition;
    androidCondition->cond = new pthread_cond_t;
    pthread_cond_init(androidCondition->cond, nullptr);
    return androidCondition;
}

void PlatformAndroidCondition::Destroy(PlatformBaseCondition *condition) {
    assert(condition);
    PlatformAndroidCondition *androidCondition = static_cast<PlatformAndroidCondition *>(condition);
    delete androidCondition->cond;
}

void PlatformAndroidCondition::Wait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex) {
    const PlatformAndroidCondition *androidCondition = static_cast<const PlatformAndroidCondition *>(condition);
    const PlatformAndroidMutex *androidMutex = static_cast<const PlatformAndroidMutex *>(mutex);

    pthread_cond_wait(androidCondition->cond, androidMutex->mutex);
}

struct timespec *MillisecondsFromNow(struct timespec *time, int millisecs) {
    const int64_t NANOSEC_PER_MILLISEC = 1000000;
    const int64_t NANOSEC_PER_SEC = 1000000000;

    struct timeval tvstruct;
    gettimeofday(&tvstruct, nullptr);

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

bool PlatformAndroidCondition::TimedWait(const PlatformBaseCondition *condition, const PlatformBaseMutex *mutex, int ms) {
    timespec ts;
    MillisecondsFromNow(&ts, ms);

    const PlatformAndroidCondition *androidCondition = static_cast<const PlatformAndroidCondition *>(condition);
    const PlatformAndroidMutex *androidMutex = static_cast<const PlatformAndroidMutex *>(mutex);

    int ret = pthread_cond_timedwait(androidCondition->cond, androidMutex->mutex, &ts);
    if (ret == ETIMEDOUT || ret == EINVAL) {
        return false;
    }
    return true;
}

void PlatformAndroidCondition::Broadcast(const PlatformBaseCondition *condition) {
    const PlatformAndroidCondition *androidCondition = static_cast<const PlatformAndroidCondition *>(condition);

    pthread_cond_broadcast(androidCondition->cond);
}

BE_NAMESPACE_END
