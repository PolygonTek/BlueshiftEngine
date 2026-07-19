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

#include "Core/Heap.h"
#include "Containers/Array.h"

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
    BlockAllocator<T, N> &  operator=(const BlockAllocator<T, N> &rhs) = delete;

                            /// Returns the count of allocated objects.
    int                     GetAllocCount() const { return allocCount; }

                            /// Returns total size of allocated memory.
    size_t                  Allocated() const { return totalCount * sizeof(T); }

                            /// Returns total size of allocated memory including size of (*this).
    size_t                  Size() const { return sizeof(*this) + Allocated(); }

                            /// Allocates a new object.
    T *                     Alloc();

                            /// Free an object.
    void                    Free(T *obj);

                            /// Reserves blocks.
    void                    ReserveBlocks(int numBlocks);
   
                            /// Free empty blocks and rebuild free elements chain.
    void                    FreeEmptyBlocks();

                            /// Free all allocated blocks.
                            /// This function doesn't call destructor for each allocated objects.
    void                    FreeAllBlocks();

private:
    union Element {
        T *                 data;                   // T *
        Element *           next;                   // next free element
        byte                buffer[AlignUp(Max(sizeof(T), sizeof(Element *)), Alignment)];  // actual data that is aligned by
    };

    struct Block {
        Element             elements[N];            // each blocks have fixed count of elements
        Block *             next;                   // next block
        Element *           free;                   // list with free elements in this block (temp used only by FreeEmptyBlocks)
        int                 freeCount;              // number of free elements in this block (temp used only by FreeEmptyBlocks)
    };

    void                    AllocNewBlock();

    Block *                 blocks;                 ///< Head pointer of the blocks
    Element *               freeElements;           ///< Head pointer of the free elements
    int                     totalCount;             ///< Total count of the allocated objects
    int                     allocCount;             ///< Total count of the allocated objects in used
    bool                    clearedAllocs;
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

/// Block based linear allocator for variable size allocations.
/// Allocated memory stays valid until Reset() or Shutdown().
class LinearAllocator {
public:
    LinearAllocator() = default;
    LinearAllocator(size_t blockSize, int alignment = 32);
    LinearAllocator(const LinearAllocator &rhs) = delete;
    ~LinearAllocator();

    LinearAllocator &       operator=(const LinearAllocator &rhs) = delete;

    void                    Init(size_t blockSize, int alignment = 32);
    void                    Shutdown();
    void                    Reset();

    [[nodiscard]] void *    Alloc(size_t size);
    [[nodiscard]] void *    ClearedAlloc(size_t size);

    size_t                  GetBlockSize() const { return blockSize; }
    int                     GetAlignment() const { return alignment; }

private:
    struct Block {
        Block *             next;
        size_t              size;
        size_t              used;
        byte *              base;
    };

    [[nodiscard]] Block *   AllocBlock(size_t minimumSize);

    Block *                 headBlock = nullptr;
    Block *                 currentBlock = nullptr;
    size_t                  blockSize = 0;
    int                     alignment = 0;
};

BE_INLINE LinearAllocator::LinearAllocator(size_t blockSize, int alignment) {
    Init(blockSize, alignment);
}

BE_INLINE LinearAllocator::~LinearAllocator() {
    Shutdown();
}

BE_INLINE void LinearAllocator::Init(size_t blockSize, int alignment) {
    assert(!headBlock && !currentBlock);
    assert(blockSize > 0);
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0);

    this->blockSize = blockSize;
    this->alignment = alignment;
    headBlock = AllocBlock(blockSize);
    currentBlock = headBlock;
}

BE_INLINE void LinearAllocator::Shutdown() {
    Block *nextBlock;

    for (Block *block = headBlock; block; block = nextBlock) {
        nextBlock = block->next;
        Mem_Free(block);
    }

    headBlock = nullptr;
    currentBlock = nullptr;
    blockSize = 0;
    alignment = 0;
}

BE_INLINE void LinearAllocator::Reset() {
    assert(headBlock);

    currentBlock = headBlock;

    for (Block *block = headBlock; block; block = block->next) {
        block->used = 0;
    }
}

BE_INLINE LinearAllocator::Block *LinearAllocator::AllocBlock(size_t minimumSize) {
    const size_t size = Max(blockSize, minimumSize);
    Block *block = (Block *)Mem_Alloc(sizeof(*block) + alignment - 1 + size);
    assert(block);

    block->next = nullptr;
    block->size = size;
    block->used = 0;
    block->base = (byte *)AlignUp((uintptr_t)block + sizeof(*block), alignment);
    return block;
}

BE_INLINE void *LinearAllocator::Alloc(size_t size) {
    assert(currentBlock);

    const size_t alignedSize = AlignUp(size, alignment);
    Block *lastBlock = currentBlock;

    for (Block *block = currentBlock; block; block = block->next) {
        lastBlock = block;
        if (block->size - block->used >= alignedSize) {
            void *alloc = block->base + block->used;
            block->used += alignedSize;
            currentBlock = block;
            return alloc;
        }
    }

    Block *newBlock = AllocBlock(alignedSize);
    lastBlock->next = newBlock;
    currentBlock = newBlock;
    currentBlock->used = alignedSize;
    return currentBlock->base;
}

BE_INLINE void *LinearAllocator::ClearedAlloc(size_t size) {
    void *mem = Alloc(size);
    memset(mem, 0, size);
    return mem;
}

/// BlockAllocator based allocator for transient objects.
/// Returned object addresses stay valid until Reset() or Shutdown().
template <typename T, int N>
class DynamicObjectAllocator {
public:
    DynamicObjectAllocator(bool clear = false) :
        blockAllocator(clear) {
    }

    DynamicObjectAllocator(const DynamicObjectAllocator<T, N> &rhs) = delete;
    ~DynamicObjectAllocator() {
        Shutdown();
    }

    DynamicObjectAllocator<T, N> &operator=(const DynamicObjectAllocator<T, N> &rhs) = delete;

    int GetAllocCount() const { return allocatedObjects.Count(); }

    void Reserve(int objectCount) {
        blockAllocator.ReserveBlocks((objectCount + N - 1) / N);
        allocatedObjects.Reserve(objectCount);
    }

    T &Alloc() {
        T *object = blockAllocator.Alloc();
        allocatedObjects.Append(object);
        return *object;
    }

    void Reset() {
        for (T *object : allocatedObjects) {
            blockAllocator.Free(object);
        }
        allocatedObjects.SetCount(0, false);
    }

    void Shutdown() {
        Reset();
        blockAllocator.FreeAllBlocks();
        allocatedObjects.Clear();
    }

    void FreeEmptyBlocks() {
        blockAllocator.FreeEmptyBlocks();
    }

private:
    BlockAllocator<T, N>    blockAllocator;
    Array<T *>              allocatedObjects;
};

BE_NAMESPACE_END
