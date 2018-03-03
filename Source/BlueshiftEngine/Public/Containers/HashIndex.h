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

    HashIndex

    Fast hash table for indexes

    Does not allocate memory until the first hash/index pair is added.

    hashTable[hash] -> index1
    indexChain[index1] -> index2
    indexChain[index2] -> -1

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Vec3;
class Guid;

/// Hash index table
class BE_API HashIndex {
public:
    static constexpr int DefaultHashSize = 1 << 10;
    static constexpr int DefaultIndexGranularity = 1 << 10;

    /// Constructs hash index with initial size.
    HashIndex(const int initialHashSize = DefaultHashSize, const int initialIndexSize = DefaultHashSize);
    
    /// Copy constructor.
    HashIndex(const HashIndex &rhs);

    /// Copy from another hash index.
    HashIndex &operator=(const HashIndex &rhs);
    
    /// Destructs.
    ~HashIndex();

                    /// Returns size of hash table.
    int             GetHashSize() const { return hashSize; }

                    /// Returns size of index chain table.
    int             GetIndexSize() const { return indexSize; }

                    /// Sets the new granularity.
    void            SetGranularity(const int newGranularity);

                    /// Gets the current granularity.
    int             GetGranularity() const { return granularity; }

                    /// Returns total size of allocated memory.
    size_t          Allocated() const { return hashSize * sizeof(*hashTable) + indexSize * sizeof(*indexChain); }

                    /// Returns total size of allocated memory including size of this type.
    size_t          Size() const { return Allocated() + sizeof(*this); }

                    /// Clears hash table (no memory free)
    void            Clear();

                    /// Clears hash table and free memory in use
    void            Clear(const int newHashSize, const int newIndexSize);

                    /// Frees all allocated memory
    void            Free();

                    /// Finds index with the given hash key.
                    /// Returns -1 if not found.
    int             First(const int hash) const;

                    /// Finds next index
                    /// Returns -1 if not found.
    int             Next(const int index) const;

                    /// Resizes index size
    void            ResizeIndex(const int newIndexSize);

                    /// Adds hash/index pair
    void            Add(const int hash, const int index);

                    /// Removes hash/index pair
    void            Remove(const int hash, const int index);

                    /// Adds hash/index pair for a inserted element
    void            InsertIndex(const int hash, const int index);

                    /// Removes hash/index pair for a removed element
    void            RemoveIndex(const int hash, const int index);

                    // hash table 에 분산된 index 개수에 대한 표준 편차를 구한다.
    float           GetStandardDeviation() const;

                    /// Generate hash value
    int             GenerateHash(const char *string, bool caseSensitive = true) const;
    int             GenerateHash(const wchar_t *string, bool caseSensitive = true) const;
    int             GenerateHash(const Vec3 &v) const;
    int             GenerateHash(const int n1, const int n2) const;
    int             GenerateHash(const int64_t i64) const;
    int             GenerateHash(const Guid &guid) const;

                    /// Swaps hash index 'other' with this hash index.
    void            Swap(HashIndex &other);

    static int      EmptyTable[1];

private:
                    /// Clears and allocates memory for use
    void            Allocate(const int newHashSize, const int newIndexSize);

    int *           hashTable;          ///< hash to index table
    int *           indexChain;         ///< index to index table
    int             hashSize;           ///< hash size
    int             indexSize;          ///< index size
    int             granularity;        ///< index chain allocation granularity
    int             hashMask;           ///< hashSize - 1
    int             lookUpMask;         ///< 0 means initial state, -1 is not
};

BE_INLINE HashIndex::~HashIndex() {
    Free();
}

BE_INLINE void HashIndex::SetGranularity(const int newGranularity) {
    assert(newGranularity > 0);
    granularity = newGranularity;
}

BE_INLINE void HashIndex::Clear() {
    // only clear the hash table because clearing the indexChain is not really needed
    if (hashTable != EmptyTable) {
        memset(hashTable, 0xFF, hashSize * sizeof(hashTable[0]));
    }
}

