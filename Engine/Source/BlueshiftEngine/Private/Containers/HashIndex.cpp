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
#include "Core/Str.h"
#include "Core/WStr.h"
#include "Core/Guid.h"
#include "Math/Math.h"
#include "Containers/HashIndex.h"

BE_NAMESPACE_BEGIN

int HashIndex::EmptyTable[1] = { -1 };

HashIndex::HashIndex(const int initialHashSize, const int initialIndexSize) {
    assert(Math::IsPowerOfTwo(initialHashSize));

    hashSize = initialHashSize;
    hashTable = EmptyTable;

    indexSize = initialIndexSize;
    indexChain = EmptyTable;

    granularity = DefaultIndexGranularity;
    hashMask = hashSize - 1;
    lookUpMask = 0;
}

HashIndex::HashIndex(const HashIndex &other) {
    *this = other;
}

void HashIndex::Allocate(const int newHashSize, const int newIndexSize) {
    assert(Math::IsPowerOfTwo(newHashSize));

    Free();

    hashSize = newHashSize;
    hashTable = new int [hashSize];
    memset(hashTable, 0xFF, hashSize * sizeof(hashTable[0]));

    indexSize = newIndexSize;
    indexChain = new int [indexSize];
    memset(indexChain, 0xFF, indexSize * sizeof(indexChain[0]));
    
    hashMask = hashSize - 1;
    lookUpMask = -1;
}

void HashIndex::Free() {
    if (hashTable != EmptyTable) {
        delete [] hashTable;
        hashTable = EmptyTable;
    }

    if (indexChain != EmptyTable) {
        delete [] indexChain;
        indexChain = EmptyTable;
    }
    lookUpMask = 0;
}

HashIndex &HashIndex::operator=(const HashIndex &rhs) {
    granularity = rhs.granularity;
    hashMask = rhs.hashMask;
    lookUpMask = rhs.lookUpMask;

    if (rhs.lookUpMask == 0) {
        hashSize = rhs.hashSize;
        indexSize = rhs.indexSize;
        Free();
    } else {
        if (rhs.hashSize != hashSize || hashTable == EmptyTable) {
            if (hashTable != EmptyTable) {
                delete[] hashTable;
            }

            hashSize = rhs.hashSize;
            hashTable = new int[hashSize];
        }

        if (rhs.indexSize != indexSize || indexChain == EmptyTable) {
            if (indexChain != EmptyTable) {
                delete[] indexChain;
            }

            indexSize = rhs.indexSize;
            indexChain = new int[indexSize];
        }
        memcpy(hashTable, rhs.hashTable, hashSize * sizeof(hashTable[0]));
        memcpy(indexChain, rhs.indexChain, indexSize * sizeof(indexChain[0]));
    }

    return *this;
}

void HashIndex::ResizeIndex(const int newIndexSize) {
    if (newIndexSize <= indexSize) {
        return;
    }

    int newSize;
    int mod = newIndexSize % granularity;
    if (!mod) {
        newSize = newIndexSize;
    } else {
        newSize = newIndexSize + granularity - mod;
    }

    if (indexChain == EmptyTable) {
        // 초기화된 상태이므로 indexSize 만 변경하고 나온다.
        indexSize = newSize;
        return;
    }

    int *oldIndexChain = indexChain;

    // allocate new index chain data
    indexChain = new int [newSize];
    // copy old index chain data
    memcpy(indexChain, oldIndexChain, indexSize * sizeof(int));
    // fill -1 for redundant values
    memset(indexChain + indexSize, 0xFF, (newSize - indexSize) * sizeof(int)); 
    // delete old index chain data
    delete [] oldIndexChain;
    // update new index size
    indexSize = newSize;
}

float HashIndex::GetStandardDeviation() const {
    if (hashTable == EmptyTable) {
        return 0;
    }

    int totalItems = 0;

    int *numHashItems = new int [hashSize];

    for (int i = 0; i < hashSize; i++) {
        numHashItems[i] = 0;
        for (int index = hashTable[i]; index >= 0; index = indexChain[index]) {
            numHashItems[i]++;
        }
        totalItems += numHashItems[i];
    }

    // if no items in hash
    if (totalItems <= 1) {
        delete [] numHashItems;
        return 0;
    }

    int average = totalItems / hashSize;
    int s = 0;
    for (int i = 0; i < hashSize; i++) {
        int e = numHashItems[i] - average;
        s += e * e;
    }

    delete [] numHashItems;

    float v = (float)s / totalItems;
    return sqrtf(v);
}

int HashIndex::GenerateHash(const char *string, bool caseSensitive) const {
    if (caseSensitive) {
        return (Str::Hash(string) & hashMask);
    } else {
        return (Str::IHash(string) & hashMask);
    }
}

int HashIndex::GenerateHash(const wchar_t *string, bool caseSensitive) const {
    if (caseSensitive) {
        return (WStr::Hash(string) & hashMask);
    } else {
        return (WStr::IHash(string) & hashMask);
    }
}

int HashIndex::GenerateHash(const Vec3 &v) const {
    return ((((int)v[0]) + ((int)v[1]) + ((int)v[2])) & hashMask);
}

int HashIndex::GenerateHash(const Guid &guid) const {
    return (Guid::Hash(guid) & hashMask);
}

BE_NAMESPACE_END
