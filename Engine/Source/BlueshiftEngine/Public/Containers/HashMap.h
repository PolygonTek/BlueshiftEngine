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

    HashMap

    NOTE:
    - key/value pair 가 Array 로 되어있다.
    - 추가된 순서대로 순차적으로 iteration 할 수 있다.
    - 삭제할 경우 메모리 copy 가 일어나서 느릴 수 있다.
    - HashTable 에 비해 추가가 빠르다. bucket list 는 가장 단순한 형태.

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"
#include "Containers/Pair.h"
#include "Containers/Array.h"
#include "Containers/HashIndex.h"

BE_NAMESPACE_BEGIN

struct HashGeneratorDefault {
    template <typename ValueT>
    static int Hash(const HashIndex &hasher, const ValueT &value) {
        return hasher.GenerateHash(value);
    }
};

struct HashGeneratorNumeric {
    template <typename ValueT>
    static int Hash(const HashIndex &hasher, const ValueT &value) {
        return hasher.GenerateHash(value, 0);
    }
};

struct HashGeneratorIHash {
    template <typename ValueT>
    static int Hash(const HashIndex &hasher, const ValueT &value) {
        return hasher.GenerateHash(value, false);
    }
};

struct HashCompareDefault {
    template <typename Type>
    static bool Compare(const Type &lhs, const Type &rhs) {
        return lhs == rhs;
    }
};

struct HashCompareStrCmp {
    template <typename Type>
    static bool Compare(const Type &lhs, const Type &rhs) {
        return Str::Cmp(lhs, rhs) == 0;
    }
};

struct HashCompareStrIcmp {
    template <typename Type>
    static bool Compare(const Type &lhs, const Type &rhs) {
        return Str::Icmp(lhs, rhs) == 0;
    }
};

#define HASH_MAP_TEMPLATE   HashMap<KeyT, ValueT, HashCompareT, HashGeneratorT>

/// Hash map
template <typename KeyT, typename ValueT, typename HashCompareT = HashCompareDefault, typename HashGeneratorT = HashGeneratorDefault>
class HashMap {
public:
    using KV = Pair<KeyT, ValueT>;
    using KVArray = Array<KV>;

    /// Constructs empty hash map
    HashMap() {}

    /// Constructs from other hash map
    HashMap(const HASH_MAP_TEMPLATE &other);
    
    /// Destructs
    ~HashMap();

                            /// Initializes with the given parameters
    void                    Init(int hashSize, int indexSize, int granularity);

                            /// Returns number of hash buckets
    int                     Count() const { return pairs.Count(); }

                            /// Returns total size of allocated memory
    size_t                  Allocated() const { return pairs.Allocated() + hashIndex.Allocated(); }

                            /// Returns total size of allocated memory including size of this type
    size_t                  Size() const { return pairs.Size() + hashIndex.Size() + sizeof(*this); }

                            /// Direct access of pair array
    const KVArray &         GetPairs() const { return pairs; }
    KVArray &               GetPairs() { return pairs; }

                            // key 값으로 search 해서 value pointer 를 얻는다.
    KV *                    Get(const KeyT &key);
    const KV *              Get(const KeyT &key) const;

                            // index 에 해당하는 value pointer 를 리턴한다.
                            // index 로 전체 bucket 들을 iteration 할 수 있지만 bucket 이 추가 or 삭제되면 기존 index 는 더 이상 유효하지 않을 수 있다.
    KV *                    GetByIndex(int index);
    const KV *              GetByIndex(int index) const;

                            // key/value 추가. 이미 존재하는 key 라면 value 값만 바꾼다.
    ValueT &                Set(const KeyT &key, const ValueT &value);

                            // key 값으로 search 해서 value 참조 리턴. 없다면 새로 bucket 추가 후 참조 리턴
    ValueT &                operator[](const KeyT &key);

                            // index 로 전체 bucket 들을 iteration 할 수 있지만 bucket 이 추가 or 삭제되면 기존 index 는 더 이상 유효하지 않을 수 있다.
    const KeyT &            GetKey(int index) const;

                            // key 값에 해당하는 hash bucket 을 삭제한다. 삭제된 array element 에 대한 메모리 정리가 일어난다.
    bool                    Remove(const KeyT &key);

                            /// Clears all hash buckets.
    void                    Clear();

                            // 모든 hash bucket 을 삭제하고, 해당하는 value 도 삭제한다.
    void                    DeleteContents(bool clear = true);

                            // hash table 에 분산된 index 개수에 대한 표준 편차를 구한다.
    float                   GetStandardDeviation() const;

                            /// Swaps hash map 'other' with this hash map.
    void                    Swap(HashMap &other);

private:
    KVArray                 pairs;      ///< bucket list (key/value pair)
    HashIndex               hashIndex;  ///< pairs 에 대한 hash index
};

