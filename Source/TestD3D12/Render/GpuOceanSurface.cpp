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
#include "GpuOceanSurface.h"
#include "ShaderPipelineBootstrap.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/Texture/Texture.h"
#include "Render/Texture/TextureManager.h"
#include "Shaders/ShaderInterop.h"
#include "Math/AABB.h"
#include "Math/OBB.h"
#include "Math/Frustum.h"

static constexpr uint32_t OceanSurfaceGridLodSteps[] = { 1u, 3u, 5u };
static constexpr float MinOceanSurfaceShaderDivisor = 1.0e-4f;

static_assert(GpuOcean::CascadeCount == OceanCascadeCount);
static_assert(COUNT_OF(OceanSurfaceGridLodSteps) == GpuOceanSurface::GridLodCount);
static_assert(GpuOceanSurface::GridLodCount == OceanSurfaceGridLodCount);
static_assert(GpuOceanSurface::GridLodCount == 3u);
static_assert((GpuOceanSurface::GridResolution - 1u) % OceanSurfaceGridLodSteps[GpuOceanSurface::GridLodCount - 1u] == 0u);

bool GpuOceanSurface::Settings::IsValid() const {
    return gridTileCountPerAxis > 0 && gridTileCountPerAxis <= MaxGridTileCountPerAxis && gridSpacing > 0.0f;
}

void GpuOceanSurface::Init(GpuOcean *gpuOcean) {
    assert(gpuOcean);

    this->gpuOcean = gpuOcean;
    queryResultCache.Clear();
    queryResultCache.Init(MaxQueryCount, MaxQueryCount, MaxQueryCount);

    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel shaderModel = RHI::Renderer::MinimumShaderModel();
    constexpr bool asyncPipelineBuild = true;

    queryShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_sampleOceanSurfaceShader", shaderModel, "Water/SampleOceanSurface.hlsl", baseDir, "CSMain");
    queryPipeline = shaderPipelineCache.GetComputePipeline(queryShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    shaderTemplate = shaderTemplateManager.GetShaderTemplate("Source/TestD3D12/Shaders/Water/OceanSurface");
    if (!shaderTemplate) {
        BE_WARNLOG("GpuOceanSurface::Init: failed to load ocean surface shader template\n");
    }

    foamDetailTexture = textureManager.GetTexture("Data/EngineTextures/water_form.PNG", Texture::Flag::LinearColorSpace);
    if (foamDetailTexture == textureManager.defaultTexture) {
        BE_WARNLOG("GpuOceanSurface::Init: failed to load ocean foam detail texture\n");
        foamDetailTexture.reset();
    }

    CreateGrid();
}

void GpuOceanSurface::Shutdown() {
    DestroyQueryResources();
    queryResultCache.Clear();
    DestroyGrid();

    foamDetailTexture.reset();
    queryPipeline.reset();
    queryShaderTemplate.reset();
    shaderTemplate.reset();
    gpuOcean = nullptr;
}

void GpuOceanSurface::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "GpuOceanSurface";

    bootstrap.TrackPipeline(groupName, queryPipeline);
}

