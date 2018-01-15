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

    Hash table

    Similar with STL hash_map/unorderd_map
    The key should be unique

    NOTE:
    - 검색/삭제가 HashMap 에 비해 빠르다.
    - bucket list 는 오름차순 정렬된 링크드 리스트로 구현되어있다.

-------------------------------------------------------------------------------
*/

#include "Core/Heap.h"
#include "Core/Allocator.h"
#include "Core/Guid.h"
#include "Core/Str.h"
#include "Core/WStr.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

// HashBucket class template
template <typename KeyT, class ValueT> 
class HashBucket {
public:
    HashBucket() {}
    HashBucket(const KeyT &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {
    }

    static int GenerateHash(const KeyT &key, const int tableMask) {
        return ((int)key) & tableMask;
    }

    static int Compare(const KeyT &key1, const KeyT &key2) {
        if (key1 < key2) {
            return -1;
        } else if (key1 > key2) {
            return 1;
        }
        return 0;
    }

public:
    KeyT                    key;
    ValueT                  value;
    HashBucket<KeyT, ValueT> *next;
};

// HashBucket class template specialization with hash key type is given Guid
template <typename ValueT>
class HashBucket<Guid, ValueT> {
public:
    HashBucket() {}
    HashBucket(const Guid &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {}

    static int GenerateHash(const Guid &key, const int tableMask) {
        return Guid::Hash(key) & tableMask;
    }

    static int Compare(const Guid &key1, const Guid &key2) {
        if (key1 < key2) {
            return -1;
        } else if (key1 > key2) {
            return 1;
        }
        return 0;
    }

public:
    Guid                    key;
    ValueT                  value;
    HashBucket<Guid, ValueT> *next;
};

// HashBucket class template specialization with hash key type is given Str
template <typename ValueT>
class HashBucket<Str, ValueT> {
public:
    HashBucket() {}
    HashBucket(const Str &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {}

    static int GenerateHash(const Str &key, const int tableMask) {
        return Str::Hash(key) & tableMask;
    }

    static int Compare(const Str &key1, const Str &key2) {
        return Str::Icmp(key1, key2);
    }

public:
    Str                     key;
    ValueT                  value;
    HashBucket<Str, ValueT> *next;
};

// HashBucket class template specialization with hash key type is given string pointer
template <typename ValueT>
class HashBucket<const char *, ValueT> {
public:
    HashBucket() {}
    HashBucket(const char * const &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {}

    static int GenerateHash(const char * const &key, const int tableMask) {
        return Str::Hash(key) & tableMask;
    }

    static int Compare(const char * const &key1, const char * const &key2) {
        return Str::Icmp(key1, key2);
    }

public:
    Str                     key;
    ValueT                  value;
    HashBucket<Str, ValueT> *next;
};

// HashBucket class template specialization with hash key type is given WStr
template <typename ValueT>
class HashBucket<WStr, ValueT> {
public:
    HashBucket() {}
    HashBucket(const WStr &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {
    }

    static int GenerateHash(const WStr &key, const int tableMask) {
        return WStr::Hash(key) & tableMask;
    }

    static int Compare(const WStr &key1, const WStr &key2) {
        return WStr::Icmp(key1, key2);
    }

public:
    WStr                    key;
    ValueT                  value;
    HashBucket<WStr, ValueT> *next;
};

// HashBucket class template specialization with hash key type is given wide string pointer
template <typename ValueT>
class HashBucket<const wchar_t *, ValueT> {
public:
    HashBucket() {}
    HashBucket(const wchar_t * const &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {
    }

    static int GenerateHash(const wchar_t * const &key, const int tableMask) {
        return WStr::Hash(key) & tableMask;
    }

    static int Compare(const wchar_t * const &key1, const wchar_t * const &key2) {
        return WStr::Icmp(key1, key2);
    }

public:
    WStr                    key;
    ValueT                  value;
    HashBucket<WStr, ValueT> *next;
};

/// Hash table
template <typename KeyT, typename ValueT, int BucketGranularity = 256>
class HashTable {
    using THashBucket = HashBucket<KeyT, ValueT>;

public:
    /// Constructs empty hash table with the given table size.
    HashTable(int newTableSize = 256);

    /// Copy constructs
    HashTable(const HashTable<KeyT, ValueT> &other);

    /// Destructs
    ~HashTable();

                        /// Returns number of hash buckets.
    int                 Count() const { return numEntries; }
    
