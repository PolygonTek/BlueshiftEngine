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

static constexpr float MinOceanSurfaceShaderDivisor = 1.0e-4f;

static_assert(GpuOcean::CascadeCount == OceanCascadeCount);

bool GpuOceanSurface::Settings::IsValid() const {
    return clipmap.IsValid() && std::isfinite(height) && std::isfinite(maxDisplacement) && maxDisplacement >= 0.0f;
}

void GpuOceanSurface::Init(GpuOcean *gpuOcean) {
    assert(gpuOcean);

    this->gpuOcean = gpuOcean;
    queryResultCache.Clear();
    queryResultCache.Init(MaxQueryCount, MaxQueryCount, MaxQueryCount);

    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel shaderModel = RHI::Renderer::MinimumShaderModel();
    constexpr bool asyncBuild = true;

    queryShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_sampleOceanSurfaceShader", shaderModel, "Ocean/SampleOceanSurface.hlsl", baseDir, "CSMain");
    queryPipeline = shaderPipelineCache.GetComputePipeline(queryShaderTemplate, {}, asyncBuild);

    shaderTemplate = shaderTemplateManager.GetShaderTemplate("Source/TestD3D12/Shaders/Ocean/OceanSurface");
    if (!shaderTemplate) {
        BE_WARNLOG("GpuOceanSurface::Init: failed to load ocean surface shader template\n");
    }

    foamDetailTexture = textureManager.GetTexture("Data/EngineTextures/water_foam.png", Texture::Flag::LinearColorSpace);
    if (foamDetailTexture == textureManager.missingTexture) {
        BE_WARNLOG("GpuOceanSurface::Init: failed to load ocean foam detail texture\n");
        foamDetailTexture.reset();
    }

    clipmap.Init();
}

