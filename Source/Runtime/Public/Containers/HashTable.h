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

/*
-------------------------------------------------------------------------------

    Hash table

    Similar with STL hash_map/unorderd_map.
    The key should be unique.

    NOTE:
    - Search / delete is faster than HashMap.
    - The bucket list is implemented as a linked list sorted in ascending order.

-------------------------------------------------------------------------------
*/

#include "Core/Heap.h"
#include "Core/Allocator.h"
#include "Core/Guid.h"
#include "Core/Str.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

/// HashBucket class template.
template <typename KeyT, class ValueT> 
class HashBucket {
public:
    HashBucket() = default;
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

/// HashBucket class template specialization with hash key type is given Guid.
template <typename ValueT>
class HashBucket<Guid, ValueT> {
public:
    HashBucket() = default;
    HashBucket(const Guid &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {}

    static int GenerateHash(const Guid &key, const int tableMask) {
        return key.ToHash() & tableMask;
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

/// HashBucket class template specialization with hash key type is given Str.
template <typename ValueT>
class HashBucket<Str, ValueT> {
public:
    HashBucket() = default;
    HashBucket(const Str &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {}

    static int GenerateHash(const Str &key, const int tableMask) {
        return Str::Hash(key) & tableMask;
    }

    static int Compare(const Str &key1, const Str &key2) {
        return Str::Cmp(key1, key2);
    }

public:
    Str                     key;
    ValueT                  value;
    HashBucket<Str, ValueT> *next;
};

/// HashBucket class template specialization with hash key type is given string pointer.
template <typename ValueT>
class HashBucket<const char *, ValueT> {
public:
    HashBucket() = default;
    HashBucket(const char * const &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {}

    static int GenerateHash(const char * const &key, const int tableMask) {
        return Str::Hash(key) & tableMask;
    }

    static int Compare(const char * const &key1, const char * const &key2) {
        return Str::Cmp(key1, key2);
    }

public:
    Str                     key;
    ValueT                  value;
    HashBucket<const char *, ValueT> *next;
};

/// HashBucket class template specialization with hash key type is pointer type.
template <typename KeyT, typename ValueT>
class HashBucket<KeyT*, ValueT> {
public:
    HashBucket() = default;
    HashBucket(const KeyT* const &key, const ValueT &value, HashBucket *next)
        : key(key), value(value), next(next) {}

    static int GenerateHash(const KeyT* const &key, const int tableMask) {
        return ((intptr_t)key) & tableMask;
    }

    static int Compare(const KeyT* const &key1, const KeyT* const &key2) {
        if (key1 < key2) {
            return -1;
        }
        else if (key1 > key2) {
            return 1;
        }
        return 0;
    }

public:
    KeyT *                  key;
    ValueT                  value;
    HashBucket<KeyT*, ValueT> *next;
};

/// Hash table
template <typename KeyT, typename ValueT, int BucketGranularity = 256>
class HashTable {
    using THashBucket = HashBucket<KeyT, ValueT>;

public:
    class Entry;
    class ConstEntry;
    class Iterator;
    class ConstIterator;

    /// Constructs empty hash table with the given table size.
    HashTable(int newTableSize = 256);

    /// Copy constructs
    HashTable(const HashTable<KeyT, ValueT, BucketGranularity> &rhs) = delete;

    /// Copy from another hash table.
    HashTable &operator=(const HashTable<KeyT, ValueT, BucketGranularity> &rhs) = delete;

    /// Constructs from rvalue hash table.
    HashTable(HashTable &&rhs) noexcept = delete;

    /// Move from another hash table.
    HashTable &operator=(HashTable &&rhs) noexcept = delete;

    /// Destructs
    ~HashTable();

                            /// Returns number of hash buckets.
    int                     Count() const { return numEntries; }
    
                            /// Returns total size of allocated memory.
    size_t                  Allocated() const { return sizeof(*heads) * tableSize + blockAllocator.Allocated(); }

                            /// Returns total size of allocated memory including size of this type.
    size_t                  Size() const { return Allocated() + sizeof(*this); }

                            /// Searches the entire hash bucket in order and return the value pointer of the entry that matches the given index.
                            /// You can iterate entire buckets with an index, but when a bucket is added or removed, the existing index may no longer be valid.
    ValueT *                GetByIndex(int index) const;

                            /// Searches the entire hash bucket in order and get the key of the entry that matches the given index.
                            /// You can iterate entire buckets with an index, but when a bucket is added or removed, the existing index may no longer be valid.
    bool                    GetKeyByIndex(int index, KeyT &key) const;

    Iterator                begin();
    ConstIterator           begin() const;
    ConstIterator           cbegin() const;
    Iterator                end();
    ConstIterator           end() const;
    ConstIterator           cend() const;

                            /// Finds the value by key and get the value pointer.
    bool                    Get(const KeyT &key, ValueT *value) const;

                            /// Finds the value by key and get the value pointer.
    bool                    Get(const KeyT &key, ValueT **value = nullptr);

                            /// Finds the value by key and get the value pointer.
    bool                    Get(const KeyT &key, const ValueT **value = nullptr) const;

                            /// Adds key / value. If the key already exists, only the value will be changed.
    void                    Set(const KeyT &key, const ValueT &value);

                            /// Searches by key and return value reference. If not, add a new bucket and return a reference
    ValueT &                operator[](const KeyT &key);

                            /// Removes bucket with the given key.
    bool                    Remove(const KeyT &key);
        
                            /// Clears all hash buckets.
    void                    Clear();

                            /// Clears all hash buckets and delete each values.
    void                    DeleteContents();

                            /// Get the variance of the distributed hash buckets.
    float                   GetVariance() const;

    class Entry {
    public:
        Entry(const KeyT &key, ValueT &value) : key(key), value(value) {}

        const KeyT &        key;
        ValueT &            value;
    };

    class ConstEntry {
    public:
        ConstEntry(const KeyT &key, const ValueT &value) : key(key), value(value) {}

        const KeyT &        key;
        const ValueT &      value;
    };

    class Iterator {
    public:
        Iterator() : hashTable(nullptr), bucketIndex(0), node(nullptr) {}
        Iterator(HashTable *hashTable, int bucketIndex) : hashTable(hashTable), bucketIndex(bucketIndex), node(nullptr) {
            SkipEmptyBuckets();
        }

        Entry operator*() const {
            assert(node);
            return Entry(node->key, node->value);
        }

        Iterator &operator++() {
            if (node && node->next) {
                node = node->next;
            } else {
                bucketIndex++;
                SkipEmptyBuckets();
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator &rhs) const {
            return hashTable == rhs.hashTable && bucketIndex == rhs.bucketIndex && node == rhs.node;
        }

        bool operator!=(const Iterator &rhs) const {
            return !(*this == rhs);
        }

    private:
        void SkipEmptyBuckets() {
            if (!hashTable) {
                node = nullptr;
                return;
            }

            while (bucketIndex < hashTable->tableSize) {
                node = hashTable->heads[bucketIndex];
                if (node) {
                    return;
                }
                bucketIndex++;
            }

            node = nullptr;
        }

        HashTable *         hashTable;
        int                 bucketIndex;
        THashBucket *       node;
    };

    class ConstIterator {
    public:
        ConstIterator() : hashTable(nullptr), bucketIndex(0), node(nullptr) {}
        ConstIterator(const HashTable *hashTable, int bucketIndex) : hashTable(hashTable), bucketIndex(bucketIndex), node(nullptr) {
            SkipEmptyBuckets();
        }

        ConstEntry operator*() const {
            assert(node);
            return ConstEntry(node->key, node->value);
        }

        ConstIterator &operator++() {
            if (node && node->next) {
                node = node->next;
            } else {
                bucketIndex++;
                SkipEmptyBuckets();
            }
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const ConstIterator &rhs) const {
            return hashTable == rhs.hashTable && bucketIndex == rhs.bucketIndex && node == rhs.node;
        }

        bool operator!=(const ConstIterator &rhs) const {
            return !(*this == rhs);
        }

    private:
        void SkipEmptyBuckets() {
            if (!hashTable) {
                node = nullptr;
                return;
            }

            while (bucketIndex < hashTable->tableSize) {
                node = hashTable->heads[bucketIndex];
                if (node) {
                    return;
                }
                bucketIndex++;
            }

            node = nullptr;
        }

        const HashTable *   hashTable;
        int                 bucketIndex;
        const THashBucket * node;
    };

private:
    BlockAllocator<THashBucket, BucketGranularity> blockAllocator;

    THashBucket **          heads;              // hash table
    int                     tableSize;          // hash table size
    int                     numEntries;         // number of entries
    int                     tableSizeMask;      // tableSize - 1
};

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE HashTable<KeyT, ValueT, BucketGranularity>::HashTable(int newTableSize) {
    assert(newTableSize > 0);
    assert(IsPowerOfTwo(newTableSize));

    tableSize = newTableSize;
    tableSizeMask = tableSize - 1;

    heads = BE_NEW THashBucket *[tableSize];
    memset(heads, 0, sizeof(*heads) * tableSize);
    
    numEntries = 0;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE HashTable<KeyT, ValueT, BucketGranularity>::~HashTable() {
    Clear();
    delete [] heads;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE typename HashTable<KeyT, ValueT, BucketGranularity>::Iterator HashTable<KeyT, ValueT, BucketGranularity>::begin() {
    return Iterator(this, 0);
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE typename HashTable<KeyT, ValueT, BucketGranularity>::ConstIterator HashTable<KeyT, ValueT, BucketGranularity>::begin() const {
    return ConstIterator(this, 0);
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE typename HashTable<KeyT, ValueT, BucketGranularity>::ConstIterator HashTable<KeyT, ValueT, BucketGranularity>::cbegin() const {
    return ConstIterator(this, 0);
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE typename HashTable<KeyT, ValueT, BucketGranularity>::Iterator HashTable<KeyT, ValueT, BucketGranularity>::end() {
    return Iterator(this, tableSize);
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE typename HashTable<KeyT, ValueT, BucketGranularity>::ConstIterator HashTable<KeyT, ValueT, BucketGranularity>::end() const {
    return ConstIterator(this, tableSize);
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE typename HashTable<KeyT, ValueT, BucketGranularity>::ConstIterator HashTable<KeyT, ValueT, BucketGranularity>::cend() const {
    return ConstIterator(this, tableSize);
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE ValueT *HashTable<KeyT, ValueT, BucketGranularity>::GetByIndex(int index) const {
    if ((index < 0) || (index >= numEntries)) {
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
    if ((index < 0) || (index >= numEntries)) {
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
BE_INLINE bool HashTable<KeyT, ValueT, BucketGranularity>::Get(const KeyT &key, ValueT **value) {
    int hash = THashBucket::GenerateHash(key, tableSizeMask);
    for (THashBucket *node = heads[hash]; node != nullptr; node = node->next) {
        int s = node->Compare(node->key, key);
        if (s == 0) {
            if (value) {
                *value = &node->value;
            }
            return true;
        }

        if (s > 0) {
            // not found in ascending order
            break;
        }
    }
    if (value) {
        *value = nullptr;
    }
    return false;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE bool HashTable<KeyT, ValueT, BucketGranularity>::Get(const KeyT &key, const ValueT **value) const {
    int hash = THashBucket::GenerateHash(key, tableSizeMask);
    for (THashBucket *node = heads[hash]; node != nullptr; node = node->next) {
        int s = node->Compare(node->key, key);
        if (s == 0) {
            if (value) {
                *value = &node->value;
            }
            return true;
        }

        if (s > 0) {
            // not found in ascending order
            break;
        }
    }
    if (value) {
        *value = nullptr;
    }
    return false;
}

template <typename KeyT, typename ValueT, int BucketGranularity>
BE_INLINE void HashTable<KeyT, ValueT, BucketGranularity>::Set(const KeyT &key, const ValueT &value) {
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
    newBucket->value = {};
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
            int s = THashBucket::Compare(node->key, key);
            if (s == 0) {
                if (prev) {
                    prev->next = node->next;
                } else {
                    *head = node->next;
                }

                blockAllocator.Free(node);
                numEntries--;
                return true;
            }

            if (s > 0) {
                // not found in ascending order
                break;
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
    static_assert(std::is_pointer<ValueT>::value, "HashTable::DeleteContents requires pointer values");

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
float HashTable<KeyT, ValueT, BucketGranularity>::GetVariance() const {
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

    return (float)s / numEntries;
}

BE_NAMESPACE_END
