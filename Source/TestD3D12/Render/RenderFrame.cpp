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
#include "RenderFrame.h"
#include "RenderContext.h"
#include "RenderInternal.h"
#include "ShadowAtlas.h"
#include "Animation/Skeleton.h"
#include "Material/Material.h"
#include "Texture/Texture.h"

uint64_t                    RenderFrame::globalFrameSerial = 0;

static constexpr uint32_t   MaxRenderTaskThreads = 32;
static constexpr size_t     FrameMemoryBlockSize = 0x4000000;
static constexpr int        FrameMemoryAlignment = 32;
static constexpr uint32_t   MaxFrameCamera = 1u << 3;
static constexpr uint32_t   MaxFrameObjects = 1u << 20;
static constexpr uint32_t   MaxFrameTexts = MaxFrameObjects;
static constexpr uint32_t   MaxFrameLights = static_cast<uint32_t>(UINT16_MAX) + 1u; // All cameras combined; indices are uint16_t.
static constexpr uint32_t   FrameObjectChunkCapacity = 8192;
static constexpr uint32_t   FrameSkinnedMeshChunkCapacity = 256;
static constexpr uint32_t   FrameTextChunkCapacity = 256;
static constexpr uint32_t   FrameLightChunkCapacity = 1024;

static_assert(std::is_trivially_destructible<FrameOceanSurface>::value, "FrameOceanSurface must not require destruction before the frame arena is reset");

void RenderFrame::Init(int32_t frameIndex, int32_t numFrames) {
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
        BE_WARNLOG("RenderFrame::Init: failed to initialize shader material table\n");
    }

    if (!shaderLightTable.Init()) {
        BE_WARNLOG("RenderFrame::Init: failed to initialize shader light table\n");
    }

    if (!shaderLightCullingTable.Init()) {
        BE_WARNLOG("RenderFrame::Init: failed to initialize shader light culling table\n");
    }

    if (!shaderSurfaceTable.Init()) {
        BE_WARNLOG("RenderFrame::Init: failed to initialize shader surface table\n");
    }
}

void RenderFrame::Shutdown() {
    ClearPendingImGuiOverlay();

    for (int32_t threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        RHI::renderer->DestroyFrameThreadData(threadData[threadIndex]);
    }

    delete [] threadData;
    threadData = nullptr;

    FreeFrameLights();

    FreeFrameTexts();

    FreeFrameSkinnedMeshes();

    FreeFrameObjects();

    FreeFrameCameras();

    ClearRetainedMaterials();

    ClearRetainedMeshInstances();

    ClearRetainedSkeletons();

    ClearRetainedFonts();

    ClearRetainedTextures();

    shaderMaterialTable.Shutdown();
    shaderLightTable.Shutdown();
    shaderLightCullingTable.Shutdown();
    shaderSurfaceTable.Shutdown();
    shaderShadowTable.Shutdown();

    frameOceanSurface = nullptr;
    frameArena.Shutdown();
}

void *RenderFrame::MemAlloc(int size) {
    assert(BE1::Engine::IsInMainThread());
    assert(frameSyncState == FrameSyncState::RenderCompleted);
    return frameArena.Alloc(size);
}

void *RenderFrame::ClearedMemAlloc(int size) {
    assert(BE1::Engine::IsInMainThread());
    assert(frameSyncState == FrameSyncState::RenderCompleted);
    return frameArena.ClearedAlloc(size);
}

void RenderFrame::BeginFrame(const RenderFrameSettings &renderFrameSettings) {
    PROFILER_CPU_SCOPED_EVENT("RenderFrame::BeginFrame");

    // 프론트 엔드에서 이번 프레임의 프레임 데이터를 사용하기 위해서는, 백엔드에서의 사용이 완료되야 한다.
    WaitFrameFence();

    this->renderFrameSettings = renderFrameSettings;

    ClearPendingImGuiOverlay();

    frameTime = 0.0f;
    shadowAtlasDebugPreview = nullptr;
    elapsedTime = 0.0f;
    elapsedPrevTime = 0.0f;
    frameCount = 0;
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

void RenderFrame::EndFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderFrame::EndFrame");

    // 커맨드 버퍼에 커맨드의 끝을 기록.
    CmdEnd();

    // 커맨드 버퍼 비우기.
    commands.used = 0;
}