void GpuOceanSurface::Shutdown() {
    DestroyQueryResources();
    queryResultCache.Clear();
    clipmap.Shutdown();

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

bool GpuOceanSurface::Draw(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const RHI::PipelineState *pipelineState,
    const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer, const Settings &settings, const View &view,
    const RHI::Texture *sceneColorTexture, const RHI::Texture *sceneDepthTexture, DrawStats *drawStats) const {
    PROFILER_CPU_SCOPED_EVENT("GpuOceanSurface::Draw");

    PreparedDraw preparedDraw;
    if (!PrepareDraw(frameThreadData, settings, view, sceneColorTexture, sceneDepthTexture, preparedDraw)) {
        return false;
    }
    return DrawPrepared(commandList, pipelineState, frameConstantBuffer, cameraConstantBuffer, preparedDraw, true, drawStats);
}

bool GpuOceanSurface::PrepareDraw(RHI::FrameThreadData *frameThreadData, const Settings &settings, const View &view,
    const RHI::Texture *sceneColorTexture, const RHI::Texture *sceneDepthTexture, PreparedDraw &preparedDraw) const {
    PROFILER_CPU_SCOPED_EVENT("GpuOceanSurface::PrepareDraw");

    preparedDraw = {};
    assert(settings.IsValid());
    if (!settings.IsValid() || !frameThreadData) {
        return false;
    }
    assert(view.renderWidth > 0u && view.renderHeight > 0u);

    if (!gpuOcean || !clipmap.IsReady() || !gpuOcean->AreCascadeOutputsReady()) {
        return false;
    }

    const RHI::Texture *fieldTextureArray = gpuOcean->GetFieldTextureArray();
    const RHI::Texture *foamHistoryTextureArray = gpuOcean->GetFoamHistoryTextureArray();
    if (!fieldTextureArray || fieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !foamHistoryTextureArray || foamHistoryTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    RHI::Texture *fallbackTexture = textureManager.whiteTexture->GetRHITexture();

    RHI::Texture *foamDetailMap = foamDetailTexture ? foamDetailTexture->GetReadyRHITexture() : nullptr;
    if (!foamDetailMap) {
        foamDetailMap = fallbackTexture;
    }

    assert((sceneColorTexture != nullptr) == (sceneDepthTexture != nullptr));
    const bool refractionEnabled = settings.refractionEnabled && sceneColorTexture && sceneDepthTexture;
    assert(!refractionEnabled || (sceneColorTexture->GetWidth() == sceneDepthTexture->GetWidth() && sceneColorTexture->GetHeight() == sceneDepthTexture->GetHeight()));
    const RHI::Texture *boundSceneColorTexture = refractionEnabled ? sceneColorTexture : fallbackTexture;
    const RHI::Texture *boundSceneDepthTexture = refractionEnabled ? sceneDepthTexture : fallbackTexture;

    GeometryClipmap::View clipmapView;
    clipmapView.origin = view.origin;
    clipmapView.frustum = view.frustum;
    clipmapView.box = view.box;
    clipmapView.infiniteZFar = view.infiniteZFar;
    clipmapView.frustumCulling = settings.frustumCulling;
    // FFT field 의 최대 변위를 CPU 에서 알 수 없으므로 설정된 보수적 최대값으로 모든 축을 확장한다.
    clipmapView.minHeight = settings.height - settings.maxDisplacement;
    clipmapView.maxHeight = settings.height + settings.maxDisplacement;
    clipmapView.maxHorizontalDisplacement = settings.maxDisplacement;
    if (!clipmap.BuildBatch(settings.clipmap, clipmapView, preparedDraw.patchBatch)) {
        return false;
    }
    if (preparedDraw.patchBatch.instanceCount == 0u) {
        preparedDraw.valid = true;
        return true;
    }

    preparedDraw.patchInstanceBuffer = frameThreadData->AllocStructuredBuffer(
        sizeof(GeometryClipmapPatch), preparedDraw.patchBatch.instanceCount, preparedDraw.patchBatch.instances);
    if (!preparedDraw.patchInstanceBuffer) {
        return false;
    }

    // 캐스케이드 별 patch size 와 기본 간격의 mip LOD. level offset 을 더한 후 sampler 가 clamp 한다.
    float cascadePatchSizes[4] = {};
    float cascadeVertexMipLevels[4] = {};
    const float fieldTextureResolution = static_cast<float>(fieldTextureArray->GetWidth());
    for (uint32_t cascadeIndex = 0; cascadeIndex < GpuOcean::CascadeCount; ++cascadeIndex) {
        const float patchSize = gpuOcean->GetCascadePatchSize(cascadeIndex);
        cascadePatchSizes[cascadeIndex] = patchSize;
        cascadeVertexMipLevels[cascadeIndex] = BE1::Math::Log2(settings.clipmap.baseSpacing * fieldTextureResolution / patchSize);
    }

    OceanSurfaceParams params = {};
    params.cascadePatchSizes = float4(cascadePatchSizes);
    params.cascadeVertexMipLevels = float4(cascadeVertexMipLevels);
    params.surfaceHeight = settings.height;
    params.roughness = settings.roughness;
    const BE1::Color3 linearWaterColor = settings.waterColor.SRGBToLinear();
    params.waterColor = float3(linearWaterColor);
    params.fresnelF0 = settings.fresnelF0;
    const BE1::Color3 linearScatterColor = settings.scatterColor.SRGBToLinear();
    params.scatterColor = float3(linearScatterColor);
    params.scatterHeight = BE1::Max(settings.scatterHeight, MinOceanSurfaceShaderDivisor);
    const BE1::Color3 linearFoamColor = settings.foamColor.SRGBToLinear();
    params.foamColor = float3(linearFoamColor);
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
    params.surfaceOptions = gpuOcean->IsFoamEnabled() ? OceanSurfaceOptionBitFoamEnabled : 0u;
    if (refractionEnabled) {
        params.surfaceOptions |= OceanSurfaceOptionBitRefractionEnabled;
    }

    preparedDraw.surfaceConstantBuffer = frameThreadData->AllocConstant(sizeof(params), &params);
    if (!preparedDraw.surfaceConstantBuffer) {
        return false;
    }

    preparedDraw.fieldTextureArray = fieldTextureArray;
    preparedDraw.foamHistoryTextureArray = foamHistoryTextureArray;
    preparedDraw.foamDetailMap = foamDetailMap;
    preparedDraw.sceneColorTexture = boundSceneColorTexture;
    preparedDraw.sceneDepthTexture = boundSceneDepthTexture;
    preparedDraw.instanceBufferUploadBytes = sizeof(GeometryClipmapPatch) * preparedDraw.patchBatch.instanceCount;
    preparedDraw.valid = true;
    return true;
}

bool GpuOceanSurface::DrawPrepared(RHI::CommandList *commandList, const RHI::PipelineState *pipelineState,
    const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer,
    const PreparedDraw &preparedDraw, bool countInstanceUpload, DrawStats *drawStats) const {
    PROFILER_CPU_SCOPED_EVENT("GpuOceanSurface::DrawPrepared");

    if (drawStats) {
        *drawStats = {};
    }
    if (!preparedDraw.valid || !pipelineState || !frameConstantBuffer || !cameraConstantBuffer) {
        return false;
    }
    if (preparedDraw.patchBatch.instanceCount == 0u) {
        return true;
    }
    if (!preparedDraw.patchInstanceBuffer || !preparedDraw.surfaceConstantBuffer || !preparedDraw.fieldTextureArray ||
        !preparedDraw.foamHistoryTextureArray || !preparedDraw.foamDetailMap || !preparedDraw.sceneColorTexture || !preparedDraw.sceneDepthTexture) {
        return false;
    }

    RHI::renderer->SetPSO(commandList, pipelineState);
    RHI::renderer->SetIndexBuffer(commandList, clipmap.GetIndexBuffer());
    RHI::renderer->SetConstantBuffer(commandList, 0, frameConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 2, preparedDraw.surfaceConstantBuffer);

    RHI::renderer->SetBuffer(commandList, 0, false, preparedDraw.patchInstanceBuffer);
    RHI::renderer->SetTexture(commandList, 1, false, preparedDraw.fieldTextureArray);
    RHI::renderer->SetTexture(commandList, 2, false, preparedDraw.foamHistoryTextureArray);
    RHI::renderer->SetTexture(commandList, 3, false, preparedDraw.foamDetailMap);

    RHI::renderer->SetTexture(commandList, 4, false, preparedDraw.sceneColorTexture);
    RHI::renderer->SetTexture(commandList, 5, false, preparedDraw.sceneDepthTexture);

    if (drawStats && countInstanceUpload) {
        drawStats->instanceBufferAllocCount = 1;
        drawStats->instanceBufferUploadBytes = preparedDraw.instanceBufferUploadBytes;
    }

    for (const GeometryClipmap::DrawRange &draw : preparedDraw.patchBatch.draws) {
        if (draw.instanceCount == 0) {
            continue;
        }
        RHI::renderer->SetConstants(commandList, 3, &draw.instanceOffset, sizeof(draw.instanceOffset), 0);
        RHI::renderer->DrawIndexedInstanced(commandList, draw.indexCount, draw.instanceCount, draw.firstIndex, 0, 0);
        if (drawStats) {
            drawStats->drawCount++;
        }
    }
    if (drawStats) {
        drawStats->triangleCount = preparedDraw.patchBatch.triangleCount;
    }
    return true;
}
