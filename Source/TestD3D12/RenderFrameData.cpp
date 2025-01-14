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
#include "RenderFrameData.h"
#include "VisObject.h"

static constexpr uint32_t MaxMemSizePerBlock = 0x1000000;
static constexpr uint32_t MemAlignSize = 32;

void RenderFrameData::Init(int numThreads) {
    InitMemBlocks();

    this->numThreads = numThreads;

    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        threadData[threadIndex] = RHI::renderer->CreateFrameThreadData();
    }
}

void RenderFrameData::Shutdown() {
    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        RHI::renderer->DestroyFrameThreadData(threadData[threadIndex]);
    }

    FreeVisCamera();

    FreeVisObjects();

    ClearMemBlocks();
}

void RenderFrameData::BeginFrame() {
    // 쓰레드 별로 사용할 자원을 Reset 한다.
    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        threadData[threadIndex]->BeginFrame();
    }

    // 이번 프레임에 사용할 프레임 데이터를 사용하기 위해서는, GPU 에서 이전 프레임에 대한 렌더링이 완료되야 한다.
    RHI::renderer->WaitFence(fenceValue);
}

void RenderFrameData::EndFrame() {
    fenceValue = RHI::renderer->SignalFence(RHI::CommandQueueType::Graphics);
}

void RenderFrameData::InitMemBlocks() {
    headBlock = AllocMemBlock();
    currentBlock = headBlock;
}

void RenderFrameData::ClearMemBlocks() {
    MemBlock *nextBlock;

    for (MemBlock *block = headBlock; block; block = nextBlock) {
        nextBlock = block->next;
        Mem_Free(block);
    }

    headBlock = nullptr;
    currentBlock = nullptr;
}

RenderFrameData::MemBlock *RenderFrameData::AllocMemBlock() {
    MemBlock *block = (MemBlock *)Mem_Alloc(sizeof(*block) + MemAlignSize - 1 + MaxMemSizePerBlock);
    if (!block) {
        BE_FATALERROR("RenderFrameData::AllocMemBlock: failed to allocate memory");
    }

    block->next = nullptr;
    block->size = MaxMemSizePerBlock;
    block->used = 0;
    block->base = (byte *)BE1::AlignUp((intptr_t)block + sizeof(*block), MemAlignSize);
    return block;
}

void *RenderFrameData::MemAlloc(int size) {
    size = BE1::AlignUp(size, MemAlignSize);
    if (size > MaxMemSizePerBlock) {
        BE_FATALERROR("RenderFrameData::MemAlloc: %i exceeded MaxMemSizePerBlock", size);
    }

    for (MemBlock *block = currentBlock; block; block = block->next) {
        if (block->size - block->used >= size) {
            void *alloc = block->base + block->used;
            block->used += size;
            currentBlock = block;
            return alloc;
        }
    }

    MemBlock *newBlock = AllocMemBlock();
    currentBlock->next = newBlock;
    currentBlock = newBlock;
    currentBlock->used = size;

    return currentBlock->base;
}

void *RenderFrameData::ClearedMemAlloc(int size) {
    void *mem = MemAlloc(size);
    BE1::simdProcessor->Memset(mem, 0, size);
    return mem;
}

void RenderFrameData::ClearMemAllocs() {
    FreeVisCamera();

    FreeVisObjects();

    // Reset the mem allocation to the first block.
    currentBlock = headBlock;

    // Clear all the blocks.
    for (MemBlock *block = headBlock; block; block = block->next) {
        block->used = 0;
    }
}

VisCamera *RenderFrameData::AllocVisCamera() {
    assert(!visCamera);

    this->visCamera = (VisCamera *)MemAlloc(sizeof(VisCamera));

    // placement new 로 생성자 호출
    new (visCamera) VisCamera();

    return visCamera;
}

void RenderFrameData::FreeVisCamera() {
    if (!visCamera) {
        return;
    }

    // 소멸자 호출
    (visCamera)->~VisCamera();
    
    visCamera = nullptr;
}

VisObject *RenderFrameData::AllocVisObjects(int numVisObjects) {
    assert(!visObjects);

    this->numVisObjects = numVisObjects;
    this->visObjects = (VisObject *)MemAlloc(sizeof(VisObject) * numVisObjects);

    // placement new 로 생성자 호출
    for (int visObjectIndex = 0; visObjectIndex < numVisObjects; ++visObjectIndex) {
        new (visObjects + visObjectIndex) VisObject();
    }
    return visObjects;
}

void RenderFrameData::FreeVisObjects() {
    if (!visObjects) {
        return;
    }

    // 소멸자 호출
    for (int visObjectIndex = 0; visObjectIndex < numVisObjects; ++visObjectIndex) {
        (visObjects + visObjectIndex)->~VisObject();
    }

    visObjects = nullptr;
    numVisObjects = 0;
}