GpuOceanSurface::QueryReadbackFrame *GpuOceanSurface::DispatchQueries(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanSurfaceQueryInput *queries, uint32_t queryCount, float surfaceHeight) {
    PROFILER_CPU_SCOPED_EVENT("GpuOceanSurface::DispatchQueries");

    if (!gpuOcean || !queries || queryCount == 0 || queryCount > MaxQueryCount) {
        return nullptr;
    }

    const RHI::Texture *fieldTextureArray = gpuOcean->GetFieldTextureArray();
    if (!gpuOcean->AreCascadeOutputsReady() || !fieldTextureArray || fieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return nullptr;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(queryPipeline);
    if (!pso || !EnsureQueryResources()) {
        return nullptr;
    }

    // 이번 query dispatch 결과를 받을 유휴 readback frame 을 찾는다.
    QueryReadbackFrame *readbackFrame = nullptr;
    uint32_t readbackFrameIndex = queryReadbackFrameIndex;
    for (uint32_t attempt = 0; attempt < COUNT_OF(queryReadbackFrames); ++attempt) {
        QueryReadbackFrame &candidate = queryReadbackFrames[readbackFrameIndex];
        if (candidate.buffer && candidate.state.load() == QueryReadbackFrame::State::Idle) {
            readbackFrame = &candidate;
            break;
        }
        readbackFrameIndex = (readbackFrameIndex + 1u) % COUNT_OF(queryReadbackFrames);
    }

    if (!readbackFrame) {
        return nullptr;
    }

    RHI::Buffer *queryInputBuffer = frameThreadData->AllocStructuredBuffer(sizeof(OceanSurfaceQueryInput), queryCount, queries);
    if (!queryInputBuffer) {
        return nullptr;
    }

    float cascadePatchSizes[4] = {};
    static_assert(GpuOcean::CascadeCount <= COUNT_OF(cascadePatchSizes));
    for (uint32_t cascadeIndex = 0; cascadeIndex < GpuOcean::CascadeCount; ++cascadeIndex) {
        cascadePatchSizes[cascadeIndex] = gpuOcean->GetCascadePatchSize(cascadeIndex);
    }

    OceanSurfaceQueryParams params = {};
    params.cascadePatchSizes = float4(cascadePatchSizes);
    params.surfaceHeight = surfaceHeight;
    params.displacementScale = gpuOcean->GetDisplacementScale();
    params.sampleTime = gpuOcean->GetSimulationTime();
    params.queryCount = queryCount;

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(params), &params);
    if (!constantBuffer) {
        return nullptr;
    }

    readbackFrame->queryCount = queryCount;

    // fence 값이 등록되기 전까지 다른 dispatch 가 이 readback frame 을 재사용하지 못하게 한다.
    readbackFrame->state.store(QueryReadbackFrame::State::Resolving);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(queryOutputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, fieldTextureArray);
    RHI::renderer->SetBuffer(commandList, 1, false, queryInputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, queryOutputBuffer, queryOutputUAV);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);
    RHI::renderer->Dispatch(commandList, BE1::DivCeil(queryCount, OceanSurfaceQueryBlockSize), 1, 1);

    // compute shader 가 생성한 결과를 CPU readback buffer 로 복사한다.
    const uint32_t resultSize = queryCount * sizeof(OceanSurfaceQueryResult);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(queryOutputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::CopySrc));
    RHI::renderer->CopyBuffer(commandList, readbackFrame->buffer, 0, queryOutputBuffer, 0, resultSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(queryOutputBuffer, RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::Undefined));

    queryReadbackFrameIndex = (readbackFrameIndex + 1u) % COUNT_OF(queryReadbackFrames);
    return readbackFrame;
}

void GpuOceanSurface::ConsumeCompletedQueries() {
    assert(BE1::Engine::IsInMainThread());

    for (QueryReadbackFrame &readbackFrame : queryReadbackFrames) {
        if (readbackFrame.state.load() != QueryReadbackFrame::State::Submitted) {
            continue;
        }

        const uint64_t fenceValue = readbackFrame.fenceValue.load();
        if (!RHI::renderer->IsFenceComplete(RHI::CommandQueueType::Graphics, fenceValue)) {
            continue;
        }

        const OceanSurfaceQueryResult *results = static_cast<const OceanSurfaceQueryResult *>(readbackFrame.buffer->mappedPtr);
        for (uint32_t resultIndex = 0; resultIndex < readbackFrame.queryCount; ++resultIndex) {
            const OceanSurfaceQueryResult &result = results[resultIndex];
            queryResultCache.Set(result.queryId, result);
        }

        // readback frame 을 재사용할 수 있도록 상태를 초기화한다.
        readbackFrame.queryCount = 0;
        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(QueryReadbackFrame::State::Idle);
    }
}

bool GpuOceanSurface::GetQueryResult(uint32_t queryId, OceanSurfaceQueryResult &outResult) const {
    assert(BE1::Engine::IsInMainThread());

    const BE1::HashMap<uint32_t, OceanSurfaceQueryResult>::KV *cachedResult = queryResultCache.Get(queryId);
    if (!cachedResult) {
        return false;
    }

    outResult = cachedResult->second;
    return true;
}

