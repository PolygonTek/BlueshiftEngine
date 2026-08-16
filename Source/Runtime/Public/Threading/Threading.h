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

#pragma once

#include "Platform/PlatformThread.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Mutex

-------------------------------------------------------------------------------
*/

class BE_API Mutex {
public:
    explicit Mutex(int spinCount = 4000);
    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;
    Mutex(Mutex &&) = delete;
    Mutex &operator=(Mutex &&) = delete;
    ~Mutex();

    void                    Lock() { PlatformMutex::Lock(platformMutex); }
    [[nodiscard]] bool      TryLock() { return PlatformMutex::TryLock(platformMutex); }
    void                    Unlock() { PlatformMutex::Unlock(platformMutex); }

                            /// Returns the underlying platform mutex for interoperability with platform synchronization primitives.
    PlatformMutex *         GetPlatformMutex() { return platformMutex; }

private:
    PlatformMutex *         platformMutex;
};

/*
-------------------------------------------------------------------------------

    ScopedLock

-------------------------------------------------------------------------------
*/

class BE_API ScopedLock {
    friend class Condition;

public:
    ScopedLock(Mutex &mu) : ScopedLock(mu.GetPlatformMutex()) {}
    ScopedLock(PlatformMutex *mu) : mutex(mu) { PlatformMutex::Lock(mutex); }
    ~ScopedLock() { PlatformMutex::Unlock(mutex); }
    ScopedLock() = delete;
    ScopedLock(const ScopedLock &) = delete;
    ScopedLock &operator=(const ScopedLock &) = delete;

private:
    PlatformMutex *         mutex;
};

/*
-------------------------------------------------------------------------------

    RWLock

-------------------------------------------------------------------------------
*/

class BE_API RWLock {
public:
    RWLock();
    RWLock(const RWLock &) = delete;
    RWLock &operator=(const RWLock &) = delete;
    RWLock(RWLock &&) = delete;
    RWLock &operator=(RWLock &&) = delete;
    ~RWLock();

    void                    ReadLock() { PlatformSRWLock::AquireReadLock(platformLock); }
    [[nodiscard]] bool      TryReadLock() { return PlatformSRWLock::TryAquireReadLock(platformLock); }
    void                    ReadUnlock() { PlatformSRWLock::ReleaseReadLock(platformLock); }

    void                    WriteLock() { PlatformSRWLock::AquireWriteLock(platformLock); }
    [[nodiscard]] bool      TryWriteLock() { return PlatformSRWLock::TryAquireWriteLock(platformLock); }
    void                    WriteUnlock() { PlatformSRWLock::ReleaseWriteLock(platformLock); }

                            /// Returns the underlying platform read-write lock for interoperability with platform synchronization primitives.
    PlatformSRWLock *       GetPlatformSRWLock() { return platformLock; }

private:
    PlatformSRWLock *       platformLock;
};

/*
-------------------------------------------------------------------------------

    ScopedReadLock, ScopedWriteLock

-------------------------------------------------------------------------------
*/

class BE_API ScopedReadLock {
    friend class Condition;

public:
    ScopedReadLock(RWLock &lock) : ScopedReadLock(lock.GetPlatformSRWLock()) {}
    ScopedReadLock(PlatformSRWLock *lock) : readLock(lock) { PlatformSRWLock::AquireReadLock(readLock); }
    ~ScopedReadLock() { PlatformSRWLock::ReleaseReadLock(readLock); }
    ScopedReadLock() = delete;
    ScopedReadLock(const ScopedReadLock &) = delete;
    ScopedReadLock &operator=(const ScopedReadLock &) = delete;

private:
    PlatformSRWLock *       readLock;
};

class BE_API ScopedWriteLock {
    friend class Condition;

public:
    ScopedWriteLock(RWLock &lock) : ScopedWriteLock(lock.GetPlatformSRWLock()) {}
    ScopedWriteLock(PlatformSRWLock *lock) : writeLock(lock) { PlatformSRWLock::AquireWriteLock(writeLock); }
    ~ScopedWriteLock() { PlatformSRWLock::ReleaseWriteLock(writeLock); }
    ScopedWriteLock() = delete;
    ScopedWriteLock(const ScopedWriteLock &) = delete;
    ScopedWriteLock &operator=(const ScopedWriteLock &) = delete;

private:
    PlatformSRWLock *       writeLock;
};

