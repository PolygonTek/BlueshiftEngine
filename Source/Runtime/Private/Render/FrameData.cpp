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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Heap.h"
#include "SIMD/SIMD.h"

BE_NAMESPACE_BEGIN

static constexpr int MaxMemSizePerBlock = 0x100000;
static constexpr int AlignSize = 32;

FrameData   frameData;

void FrameData::Init() {
    Shutdown();

    headBlock = AllocBlock();
    currentBlock = headBlock;
    commands.used = 0;
}

void FrameData::Shutdown() {
    MemBlock *nextBlock;

    for (MemBlock *block = headBlock; block; block = nextBlock) {
        nextBlock = block->next;
        Mem_Free(block);
    }

    headBlock = nullptr;
    currentBlock = nullptr;
}

FrameData::MemBlock *FrameData::AllocBlock() {
    MemBlock *block = (MemBlock *)Mem_Alloc(sizeof(*block) + AlignSize - 1 + MaxMemSizePerBlock);
    if (!block) {
        BE_FATALERROR("FrameData::AllocBlock: failed to allocate memory");
    }

    block->base = (byte *)AlignUp((intptr_t)block + sizeof(*block), AlignSize);
    block->size = MaxMemSizePerBlock;
    block->used = 0;
    block->next = nullptr;
    return block;
}

void FrameData::ToggleFrame() {
    // Reset the mem allocation to the first block.
    currentBlock = headBlock;

    // Clear all the blocks.
    for (MemBlock *block = headBlock; block; block = block->next) {
        block->used = 0;
    }
}

void *FrameData::Alloc(int size) {
    size = AlignUp(size, AlignSize);
    if (size > MaxMemSizePerBlock) {
        BE_FATALERROR("FrameData::Alloc: %i exceeded MaxMemSizePerBlock", size);
    }

    for (MemBlock* block = currentBlock; block; block = block->next) {
        if (block->size - block->used >= size) {
            void *alloc = block->base + block->used;
            block->used += size;
            currentBlock = block;
            return alloc;
        }
    }

    MemBlock *newBlock = AllocBlock();
    currentBlock->next = newBlock;
    currentBlock = newBlock;
    currentBlock->used = size;

    return currentBlock->base;
}

void *FrameData::ClearedAlloc(int size) {
    void *mem = Alloc(size);
    simdProcessor->Memset(mem, 0, size);
    return mem;
}

BE_NAMESPACE_END
