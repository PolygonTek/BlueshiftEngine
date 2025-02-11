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

class ScopedLock {
public:
    ScopedLock(PlatformMutex *mu) : mutex(mu) {
        PlatformMutex::Lock(mutex);
    }
    ~ScopedLock() {
        PlatformMutex::Unlock(mutex);
    }
    ScopedLock() = delete;
    ScopedLock(const ScopedLock &) = delete;
    ScopedLock &operator=(const ScopedLock &) = delete;

private:
    PlatformMutex *mutex;
};

class ScopedReadLock {
public:
    ScopedReadLock(PlatformSRWLock *lock) : readLock(lock) {
        PlatformSRWLock::AquireReadLock(readLock);
    }
    ~ScopedReadLock() {
        PlatformSRWLock::ReleaseReadLock(readLock);
    }
    ScopedReadLock() = delete;
    ScopedReadLock(const ScopedReadLock &) = delete;
    ScopedReadLock &operator=(const ScopedReadLock &) = delete;

private:
    PlatformSRWLock *readLock;
};

class ScopedWriteLock {
public:
    ScopedWriteLock(PlatformSRWLock *lock) : writeLock(lock) {
        PlatformSRWLock::AquireWriteLock(writeLock);
    }
    ~ScopedWriteLock() {
        PlatformSRWLock::ReleaseWriteLock(writeLock);
    }
    ScopedWriteLock() = delete;
    ScopedWriteLock(const ScopedReadLock &) = delete;
    ScopedWriteLock &operator=(const ScopedReadLock &) = delete;

private:
    PlatformSRWLock *writeLock;
};

BE_NAMESPACE_END