                        /// Returns total size of allocated memory.
    size_t              Allocated() const { return sizeof(heads) * tableSize + sizeof(*heads) * numEntries + blockAllocator.Allocated(); }

                        /// Returns total size of allocated memory including size of this type.
    size_t              Size() const { return sizeof(heads) * tableSize + sizeof(*heads) * numEntries + blockAllocator.Allocated() + sizeof(*this); }

                        // 전체 hash bucket 을 순서대로 검색해서 주어진 index 와 맞는 entry 의 value pointer 를 리턴
                        // index 로 전체 bucket 들을 iteration 할 수 있지만 bucket 이 추가 or 삭제되면 기존 index 는 더 이상 유효하지 않을 수 있다.
    ValueT *            GetByIndex(int index) const;

                        // 전체 hash bucket 을 순서대로 검색해서 주어진 index 와 맞는 entry 의 key 를 얻는다
                        // index 로 전체 bucket 들을 iteration 할 수 있지만 bucket 이 추가 or 삭제되면 기존 index 는 더 이상 유효하지 않을 수 있다.
    bool                GetKeyByIndex(int index, KeyT &key) const;

                        // key 값으로 find 해서 value pointer 를 얻는다.
    bool                Get(const KeyT &key, ValueT *value = nullptr) const;

                        // key/value 추가. 이미 존재하는 key 라면 value 값만 바꾼다.
    void                Set(const KeyT &key, ValueT &value);
    
                        // key 값으로 search 해서 value 참조 리턴. 없다면 새로 bucket 추가 후 참조 리턴
    ValueT &            operator[](const KeyT &key);

                        /// Removes bucket with the given key.
    bool                Remove(const KeyT &key);
        
                        /// Clears all hash buckets.
    void                Clear();

                        /// Clears all hash buckets and delete each values.
    void                DeleteContents();

                        // 분산된 hash bucket 의 표준편차를 구한다.
    float               GetStandardDeviation() const;

private:
    void                Copy(const HashTable<KeyT, ValueT> &other);

    BlockAllocator<THashBucket, BucketGranularity> blockAllocator;

