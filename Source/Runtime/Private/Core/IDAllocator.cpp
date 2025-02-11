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
#include "Core/IDAllocator.h"
#include "Platform/Intrinsics.h"

BE_NAMESPACE_BEGIN

// Count the number of zero bits in a block
static size_t CountZeroBits(uint64_t block) {
    return 64 - CountSetBits(block);
}

// Find the position of the first zero bit
static int FindFirstZeroBit(uint64_t block) {
    uint32_t index = CountTrailingZeros(~block);
    if (index >= 64) {
        return -1;
    }
    return index;
}

void IDAllocator::Init(size_t maxIdCount) {
    maxId = maxIdCount - 1;
    bitmap.SetCount((maxIdCount + 63) / 64);
    maxFreeBitsPerBlock.SetCount(bitmap.Count());

    Clear();
}

void IDAllocator::Clear() {
    // Initialize in blocks of 64 bits
    bitmap.Fill(0);

    // Initial value: all bits are 0
    maxFreeBitsPerBlock.Fill(64);
}

bool IDAllocator::AllocateID(uint32_t &id) {
    for (size_t blockIndex = 0; blockIndex < bitmap.Count(); ++blockIndex) {
        // If the block is not fully occupied
        if (bitmap[blockIndex] != ~0ULL) {
            int bitIndex = FindFirstZeroBit(bitmap[blockIndex]);
            if (bitIndex >= 0) {
                // Set the bit to 1
                bitmap[blockIndex] |= (1ULL << bitIndex);
                // Decrease the count of consecutive free bits
                maxFreeBitsPerBlock[blockIndex]--;
                id = static_cast<uint32_t>(64 * blockIndex + bitIndex);
                return true;
            }
        }
    }
    // No available ID
    return false;
}

bool IDAllocator::AllocateRange(uint32_t &startID, size_t count) {
    size_t blocksNeeded = (count + 63) / 64;

    for (size_t blockIndex = 0; blockIndex <= bitmap.Count() - blocksNeeded; ++blockIndex) {
        if (maxFreeBitsPerBlock[blockIndex] >= count) {
            // Check if there are enough consecutive free bits
            size_t startBitIndex = FindConsecutiveZeroBits(blockIndex, count);
            if (startBitIndex != SIZE_MAX) {
                startID = static_cast<uint32_t>(64 * blockIndex + startBitIndex);
                // Mark the range as occupied
                SetBits(blockIndex, startBitIndex, count);
                return true;
            }
        }
    }
    // No available consecutive range
    return false;
}

void IDAllocator::FreeID(uint32_t id) {
    size_t blockIndex = id / 64;
    size_t bitIndex = id % 64;

    // Set the bit to 0
    bitmap[blockIndex] &= ~(1ULL << bitIndex);
    // Increase the count of consecutive free bits
    maxFreeBitsPerBlock[blockIndex]++;
}

void IDAllocator::FreeRange(uint32_t startID, size_t count) {
    size_t globalStartIndex = startID;
    size_t globalEndIndex = startID + count;

    for (size_t i = globalStartIndex; i < globalEndIndex; ++i) {
        size_t blockIndex = i / 64;
        size_t bitIndex = i % 64;

        // Set the bit to 0
        bitmap[blockIndex] &= ~(1ULL << bitIndex);
        maxFreeBitsPerBlock[blockIndex]++;
    }
}

size_t IDAllocator::GetAvailableCount() const {
    size_t totalFree = 0;
    for (size_t blockIndex = 0; blockIndex < bitmap.Count(); ++blockIndex) {
        totalFree += CountZeroBits(bitmap[blockIndex]);
    }
    return totalFree;
}

//  Find consecutive zero bits
size_t IDAllocator::FindConsecutiveZeroBits(size_t blockIndex, size_t count) const {
    uint64_t block = bitmap[blockIndex];
    size_t consecutiveZeros = 0;
    size_t startBitIndex = SIZE_MAX;

    for (size_t bitIndex = 0; bitIndex < 64; ++bitIndex) {
        if (!(block & (1ULL << bitIndex))) {
            if (consecutiveZeros == 0) {
                // Start of a new range
                startBitIndex = bitIndex;
            }
            consecutiveZeros++;
            if (consecutiveZeros == count) {
                return startBitIndex;
            }
        } else {
            // Consecutive free bits are interrupted
            consecutiveZeros = 0;
        }
    }
    // No consecutive range found
    return SIZE_MAX;
}

// Mark a specific range of bits as occupied (set to 1)
void IDAllocator::SetBits(size_t blockIndex, size_t startBitIndex, size_t count) {
    uint64_t mask = ((1ULL << count) - 1) << startBitIndex;
    // Set the range to 1
    bitmap[blockIndex] |= mask;
    // Decrease the count of consecutive free bits
    maxFreeBitsPerBlock[blockIndex] -= count;
}

BE_NAMESPACE_END
