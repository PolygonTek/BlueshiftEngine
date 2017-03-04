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

/*
-------------------------------------------------------------------------------

    StrPool

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"
#include "Containers/Array.h"
#include "Containers/HashIndex.h"

BE_NAMESPACE_BEGIN

class StrPool;

class BE_API PoolStr : public Str {
    friend class StrPool;

public:
    PoolStr() { refCount = 0; }
    ~PoolStr() { assert(refCount == 0); }

                        /// Returns total size of allocated memory.
    size_t              Allocated() const { return Str::Allocated(); }

                        /// Returns total size of allocated memory including size of this type.
    size_t              Size() const { return sizeof(*this) + Allocated(); }

                        /// Returns a pointer to the pool this string was allocated from
    const StrPool *     GetPool() const { return pool; }

private:
    StrPool *           pool;       ///< pool pointer
    mutable int         refCount;   ///< reference count
};

class BE_API StrPool {
public:
    StrPool();

    void                SetCaseSensitive(bool caseSensitive) { this->caseSensitive = caseSensitive; }

                        /// Returns number of strings
    int                 Count() const { return pool.Count(); }

                        /// Returns total size of allocated memory
    size_t              Allocated() const;

                        /// Returns total size of allocated memory including size of this type
    size_t              Size() const;

    const PoolStr *     operator[](int index) const { return pool[index]; }

                        // pool 에서 찾아서 reference count 를 증가시킨다. 없으면 string 을 새로 할당한다.
    const PoolStr *     AllocString(const char *string);

                        // reference count 를 감소 시킨다. reference count <= 0 이면 pool 에서 삭제한다.
    void                ReleaseString(const PoolStr *poolStr);

                        // 같은 pool 에 존재하는 string 이면 reference count 만 증가시킨다, 없으면 새로 할당한다.
    const PoolStr *     CopyString(const PoolStr *poolStr);

                        // pool 에 있는 모든 string 삭제, pool 초기화
    void                Clear();

private:
    bool                caseSensitive;
    Array<PoolStr *>    pool;
    HashIndex           poolHash;
};

BE_INLINE StrPool::StrPool() {
    caseSensitive = true;
}

BE_INLINE size_t StrPool::Allocated() const {
    size_t size = pool.Allocated() + poolHash.Allocated();
    for (int i = 0; i < pool.Count(); i++) {
        size += pool[i]->Allocated();
    }

    return size;
}

BE_INLINE size_t StrPool::Size() const {
    size_t size = pool.Size() + poolHash.Size();
    for (int i = 0; i < pool.Count(); i++) {
        size += pool[i]->Size();
    }

    return size;
}

BE_INLINE const PoolStr *StrPool::AllocString(const char *string) {
    int hash = poolHash.GenerateHash(string, caseSensitive);
    if (caseSensitive) {
        for (int i = poolHash.First(hash); i != -1; i = poolHash.Next(i)) {
            if (pool[i]->Cmp(string) == 0) {
                pool[i]->refCount++;
                return pool[i];
            }
        }
    } else {
        for (int i = poolHash.First(hash); i != -1; i = poolHash.Next(i)) {
            if (pool[i]->Icmp(string) == 0) {
                pool[i]->refCount++;
                return pool[i];
            }
        }
    }
    
    PoolStr *poolStr = new PoolStr;
    *static_cast<Str *>(poolStr) = string;
    poolStr->pool = this;
    poolStr->refCount = 1;
    poolHash.Add(hash, pool.Append(poolStr));
    return poolStr;
}

BE_INLINE void StrPool::ReleaseString(const PoolStr *poolStr) {
    assert(poolStr->refCount >= 1);
    assert(poolStr->pool == this);

    poolStr->refCount--;

    if (poolStr->refCount <= 0) {
        int hash = poolHash.GenerateHash(poolStr->c_str(), caseSensitive);
        int i;
        if (caseSensitive) { 
            for (i = poolHash.First(hash); i != -1; i = poolHash.Next(i)) {
                if (!pool[i]->Cmp(poolStr->c_str())) {
                    break;
                }
            }
        } else {
            for (i = poolHash.First(hash); i != -1; i = poolHash.Next(i)) {
                if (!pool[i]->Icmp(poolStr->c_str())) {
                    break;
                }
            }
        }

        assert(i != -1);
        assert(pool[i] == poolStr);

        delete pool[i];
        pool.RemoveIndex(i);
        poolHash.RemoveIndex(hash, i);
    }
}

BE_INLINE const PoolStr *StrPool::CopyString(const PoolStr *poolStr) {
    assert(poolStr->refCount >= 1);

    if (poolStr->pool == this) {
        // the string is from this pool so just increase the user count
        poolStr->refCount++;
        return poolStr;
    } else {
        // the string is from another pool so it needs to be re-allocated from this pool.
        return AllocString(poolStr->c_str());
    }
}

BE_INLINE void StrPool::Clear() {
    for (int i = 0; i < pool.Count(); i++) {
        pool[i]->refCount = 0;
    }
    
    pool.DeleteContents(true);
    poolHash.Free();
}

BE_NAMESPACE_END
