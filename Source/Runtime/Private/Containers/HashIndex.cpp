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

#include "Precompiled.h"
#include "Core/Str.h"
#include "Core/Guid.h"
#include "Math/Math.h"
#include "Containers/HashIndex.h"

BE_NAMESPACE_BEGIN

int HashIndex::EmptyTable[1] = { -1 };

HashIndex::HashIndex(const int initialHashSize, const int initialIndexSize) {
    assert(IsPowerOfTwo(initialHashSize));

    hashSize = initialHashSize;
    hashTable = EmptyTable;

    indexSize = initialIndexSize;
    indexChain = EmptyTable;

    granularity = DefaultIndexGranularity;
    hashMask = hashSize - 1;
    lookUpMask = 0;
}

void HashIndex::Allocate(const int newHashSize, const int newIndexSize) {
    assert(IsPowerOfTwo(newHashSize));

    Free();

    hashSize = newHashSize;
    hashTable = BE_NEW int [hashSize];
    memset(hashTable, 0xFF, hashSize * sizeof(hashTable[0]));

    indexSize = newIndexSize;
    indexChain = BE_NEW int [indexSize];
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
    if (this != &rhs) {
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
                hashTable = BE_NEW int[hashSize];
            }

            if (rhs.indexSize != indexSize || indexChain == EmptyTable) {
                if (indexChain != EmptyTable) {
                    delete[] indexChain;
                }

                indexSize = rhs.indexSize;
                indexChain = BE_NEW int[indexSize];
            }
            memcpy(hashTable, rhs.hashTable, hashSize * sizeof(hashTable[0]));
            memcpy(indexChain, rhs.indexChain, indexSize * sizeof(indexChain[0]));
        }
    }
    return *this;
}

HashIndex &HashIndex::operator=(HashIndex &&rhs) noexcept {
    if (this != &rhs) {
        BE1::Swap(hashTable, rhs.hashTable);
        BE1::Swap(indexChain, rhs.indexChain);
        BE1::Swap(hashSize, rhs.hashSize);
        BE1::Swap(indexSize, rhs.indexSize);
        BE1::Swap(granularity, rhs.granularity);
        BE1::Swap(hashMask, rhs.hashMask);
        BE1::Swap(lookUpMask, rhs.lookUpMask);
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
        // Since it was initialized, just set indexSize.
        indexSize = newSize;
        return;
    }

    int *oldIndexChain = indexChain;

    // Allocate new index chain data.
    indexChain = BE_NEW int [newSize];
    // Copy old index chain data.
    memcpy(indexChain, oldIndexChain, indexSize * sizeof(int));
    // Fill -1 for redundant values.
    memset(indexChain + indexSize, 0xFF, (newSize - indexSize) * sizeof(int)); 
    // Delete old index chain data.
    delete [] oldIndexChain;
    // Update new index size.
    indexSize = newSize;
}

float HashIndex::GetVariance() const {
    if (hashTable == EmptyTable) {
        return 0;
    }

    int totalItems = 0;

    int *numHashItems = BE_NEW int [hashSize];

    for (int i = 0; i < hashSize; i++) {
        numHashItems[i] = 0;
        for (int index = hashTable[i]; index >= 0; index = indexChain[index]) {
            numHashItems[i]++;
        }
        totalItems += numHashItems[i];
    }

    // If no items in hash..
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

    return (float)s / totalItems;
}

int HashIndex::GenerateHash(const char *string, bool caseSensitive) const {
    if (caseSensitive) {
        return (Str::Hash32(string) & hashMask);
    } else {
        return (Str::IHash32(string) & hashMask);
    }
}

BE_NAMESPACE_END
