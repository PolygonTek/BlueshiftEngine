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

#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

/// Block based allocator for fixed size objects.
template <typename T, int N>
class BlockAllocator {
public:
    static constexpr int Alignment = 16;

    /// Constructor with the options of cleared allocation.
    BlockAllocator(bool clear = false);
    /// Prevents copy constructor.
    BlockAllocator(const BlockAllocator<T, N> &rhs) = delete;
    /// Destructor.
    ~BlockAllocator();

                        /// Prevents assignment operator.
    BlockAllocator<T, N> &operator=(const BlockAllocator<T, N> &rhs) = delete;

                        /// Returns the count of allocated objects.
    int                 GetAllocCount() const { return allocCount; }

                        /// Returns total size of allocated memory.
    size_t              Allocated() const { return totalCount * sizeof(T); }

                        /// Returns total size of allocated memory including size of (*this).
    size_t              Size() const { return sizeof(*this) + Allocated(); }

                        /// Allocates a new object.
    T *                 Alloc();

                        /// Free an object.
    void                Free(T *obj);

                        /// Reserves blocks.
    void                ReserveBlocks(int numBlocks);
   
                        /// Free empty blocks and rebuild free elements chain. 
    void                FreeEmptyBlocks();

                        /// Free all allocated blocks.
                        /// This function doesn't call destructor for each allocated objects.
    void                FreeAllBlocks();

private:
    union Element {
        T *             data;                   // T *
        Element *       next;                   // next free element
        byte            buffer[AlignUp(Max(sizeof(T), sizeof(Element *)), Alignment)];  // actual data that is aligned by
    };

    struct Block {
        Element         elements[N];            // each blocks have fixed count of elements
        Block *         next;                   // next block
        Element *       free;                   // list with free elements in this block (temp used only by FreeEmptyBlocks)
        int             freeCount;              // number of free elements in this block (temp used only by FreeEmptyBlocks)
    };

    void                AllocNewBlock();

    Block *             blocks;                 ///< Head pointer of the blocks
    Element *           freeElements;           ///< Head pointer of the free elements
    int                 totalCount;             ///< Total count of the allocated objects
    int                 allocCount;             ///< Total count of the allocated objects in used
    bool                clearedAllocs;
};

template <typename T, int N>
BE_INLINE BlockAllocator<T, N>::BlockAllocator(bool clear) :
    blocks(nullptr),
    freeElements(nullptr),
    totalCount(0),
    allocCount(0),
    clearedAllocs(clear) {
}

template <typename T, int N>
BE_INLINE BlockAllocator<T, N>::~BlockAllocator() {
    FreeAllBlocks();
}

template <typename T, int N>
BE_INLINE void BlockAllocator<T, N>::AllocNewBlock() {
    // Allocates new block
    Block *block = (Block *)Mem_Alloc16(sizeof(Block));
    block->next = blocks;
    blocks = block;

    for (int i = 0; i < N; i++) {
        block->elements[i].next = freeElements;
        freeElements = &block->elements[i];
        // Each elements address should be aligned by 16 bytes boundary.
        assert(IsAligned((uintptr_t)freeElements, Alignment));
    }
    totalCount += N;
}

template <typename T, int N>
BE_INLINE void BlockAllocator<T, N>::FreeAllBlocks() {
    while (blocks) {
        Block *block = blocks;
        blocks = blocks->next;
        Mem_AlignedFree(block);
    }
    blocks = nullptr;
    freeElements = nullptr;
    totalCount = 0;
    allocCount = 0;
}

template <typename T, int N>
BE_INLINE T *BlockAllocator<T, N>::Alloc() {
    if (!freeElements) {
        AllocNewBlock();
    }

    allocCount++;
    Element *element = freeElements;
    freeElements = freeElements->next;
    element->next = nullptr;

    T *t = (T *)element->buffer;
    if (clearedAllocs) {
        memset(t, 0, sizeof(T));
    }
    new (t) T;
    return t;
}

template <typename T, int N>
BE_INLINE void BlockAllocator<T, N>::Free(T *obj) {
    if (!obj) {
        return;
    }

    obj->~T();

    Element *element = (Element *)(obj);
    element->next = freeElements;
    freeElements = element;
    allocCount--;
}

template <typename T, int N>
BE_INLINE void BlockAllocator<T, N>::ReserveBlocks(int numBlocks) {
    int currentNumBlocks = 0;
    for (Block *block = blocks; block; block = block->next) {
        currentNumBlocks++;
    }
    for (int i = currentNumBlocks; i < numBlocks; i++) {
        AllocNewBlock();
    }
}

template <typename T, int N>
BE_INLINE void BlockAllocator<T, N>::FreeEmptyBlocks() {
    // first count how many free elements are in each block
    // and build up a free chain per block
    for (Block *block = blocks; block; block = block->next) {
        block->free = nullptr;
        block->freeCount = 0;
    }

    Element *nextElement;
    for (Element *element = freeElements; element; element = nextElement) {
        nextElement = element->next;

        for (Block *block = blocks; block; block = block->next) {
            if (element >= &block->elements[0] && element < &block->elements[N]) {
                element->next = block->free;
                block->free = element;
                block->freeCount++;
                break;
            }
        }
        // if this assert fires, we couldn't find the element in any block
        assert(element->next != nextElement);
    }

    // now free all blocks whose free count == N
    Block *prevBlock = nullptr;
    Block *nextBlock;
    for (Block *block = blocks; block; block = nextBlock) {
        nextBlock = block->next;

        if (block->freeCount == N) {
            if (!prevBlock) {
                assert(blocks == block);
                blocks = block->next;
            } else {
                assert(prevBlock->next == block);
                prevBlock->next = block->next;
            }
            Mem_AlignedFree(block);
            totalCount -= N;
        } else {
            prevBlock = block;
        }
    }

    // now rebuild the free chain
    freeElements = nullptr;
    for (Block *block = blocks; block; block = block->next) {
        for (Element *element = block->free; element; element = nextElement) {
            nextElement = element->next;
            element->next = freeElements;
            freeElements = element;
        }
    }
}

BE_NAMESPACE_END