/*
-------------------------------------------------------------------------------

    Condition

-------------------------------------------------------------------------------
*/

class BE_API Condition {
public:
    Condition();
    Condition(const Condition &) = delete;
    Condition &operator=(const Condition &) = delete;
    Condition(Condition &&) = delete;
    Condition &operator=(Condition &&) = delete;
    ~Condition();

                            /// Atomically releases the lock and waits until signaled, then re-acquires the lock before returning.
    void                    Wait(ScopedLock &lock) { PlatformCondition::Wait(platformCondition, lock.mutex); }
    bool                    TimedWait(ScopedLock &lock, int milliseconds) { return PlatformCondition::TimedWait(platformCondition, lock.mutex, milliseconds); }

    void                    Wait(ScopedReadLock &lock) { PlatformCondition::Wait(platformCondition, lock.readLock, false); }
    bool                    TimedWait(ScopedReadLock &lock, int milliseconds) { return PlatformCondition::TimedWait(platformCondition, lock.readLock, false, milliseconds); }

    void                    Wait(ScopedWriteLock &lock) { PlatformCondition::Wait(platformCondition, lock.writeLock, true); }
    bool                    TimedWait(ScopedWriteLock &lock, int milliseconds) { return PlatformCondition::TimedWait(platformCondition, lock.writeLock, true, milliseconds); }

    template <typename Predicate>
    void                    Wait(ScopedLock &lock, Predicate &&waitFinishCondition);
    template <typename Predicate>
    bool                    TimedWait(ScopedLock &lock, int milliseconds, Predicate &&waitFinishCondition);

    template <typename Predicate>
    void                    Wait(ScopedReadLock &lock, Predicate &&waitFinishCondition);
    template <typename Predicate>
    bool                    TimedWait(ScopedReadLock &lock, int milliseconds, Predicate &&waitFinishCondition);

    template <typename Predicate>
    void                    Wait(ScopedWriteLock &lock, Predicate &&waitFinishCondition);
    template <typename Predicate>
    bool                    TimedWait(ScopedWriteLock &lock, int milliseconds, Predicate &&waitFinishCondition);

                            /// Wakes one waiting thread. The caller must hold the lock used by the waiter.
    void                    Signal() { PlatformCondition::Signal(platformCondition); }
                            /// Wakes all waiting threads. The caller must hold the lock used by the waiters.
    void                    Broadcast() { PlatformCondition::Broadcast(platformCondition); }

private:
    PlatformCondition *     platformCondition;
};

template <typename Predicate>
BE_INLINE void Condition::Wait(ScopedLock &lock, Predicate &&waitFinishCondition) {
    PlatformCondition::Wait(platformCondition, lock.mutex, waitFinishCondition);
}

template <typename Predicate>
BE_INLINE bool Condition::TimedWait(ScopedLock &lock, int milliseconds, Predicate &&waitFinishCondition) {
    return PlatformCondition::TimedWait(platformCondition, lock.mutex, milliseconds, waitFinishCondition);
}

template <typename Predicate>
BE_INLINE void Condition::Wait(ScopedReadLock &lock, Predicate &&waitFinishCondition) {
    PlatformCondition::Wait(platformCondition, lock.readLock, false, waitFinishCondition);
}

template <typename Predicate>
BE_INLINE bool Condition::TimedWait(ScopedReadLock &lock, int milliseconds, Predicate &&waitFinishCondition) {
    return PlatformCondition::TimedWait(platformCondition, lock.readLock, false, milliseconds, waitFinishCondition);
}

template <typename Predicate>
BE_INLINE void Condition::Wait(ScopedWriteLock &lock, Predicate &&waitFinishCondition) {
    PlatformCondition::Wait(platformCondition, lock.writeLock, true, waitFinishCondition);
}

template <typename Predicate>
BE_INLINE bool Condition::TimedWait(ScopedWriteLock &lock, int milliseconds, Predicate &&waitFinishCondition) {
    return PlatformCondition::TimedWait(platformCondition, lock.writeLock, true, milliseconds, waitFinishCondition);
}

BE_NAMESPACE_END
