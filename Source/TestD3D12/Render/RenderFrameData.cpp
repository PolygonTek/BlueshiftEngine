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
#include "RenderFrameData.h"
#include "RenderInternal.h"

static constexpr uint32_t   MaxRenderTaskThreads = 32;
static constexpr uint32_t   MaxMemSizePerBlock = 0x1000000;
static constexpr uint32_t   MemAlignSize = 32;

void RenderFrameData::Init() {
    InitMemBlocks();

#ifdef USE_RENDER_TASK
    // 렌더 태스크 스레드 별 데이터를 준비한다.
    // 렌더 태스크 스레드는 태스크 매니져의 스레드 개수를 넘을 수 없다.
    // 태스크 매니져의 스레드는 물리 코어 개수만큼 미리 생성된다.
    numRenderTaskThreads = BE1::Min(BE1::Engine::taskManager->NumThreads(), MaxRenderTaskThreads);
#else
    numRenderTaskThreads = 1;
#endif

    threadData = new RHI::FrameThreadData *[numRenderTaskThreads];

    for (int threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        threadData[threadIndex] = RHI::renderer->CreateFrameThreadData();
    }
}

void RenderFrameData::Shutdown() {
    for (int threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        RHI::renderer->DestroyFrameThreadData(threadData[threadIndex]);
    }

    delete [] threadData;

    FreeVisObjects();

    FreeVisCameras();

    ClearMemBlocks();
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

void RenderFrameData::BeginFrameMemAllocs() {
    FreeVisCameras();

    FreeVisObjects();

    // Reset the mem allocation to the first block.
    currentBlock = headBlock;

    // Clear all the blocks.
    for (MemBlock *block = headBlock; block; block = block->next) {
        block->used = 0;
    }

    InitVisCameras(16);

    InitVisObjects(16384);
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

void RenderFrameData::InitVisCameras(uint32_t maxVisCameras) {
    assert(!visCameras);

    this->maxVisCameras = maxVisCameras;
    this->numVisCameras = 0;
    this->visCameras = (VisCamera *)MemAlloc(sizeof(VisCamera) * maxVisCameras);
}

void RenderFrameData::FreeVisCameras() {
    if (!visCameras) {
        return;
    }

    // 소멸자 호출
    for (int i = 0; i < numVisCameras; ++i) {
        (visCameras + i)->~VisCamera();
    }

    visCameras = nullptr;
    numVisCameras = 0;
}

// TODO: 한번에 할당하지 않고, AllocVisObject 에서 최대 개수를 넘어가도 새로운 chunk 를 할당하는 방식으로 수정할 것
void RenderFrameData::InitVisObjects(int maxVisObjects) {
    assert(!visObjects);

    this->maxVisObjects = maxVisObjects;
    this->numVisObjects = 0;
    this->visObjects = (VisObject *)MemAlloc(sizeof(VisObject) * maxVisObjects);
}

void RenderFrameData::FreeVisObjects() {
    if (!visObjects) {
        return;
    }

    // 소멸자 호출
    for (int i = 0; i < numVisObjects; ++i) {
        (visObjects + i)->~VisObject();
    }

    visObjects = nullptr;
    numVisObjects = 0;
}

VisCamera *RenderFrameData::AllocVisCamera() {
    if (numVisCameras + 1 > maxVisCameras) {
        return nullptr;
    }

    // placement new 생성자 호출
    VisCamera *newVisCamera = &visCameras[numVisCameras];
    new (newVisCamera) VisCamera();

    newVisCamera->index = numVisCameras++;

    return newVisCamera;
}

VisObject *RenderFrameData::AllocVisObject() {
    if (numVisObjects + 1 > maxVisObjects) {
        return nullptr;
    }

    // placement new 생성자 호출
    VisObject *newVisObject = &visObjects[numVisObjects];
    new (newVisObject) VisObject();
    
    newVisObject->index = numVisObjects++;

    return newVisObject;
}

void *RenderFrameData::GetCommandBuffer(uint32_t bytes) {
    RenderCommandBuffer *cmds = GetCommands();

    if (cmds->used + bytes + sizeof(RenderCommandId) >= RenderCommandBuffer::BufferSize) {
        BE_WARNLOG("RenderFrameData::GetCommandBuffer: not enough command buffer space\n");
        return nullptr;
    }

    cmds->used += bytes;

    return cmds->buffer + cmds->used - bytes;
}
