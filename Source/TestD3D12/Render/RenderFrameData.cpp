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
static constexpr uint32_t   MaxMemSizePerBlock = 0x4000000;
static constexpr uint32_t   MemAlignSize = 32;

void RenderFrameData::Init(int32_t frameIndex, int32_t numFrames) {
    InitMemBlocks();

#ifdef USE_RENDER_TASK
    // 렌더 태스크 스레드 개수는 태스크 매니져의 스레드 개수를 넘을 수 없다.
    // 태스크 매니져의 스레드는 엔진에서 물리 코어 개수만큼 미리 생성된다.
    numRenderTaskThreads = BE1::Min(BE1::Engine::taskManager->NumThreads(), MaxRenderTaskThreads);
#else
    numRenderTaskThreads = 1;
#endif

    threadData = new RHI::FrameThreadData *[numRenderTaskThreads];

    // 필요한 만큼 렌더 태스크 스레드 데이터를 준비한다.
    for (int32_t threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        threadData[threadIndex] = RHI::renderer->CreateFrameThreadData(frameIndex, numFrames, threadIndex, numRenderTaskThreads);
    }
}

void RenderFrameData::Shutdown() {
    for (int32_t threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        RHI::renderer->DestroyFrameThreadData(threadData[threadIndex]);
    }

    delete [] threadData;
    threadData = nullptr;

    FreeVisObjects();

    FreeVisCameras();

    ClearRetainedTextures();

    ClearRetainedMeshInstances();

    ClearRetainedFonts();

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

    ClearRetainedTextures();

    ClearRetainedMeshInstances();

    ClearRetainedFonts();

    // Reset the mem allocation to the first block.
    currentBlock = headBlock;

    // Clear all the blocks.
    for (MemBlock *block = headBlock; block; block = block->next) {
        block->used = 0;
    }

    InitVisCameras(16);

    InitVisObjects(65536);
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

void RenderFrameData::ClearRetainedTextures() {
    for (int i = 0; i < retainedTextures.Count(); ++i) {
        retainedTextures[i].reset();
    }
    retainedTextures.SetCount(0, false);
    retainedTextureHash.Clear();
}

void RenderFrameData::ClearRetainedMeshInstances() {
    for (int i = 0; i < retainedMeshInstances.Count(); ++i) {
        retainedMeshInstances[i].reset();
    }
    retainedMeshInstances.SetCount(0, false);
    retainedMeshInstanceHash.Clear();
}

void RenderFrameData::ClearRetainedFonts() {
    for (int i = 0; i < retainedFonts.Count(); ++i) {
        retainedFonts[i].reset();
    }
    retainedFonts.SetCount(0, false);
    retainedFontHash.Clear();
}

void RenderFrameData::RetainTexture(const std::shared_ptr<Texture> &texture) {
    if (!texture) {
        return;
    }

    Texture *texturePtr = texture.get();
    int hash = retainedTextureHash.GenerateHash(texturePtr);

    for (int i = retainedTextureHash.First(hash); i != -1; i = retainedTextureHash.Next(i)) {
        if (retainedTextures[i].get() == texturePtr) {
            return;
        }
    }

    int index = retainedTextures.Append(texture);
    retainedTextureHash.Add(hash, index);
}

void RenderFrameData::RetainMeshInstance(const std::shared_ptr<MeshInstance> &meshInstance) {
    if (!meshInstance) {
        return;
    }

    MeshInstance *meshInstancePtr = meshInstance.get();
    int hash = retainedMeshInstanceHash.GenerateHash(meshInstancePtr);

    for (int i = retainedMeshInstanceHash.First(hash); i != -1; i = retainedMeshInstanceHash.Next(i)) {
        if (retainedMeshInstances[i].get() == meshInstancePtr) {
            return;
        }
    }

    int index = retainedMeshInstances.Append(meshInstance);
    retainedMeshInstanceHash.Add(hash, index);
}

void RenderFrameData::RetainFont(const std::shared_ptr<Font> &font) {
    if (!font) {
        return;
    }

    Font *fontPtr = font.get();
    int hash = retainedFontHash.GenerateHash(fontPtr);

    for (int i = retainedFontHash.First(hash); i != -1; i = retainedFontHash.Next(i)) {
        if (retainedFonts[i].get() == fontPtr) {
            return;
        }
    }

    int index = retainedFonts.Append(font);
    retainedFontHash.Add(hash, index);
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