bool GpuOceanSurface::EnsureQueryResources() {
    if (queryOutputBuffer) {
        return true;
    }

    DestroyQueryResources();

    const uint32_t bufferSize = MaxQueryCount * sizeof(OceanSurfaceQueryResult);
    queryOutputBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::UnorderedAccess, bufferSize, nullptr);
    if (!queryOutputBuffer) {
        BE_WARNLOG("GpuOceanSurface::EnsureQueryResources: failed to create output buffer\n");
        return false;
    }
    RHI::renderer->SetName(queryOutputBuffer, "GpuOceanSurface::queryOutputBuffer");

    queryOutputUAV = RHI::renderer->CreateView(queryOutputBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(OceanSurfaceQueryResult), 0, bufferSize);
    if (!queryOutputUAV.IsValid()) {
        BE_WARNLOG("GpuOceanSurface::EnsureQueryResources: failed to create output UAV\n");
        DestroyQueryResources();
        return false;
    }

    for (int i = 0; i < COUNT_OF(queryReadbackFrames); ++i) {
        QueryReadbackFrame &readbackFrame = queryReadbackFrames[i];
        readbackFrame.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Readback, RHI::BufferFlag::None, bufferSize, nullptr);
        if (!readbackFrame.buffer) {
            BE_WARNLOG("GpuOceanSurface::EnsureQueryResources: failed to create readback buffer %i\n", i);
            DestroyQueryResources();
            return false;
        }

        RHI::renderer->SetName(readbackFrame.buffer, BE1::va("GpuOceanSurface::queryReadbackFrames[%i].buffer", i));
        readbackFrame.queryCount = 0;
        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(QueryReadbackFrame::State::Idle);
    }

    queryReadbackFrameIndex = 0;
    return true;
}

void GpuOceanSurface::DestroyQueryResources() {
    for (QueryReadbackFrame &readbackFrame : queryReadbackFrames) {
        if (readbackFrame.buffer) {
            RHI::renderer->DestroyBuffer(readbackFrame.buffer);
            readbackFrame.buffer = nullptr;
        }
        readbackFrame.queryCount = 0;
        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(QueryReadbackFrame::State::Idle);
    }

    if (queryOutputBuffer) {
        RHI::renderer->DestroyBuffer(queryOutputBuffer);
        queryOutputBuffer = nullptr;
    }
    queryOutputUAV = RHI::ViewID::InvalidID;
    queryReadbackFrameIndex = 0;
}

static constexpr uint32_t GetOceanSurfaceGridLodIndexCount(uint32_t lodLevel) {
    const uint32_t quadCountPerAxis = (GpuOceanSurface::GridResolution - 1u) / OceanSurfaceGridLodSteps[lodLevel];
    return quadCountPerAxis * quadCountPerAxis * 6u;
}

