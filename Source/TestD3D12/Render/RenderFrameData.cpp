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
#include "Material.h"
#include "Texture.h"

uint64_t                    RenderFrameData::globalFrameSerial = 0;
static constexpr uint32_t   MaxRenderTaskThreads = 32;
static constexpr uint32_t   MaxMemSizePerBlock = 0x4000000;
static constexpr uint32_t   MemAlignSize = 32;

static RHI::PipelineState *SnapshotPSO(const SharedPtr<ShaderPipeline> &shaderPipeline) {
    if (!shaderPipeline || !shaderPipeline->IsReady()) {
        return nullptr;
    }

    return shaderPipeline->GetPSO();
}

void RenderFrameData::CmdBeginContext(RenderContext *context) {
    BeginContextRenderCommand *cmd = (BeginContextRenderCommand *)GetCommandBuffer(sizeof(BeginContextRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::BeginContext;
    cmd->renderContext = context;
    cmd->imagePSO = nullptr;

    if (!context) {
        return;
    }

    cmd->imagePSO = SnapshotPSO(context->imagePSO);
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
}

void RenderFrameData::Shutdown() {
    for (int32_t threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        RHI::renderer->DestroyFrameThreadData(threadData[threadIndex]);
    }

    delete [] threadData;
    threadData = nullptr;

    FreeVisObjects();

    FreeVisCameras();

    ClearRetainedMaterials();

    ClearRetainedMeshInstances();

    ClearRetainedFonts();

    DestroyShaderMaterialBuffers();

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

    ClearRetainedMaterials();

    ClearRetainedMeshInstances();

    ClearRetainedFonts();

    ClearShaderMaterialTable();

    // Reset the mem allocation to the first block.
    currentBlock = headBlock;

    // Clear all the blocks.
    for (MemBlock *block = headBlock; block; block = block->next) {
        block->used = 0;
    }

    InitVisCameras(16);

    // NOTE: visObject 에 대한 sort key 가 16 비트이므로 0xFFFF = 65536 을 최대로 설정한다.
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

void RenderFrameData::ClearShaderMaterialTable() {
    shaderMaterialSources.SetCount(0, false);
    shaderMaterialTable.SetCount(0, false);
    shaderMaterialHash.Clear();
    shaderMaterialTableDirty = false;
}

void RenderFrameData::DestroyShaderMaterialBuffers() {
    if (shaderMaterialBuffer) {
        RHI::renderer->DestroyBuffer(shaderMaterialBuffer);
        shaderMaterialBuffer = nullptr;
    }

    if (shaderMaterialUploadBuffer) {
        RHI::renderer->DestroyBuffer(shaderMaterialUploadBuffer);
        shaderMaterialUploadBuffer = nullptr;
    }

    shaderMaterialBufferView = RHI::ViewID::InvalidID;
    shaderMaterialBufferState = RHI::GPUResourceState::Undefined;
    shaderMaterialBufferCapacity = 0;
    shaderMaterialTableBindlessIndex = 0;
    shaderMaterialTableDirty = false;
}

bool RenderFrameData::EnsureShaderMaterialBufferCapacity(uint32_t shaderMaterialCount) {
    if (shaderMaterialCount <= shaderMaterialBufferCapacity) {
        return true;
    }

    // 초기 개수는 256 으로 시작해서, 필요한 용량이 될 때까지 2배씩 늘린다.
    uint32_t newCapacity = shaderMaterialBufferCapacity > 0 ? shaderMaterialBufferCapacity : 256;
    while (newCapacity < shaderMaterialCount) {
        newCapacity <<= 1;
    }

    DestroyShaderMaterialBuffers();

    // 디폴트 버퍼와 업로드 버퍼를 생성한다.
    uint32_t bufferSize = newCapacity * sizeof(ShaderMaterial);
    shaderMaterialBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    shaderMaterialUploadBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Upload, RHI::BufferFlag::None, bufferSize, nullptr);
    if (!shaderMaterialBuffer || !shaderMaterialUploadBuffer) {
        DestroyShaderMaterialBuffers();
        return false;
    }

    RHI::renderer->SetName(shaderMaterialBuffer, "shaderMaterialBuffer");
    RHI::renderer->SetName(shaderMaterialUploadBuffer, "shaderMaterialUploadBuffer");

    shaderMaterialBufferView = RHI::renderer->CreateView(shaderMaterialBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(ShaderMaterial), 0, bufferSize);
    if (!shaderMaterialBufferView.IsValid()) {
        DestroyShaderMaterialBuffers();
        return false;
    }

    shaderMaterialBufferCapacity = newCapacity;
    shaderMaterialBufferState = RHI::GPUResourceState::Undefined;
    shaderMaterialTableBindlessIndex = shaderMaterialBuffer->GetBindlessIndex(RHI::ViewType::SRV, shaderMaterialBufferView);
    return true;
}

static const Texture *GetDefaultMaterialTexture() {
    if (textureManager.whiteTexture) {
        return textureManager.whiteTexture.get();
    }
    if (textureManager.defaultTexture) {
        return textureManager.defaultTexture.get();
    }
    return nullptr;
}

static const Texture *ResolveMaterialBaseColorTexture(const Material *material) {
    if (material) {
        const SharedPtr<Texture> *textureProperty = material->GetTextureProperty("baseColorTexture");
        if (textureProperty && *textureProperty) {
            return textureProperty->get();
        }
    }

    return textureManager.defaultTexture.get();
}

static const RHI::Texture *GetReadyRHITexture(const Texture *texture) {
    if (!texture || !texture->IsReadyForRendering()) {
        return nullptr;
    }

    return texture->GetRHITexture();
}

void RenderFrameData::BuildShaderMaterial(const Material *material, ShaderMaterial &outShaderMaterial) const {
    outShaderMaterial = {};
    outShaderMaterial.streamingMaterialIndex = TextureStreamingInvalidIndex;
    outShaderMaterial.baseColorFactor = float4(1.0f, 1.0f, 1.0f, 1.0f);

    const Texture *texture = ResolveMaterialBaseColorTexture(material);
    const RHI::Texture *rhiTexture = GetReadyRHITexture(texture);
    if (!rhiTexture) {
        texture = GetDefaultMaterialTexture();
        rhiTexture = GetReadyRHITexture(texture);
    }

    if (rhiTexture) {
        outShaderMaterial.baseColorTextureIndex = rhiTexture->GetBindlessIndex(RHI::ViewType::SRV);
    }

    if (material) {
        outShaderMaterial.streamingMaterialIndex = material->GetStreamingListIndex();
    }
}

uint32_t RenderFrameData::RegisterShaderMaterial(const Material *material) {
    assert(material);

    int hash = shaderMaterialHash.GenerateHash(material);
    for (int index = shaderMaterialHash.First(hash); index != -1; index = shaderMaterialHash.Next(index)) {
        const Material *sourceMaterial = shaderMaterialSources[index];
        if (sourceMaterial == material) {
            return index;
        }
    }

    uint32_t newShaderMaterialIndex = shaderMaterialSources.Count();
    shaderMaterialSources.Append(material);
    shaderMaterialHash.Add(hash, newShaderMaterialIndex);
    shaderMaterialTableDirty = true;

    return newShaderMaterialIndex;
}

bool RenderFrameData::UploadShaderMaterialTable(RHI::CommandList *commandList) {
    if (!shaderMaterialTableDirty || shaderMaterialSources.Count() == 0) {
        return true;
    }

    // shader material 테이블을 구성한다.
    const int numShaderMaterialSources = shaderMaterialSources.Count();
    shaderMaterialTable.SetCount(numShaderMaterialSources, false);
    for (int shaderMaterialIndex = 0; shaderMaterialIndex < numShaderMaterialSources; shaderMaterialIndex++) {
        BuildShaderMaterial(shaderMaterialSources[shaderMaterialIndex], shaderMaterialTable[shaderMaterialIndex]);
    }

    if (!EnsureShaderMaterialBufferCapacity(shaderMaterialTable.Count())) {
        return false;
    }

    // 업로드 버퍼에 shader material 테이블 데이터를 복사한다.
    const uint32_t uploadSize = shaderMaterialTable.Count() * sizeof(ShaderMaterial);
    BE1::simdProcessor->Memcpy(shaderMaterialUploadBuffer->mappedPtr, shaderMaterialTable.Ptr(), uploadSize);

    // 디폴트 버퍼에 업로드 버퍼의 데이터를 복사한다.
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(shaderMaterialBuffer, shaderMaterialBufferState, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, shaderMaterialBuffer, 0, shaderMaterialUploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(shaderMaterialBuffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));

    shaderMaterialBufferState = RHI::GPUResourceState::ShaderResource;
    shaderMaterialTableDirty = false;
    return true;
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