template <typename ValueT>
class StrHashMap : public HashMap<Str, ValueT> {
};

template <typename ValueT>
class StrIHashMap : public HashMap<Str, ValueT, HashCompareStrIcmp, HashGeneratorIHash> {
};

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE HASH_MAP_TEMPLATE::HashMap(const HASH_MAP_TEMPLATE &other) {
    pairs = other.pairs;
    hashIndex = other.hashIndex;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE HASH_MAP_TEMPLATE::~HashMap() {
    Clear();
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE void HASH_MAP_TEMPLATE::Init(int hashSize, int indexSize, int granularity) {
    pairs.Resize(indexSize, granularity);
    hashIndex.Clear(hashSize, indexSize);
    hashIndex.SetGranularity(granularity);
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE typename HASH_MAP_TEMPLATE::KV *HASH_MAP_TEMPLATE::GetByIndex(int index) {
    if (index >= 0 && index < pairs.Count()) {
        return &pairs[index];
    }
    return nullptr;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE const typename HASH_MAP_TEMPLATE::KV *HASH_MAP_TEMPLATE::GetByIndex(int index) const {
    if (index >= 0 && index < pairs.Count()) {
        return &pairs[index];
    }
    return nullptr;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE typename HASH_MAP_TEMPLATE::KV *HASH_MAP_TEMPLATE::Get(const KeyT &key) {
    int hash = HashGeneratorT::Hash(hashIndex, key);

    for (int index = hashIndex.First(hash); index != -1; index = hashIndex.Next(index)) {
        if (HashCompareT::Compare(pairs[index].first, key) == true) {
            return &pairs[index];
        }
    }
    return nullptr;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE const typename HASH_MAP_TEMPLATE::KV *HASH_MAP_TEMPLATE::Get(const KeyT &key) const {
    int hash = HashGeneratorT::Hash(hashIndex, key);

    for (int index = hashIndex.First(hash); index != -1; index = hashIndex.Next(index)) {
        if (HashCompareT::Compare(pairs[index].first, key) == true) {
            return &pairs[index];
        }
    }
    return nullptr;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE ValueT &HASH_MAP_TEMPLATE::Set(const KeyT &key, const ValueT &value) {
    KV *existingEntry = Get(key);
    if (existingEntry) {
        existingEntry->second = value;
        return existingEntry->second;
    }

    Pair<KeyT, ValueT> &pair = pairs.Alloc();
    pair.first = key;
    pair.second = value;
    hashIndex.Add(HashGeneratorT::Hash(hashIndex, key), pairs.Count() - 1);
    return pair.second;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE ValueT &HASH_MAP_TEMPLATE::operator[](const KeyT &key) {
    KV *existingEntry;
    if ((existingEntry = Get(key))) {
        return existingEntry->second;
    }
    
    Pair<KeyT, ValueT> &pair = pairs.Alloc();
    pair.first = key;
    hashIndex.Add(HashGeneratorT::Hash(hashIndex, key), pairs.Count() - 1);
    return pair.second;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE bool HASH_MAP_TEMPLATE::Remove(const KeyT &key) {
    int hash = HashGeneratorT::Hash(hashIndex, key);

    for (int index = hashIndex.First(hash); index != HashIndex::EmptyTable[0]; index = hashIndex.Next(index)) {
        if (HashCompareT::Compare(pairs[index].first, key) == true) {
            pairs.RemoveIndex(index);
            hashIndex.RemoveIndex(hash, index);
            return true;
        }
    }
    return false;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE const KeyT &HASH_MAP_TEMPLATE::GetKey(int index) const {
    if (index >= 0 && index < pairs.Count()) {
        return pairs[index].first;
    }
    static KeyT blank;
    return blank;
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE void HASH_MAP_TEMPLATE::Clear() {
    pairs.Clear();
    hashIndex.Free();
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE void HASH_MAP_TEMPLATE::DeleteContents(bool clear) {
    for (int index = 0; index < pairs.Count(); index++) {
        delete pairs[index].second;
        pairs[index].second = nullptr;
    }

    if (clear) {
        pairs.Clear();
        hashIndex.Free();
    }
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE float HASH_MAP_TEMPLATE::GetStandardDeviation() const {
    return hashIndex.GetStandardDeviation();
}

template <typename KeyT, typename ValueT, typename HashCompareT, typename HashGeneratorT>
BE_INLINE void HASH_MAP_TEMPLATE::Swap(HashMap &other) {
    pairs.Swap(other.pairs);
    hashIndex.Swap(other.hashIndex);
}

BE_NAMESPACE_END