    THashBucket **      heads;              // hash table
    int                 tableSize;          // hash table size
    int                 numEntries;         // number of entries
    int                 tableSizeMask;      // tableSize - 1
};

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE HashTable<KeyT, ValueT, BucketGranularity>::HashTable(int newTableSize) {
    assert(newTableSize > 0);
    assert(Math::IsPowerOfTwo(newTableSize));

    tableSize = newTableSize;
    tableSizeMask = tableSize - 1;

    heads = new THashBucket *[tableSize];
    memset(heads, 0, sizeof(*heads) * tableSize);
    
    numEntries = 0;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE HashTable<KeyT, ValueT, BucketGranularity>::HashTable(const HashTable<KeyT, ValueT> &other) {
    Copy(other);
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE HashTable<KeyT, ValueT, BucketGranularity>::~HashTable() {
    Clear();
    delete [] heads;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE ValueT *HashTable<KeyT, ValueT, BucketGranularity>::GetByIndex(int index) const {
    if ((index < 0) || (index > numEntries)) {
        assert(0);
        return nullptr;
    }

    int count = 0;

    for (int i = 0; i < tableSize; i++) {
        for (THashBucket *node = heads[i]; node != nullptr; node = node->next) {
            if (count == index) {
                return &node->value;
            }
            count++;
        }
    }

    return nullptr;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE bool HashTable<KeyT, ValueT, BucketGranularity>::GetKeyByIndex(int index, KeyT &key) const {
    if ((index < 0) || (index > numEntries)) {
        assert(0);
        return false;
    }

    int count = 0;

    for (int i = 0; i < tableSize; i++) {
        for (THashBucket *node = heads[i]; node != nullptr; node = node->next) {
            if (count == index) {
                key = node->key;
                return true;
            }
            count++;
        }
    }

    return false;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE bool HashTable<KeyT, ValueT, BucketGranularity>::Get(const KeyT &key, ValueT *value) const {
    int hash = THashBucket::GenerateHash(key, tableSizeMask);
    for (THashBucket *node = heads[hash]; node != nullptr; node = node->next) {
        int s = node->Compare(node->key, key);
        if (s == 0) {
            if (value) {
                *value = node->value;
            }
            return true;
        }

        if (s > 0) {
            // not found in ascending order
            break;
        }
    }

    return false;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE void HashTable<KeyT, ValueT, BucketGranularity>::Set(const KeyT &key, ValueT &value) {
    int hash = THashBucket::GenerateHash(key, tableSizeMask);
    THashBucket **nextPtr = &(heads[hash]);
    THashBucket *node;

    for (node = *nextPtr; node != nullptr; nextPtr = &(node->next), node = *nextPtr) {
        int s = node->Compare(node->key, key);
        if (s == 0) { 
            // change value and return if the key already exist
            node->value = value;
            return;
        }
        
        if (s > 0) {
            // break to link ascending order
            break;
        }
    }

    THashBucket *newBucket = blockAllocator.Alloc();
    newBucket->key = key;
    newBucket->value = value;
    newBucket->next = node;
    *nextPtr = newBucket;

    numEntries++;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE ValueT &HashTable<KeyT, ValueT, BucketGranularity>::operator[](const KeyT &key) {
    int hash = THashBucket::GenerateHash(key, tableSizeMask);
    THashBucket **nextPtr = &(heads[hash]);
    THashBucket *node;

    for (node = *nextPtr; node != nullptr; nextPtr = &(node->next), node = *nextPtr) {
        int s = node->Compare(node->key, key);
        if (s == 0) {
            // return value if the key already exist
            return node->value;
        }
        
        if (s > 0) {
            // break to link ascending order
            break;
        }
    }

    THashBucket *newBucket = blockAllocator.Alloc();
    newBucket->key = key;
    newBucket->next = node;
    *nextPtr = newBucket;

    numEntries++;

    return newBucket->value;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE bool HashTable<KeyT, ValueT, BucketGranularity>::Remove(const KeyT &key) {
    int hash = THashBucket::GenerateHash(key, tableSizeMask);
    THashBucket **head = &heads[hash];
    if (*head) {
        THashBucket *prev = nullptr;
        THashBucket *node;

        for (node = *head; node != nullptr; prev = node, node = node->next) {
            if (node->key == key) {
                if (prev) {
                    prev->next = node->next;
                } else {
                    *head = node->next;
                }

                blockAllocator.Free(node);
                numEntries--;
                return true;
            }
        }
    }

    return false;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE void HashTable<KeyT, ValueT, BucketGranularity>::Clear() {
    for (int i = 0; i < tableSize; i++) {
        THashBucket *next = heads[i];

        while (next != nullptr) {
            THashBucket *node = next;
            next = next->next;
            blockAllocator.Free(node);
        }

        heads[i] = nullptr;
    }

    numEntries = 0;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE void HashTable<KeyT, ValueT, BucketGranularity>::DeleteContents() {
    for (int i = 0; i < tableSize; i++) {
        THashBucket *next = heads[i];

        while (next != nullptr) {
            THashBucket *node = next;
            next = next->next;
            delete node->value;
            blockAllocator.Free(node);
        }

        heads[i] = nullptr;
    }

    numEntries = 0;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE void HashTable<KeyT, ValueT, BucketGranularity>::Copy(const HashTable<KeyT, ValueT> &other) {
    if (&other == this) {
        return;
    }
    assert(other.tableSize > 0);

    tableSize       = other.tableSize;
    heads           = new THashBucket *[tableSize];
    numEntries      = other.numEntries;
    tableSizeMask   = other.tableSizeMask;

    for (int i = 0; i < tableSize; i++) {
        if (!other.heads[i]) {
            heads[i] = nullptr;
            continue;
        }

        THashBucket **prev = &heads[i];
        for (THashBucket *node = other.heads[i]; node != nullptr; node = node->next) {
            THashBucket *newBucket = blockAllocator.Alloc();
            newBucket->key = node->key;
            newBucket->value = node->value;
            newBucket->next = nullptr;
            
            *prev = newBucket;
            prev = &(*prev)->next;
        }
    }
}

template <typename KeyT, typename ValueT, int BucketGranularity>
float HashTable<KeyT, ValueT, BucketGranularity>::GetStandardDeviation() const {
    // if no items in hash
    if (!numEntries) {
        return 0;
    }

    int average = numEntries / tableSize;
    int s = 0;
    for (int i = 0; i < tableSize; i++) {
        int numItems = 0;
        for (THashBucket *node = heads[i]; node != nullptr; node = node->next) {
            numItems++;
        }
        
        int e = numItems - average;
        s += e * e;
    }

    float v = (float)s / numEntries;
    return sqrtf(v);
}

BE_NAMESPACE_END
