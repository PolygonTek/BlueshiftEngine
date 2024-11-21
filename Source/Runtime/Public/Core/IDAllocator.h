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

#include "Containers/Array.h"

BE_NAMESPACE_BEGIN

class IDAllocator {
public:
    IDAllocator() = default;
    IDAllocator(size_t maxIdCount) { Init(maxIdCount); }

                        /// Initialize ID allocator.
    void                Init(size_t maxIdCount);

                        /// Clear all IDs
    void                Clear();

                        /// Allocate a single ID.
    bool                AllocateID(uint32_t &id);

                        /// Allocate a consecutive range of IDs.
    bool                AllocateRange(uint32_t &startID, size_t count);

                        /// Free a single ID.
    void                FreeID(uint32_t id);

                        /// Free a consecutive range of IDs.
    void                FreeRange(uint32_t startID, size_t count);

                        /// Get the count of available IDs.
    size_t              GetAvailableCount() const;

private:
    size_t              FindConsecutiveZeroBits(size_t blockIndex, size_t count) const;
    void                SetBits(size_t blockIndex, size_t startBitIndex, size_t count);

    size_t              maxId = 0;              // Maximum number of IDs
    Array<uint64_t>     bitmap;                 // Bitmap (64 bits per block)
    Array<size_t>       maxFreeBitsPerBlock;    // Maximum consecutive free bits per block
};

BE_NAMESPACE_END