bool GpuOceanSurface::CreateGrid() {
    static_assert(GridResolution == OceanSurfaceGridResolution);
    static_assert(GridResolution * GridResolution <= 1u << 16u);

    if (gridIndexBuffer) {
        return true;
    }

    DestroyGrid();

    uint32_t totalIndexCount = 0;
    for (uint32_t lodLevel = 0; lodLevel < GridLodCount; ++lodLevel) {
        gridLodRanges[lodLevel].firstIndex = totalIndexCount;
        gridLodRanges[lodLevel].indexCount = GetOceanSurfaceGridLodIndexCount(lodLevel);
        totalIndexCount += gridLodRanges[lodLevel].indexCount;
    }

    BE1::Array<uint16_t> indices;
    indices.SetCount(totalIndexCount);

    uint32_t indexOffset = 0;
    for (uint32_t lodLevel = 0; lodLevel < GridLodCount; ++lodLevel) {
        const uint32_t gridStep = OceanSurfaceGridLodSteps[lodLevel];
        const GridLodRange &lodRange = gridLodRanges[lodLevel];
        assert(indexOffset == lodRange.firstIndex);

        for (uint32_t y = 0; y < GridResolution - 1; y += gridStep) {
            for (uint32_t x = 0; x < GridResolution - 1; x += gridStep) {
                const uint32_t bottomLeft = y * GridResolution + x;
                const uint32_t bottomRight = bottomLeft + gridStep;
                const uint32_t topLeft = bottomLeft + gridStep * GridResolution;
                const uint32_t topRight = topLeft + gridStep;

                indices[indexOffset++] = static_cast<uint16_t>(bottomLeft);
                indices[indexOffset++] = static_cast<uint16_t>(bottomRight);
                indices[indexOffset++] = static_cast<uint16_t>(topLeft);
                indices[indexOffset++] = static_cast<uint16_t>(topLeft);
                indices[indexOffset++] = static_cast<uint16_t>(bottomRight);
                indices[indexOffset++] = static_cast<uint16_t>(topRight);
            }
        }
        assert(indexOffset == lodRange.firstIndex + lodRange.indexCount);
    }

    gridIndexBuffer = RHI::renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indices[0]), totalIndexCount, indices.Ptr());
    if (!gridIndexBuffer) {
        BE_WARNLOG("GpuOceanSurface::CreateGrid: failed to create packed surface grid index buffer\n");
        DestroyGrid();
        return false;
    }
    RHI::renderer->SetName(gridIndexBuffer, "GpuOceanSurface::gridIndexBuffer");
    return true;
}

void GpuOceanSurface::DestroyGrid() {
    if (gridIndexBuffer) {
        RHI::renderer->DestroyIndexBuffer(gridIndexBuffer);
        gridIndexBuffer = nullptr;
    }
    for (GridLodRange &lodRange : gridLodRanges) {
        lodRange = {};
    }
}

static uint32_t GetOceanSurfaceTileLodLevel(float tileX, float tileY) {
    const float tileRingDistance = BE1::Max(BE1::Math::Abs(tileX), BE1::Math::Abs(tileY));
    if (tileRingDistance <= 1.5f) {
        return 0u;
    }
    if (tileRingDistance <= 2.5f) {
        return 1u;
    }
    return 2u;
}

static constexpr uint32_t MaxOceanSurfaceTileCount = GpuOceanSurface::MaxGridTileCountPerAxis * GpuOceanSurface::MaxGridTileCountPerAxis;

struct OceanSurfaceTileBatch {
    OceanSurfaceTileInstance    instances[MaxOceanSurfaceTileCount];
    uint32_t                    lodInstanceCounts[GpuOceanSurface::GridLodCount];
    uint32_t                    lodInstanceOffsets[GpuOceanSurface::GridLodCount];
    uint32_t                    instanceCount;
    float                       gridSnapSpacing;
};

