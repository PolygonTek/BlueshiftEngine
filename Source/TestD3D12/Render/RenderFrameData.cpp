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
static constexpr size_t     FrameMemoryBlockSize = 0x4000000;
static constexpr int        FrameMemoryAlignment = 32;
static constexpr uint32_t   MaxVisCamera = 1u << 3;
static constexpr uint32_t   MaxVisObjects = 1u << 20;
static constexpr uint32_t   MaxVisTexts = MaxVisObjects;
static constexpr uint32_t   MaxVisLights = 1u << 17;
static constexpr uint32_t   VisObjectChunkCapacity = 8192;
static constexpr uint32_t   VisTextChunkCapacity = 256;
static constexpr uint32_t   VisLightChunkCapacity = 1024;

void RenderFrameData::Init(int32_t frameIndex, int32_t numFrames) {
    frameArena.Init(FrameMemoryBlockSize, FrameMemoryAlignment);

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

    if (!shaderLightTable.Init()) {
        BE_WARNLOG("RenderFrameData::Init: failed to initialize shader light table\n");
    }

    if (!shaderLightCullingTable.Init()) {
        BE_WARNLOG("RenderFrameData::Init: failed to initialize shader light culling table\n");
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

    FreeVisTexts();

    FreeVisObjects();

    FreeVisCameras();

    ClearRetainedMaterials();

    ClearRetainedMeshInstances();

    ClearRetainedFonts();

    ClearRetainedTextures();

    shaderMaterialTable.Shutdown();
    shaderLightTable.Shutdown();
    shaderLightCullingTable.Shutdown();
    shaderSurfaceTable.Shutdown();

    frameArena.Shutdown();
}

void RenderFrameData::BeginFrame(const RenderFrameSettings &renderFrameSettings) {
    PROFILER_CPU_SCOPED_EVENT("RenderFrameData::BeginFrame");

    // 프론트 엔드에서 이번 프레임의 프레임 데이터를 사용하기 위해서는, 백엔드에서의 사용이 완료되야 한다.
    WaitFrameFence();

    this->renderFrameSettings = renderFrameSettings;

    ClearPendingImGuiOverlay();

    frameTime = 0.0f;
    elapsedTime = 0.0f;
    elapsedPrevTime = 0.0f;
    frameCount = 0;
    hasVisOceanSurface = false;
    visOceanSurface.surfaceQueries = nullptr;
    visOceanSurface.surfaceQueryCount = 0;
    visOceanSurface.settingsRevision = 0;
    statEnabled = false;
    maxShaderSurfaceCount = 0;
    numLightCullingZBins = 0;
    numLightCullingWords = 0;

    // 스레드 별로 사용할 자원을 초기화한다.
    for (int32_t threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        threadData[threadIndex]->Reset();
    }

    frameSerial = ++globalFrameSerial;

    // 프레임 데이터를 초기화한다.
    ResetFrameData();
}

void RenderFrameData::EndFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderFrameData::EndFrame");

    // 커맨드 버퍼에 커맨드의 끝을 기록.
    CmdEnd();

    // 커맨드 버퍼 비우기.
    commands.used = 0;
}

void RenderFrameData::RegisterVisOceanSurface(const GpuOcean::SimulationSettings &simulationSettings, const GpuOcean::SurfaceSettings &surfaceSettings, uint64_t settingsRevision, const OceanSurfaceQueryInput *surfaceQueries, uint32_t surfaceQueryCount) {
    if (hasVisOceanSurface) {
        assert(visOceanSurface.settingsRevision == settingsRevision); // 현재 한 frame 에서는 하나의 ocean simulation 만 지원한다.
        return;
    }

    visOceanSurface.simulationSettings = simulationSettings;
    visOceanSurface.surfaceSettings = surfaceSettings;
    visOceanSurface.settingsRevision = settingsRevision;

    if (surfaceQueries && surfaceQueryCount > 0) {
        const int queryDataSize = static_cast<int>(surfaceQueryCount * sizeof(OceanSurfaceQueryInput));
        OceanSurfaceQueryInput *queryCopy = static_cast<OceanSurfaceQueryInput *>(MemAlloc(queryDataSize));
        if (queryCopy) {
            BE1::simdProcessor->Memcpy(queryCopy, surfaceQueries, queryDataSize);

            visOceanSurface.surfaceQueries = queryCopy;
            visOceanSurface.surfaceQueryCount = surfaceQueryCount;
        }
    }
    hasVisOceanSurface = true;
}