BE_INLINE void HashIndex::Clear(const int newHashSize, const int newIndexSize) {
    Free();

    hashSize = newHashSize;
    indexSize = newIndexSize;
}

BE_INLINE int HashIndex::First(const int hash) const {
    return hashTable[hash & hashMask & lookUpMask];
}

BE_INLINE int HashIndex::Next(const int index) const {
    assert(index >= 0 && index < indexSize);
    return indexChain[index & lookUpMask];
}

BE_INLINE void HashIndex::Add(const int hash, const int index) {
    assert(index >= 0);

    if (hashTable == EmptyTable) {
        Allocate(hashSize, index >= indexSize ? index + 1 : indexSize);
    } else if (index >= indexSize) {
        ResizeIndex(index + 1);
    }
    int h = (hash & hashMask);
    indexChain[index] = hashTable[h];
    hashTable[h] = index;
}

BE_INLINE void HashIndex::Remove(const int hash, const int index) {
    assert(index >= 0 && index < indexSize);

    if (hashTable == EmptyTable) {
        return;
    }
    
    int h = (hash & hashMask);
    if (hashTable[h] == index) {
        hashTable[h] = indexChain[index];
    } else {
        for (int i = hashTable[h]; i != -1; i = indexChain[i]) {
            if (indexChain[i] == index) {
                indexChain[i] = indexChain[index];
                break;
            }
        }
    }
    indexChain[index] = -1;
}

BE_INLINE void HashIndex::InsertIndex(const int hash, const int index) {
    if (hashTable != EmptyTable) {
        int maxIndex = index;

        for (int i = 0; i < hashSize; i++) {
            if (hashTable[i] >= index) {
                hashTable[i]++;
                if (hashTable[i] > maxIndex) {
                    maxIndex = hashTable[i];
                }
            }
        }

        for (int i = 0; i < indexSize; i++) {
            if (indexChain[i] >= index) {
                indexChain[i]++;
                if (indexChain[i] > maxIndex) {
                    maxIndex = indexChain[i];
                }
            }
        }

        if (maxIndex >= indexSize) {
            ResizeIndex(maxIndex + 1);
        }

        for (int i = maxIndex; i > index; i--) {
            indexChain[i] = indexChain[i - 1];
        }
        indexChain[index] = -1;
    }

    Add(hash, index);
}

BE_INLINE void HashIndex::RemoveIndex(const int hash, const int index) {
    Remove(hash, index);

    if (hashTable != EmptyTable) {
        int maxIndex = index;

        for (int i = 0; i < hashSize; i++) {
            if (hashTable[i] >= index) {
                if (hashTable[i] > maxIndex) {
                    maxIndex = hashTable[i];
                }
                hashTable[i]--;
            }
        }

        for (int i = 0; i < indexSize; i++) {
            if (indexChain[i] >= index) {
                if (indexChain[i] > maxIndex) {
                    maxIndex = indexChain[i];
                }
                indexChain[i]--;
            }
        }

        for (int i = index; i < maxIndex; i++) {
            indexChain[i] = indexChain[i + 1];
        }
        indexChain[maxIndex] = -1;
    }
}

BE_INLINE int HashIndex::GenerateHash(const int n1, const int n2) const {
    return ((n1 + n2) & hashMask);
}

BE_INLINE int HashIndex::GenerateHash(const int64_t i64) const {
    if (sizeof(i64) == sizeof(int)) {
        return (i64 & hashMask);
    } else {
        return ((((i64 >> 32) & 0xffffffff) ^ (i64 & 0xffffffff)) & hashMask);
    }
}

BE_INLINE void HashIndex::Swap(HashIndex &other) {
    BE1::Swap(hashSize, other.hashSize);
    BE1::Swap(hashTable, other.hashTable);
    BE1::Swap(indexSize, other.indexSize);
    BE1::Swap(indexChain, other.indexChain);
    BE1::Swap(granularity, other.granularity);
    BE1::Swap(hashMask, other.hashMask);
    BE1::Swap(lookUpMask, other.lookUpMask);
}

BE_NAMESPACE_END