static bool BuildOceanSurfaceTileBatch(const GpuOceanSurface::Settings &settings, const GpuOceanSurface::View &view, OceanSurfaceTileBatch &batch) {
    batch = {};

    const bool hasPerspectiveFrustum = view.frustum != nullptr;
    const bool hasOrthographicBox = view.box != nullptr;
    if (settings.frustumCulling && hasPerspectiveFrustum == hasOrthographicBox) {
        return false;
    }

    const uint32_t tileCountPerAxis = settings.gridTileCountPerAxis;
    const float tileCenterOffset = static_cast<float>(tileCountPerAxis - 1u) * 0.5f;

    uint32_t tileLodLevels[MaxOceanSurfaceTileCount] = {};
    uint32_t tileLodCounts[GpuOceanSurface::GridLodCount] = {};
    for (uint32_t tileY = 0; tileY < tileCountPerAxis; ++tileY) {
        for (uint32_t tileX = 0; tileX < tileCountPerAxis; ++tileX) {
            const float tilePositionX = static_cast<float>(tileX) - tileCenterOffset;
            const float tilePositionY = static_cast<float>(tileY) - tileCenterOffset;
            const uint32_t tileIndex = tileY * tileCountPerAxis + tileX;
            const uint32_t lodLevel = GetOceanSurfaceTileLodLevel(tilePositionX, tilePositionY);
            tileLodLevels[tileIndex] = lodLevel;
            tileLodCounts[lodLevel]++;
        }
    }

    // 모든 tile LOD 의 world-space 정점 격자 위상이 유지되도록 활성 stride 의 최소공배수 간격으로 grid 원점을 이동한다.
    const uint32_t gridSnapStep = tileLodCounts[2] > 0u ? 15u : (tileLodCounts[1] > 0u ? 3u : 1u);
    batch.gridSnapSpacing = settings.gridSpacing * static_cast<float>(gridSnapStep);
    const BE1::Vec2 snappedCameraPosition(
        BE1::Math::Round(view.origin.x / batch.gridSnapSpacing) * batch.gridSnapSpacing,
        BE1::Math::Round(view.origin.y / batch.gridSnapSpacing) * batch.gridSnapSpacing);

    const float tileSize = static_cast<float>(GpuOceanSurface::GridResolution - 1u) * settings.gridSpacing;
    const float tileHalfSize = tileSize * 0.5f;
    BE1::Frustum expandedSurfaceFrustum;
    const BE1::Frustum *surfaceFrustum = view.frustum;
    if (settings.frustumCulling && hasPerspectiveFrustum && view.infiniteZFar) {
        expandedSurfaceFrustum = *view.frustum;

        // Infinite-far 투영에서 유한 camera far plane 때문에 실제로 투영 가능한 외곽 tile 이 제거되지 않도록 전체 grid 를 포함한다.
        const float gridHalfSize = static_cast<float>(tileCountPerAxis) * tileHalfSize;
        const float maxOffsetX = BE1::Math::Abs(snappedCameraPosition.x - view.origin.x) + gridHalfSize + settings.maxDisplacement;
        const float maxOffsetY = BE1::Math::Abs(snappedCameraPosition.y - view.origin.y) + gridHalfSize + settings.maxDisplacement;
        const float maxOffsetZ = BE1::Math::Abs(settings.height - view.origin.z) + settings.maxDisplacement;
        const float requiredFarDistance = BE1::Math::Sqrt(maxOffsetX * maxOffsetX + maxOffsetY * maxOffsetY + maxOffsetZ * maxOffsetZ);
        if (requiredFarDistance > expandedSurfaceFrustum.GetFarDistance()) {
            expandedSurfaceFrustum.MoveFarDistance(requiredFarDistance);
        }
        surfaceFrustum = &expandedSurfaceFrustum;
    }

    bool tileVisible[MaxOceanSurfaceTileCount] = {};
    if (settings.frustumCulling) {
        for (uint32_t tileY = 0; tileY < tileCountPerAxis; ++tileY) {
            for (uint32_t tileX = 0; tileX < tileCountPerAxis; ++tileX) {
                const uint32_t tileIndex = tileY * tileCountPerAxis + tileX;
                const float tilePositionX = static_cast<float>(tileX) - tileCenterOffset;
                const float tilePositionY = static_cast<float>(tileY) - tileCenterOffset;
                const BE1::Vec3 tileCenter(
                    snappedCameraPosition.x + tilePositionX * tileSize,
                    snappedCameraPosition.y + tilePositionY * tileSize,
                    settings.height);

                // FFT field 의 최대 변위를 CPU 에서 알 수 없으므로 설정된 보수적 최대값으로 모든 축을 확장한다.
                const BE1::AABB tileBounds(
                    tileCenter - BE1::Vec3(tileHalfSize, tileHalfSize, 0.0f),
                    tileCenter + BE1::Vec3(tileHalfSize, tileHalfSize, 0.0f));
                const BE1::AABB expandedTileBounds = tileBounds.Expand(settings.maxDisplacement);
                const bool culled = hasPerspectiveFrustum ? surfaceFrustum->CullAABB(expandedTileBounds) : !view.box->IsIntersectAABB(expandedTileBounds);
                if (!culled) {
                    tileVisible[tileIndex] = true;
                    batch.instanceCount++;
                    batch.lodInstanceCounts[tileLodLevels[tileIndex]]++;
                }
            }
        }
    } else {
        batch.instanceCount = tileCountPerAxis * tileCountPerAxis;
        for (uint32_t tileIndex = 0; tileIndex < batch.instanceCount; ++tileIndex) {
            tileVisible[tileIndex] = true;
        }
        for (uint32_t lodLevel = 0; lodLevel < GpuOceanSurface::GridLodCount; ++lodLevel) {
            batch.lodInstanceCounts[lodLevel] = tileLodCounts[lodLevel];
        }
    }

    if (batch.instanceCount == 0u) {
        return true;
    }

    uint32_t lodInstanceWriteOffsets[GpuOceanSurface::GridLodCount] = {};
    for (uint32_t lodLevel = 1; lodLevel < GpuOceanSurface::GridLodCount; ++lodLevel) {
        batch.lodInstanceOffsets[lodLevel] = batch.lodInstanceOffsets[lodLevel - 1u] + batch.lodInstanceCounts[lodLevel - 1u];
    }
    for (uint32_t lodLevel = 0; lodLevel < GpuOceanSurface::GridLodCount; ++lodLevel) {
        lodInstanceWriteOffsets[lodLevel] = batch.lodInstanceOffsets[lodLevel];
    }

    for (uint32_t tileY = 0; tileY < tileCountPerAxis; ++tileY) {
        for (uint32_t tileX = 0; tileX < tileCountPerAxis; ++tileX) {
            const uint32_t tileIndex = tileY * tileCountPerAxis + tileX;
            if (!tileVisible[tileIndex]) {
                continue;
            }
            const uint32_t lodLevel = tileLodLevels[tileIndex];
            auto GetNeighborLodLevel = [&](int32_t neighborX, int32_t neighborY) {
                if (neighborX < 0 || neighborY < 0 || neighborX >= static_cast<int32_t>(tileCountPerAxis) || neighborY >= static_cast<int32_t>(tileCountPerAxis)) {
                    return lodLevel;
                }
                return tileLodLevels[neighborY * tileCountPerAxis + neighborX];
            };

            const uint32_t leftLodLevel = GetNeighborLodLevel(static_cast<int32_t>(tileX) - 1, tileY);
            const uint32_t rightLodLevel = GetNeighborLodLevel(static_cast<int32_t>(tileX) + 1, tileY);
            const uint32_t bottomLodLevel = GetNeighborLodLevel(tileX, static_cast<int32_t>(tileY) - 1);
            const uint32_t topLodLevel = GetNeighborLodLevel(tileX, static_cast<int32_t>(tileY) + 1);

            OceanSurfaceTileInstance &tileInstance = batch.instances[lodInstanceWriteOffsets[lodLevel]++];
            tileInstance.tilePosition = float2(static_cast<float>(tileX) - tileCenterOffset, static_cast<float>(tileY) - tileCenterOffset);
            tileInstance.lodLevel = lodLevel;
            // left, right, bottom, top 순서로 이웃 LOD 를 8비트씩 패킹한다.
            tileInstance.packedNeighborLodLevels = leftLodLevel | (rightLodLevel << 8u) | (bottomLodLevel << 16u) | (topLodLevel << 24u);
        }
    }

    for (uint32_t lodLevel = 0; lodLevel < GpuOceanSurface::GridLodCount; ++lodLevel) {
        assert(lodInstanceWriteOffsets[lodLevel] == batch.lodInstanceOffsets[lodLevel] + batch.lodInstanceCounts[lodLevel]);
    }

    return true;
}