void RenderFrame::ResetFrameData() {
    FreeFrameCameras();

    FreeFrameLights();

    FreeFrameTexts();

    FreeFrameSkinnedMeshes();

    FreeFrameObjects();

    ClearRetainedMaterials();

    ClearRetainedMeshInstances();

    ClearRetainedSkeletons();

    ClearRetainedFonts();

    ClearRetainedTextures();

    shaderMaterialTable.Clear();
    shaderLightTable.Clear();
    shaderSurfaceTable.Clear();

    frameOceanSurface = nullptr;
    frameArena.Reset();

    InitFrameCameras(MaxFrameCamera);
}

void RenderFrame::InitFrameCameras(uint32_t maxFrameCameras) {
    assert(!frameCameras);

    this->maxFrameCameras = maxFrameCameras;
    this->numFrameCameras = 0;
    this->frameCameras = (FrameCamera *)MemAlloc(sizeof(FrameCamera) * maxFrameCameras);
}

void RenderFrame::FreeFrameCameras() {
    if (!frameCameras) {
        return;
    }

    // 소멸자 호출
    for (int i = 0; i < numFrameCameras; ++i) {
        (frameCameras + i)->~FrameCamera();
    }

    frameCameras = nullptr;
    numFrameCameras = 0;
}

void RenderFrame::FreeFrameObjects() {
    for (Chunk *chunk = firstFrameObjectChunk; chunk; chunk = chunk->next) {
        FrameObject *frameObjects = static_cast<FrameObject *>(chunk->elements);
        for (uint32_t i = 0; i < chunk->count; ++i) {
            frameObjects[i].~FrameObject();
        }
    }

    firstFrameObjectChunk = nullptr;
    lastFrameObjectChunk = nullptr;
    numFrameObjects = 0;
}

void RenderFrame::FreeFrameSkinnedMeshes() {
    // 공유 자원의 수명은 retained 배열이 관리하므로 개별 소멸자 호출 없이 청크를 초기화한다.
    static_assert(std::is_trivially_destructible<FrameSkinnedMesh>::value, "FrameSkinnedMesh must not require destruction");
    firstFrameSkinnedMeshChunk = nullptr;
    lastFrameSkinnedMeshChunk = nullptr;
    numFrameSkinnedMeshes = 0;
}

void RenderFrame::FreeFrameTexts() {
    for (Chunk *chunk = firstFrameTextChunk; chunk; chunk = chunk->next) {
        FrameText *frameTexts = static_cast<FrameText *>(chunk->elements);
        for (uint32_t i = 0; i < chunk->count; ++i) {
            frameTexts[i].~FrameText();
        }
    }

    firstFrameTextChunk = nullptr;
    lastFrameTextChunk = nullptr;
    numFrameTexts = 0;
}

void RenderFrame::FreeFrameLights() {
    for (Chunk *chunk = firstFrameLightChunk; chunk; chunk = chunk->next) {
        FrameLight *frameLights = static_cast<FrameLight *>(chunk->elements);
        for (uint32_t i = 0; i < chunk->count; ++i) {
            frameLights[i].~FrameLight();
        }
    }

    firstFrameLightChunk = nullptr;
    lastFrameLightChunk = nullptr;
    numFrameLights = 0;
    numFrameLocalLights = 0;
    numFramePointLights = 0;
    numFrameSpotLights = 0;
    numFrameBoxDirLights = 0;
    primaryDirectionalLight = nullptr;
}

