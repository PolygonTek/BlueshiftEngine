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
#include "Simd/Simd.h"

BE_NAMESPACE_BEGIN

static const int MEMORY_BLOCK_SIZE = 0x100000;

FrameData   frameData;

void FrameData::Init() {
    Shutdown();

    int size = MEMORY_BLOCK_SIZE;
    MemBlock *block = (MemBlock *)Mem_Alloc(sizeof(*block) + 15 + size);
    if (!block) {
        BE_FATALERROR(L"FrameData::Init: failed to allocate memory");
    }
    block->base = (byte *)AlignUp((intptr_t)block + sizeof(*block), 16);
    block->size = size;
    block->used = 0;
    block->next = nullptr;

    this->mem = block;
    this->alloc = block;
    this->commands.used = 0;
}

void FrameData::Shutdown() {
    MemBlock *nextBlock;
    for (MemBlock *block = this->mem; block; block = nextBlock) {
        nextBlock = block->next;
        Mem_Free(block);
    }
    
    this->mem = nullptr;
}

void FrameData::ToggleFrame() {
    // reset the mem allocation to the first block
    this->alloc = this->mem;

    // clear all the blocks
    for (MemBlock *block = this->mem; block; block = block->next) {
        block->used = 0;
    }
}

void *FrameData::Alloc(int bytes) {
    bytes = AlignUp(bytes, 16);
    MemBlock *block = this->alloc;

    if (block->size - block->used >= bytes) {
        void *buf = block->base + block->used;
        block->used += bytes;
        return buf;
    }

    // advance to the next mem block if available
    block = block->next;
    // create a new block if we are at the end of the chain
    if (!block) {
        int size = MEMORY_BLOCK_SIZE;
        block = (MemBlock *)Mem_Alloc(sizeof(*block) + 15 + size);
        if (!block) {
            BE_FATALERROR(L"FrameData::Alloc: Mem_Alloc() failed");
        }
        block->base = (byte *)AlignUp((intptr_t)block + sizeof(*block), 16);
        block->size = size;
        block->used = 0;
        block->next = nullptr;
        this->alloc->next = block;
    }

    if (bytes > block->size) {
        BE_FATALERROR(L"FrameData::Alloc of %i exceeded MEMORY_BLOCK_SIZE", bytes);
    }

    this->alloc = block;
    block->used = bytes;

    return block->base;
}

void *FrameData::ClearedAlloc(int bytes) {
    void *r = Alloc(bytes);
    simdProcessor->Memset(r, 0, bytes);
    //memset(r, 0, bytes);
    return r;
}

BE_NAMESPACE_END
