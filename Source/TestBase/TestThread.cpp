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

#include "BlueshiftEngine.h"
#include "Test.h"

struct CounterTestContext {
    BE1::Mutex mutex;
    int count = 0;
};

struct CounterThreadParams {
    CounterTestContext *context;
    const char *threadName;
};

static unsigned int CounterFunc(void *param) {
    CounterThreadParams *params = static_cast<CounterThreadParams *>(param);
    CounterTestContext &context = *params->context;

    BE1::PlatformThread::SetCurrentThreadName(params->threadName);

    for (int i = 0; i < 10; i++) {
        {
            BE1::ScopedLock lock(context.mutex);
            context.count++;
            BE_LOG("TID: %i - counting %i\n", BE1::PlatformThread::GetCurrentThreadId(), context.count);
        }

        BE1::PlatformProcess::Sleep(0.001f);
    }
    return 0;
}

struct ConditionTestContext {
    BE1::Mutex mutex;
    BE1::Condition condition;
    int waitingThreadCount = 0;
    int releasedThreadCount = 0;
    bool releaseOne = false;
    bool releaseAll = false;
};

static unsigned int ConditionWaitFunc(void *param) {
    ConditionTestContext &context = *static_cast<ConditionTestContext *>(param);
    BE1::ScopedLock lock(context.mutex);

    context.waitingThreadCount++;
    context.condition.Broadcast();
    context.condition.Wait(lock, [&context] {
        return context.releaseOne || context.releaseAll;
    });

    if (context.releaseOne) {
        context.releaseOne = false;
    }
    context.releasedThreadCount++;
    context.condition.Broadcast();
    return 0;
}

struct RWLockConditionTestContext {
    BE1::RWLock rwLock;
    BE1::Condition condition;
    bool waiting = false;
    bool release = false;
};

static unsigned int RWLockConditionWaitFunc(void *param) {
    RWLockConditionTestContext &context = *static_cast<RWLockConditionTestContext *>(param);
    BE1::ScopedReadLock lock(context.rwLock);

    context.waiting = true;
    context.condition.Broadcast();
    context.condition.Wait(lock, [&context] {
        return context.release;
    });
    return 0;
}


#if defined(__WIN32__)
static unsigned int JoinAllThreadFunc(void *param) {
    BE1::PlatformProcess::Sleep(0.05f);
    static_cast<BE1::PlatformAtomic<int> *>(param)->Add(1);
    return 0;
}
#endif

void Test::TestThread() {
    CounterTestContext counterContext;
    CounterThreadParams counterParams[] = {
        { &counterContext, "Counter Thread 1" },
        { &counterContext, "Counter Thread 2" }
    };

    BE_ASSERT(counterContext.mutex.TryLock());
    counterContext.mutex.Unlock();

    BE1::Array<BE1::PlatformThread *> threads;
    threads.Append(BE1::PlatformThread::Start(CounterFunc, &counterParams[0]));
    threads.Append(BE1::PlatformThread::Start(CounterFunc, &counterParams[1]));

    BE1::PlatformThread::Join(threads[0]);
    BE1::PlatformThread::Join(threads[1]);
    BE_ASSERT(counterContext.count == 20);

    ConditionTestContext conditionContext;
    BE1::PlatformThread *conditionThreads[] = {
        BE1::PlatformThread::Start(ConditionWaitFunc, &conditionContext),
        BE1::PlatformThread::Start(ConditionWaitFunc, &conditionContext)
    };

    {
        BE1::ScopedLock lock(conditionContext.mutex);
        conditionContext.condition.Wait(lock, [&conditionContext] {
            return conditionContext.waitingThreadCount == COUNT_OF(conditionThreads);
        });

        conditionContext.releaseOne = true;
        conditionContext.condition.Signal();
        conditionContext.condition.Wait(lock, [&conditionContext] {
            return conditionContext.releasedThreadCount == 1;
        });

        conditionContext.releaseAll = true;
        conditionContext.condition.Broadcast();
        conditionContext.condition.Wait(lock, [&conditionContext] {
            return conditionContext.releasedThreadCount == COUNT_OF(conditionThreads);
        });
    }

    BE1::PlatformThread::Join(conditionThreads[0]);
    BE1::PlatformThread::Join(conditionThreads[1]);

    {
        BE1::ScopedLock lock(conditionContext.mutex);
        bool waitResult = conditionContext.condition.TimedWait(lock, 1, [] { return false; });
        BE_ASSERT(!waitResult);
    }

    RWLockConditionTestContext rwLockContext;
    bool readLocked = rwLockContext.rwLock.TryReadLock();
    BE_ASSERT(readLocked);
    if (readLocked) {
        rwLockContext.rwLock.ReadUnlock();
    }

    bool writeLocked = rwLockContext.rwLock.TryWriteLock();
    BE_ASSERT(writeLocked);
    if (writeLocked) {
        rwLockContext.rwLock.WriteUnlock();
    }

    BE1::PlatformThread *rwLockWaitThread = BE1::PlatformThread::Start(RWLockConditionWaitFunc, &rwLockContext);
    {
        BE1::ScopedWriteLock lock(rwLockContext.rwLock);
        rwLockContext.condition.Wait(lock, [&rwLockContext] {
            return rwLockContext.waiting;
        });

        rwLockContext.release = true;
        rwLockContext.condition.Signal();
    }
    BE1::PlatformThread::Join(rwLockWaitThread);

#if defined(__WIN32__)
    BE1::PlatformAtomic<int> joinedThreadCount(0);
    BE1::Array<BE1::PlatformThread *> joinAllThreads;
    joinAllThreads.Reserve(MAXIMUM_WAIT_OBJECTS + 1);

    for (int i = 0; i < MAXIMUM_WAIT_OBJECTS + 1; ++i) {
        joinAllThreads.Append(BE1::PlatformThread::Start(JoinAllThreadFunc, &joinedThreadCount));
    }

    BE1::PlatformThread::JoinAll(joinAllThreads.Count(), joinAllThreads.Ptr());
    BE_ASSERT(joinedThreadCount.GetValue() == joinAllThreads.Count());
#endif
}