void RenderFrameData::ResetFrameData() {
    FreeVisCameras();

    FreeVisLights();

    FreeVisTexts();

    FreeVisObjects();

    ClearRetainedMaterials();

    ClearRetainedMeshInstances();

    ClearRetainedFonts();

    ClearRetainedTextures();

    shaderMaterialTable.Clear();
    shaderLightTable.Clear();
    shaderSurfaceTable.Clear();

    frameArena.Reset();

    InitVisCameras(MaxVisCamera);
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

void RenderFrameData::FreeVisObjects() {
    for (Chunk *chunk = firstVisObjectChunk; chunk; chunk = chunk->next) {
        VisObject *visObjects = static_cast<VisObject *>(chunk->elements);
        for (uint32_t i = 0; i < chunk->count; ++i) {
            visObjects[i].~VisObject();
        }
    }

    firstVisObjectChunk = nullptr;
    lastVisObjectChunk = nullptr;
    numVisObjects = 0;
}

void RenderFrameData::FreeVisTexts() {
    for (Chunk *chunk = firstVisTextChunk; chunk; chunk = chunk->next) {
        VisText *visTexts = static_cast<VisText *>(chunk->elements);
        for (uint32_t i = 0; i < chunk->count; ++i) {
            visTexts[i].~VisText();
        }
    }

    firstVisTextChunk = nullptr;
    lastVisTextChunk = nullptr;
    numVisTexts = 0;
}

void RenderFrameData::FreeVisLights() {
    for (Chunk *chunk = firstVisLightChunk; chunk; chunk = chunk->next) {
        VisLight *visLights = static_cast<VisLight *>(chunk->elements);
        for (uint32_t i = 0; i < chunk->count; ++i) {
            visLights[i].~VisLight();
        }
    }

    firstVisLightChunk = nullptr;
    lastVisLightChunk = nullptr;
    numVisLights = 0;
    numVisLocalLights = 0;
    numVisPointLights = 0;
    numVisSpotLights = 0;
    numVisBoxDirLights = 0;
    primaryDirectionalLight = nullptr;
}

RenderFrameData::Chunk *RenderFrameData::AllocChunk(uint32_t elementSize, uint32_t elementCount) {
    const uint32_t elementsOffset = BE1::AlignUp(static_cast<uint32_t>(sizeof(Chunk)), FrameMemoryAlignment);
    const uint64_t allocationSize = elementsOffset + static_cast<uint64_t>(elementSize) * elementCount;
    assert(allocationSize <= INT_MAX);

    Chunk *chunk = static_cast<Chunk *>(MemAlloc(static_cast<int>(allocationSize)));
    chunk->next = nullptr;
    chunk->elements = reinterpret_cast<byte *>(chunk) + elementsOffset;
    chunk->count = 0;
    return chunk;
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
    if (numVisCameras >= maxVisCameras) {
        assert(0);
        return nullptr;
    }

    // placement new 생성자 호출
    VisCamera *newVisCamera = &visCameras[numVisCameras];
    new (newVisCamera) VisCamera();

    newVisCamera->index = numVisCameras++;

    return newVisCamera;
}

VisObject *RenderFrameData::AllocVisObject() {
    if (numVisObjects >= MaxVisObjects) {
        assert(0);
        return nullptr;
    }

    if (!lastVisObjectChunk || lastVisObjectChunk->count >= VisObjectChunkCapacity) {
        Chunk *newChunk = AllocChunk(sizeof(VisObject), VisObjectChunkCapacity);
        if (lastVisObjectChunk) {
            lastVisObjectChunk->next = newChunk;
        } else {
            firstVisObjectChunk = newChunk;
        }
        lastVisObjectChunk = newChunk;
    }

    VisObject *visObjects = static_cast<VisObject *>(lastVisObjectChunk->elements);
    VisObject *newVisObject = &visObjects[lastVisObjectChunk->count++];

    // placement new 생성자 호출
    new (newVisObject) VisObject();
    
    newVisObject->index = numVisObjects++;

    return newVisObject;
}

VisText *RenderFrameData::AllocVisText() {
    if (numVisTexts >= MaxVisTexts) {
        assert(0);
        return nullptr;
    }

    if (!lastVisTextChunk || lastVisTextChunk->count >= VisTextChunkCapacity) {
        Chunk *newChunk = AllocChunk(sizeof(VisText), VisTextChunkCapacity);
        if (lastVisTextChunk) {
            lastVisTextChunk->next = newChunk;
        } else {
            firstVisTextChunk = newChunk;
        }
        lastVisTextChunk = newChunk;
    }

    VisText *visTexts = static_cast<VisText *>(lastVisTextChunk->elements);
    VisText *newVisText = &visTexts[lastVisTextChunk->count++];

    // placement new 생성자 호출
    new (newVisText) VisText();

    numVisTexts++;

    return newVisText;
}

VisSpotLightData *RenderFrameData::AllocVisSpotLightData() {
    void *memory = MemAlloc(sizeof(VisSpotLightData));
    return new (memory) VisSpotLightData();
}

VisLight *RenderFrameData::AllocVisLight(RenderLight::Type type) {
    if (type == RenderLight::Type::Directional && primaryDirectionalLight) {
        assert(0);
        return nullptr;
    }

    if (numVisLights >= MaxVisLights) {
        assert(0);
        return nullptr;
    }

    if (!lastVisLightChunk || lastVisLightChunk->count >= VisLightChunkCapacity) {
        Chunk *newChunk = AllocChunk(sizeof(VisLight), VisLightChunkCapacity);
        if (lastVisLightChunk) {
            lastVisLightChunk->next = newChunk;
        } else {
            firstVisLightChunk = newChunk;
        }
        lastVisLightChunk = newChunk;
    }

    VisLight *visLights = static_cast<VisLight *>(lastVisLightChunk->elements);
    VisLight *newVisLight = &visLights[lastVisLightChunk->count++];

    // placement new 생성자 호출
    new (newVisLight) VisLight();

    newVisLight->index = numVisLights++;
    newVisLight->type = type;

    switch (type) {
    case RenderLight::Type::Point:
        numVisLocalLights++;
        newVisLight->typeIndex = numVisPointLights++;
        break;
    case RenderLight::Type::Spot:
        numVisLocalLights++;
        newVisLight->typeIndex = numVisSpotLights++;
        break;
    case RenderLight::Type::BoxDir:
        numVisLocalLights++;
        newVisLight->typeIndex = numVisBoxDirLights++;
        break;
    case RenderLight::Type::Directional:
        newVisLight->typeIndex = 0;
        primaryDirectionalLight = newVisLight;
        break;
    default:
        assert(0);
        newVisLight->typeIndex = 0;
        break;
    }

    return newVisLight;
}

uint32_t RenderFrameData::ReserveLightCullingZBins() {
    assert(ShaderLightZBinCount <= UINT32_MAX - numLightCullingZBins);

    const uint32_t offset = numLightCullingZBins;
    numLightCullingZBins += ShaderLightZBinCount;
    return offset;
}

uint32_t RenderFrameData::ReserveLightCullingWords(uint32_t count) {
    assert(count <= UINT32_MAX - numLightCullingWords);

    const uint32_t offset = numLightCullingWords;
    numLightCullingWords += count;
    return offset;
}

void *RenderFrameData::GetCommandBuffer(uint32_t bytes) {
    RenderCommandBuffer *cmds = GetCommands();

    assert(cmds->used <= RenderCommandBuffer::BufferSize);

    const uint32_t availableBytes = RenderCommandBuffer::BufferSize - cmds->used;
    if (bytes > availableBytes) {
        BE_WARNLOG("RenderFrameData::GetCommandBuffer: not enough command buffer space\n");
        return nullptr;
    }

    void *commandBuffer = cmds->buffer + cmds->used;
    cmds->used += bytes;
    return commandBuffer;
}

bool RenderFrameData::CmdRenderWork(RenderWork *renderWork) {
    if (!renderWork) {
        return false;
    }

    RenderWorkRenderCommand *cmd = (RenderWorkRenderCommand *)GetCommandBuffer(sizeof(RenderWorkRenderCommand));
    if (!cmd) {
        return false;
    }

    cmd->commandId = RenderCommandId::RenderWork;
    cmd->renderWork = renderWork;
    return true;
}

void RenderFrameData::CmdBeginContext(RenderContext *context, bool gpuTiming, bool textureStreamingFeedback, bool swapChainOutput) {
    BeginContextRenderCommand *cmd = (BeginContextRenderCommand *)GetCommandBuffer(sizeof(BeginContextRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::BeginContext;
    cmd->renderContext = context;
    cmd->imagePSO = ShaderPipeline::GetReadyPSO(context->imagePipeline);
    cmd->hdrOutputPSO = ShaderPipeline::GetReadyPSO(context->hdrOutputPipeline);
    cmd->gpuTiming = gpuTiming;
    cmd->textureStreamingFeedback = textureStreamingFeedback;
    cmd->swapChainOutput = swapChainOutput;
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

void RenderFrameData::CmdFinalizeFrame(bool renderImGuiOverlay) {
    FinalizeFrameRenderCommand *cmd = (FinalizeFrameRenderCommand *)GetCommandBuffer(sizeof(FinalizeFrameRenderCommand));
    if (!cmd) {
        BE_FATALERROR("RenderFrameData::CmdFinalizeFrame: failed to allocate command buffer\n");
        return;
    }

    cmd->commandId = RenderCommandId::FinalizeFrame;
    cmd->renderImGuiOverlay = renderImGuiOverlay;
}

void RenderFrameData::CmdSwapBuffers() {
    SwapBuffersRenderCommand *cmd = (SwapBuffersRenderCommand *)GetCommandBuffer(sizeof(SwapBuffersRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::SwapBuffers;
}

void RenderFrameData::CmdScreenshot(int x, int y, int width, int height, const char *filename) {
    ScreenShotRenderCommand *cmd = (ScreenShotRenderCommand *)GetCommandBuffer(sizeof(ScreenShotRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::ScreenShot;
    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;
    BE1::Str::Copynz(cmd->filename, filename, COUNT_OF(cmd->filename));
}

void RenderFrameData::CmdEnd() {
    EndRenderCommand *cmd = (EndRenderCommand *)GetCommandBuffer(sizeof(EndRenderCommand));
    if (!cmd) {
        BE_FATALERROR("RenderFrameData::CmdEnd: failed to allocate command buffer\n");
        return;
    }

    cmd->commandId = RenderCommandId::End;
}