bool GpuOceanSurface::Draw(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const RHI::PipelineState *pipelineState,
    const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer, const Settings &settings, const View &view,
    const RHI::Texture *sceneColorTexture, const RHI::Texture *sceneDepthTexture, DrawStats *drawStats) const {
    PROFILER_CPU_SCOPED_EVENT("GpuOceanSurface::Draw");

    assert(settings.IsValid());
    assert(view.renderWidth > 0u && view.renderHeight > 0u);

    if (drawStats) {
        *drawStats = {};
    }

    if (!gpuOcean || !pipelineState || !frameConstantBuffer || !cameraConstantBuffer || !gridIndexBuffer || !gpuOcean->AreCascadeOutputsReady()) {
        return false;
    }

    const RHI::Texture *fieldTextureArray = gpuOcean->GetFieldTextureArray();
    const RHI::Texture *foamHistoryTextureArray = gpuOcean->GetFoamHistoryTextureArray();
    if (!fieldTextureArray || fieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !foamHistoryTextureArray || foamHistoryTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    RHI::Texture *fallbackTexture = textureManager.whiteTexture ? textureManager.whiteTexture->GetReadyRHITexture() : nullptr;
    if (!fallbackTexture) {
        return false;
    }

    RHI::Texture *foamDetailMap = foamDetailTexture ? foamDetailTexture->GetReadyRHITexture() : nullptr;
    if (!foamDetailMap) {
        foamDetailMap = fallbackTexture;
    }

    assert((sceneColorTexture != nullptr) == (sceneDepthTexture != nullptr));
    const bool refractionEnabled = settings.refractionEnabled && sceneColorTexture && sceneDepthTexture;
    assert(!refractionEnabled || (sceneColorTexture->GetWidth() == sceneDepthTexture->GetWidth() && sceneColorTexture->GetHeight() == sceneDepthTexture->GetHeight()));
    const RHI::Texture *boundSceneColorTexture = refractionEnabled ? sceneColorTexture : fallbackTexture;
    const RHI::Texture *boundSceneDepthTexture = refractionEnabled ? sceneDepthTexture : fallbackTexture;

    OceanSurfaceTileBatch tileBatch;
    if (!BuildOceanSurfaceTileBatch(settings, view, tileBatch)) {
        return false;
    }
    if (tileBatch.instanceCount == 0u) {
        return true;
    }

    RHI::Buffer *tileInstanceBuffer = frameThreadData->AllocStructuredBuffer(sizeof(OceanSurfaceTileInstance), tileBatch.instanceCount, tileBatch.instances);
    if (!tileInstanceBuffer) {
        return false;
    }

    // 캐스케이드 별 패치 사이즈 (L) 와 grid spacing 에 맞는 필드 텍스쳐의 mip LOD 계산
    float cascadePatchSizes[4] = {};
    float cascadeVertexMipLevels[4] = {};
    const float fieldTextureResolution = static_cast<float>(fieldTextureArray->GetWidth());
    const float maxFieldMipLevel = static_cast<float>(fieldTextureArray->GetMipLevelCount() - 1u);
    for (uint32_t cascadeIndex = 0; cascadeIndex < GpuOcean::CascadeCount; ++cascadeIndex) {
        const float patchSize = gpuOcean->GetCascadePatchSize(cascadeIndex);
        cascadePatchSizes[cascadeIndex] = patchSize;
        cascadeVertexMipLevels[cascadeIndex] = BE1::Clamp(BE1::Math::Log2(settings.gridSpacing * fieldTextureResolution / patchSize), 0.0f, maxFieldMipLevel);
    }

    OceanSurfaceParams params = {};
    params.cascadePatchSizes = float4(cascadePatchSizes);
    params.cascadeVertexMipLevels = float4(cascadeVertexMipLevels);
    params.gridSpacing = settings.gridSpacing;
    params.surfaceHeight = settings.height;
    params.roughness = settings.roughness;
    const BE1::Color3 linearWaterColor = settings.waterColor.SRGBToLinear();
    params.waterColor = float3(linearWaterColor.r, linearWaterColor.g, linearWaterColor.b);
    params.fresnelF0 = settings.fresnelF0;
    const BE1::Color3 linearScatterColor = settings.scatterColor.SRGBToLinear();
    params.scatterColor = float3(linearScatterColor.r, linearScatterColor.g, linearScatterColor.b);
    params.scatterHeight = BE1::Max(settings.scatterHeight, MinOceanSurfaceShaderDivisor);
    const BE1::Color3 linearFoamColor = settings.foamColor.SRGBToLinear();
    params.foamColor = float3(linearFoamColor.r, linearFoamColor.g, linearFoamColor.b);
    params.foamDetailMapSize = BE1::Max(settings.foamDetailMapSize, MinOceanSurfaceShaderDivisor);
    params.foamDetailStrength = settings.foamDetailStrength;
    params.refractionDistance = BE1::Max(settings.refractionDistance, 0.0f);
    params.absorptionDistance = BE1::Max(settings.absorptionDistance, MinOceanSurfaceShaderDivisor);
    const float sqrtF0 = BE1::Math::Sqrt(BE1::Clamp(settings.fresnelF0, 0.0f, 0.99f));
    params.airToWaterEta = (1.0f - sqrtF0) / (1.0f + sqrtF0);
    params.waterToAirEta = (1.0f + sqrtF0) / (1.0f - sqrtF0);
    // Raster pixel 좌표를 normalized UV 로 바꾸는 render 크기와 snapshot texel 크기를 분리한다.
    params.sceneSize = float2(static_cast<float>(view.renderWidth), static_cast<float>(view.renderHeight));
    params.sceneSizeRcp = float2(1.0f / params.sceneSize.x, 1.0f / params.sceneSize.y);
    params.sceneSnapshotSize = float2(static_cast<float>(boundSceneDepthTexture->GetWidth()), static_cast<float>(boundSceneDepthTexture->GetHeight()));
    params.sceneSnapshotSizeRcp = float2(1.0f / params.sceneSnapshotSize.x, 1.0f / params.sceneSnapshotSize.y);
    params.displacementScale = gpuOcean->GetDisplacementScale();
    params.cascadeFadeScale = BE1::Max(settings.cascadeFadeScale, MinOceanSurfaceShaderDivisor);
    params.gridSnapSpacing = tileBatch.gridSnapSpacing;
    params.surfaceOptions = gpuOcean->IsFoamEnabled() ? OceanSurfaceOptionBitFoamEnabled : 0u;
    if (refractionEnabled) {
        params.surfaceOptions |= OceanSurfaceOptionBitRefractionEnabled;
    }

    RHI::ConstantBuffer *surfaceConstantBuffer = frameThreadData->AllocConstant(sizeof(params), &params);
    if (!surfaceConstantBuffer) {
        return false;
    }

    RHI::renderer->SetPSO(commandList, pipelineState);
    RHI::renderer->SetIndexBuffer(commandList, gridIndexBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, frameConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 2, surfaceConstantBuffer);

    RHI::renderer->SetBuffer(commandList, 0, false, tileInstanceBuffer);
    RHI::renderer->SetTexture(commandList, 1, false, fieldTextureArray);
    RHI::renderer->SetTexture(commandList, 2, false, foamHistoryTextureArray);
    RHI::renderer->SetTexture(commandList, 3, false, foamDetailMap);

    RHI::renderer->SetTexture(commandList, 4, false, boundSceneColorTexture);
    RHI::renderer->SetTexture(commandList, 5, false, boundSceneDepthTexture);

    if (drawStats) {
        drawStats->instanceBufferAllocCount = 1;
        drawStats->instanceBufferUploadBytes = sizeof(OceanSurfaceTileInstance) * tileBatch.instanceCount;
    }

    for (uint32_t lodLevel = 0; lodLevel < GridLodCount; ++lodLevel) {
        const uint32_t instanceCount = tileBatch.lodInstanceCounts[lodLevel];
        if (instanceCount == 0) {
            continue;
        }

        const GridLodRange &lodRange = gridLodRanges[lodLevel];
        const uint32_t tileInstanceOffset = tileBatch.lodInstanceOffsets[lodLevel];
        RHI::renderer->SetConstants(commandList, 3, &tileInstanceOffset, sizeof(tileInstanceOffset), 0);
        RHI::renderer->DrawIndexedInstanced(commandList, lodRange.indexCount, instanceCount, lodRange.firstIndex, 0, 0);

        if (drawStats) {
            drawStats->drawCount++;
            drawStats->triangleCount += static_cast<uint64_t>(lodRange.indexCount / 3u) * instanceCount;
        }
    }
    return true;
}