RenderFrame::Chunk *RenderFrame::AllocChunk(uint32_t elementSize, uint32_t elementCount) {
    const uint32_t elementsOffset = BE1::AlignUp(static_cast<uint32_t>(sizeof(Chunk)), FrameMemoryAlignment);
    const uint64_t allocationSize = elementsOffset + static_cast<uint64_t>(elementSize) * elementCount;
    assert(allocationSize <= INT_MAX);

    Chunk *chunk = static_cast<Chunk *>(MemAlloc(static_cast<int>(allocationSize)));
    chunk->next = nullptr;
    chunk->elements = reinterpret_cast<byte *>(chunk) + elementsOffset;
    chunk->count = 0;
    return chunk;
}

void RenderFrame::ClearRetainedMaterials() {
    const int numRetainedMaterials = retainedMaterials.Count();
    for (int i = 0; i < numRetainedMaterials; ++i) {
        retainedMaterials[i].reset();
    }
    retainedMaterials.SetCount(0, false);
}

void RenderFrame::ClearRetainedMeshInstances() {
    const int numRetainedMeshInstances = retainedMeshInstances.Count();
    for (int i = 0; i < numRetainedMeshInstances; ++i) {
        retainedMeshInstances[i].reset();
    }
    retainedMeshInstances.SetCount(0, false);
}

void RenderFrame::ClearRetainedSkeletons() {
    const int numRetainedSkeletons = retainedSkeletons.Count();
    for (int i = 0; i < numRetainedSkeletons; ++i) {
        retainedSkeletons[i].reset();
    }
    retainedSkeletons.SetCount(0, false);
}

void RenderFrame::ClearRetainedFonts() {
    const int numRetainedFonts = retainedFonts.Count();
    for (int i = 0; i < numRetainedFonts; ++i) {
        retainedFonts[i].reset();
    }
    retainedFonts.SetCount(0, false);
}

void RenderFrame::ClearRetainedTextures() {
    const int numRetainedTextures = retainedTextures.Count();
    for (int i = 0; i < numRetainedTextures; ++i) {
        retainedTextures[i].reset();
    }
    retainedTextures.SetCount(0, false);
}

