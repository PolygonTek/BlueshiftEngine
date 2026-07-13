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
#include "RenderContext.h"
#include "RenderInternal.h"
#include "Render/Material/Material.h"
#include "Render/Texture/Texture.h"

uint64_t                    RenderFrameData::globalFrameSerial = 0;
static constexpr uint32_t   MaxRenderTaskThreads = 32;
static constexpr uint32_t   MaxMemSizePerBlock = 0x4000000;
static constexpr uint32_t   MemAlignSize = 32;

void RenderFrameData::CmdBeginContext(RenderContext *context) {
    BeginContextRenderCommand *cmd = (BeginContextRenderCommand *)GetCommandBuffer(sizeof(BeginContextRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::BeginContext;
    cmd->renderContext = context;
    cmd->imagePSO = ShaderPipeline::GetReadyPSO(context->imagePipeline);
}

void RenderFrameData::CmdDrawCamera(const VisCamera *camera) {
    DrawCameraRenderCommand *cmd = (DrawCameraRenderCommand *)GetCommandBuffer(sizeof(DrawCameraRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::DrawCamera;
    cmd->visCamera = camera;

    if (camera) {
        maxShaderSurfaceCount = BE1::Max(maxShaderSurfaceCount, camera->numVisSurfs);
    }
}

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

    if (!shaderMaterialTable.Init()) {
        BE_WARNLOG("RenderFrameData::Init: failed to initialize shader material table\n");
    }

    if (!shaderSurfaceTable.Init()) {
        BE_WARNLOG("RenderFrameData::Init: failed to initialize shader surface table\n");
    }
}

void RenderFrameData::Shutdown() {
    ClearPendingImGuiOverlay();

    for (int32_t threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        RHI::renderer->DestroyFrameThreadData(threadData[threadIndex]);
    }

    delete [] threadData;
    threadData = nullptr;

    FreeVisLights();

    FreeVisObjects();

    FreeVisCameras();

    ClearRetainedMaterials();

    ClearRetainedMeshInstances();

    ClearRetainedFonts();

    ClearRetainedTextures();

    shaderMaterialTable.Shutdown();
    shaderSurfaceTable.Shutdown();

    ClearMemBlocks();
}

void RenderFrameData::BeginFrame() {
    // 프론트 엔드에서 이번 프레임의 프레임 데이터를 사용하기 위해서는, 백엔드에서의 사용이 완료되야 한다.
    WaitFrameFence();

    ClearPendingImGuiOverlay();

    frameTime = 0.0f;
    elapsedTime = 0.0f;
    elapsedPrevTime = 0.0f;
    frameCount = 0;
    statEnabled = false;
    statRenderObjectCount = 0;
    statVisibleObjectCount = 0;
    maxShaderSurfaceCount = 0;

    // 스레드 별로 사용할 자원을 초기화한다.
    for (int32_t threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        threadData[threadIndex]->Reset();
    }

    frameSerial = ++globalFrameSerial;

    // 프레임 메모리를 초기화한다.
    BeginFrameMemAllocs();
}

void RenderFrameData::EndFrame() {
    // 커맨드 버퍼에 커맨드의 끝을 기록.
    *(uint32_t *)(commands.buffer + commands.used) = static_cast<uint32_t>(RenderCommandId::End);

    // 커맨드 버퍼 비우기.
    commands.used = 0;
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

    FreeVisLights();

    FreeVisObjects();

    ClearRetainedMaterials();

    ClearRetainedMeshInstances();

    ClearRetainedFonts();

    ClearRetainedTextures();

    shaderMaterialTable.Clear();
    shaderSurfaceTable.Clear();

    // Reset the mem allocation to the first block.
    currentBlock = headBlock;

    // Clear all the blocks.
    for (MemBlock *block = headBlock; block; block = block->next) {
        block->used = 0;
    }

    InitVisCameras(16);

    InitVisLights(10000);

    InitVisObjects(100000);
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

// TODO: 한번에 할당하지 않고, AllocVisLight 에서 최대 개수를 넘어가도 새로운 chunk 를 할당하는 방식으로 수정할 것
void RenderFrameData::InitVisLights(int maxVisLights) {
    assert(!visLights);

    this->maxVisLights = maxVisLights;
    this->numVisLights = 0;
    this->visLights = (VisLight *)MemAlloc(sizeof(VisLight) * maxVisLights);
}

void RenderFrameData::FreeVisLights() {
    if (!visLights) {
        return;
    }

    // 소멸자 호출
    for (int i = 0; i < numVisLights; ++i) {
        (visLights + i)->~VisLight();
    }

    visLights = nullptr;
    numVisLights = 0;
}

void RenderFrameData::ClearRetainedMaterials() {
    const int numRetainedMaterials = retainedMaterials.Count();
    for (int i = 0; i < numRetainedMaterials; ++i) {
        retainedMaterials[i].reset();
    }
    retainedMaterials.SetCount(0, false);
}

void RenderFrameData::ClearRetainedMeshInstances() {
    const int numRetainedMeshInstances = retainedMeshInstances.Count();
    for (int i = 0; i < numRetainedMeshInstances; ++i) {
        retainedMeshInstances[i].reset();
    }
    retainedMeshInstances.SetCount(0, false);
}

void RenderFrameData::ClearRetainedFonts() {
    const int numRetainedFonts = retainedFonts.Count();
    for (int i = 0; i < numRetainedFonts; ++i) {
        retainedFonts[i].reset();
    }
    retainedFonts.SetCount(0, false);
}

void RenderFrameData::ClearRetainedTextures() {
    const int numRetainedTextures = retainedTextures.Count();
    for (int i = 0; i < numRetainedTextures; ++i) {
        retainedTextures[i].reset();
    }
    retainedTextures.SetCount(0, false);
}

Material *RenderFrameData::RetainMaterial(const SharedPtr<Material> &material) {
    if (!material) {
        return nullptr;
    }

    Material *materialPtr = material.get();
    if (materialPtr->GetRetainedFrameSerial() != frameSerial) {
        materialPtr->SetRetainedFrameSerial(frameSerial);
        retainedMaterials.Append(material);
    }

    return materialPtr;
}

MeshInstance *RenderFrameData::RetainMeshInstance(const SharedPtr<MeshInstance> &meshInstance) {
    if (!meshInstance) {
        return nullptr;
    }

    MeshInstance *meshInstancePtr = meshInstance.get();
    if (meshInstancePtr->GetRetainedFrameSerial() != frameSerial) {
        meshInstancePtr->SetRetainedFrameSerial(frameSerial);
        retainedMeshInstances.Append(meshInstance);
    }

    return meshInstancePtr;
}

Font *RenderFrameData::RetainFont(const SharedPtr<Font> &font) {
    if (!font) {
        return nullptr;
    }

    Font *fontPtr = font.get();
    if (fontPtr->GetRetainedFrameSerial() != frameSerial) {
        fontPtr->SetRetainedFrameSerial(frameSerial);
        retainedFonts.Append(font);
    }

    return fontPtr;
}

Texture *RenderFrameData::RetainTexture(const SharedPtr<Texture> &texture) {
    if (!texture) {
        return nullptr;
    }

    Texture *texturePtr = texture.get();
    if (texturePtr->GetRetainedFrameSerial() != frameSerial) {
        texturePtr->SetRetainedFrameSerial(frameSerial);
        retainedTextures.Append(texture);
    }

    return texturePtr;
}

void RenderFrameData::SetPendingImGuiOverlay(RHI::ImGuiOverlay *overlay, void *userData) {
    ClearPendingImGuiOverlay();

    pendingImGuiOverlay.overlay = overlay;
    pendingImGuiOverlay.userData = userData;
}

void RenderFrameData::RenderPendingImGuiOverlay(RHI::CommandList *commandList) {
    PendingImGuiOverlay overlay = pendingImGuiOverlay;
    pendingImGuiOverlay = {};

    if (overlay.overlay) {
        // ImGui 의 캡쳐 데이터를 렌더링 후 폐기한다.
        RHI::renderer->RenderImGuiDrawData(GetThreadData(0), commandList, overlay.overlay, overlay.userData);
        RHI::renderer->FreeImGuiDrawData(overlay.overlay, overlay.userData);
    }
}

void RenderFrameData::ClearPendingImGuiOverlay() {
    if (pendingImGuiOverlay.overlay) {
        // ImGui 의 캡쳐 데이터를 폐기한다.
        RHI::renderer->FreeImGuiDrawData(pendingImGuiOverlay.overlay, pendingImGuiOverlay.userData);
    }
    pendingImGuiOverlay = {};
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

VisLight *RenderFrameData::AllocVisLight() {
    if (numVisLights + 1 > maxVisLights) {
        return nullptr;
    }

    // placement new 생성자 호출
    VisLight *newVisLight = &visLights[numVisLights];
    new (newVisLight) VisLight();

    newVisLight->index = numVisLights++;

    return newVisLight;
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