Material *RenderFrame::RetainMaterial(const SharedPtr<Material> &material) {
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

MeshInstance *RenderFrame::RetainMeshInstance(const SharedPtr<MeshInstance> &meshInstance) {
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

Skeleton *RenderFrame::RetainSkeleton(const SharedPtr<Skeleton> &skeleton) {
    if (!skeleton) {
        return nullptr;
    }

    Skeleton *skeletonPtr = skeleton.get();
    if (skeletonPtr->GetRetainedFrameSerial() != frameSerial) {
        skeletonPtr->SetRetainedFrameSerial(frameSerial);
        retainedSkeletons.Append(skeleton);
    }

    return skeletonPtr;
}

Font *RenderFrame::RetainFont(const SharedPtr<Font> &font) {
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

Texture *RenderFrame::RetainTexture(const SharedPtr<Texture> &texture) {
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

void RenderFrame::SetPendingImGuiOverlay(RHI::ImGuiOverlay *overlay, void *userData) {
    ClearPendingImGuiOverlay();

    pendingImGuiOverlay.overlay = overlay;
    pendingImGuiOverlay.userData = userData;
}

void RenderFrame::RenderPendingImGuiOverlay(RHI::CommandList *commandList) {
    PendingImGuiOverlay overlay = pendingImGuiOverlay;
    pendingImGuiOverlay = {};

    if (overlay.overlay) {
        // ImGui 의 캡쳐 데이터를 렌더링 후 폐기한다.
        RHI::renderer->RenderImGuiDrawData(GetThreadData(0), commandList, overlay.overlay, overlay.userData);
        RHI::renderer->FreeImGuiDrawData(overlay.overlay, overlay.userData);
    }
}

void RenderFrame::ClearPendingImGuiOverlay() {
    if (pendingImGuiOverlay.overlay) {
        // ImGui 의 캡쳐 데이터를 폐기한다.
        RHI::renderer->FreeImGuiDrawData(pendingImGuiOverlay.overlay, pendingImGuiOverlay.userData);
    }
    pendingImGuiOverlay = {};
}

FrameCamera *RenderFrame::AllocFrameCamera() {
    if (numFrameCameras >= maxFrameCameras) {
        assert(0);
        return nullptr;
    }

    // placement new 생성자 호출
    FrameCamera *newFrameCamera = &frameCameras[numFrameCameras];
    new (newFrameCamera) FrameCamera();

    newFrameCamera->index = numFrameCameras++;

    return newFrameCamera;
}

RenderFrame::ChunkRange<FrameObject> RenderFrame::GetFrameObjects(uint32_t firstIndex) const {
    assert(firstIndex <= numFrameObjects);
    if (firstIndex >= numFrameObjects) {
        return ChunkRange<FrameObject>(nullptr);
    }

    // 앞선 오브젝트를 개별 순회하지 않고 chunk 단위로 건너뛴다.
    Chunk *firstChunk = firstFrameObjectChunk;
    while (firstIndex >= firstChunk->count) {
        firstIndex -= firstChunk->count;
        firstChunk = firstChunk->next;
    }
    return ChunkRange<FrameObject>(firstChunk, firstIndex);
}

FrameObject *RenderFrame::AllocFrameObject() {
    if (numFrameObjects >= MaxFrameObjects) {
        assert(0);
        return nullptr;
    }

    if (!lastFrameObjectChunk || lastFrameObjectChunk->count >= FrameObjectChunkCapacity) {
        Chunk *newChunk = AllocChunk(sizeof(FrameObject), FrameObjectChunkCapacity);
        if (lastFrameObjectChunk) {
            lastFrameObjectChunk->next = newChunk;
        } else {
            firstFrameObjectChunk = newChunk;
        }
        lastFrameObjectChunk = newChunk;
    }

    FrameObject *frameObjects = static_cast<FrameObject *>(lastFrameObjectChunk->elements);
    FrameObject *newFrameObject = &frameObjects[lastFrameObjectChunk->count++];

    // placement new 생성자 호출
    new (newFrameObject) FrameObject();
    
    ++numFrameObjects;

    return newFrameObject;
}

RenderFrame::ChunkRange<FrameSkinnedMesh> RenderFrame::GetFrameSkinnedMeshes(uint32_t firstIndex) const {
    assert(firstIndex <= numFrameSkinnedMeshes);
    if (firstIndex >= numFrameSkinnedMeshes) {
        return ChunkRange<FrameSkinnedMesh>(nullptr);
    }

    // 앞선 카메라의 스키닝 메시를 개별 순회하지 않고 chunk 단위로 건너뛴다.
    Chunk *firstChunk = firstFrameSkinnedMeshChunk;
    while (firstIndex >= firstChunk->count) {
        firstIndex -= firstChunk->count;
        firstChunk = firstChunk->next;
    }
    return ChunkRange<FrameSkinnedMesh>(firstChunk, firstIndex);
}

FrameSkinnedMesh *RenderFrame::AllocFrameSkinnedMesh() {
    // FrameObject 하나당 최대 한 개의 스키닝 레코드만 등록한다.
    if (numFrameSkinnedMeshes >= numFrameObjects) {
        assert(0);
        return nullptr;
    }

    if (!lastFrameSkinnedMeshChunk || lastFrameSkinnedMeshChunk->count >= FrameSkinnedMeshChunkCapacity) {
        Chunk *newChunk = AllocChunk(sizeof(FrameSkinnedMesh), FrameSkinnedMeshChunkCapacity);
        if (lastFrameSkinnedMeshChunk) {
            lastFrameSkinnedMeshChunk->next = newChunk;
        } else {
            firstFrameSkinnedMeshChunk = newChunk;
        }
        lastFrameSkinnedMeshChunk = newChunk;
    }

    FrameSkinnedMesh *frameSkinnedMeshes = static_cast<FrameSkinnedMesh *>(lastFrameSkinnedMeshChunk->elements);
    FrameSkinnedMesh *newFrameSkinnedMesh = &frameSkinnedMeshes[lastFrameSkinnedMeshChunk->count++];
    new (newFrameSkinnedMesh) FrameSkinnedMesh();
    ++numFrameSkinnedMeshes;
    return newFrameSkinnedMesh;
}

RenderFrame::ChunkRange<FrameText> RenderFrame::GetFrameTexts(uint32_t firstIndex) const {
    assert(firstIndex <= numFrameTexts);
    if (firstIndex >= numFrameTexts) {
        return ChunkRange<FrameText>(nullptr);
    }

    // 앞선 텍스트를 개별 순회하지 않고 chunk 단위로 건너뛴다.
    Chunk *firstChunk = firstFrameTextChunk;
    while (firstIndex >= firstChunk->count) {
        firstIndex -= firstChunk->count;
        firstChunk = firstChunk->next;
    }
    return ChunkRange<FrameText>(firstChunk, firstIndex);
}

FrameText *RenderFrame::AllocFrameText() {
    if (numFrameTexts >= MaxFrameTexts) {
        assert(0);
        return nullptr;
    }

    if (!lastFrameTextChunk || lastFrameTextChunk->count >= FrameTextChunkCapacity) {
        Chunk *newChunk = AllocChunk(sizeof(FrameText), FrameTextChunkCapacity);
        if (lastFrameTextChunk) {
            lastFrameTextChunk->next = newChunk;
        } else {
            firstFrameTextChunk = newChunk;
        }
        lastFrameTextChunk = newChunk;
    }

    FrameText *frameTexts = static_cast<FrameText *>(lastFrameTextChunk->elements);
    FrameText *newFrameText = &frameTexts[lastFrameTextChunk->count++];

    // placement new 생성자 호출
    new (newFrameText) FrameText();

    numFrameTexts++;

    return newFrameText;
}

RenderFrame::ChunkRange<FrameLight> RenderFrame::GetFrameLights(uint32_t firstIndex) const {
    assert(firstIndex <= numFrameLights);
    if (firstIndex >= numFrameLights) {
        return ChunkRange<FrameLight>(nullptr);
    }

    // 앞선 라이트를 개별 순회하지 않고 chunk 단위로 건너뛴다.
    Chunk *firstChunk = firstFrameLightChunk;
    while (firstIndex >= firstChunk->count) {
        firstIndex -= firstChunk->count;
        firstChunk = firstChunk->next;
    }
    return ChunkRange<FrameLight>(firstChunk, firstIndex);
}

FrameSpotLightData *RenderFrame::AllocFrameSpotLightData() {
    void *memory = MemAlloc(sizeof(FrameSpotLightData));
    return new (memory) FrameSpotLightData();
}

FrameOceanSurface *RenderFrame::AllocFrameOceanSurface(const GpuOcean::SimulationSettings &simulationSettings, const GpuOceanSurface::Settings &surfaceSettings) {
    if (frameOceanSurface) {
        assert(0);
        return nullptr;
    }

    void *memory = MemAlloc(sizeof(FrameOceanSurface));
    if (!memory) {
        return nullptr;
    }
    // 기본 설정의 cascade 계산 후 덮어쓰지 않고, 전달받은 설정으로 바로 생성한다.
    frameOceanSurface = new (memory) FrameOceanSurface{simulationSettings, surfaceSettings};
    return frameOceanSurface;
}

FrameShadowAtlas *RenderFrame::AllocFrameShadowAtlas(const ShadowAtlas &shadowAtlas) {
    if (!renderFrameSettings.shadows || renderFrameSettings.shadowAtlasMaxViews == 0 ||
        !shadowAtlas.GetShadowTexture() || !shadowAtlas.GetShadowDSV().IsValid() || !shadowAtlas.GetShadowSRV().IsValid()) {
        return nullptr;
    }

    void *memory = MemAlloc(sizeof(FrameShadowAtlas));
    if (!memory) {
        return nullptr;
    }
    FrameShadowAtlas *data = new (memory) FrameShadowAtlas();
    data->shadowTexture = shadowAtlas.GetShadowTexture();
    data->shadowDSV = shadowAtlas.GetShadowDSV();
    data->shadowSRV = shadowAtlas.GetShadowSRV();
    data->size = shadowAtlas.GetSize();
    data->maxShadowViews = renderFrameSettings.shadowAtlasMaxViews;
    return data;
}

// FrameShadowAtlas 에 shadows 와 필요한 shadowViews 를 추가한다.
FrameLocalShadow *RenderFrame::AllocFrameLocalShadow(FrameShadowAtlas &frameShadowAtlas, FrameLight &shadowLight) {
    if (shadowLight.localShadowIndex != ~0u || !BE1::HasFlag(shadowLight.flags, RenderLight::Flag::CastShadows)) {
        return nullptr;
    }

    const uint32_t shadowViewCount = shadowLight.GetShadowViewCount();
    if (shadowViewCount == 0 || shadowViewCount > frameShadowAtlas.maxShadowViews - frameShadowAtlas.numShadowViews) {
        return nullptr;
    }

    if (!frameShadowAtlas.shadowViews) {
        // 최악의 경우 모두 spot shadow 일 때 numShadows == numShadowViews 이므로 maxShadowViews 만큼 shadows 를 할당한다.
        FrameLocalShadow *shadows = static_cast<FrameLocalShadow *>(MemAlloc(sizeof(FrameLocalShadow) * frameShadowAtlas.maxShadowViews));
        FrameLocalShadowView *shadowViews = static_cast<FrameLocalShadowView *>(MemAlloc(sizeof(FrameLocalShadowView) * frameShadowAtlas.maxShadowViews));
        if (!shadows || !shadowViews) {
            return nullptr;
        }

        frameShadowAtlas.shadows = shadows;
        frameShadowAtlas.shadowViews = shadowViews;
    }

    const uint32_t newShadowIndex = frameShadowAtlas.numShadows;
    FrameLocalShadow *shadow = new (&frameShadowAtlas.shadows[newShadowIndex]) FrameLocalShadow();
    shadow->light = &shadowLight;
    shadow->firstShadowView = frameShadowAtlas.numShadowViews;
    shadow->numShadowViews = shadowViewCount;

    for (uint32_t faceIndex = 0; faceIndex < shadowViewCount; ++faceIndex) {
        FrameLocalShadowView *shadowView = new (&frameShadowAtlas.shadowViews[frameShadowAtlas.numShadowViews + faceIndex]) FrameLocalShadowView();
        shadowView->shadowIndex = newShadowIndex;
        shadowView->faceIndex = faceIndex;
    }

    frameShadowAtlas.numShadowViews += shadowViewCount;
    frameShadowAtlas.numShadows++;

    shadowLight.localShadowIndex = newShadowIndex;
    return shadow;
}

FrameLight *RenderFrame::AllocFrameLight(RenderLight::Type type) {
    if (type == RenderLight::Type::Directional && primaryDirectionalLight) {
        assert(0);
        return nullptr;
    }

    if (numFrameLights >= MaxFrameLights) {
        assert(0);
        return nullptr;
    }

    if (!lastFrameLightChunk || lastFrameLightChunk->count >= FrameLightChunkCapacity) {
        Chunk *newChunk = AllocChunk(sizeof(FrameLight), FrameLightChunkCapacity);
        if (lastFrameLightChunk) {
            lastFrameLightChunk->next = newChunk;
        } else {
            firstFrameLightChunk = newChunk;
        }
        lastFrameLightChunk = newChunk;
    }

    FrameLight *frameLights = static_cast<FrameLight *>(lastFrameLightChunk->elements);
    FrameLight *newFrameLight = &frameLights[lastFrameLightChunk->count++];

    // placement new 생성자 호출
    new (newFrameLight) FrameLight();

    newFrameLight->index = static_cast<uint16_t>(numFrameLights++);
    newFrameLight->type = type;

    switch (type) {
    case RenderLight::Type::Point:
        numFrameLocalLights++;
        newFrameLight->typeIndex = static_cast<uint16_t>(numFramePointLights++);
        break;
    case RenderLight::Type::Spot:
        numFrameLocalLights++;
        newFrameLight->typeIndex = static_cast<uint16_t>(numFrameSpotLights++);
        break;
    case RenderLight::Type::BoxDir:
        numFrameLocalLights++;
        newFrameLight->typeIndex = static_cast<uint16_t>(numFrameBoxDirLights++);
        break;
    case RenderLight::Type::Directional:
        newFrameLight->typeIndex = 0;
        primaryDirectionalLight = newFrameLight;
        break;
    default:
        assert(0);
        newFrameLight->typeIndex = 0;
        break;
    }

    return newFrameLight;
}

uint32_t RenderFrame::ReserveLightCullingZBins() {
    assert(ShaderLightZBinCount <= UINT32_MAX - numLightCullingZBins);

    const uint32_t offset = numLightCullingZBins;
    numLightCullingZBins += ShaderLightZBinCount;
    return offset;
}

uint32_t RenderFrame::ReserveLightCullingWords(uint32_t count) {
    assert(count <= UINT32_MAX - numLightCullingWords);

    const uint32_t offset = numLightCullingWords;
    numLightCullingWords += count;
    return offset;
}

void *RenderFrame::GetCommandBuffer(uint32_t bytes) {
    RenderCommandBuffer *cmds = GetCommands();

    assert(cmds->used <= RenderCommandBuffer::BufferSize);

    const uint32_t availableBytes = RenderCommandBuffer::BufferSize - cmds->used;
    if (bytes > availableBytes) {
        BE_WARNLOG("RenderFrame::GetCommandBuffer: not enough command buffer space\n");
        return nullptr;
    }

    void *commandBuffer = cmds->buffer + cmds->used;
    cmds->used += bytes;
    return commandBuffer;
}

bool RenderFrame::CmdRenderWork(RenderWork *renderWork) {
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

void RenderFrame::CmdBeginContext(RenderContext *context, bool gpuTiming, bool textureStreamingFeedback, bool swapChainOutput) {
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

void RenderFrame::CmdDrawCamera(const FrameCamera *camera) {
    DrawCameraRenderCommand *cmd = (DrawCameraRenderCommand *)GetCommandBuffer(sizeof(DrawCameraRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::DrawCamera;
    cmd->frameCamera = camera;

    if (camera) {
        maxShaderSurfaceCount = BE1::Max(maxShaderSurfaceCount, camera->numFrameSurfs);
    }
}

void RenderFrame::CmdFinalizeFrame(bool renderImGuiOverlay) {
    FinalizeFrameRenderCommand *cmd = (FinalizeFrameRenderCommand *)GetCommandBuffer(sizeof(FinalizeFrameRenderCommand));
    if (!cmd) {
        BE_FATALERROR("RenderFrame::CmdFinalizeFrame: failed to allocate command buffer\n");
        return;
    }

    cmd->commandId = RenderCommandId::FinalizeFrame;
    cmd->renderImGuiOverlay = renderImGuiOverlay;
}

void RenderFrame::CmdSwapBuffers() {
    SwapBuffersRenderCommand *cmd = (SwapBuffersRenderCommand *)GetCommandBuffer(sizeof(SwapBuffersRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::SwapBuffers;
}

void RenderFrame::CmdScreenshot(int x, int y, int width, int height, const char *filename) {
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

void RenderFrame::CmdEnd() {
    EndRenderCommand *cmd = (EndRenderCommand *)GetCommandBuffer(sizeof(EndRenderCommand));
    if (!cmd) {
        BE_FATALERROR("RenderFrame::CmdEnd: failed to allocate command buffer\n");
        return;
    }

    cmd->commandId = RenderCommandId::End;
}
