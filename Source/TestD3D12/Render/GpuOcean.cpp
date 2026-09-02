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
#include "GpuOcean.h"
#include "ShaderPipelineBootstrap.h"
#include "RenderSystem.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/Texture/Texture.h"
#include "Render/Texture/TextureManager.h"
#include "Shaders/ShaderInterop.h"

static_assert(GpuOcean::CascadeCount == OceanCascadeCount);

void GpuOcean::Init() {
    simulationTime = 0.0f;
    ResetCascadeUpdateSchedule();
    foamHistoryMayContainFoamMask = 0u;
    for (bool &valid : surfaceQueryResultValid) {
        valid = false;
    }

    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel shaderModel = RHI::Renderer::MinimumShaderModel();
    constexpr bool asyncPipelineBuild = true;

    surfaceShaderTemplate = shaderTemplateManager.GetShaderTemplate("Source/TestD3D12/Shaders/Water/OceanSurface");
    if (!surfaceShaderTemplate) {
        BE_WARNLOG("GpuOcean::Init: failed to load ocean surface shader template\n");
    }

    foamDetailTexture = textureManager.GetTexture("Data/EngineTextures/water_form.PNG", Texture::Flag::LinearColorSpace);
    if (foamDetailTexture == textureManager.defaultTexture) {
        BE_WARNLOG("GpuOcean::Init: failed to load ocean foam detail texture\n");
        foamDetailTexture.reset();
    }

    CreateSurfaceGrid();

    generatePhillipsInitialSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generatePhillipsInitialOceanSpectrumShader", shaderModel, "Water/GenerateInitialSpectrum.hlsl", baseDir, "CSMain", "OCEAN_SPECTRUM_MODEL=0");
    generateTMAPositiveCosineSquaredInitialSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generateTMAPositiveCosineSquaredInitialOceanSpectrumShader", shaderModel, "Water/GenerateInitialSpectrum.hlsl", baseDir, "CSMain", "OCEAN_SPECTRUM_MODEL=1, OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL=0");
    generateTMAHasselmannInitialSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generateTMAHasselmannInitialOceanSpectrumShader", shaderModel, "Water/GenerateInitialSpectrum.hlsl", baseDir, "CSMain", "OCEAN_SPECTRUM_MODEL=1, OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL=1");
    generateTMADonelanBannerInitialSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generateTMADonelanBannerInitialOceanSpectrumShader", shaderModel, "Water/GenerateInitialSpectrum.hlsl", baseDir, "CSMain", "OCEAN_SPECTRUM_MODEL=1, OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL=2");
    generateTMAMitsuyasuInitialSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generateTMAMitsuyasuInitialOceanSpectrumShader", shaderModel, "Water/GenerateInitialSpectrum.hlsl", baseDir, "CSMain", "OCEAN_SPECTRUM_MODEL=1, OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL=3");
    generateTimeSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generateOceanTimeSpectrumShader", shaderModel, "Water/GenerateTimeSpectrum.hlsl", baseDir, "CSMain");
    resolveOceanFieldsShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_resolveOceanFieldsShader", shaderModel, "Water/ResolveOceanFields.hlsl", baseDir, "CSMain");
    sampleOceanSurfaceShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_sampleOceanSurfaceShader", shaderModel, "Water/SampleOceanSurface.hlsl", baseDir, "CSMain");

    generatePhillipsInitialSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generatePhillipsInitialSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    generateTMAPositiveCosineSquaredInitialSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generateTMAPositiveCosineSquaredInitialSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    generateTMAHasselmannInitialSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generateTMAHasselmannInitialSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    generateTMADonelanBannerInitialSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generateTMADonelanBannerInitialSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    generateTMAMitsuyasuInitialSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generateTMAMitsuyasuInitialSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    generateTimeSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generateTimeSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    resolveOceanFieldsPipeline = shaderPipelineCache.GetComputePipeline(resolveOceanFieldsShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    sampleOceanSurfacePipeline = shaderPipelineCache.GetComputePipeline(sampleOceanSurfaceShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
}

void GpuOcean::Shutdown() {
    DestroySurfaceGrid();
    DestroySurfaceQueryResources();
    DestroyResources();

    simulationTime = 0.0f;
    ResetCascadeUpdateSchedule();
    cascadeResourcesDirty = true;
    initialSpectraReady = false;
    cascadeOutputsReady = false;
    foamHistoryResetPending = true;

    generatePhillipsInitialSpectrumPipeline.reset();
    generateTMAPositiveCosineSquaredInitialSpectrumPipeline.reset();
    generateTMAHasselmannInitialSpectrumPipeline.reset();
    generateTMADonelanBannerInitialSpectrumPipeline.reset();
    generateTMAMitsuyasuInitialSpectrumPipeline.reset();
    generateTimeSpectrumPipeline.reset();
    resolveOceanFieldsPipeline.reset();
    sampleOceanSurfacePipeline.reset();

    generatePhillipsInitialSpectrumShaderTemplate.reset();
    generateTMAPositiveCosineSquaredInitialSpectrumShaderTemplate.reset();
    generateTMAHasselmannInitialSpectrumShaderTemplate.reset();
    generateTMADonelanBannerInitialSpectrumShaderTemplate.reset();
    generateTMAMitsuyasuInitialSpectrumShaderTemplate.reset();
    generateTimeSpectrumShaderTemplate.reset();
    resolveOceanFieldsShaderTemplate.reset();
    sampleOceanSurfaceShaderTemplate.reset();
    surfaceShaderTemplate.reset();
    foamDetailTexture.reset();
}

void GpuOcean::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "GpuOcean";

    bootstrap.TrackPipeline(groupName, generatePhillipsInitialSpectrumPipeline);
    bootstrap.TrackPipeline(groupName, generateTMAPositiveCosineSquaredInitialSpectrumPipeline);
    bootstrap.TrackPipeline(groupName, generateTMAHasselmannInitialSpectrumPipeline);
    bootstrap.TrackPipeline(groupName, generateTMADonelanBannerInitialSpectrumPipeline);
    bootstrap.TrackPipeline(groupName, generateTMAMitsuyasuInitialSpectrumPipeline);
    bootstrap.TrackPipeline(groupName, generateTimeSpectrumPipeline);
    bootstrap.TrackPipeline(groupName, resolveOceanFieldsPipeline);
    bootstrap.TrackPipeline(groupName, sampleOceanSurfacePipeline);
}

static void CalculateJONSWAPFetchParameters(float windSpeed, float gravity, float fetch, float &alpha, float &peakOmega) {
    // Hasselmann et al. 의 fetch-limited JONSWAP 관계식. windSpeed 는 해수면 위 10 m 풍속 U10 이다.
    const float dimensionlessFetch = gravity * fetch / (windSpeed * windSpeed);
    alpha = 0.076f * BE1::Math::Pow(dimensionlessFetch, -0.22f);
    peakOmega = BE1::Math::TwoPi * 3.5f * gravity / windSpeed * BE1::Math::Pow(dimensionlessFetch, -0.33f);
}

void GpuOcean::ApplySimulationSettings(const SimulationSettings &newSettings, uint64_t revision) {
    assert(newSettings.IsValid());

    if (revision <= appliedSimulationSettingsRevision) {
        return;
    }

    const SimulationSettings::ChangeFlag changes = simulationSettings.ClassifyChanges(newSettings);

    if (BE1::HasFlag(changes, SimulationSettings::ChangeFlag::RecreateResources)) {
        cascadeResourcesDirty = true;
    }

    const bool regenInitialSpectrum = BE1::HasFlag(changes, SimulationSettings::ChangeFlag::RegenInitialSpectrum);
    if (regenInitialSpectrum) {
        initialSpectraReady = false;
    }

    if (regenInitialSpectrum || BE1::HasFlag(changes, SimulationSettings::ChangeFlag::RefreshCascadeOutputs)) {
        cascadeOutputsReady = false;
        ResetCascadeUpdateSchedule();
    }

    if (BE1::HasFlag(changes, SimulationSettings::ChangeFlag::ResetFoamHistory)) {
        foamHistoryResetPending = true;
    }

    simulationSettings = newSettings;
    appliedSimulationSettingsRevision = revision;
}

bool GpuOcean::Update(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, float deltaTime) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::Update");

    if (deltaTime < 0.0f) {
        return false;
    }

    // 초기 스펙트럼 계수를 생성한다.
    if (!initialSpectraReady) {
        switch (simulationSettings.spectrumModel) {
        case SpectrumModel::Phillips: {
            OceanPhillipsInitialSpectrumParams params = {};
            params.spectrumScale = simulationSettings.phillips.spectrumScale;
            params.windDirection = float2(simulationSettings.windDirection.x, simulationSettings.windDirection.y);
            params.windSpeed = simulationSettings.windSpeed;
            params.gravity = simulationSettings.gravity;
            params.smallWaveDamping = simulationSettings.smallWaveDamping;
            params.oppositeWaveDamping = simulationSettings.phillips.oppositeWaveDamping;
            if (!GenerateInitialSpectraPhillips(commandList, frameThreadData, params)) {
                return false;
            }
            break;
        }
        case SpectrumModel::TMA: {
            OceanTMAInitialSpectrumParams params = {};
            params.spectrumScale = simulationSettings.tma.spectrumScale;
            params.windDirection = float2(simulationSettings.windDirection.x, simulationSettings.windDirection.y);
            params.windSpeed = simulationSettings.windSpeed;
            params.gravity = simulationSettings.gravity;
            params.smallWaveDamping = simulationSettings.smallWaveDamping;
            CalculateJONSWAPFetchParameters(simulationSettings.windSpeed, simulationSettings.gravity, simulationSettings.tma.fetch, params.alpha, params.peakOmega);
            params.depth = simulationSettings.depth;
            params.swell = simulationSettings.tma.swell;
            params.spread = simulationSettings.tma.spread;
            params.gamma = simulationSettings.tma.peakEnhancement;
            if (!GenerateInitialSpectraTMA(commandList, frameThreadData, params)) {
                return false;
            }
            break;
        }
        }
    }

    const float simulationDeltaTime = deltaTime * simulationSettings.timeScale;
    const float nextSimulationTime = simulationTime + simulationDeltaTime;

    // 각 cascade 별로 초기 스펙트럼을 시간 진화시켜서 시간 스펙트럼을 생성하고,
    // 공간 필드를 계산하여 displacement 및 derivative 텍스처를 생성한다.
    OceanTimeSpectrumParams params = {};
    params.time = nextSimulationTime;
    params.gravity = simulationSettings.gravity;
    params.depth = simulationSettings.depth;
    params.choppiness = simulationSettings.choppiness;
    params.options = simulationSettings.crossDerivativeEnabled ? OceanTimeSpectrumOptionBitCrossDerivative : 0u;

    OceanResolveFieldsParams resolveParams = {};
    resolveParams.deltaTime = simulationDeltaTime;
    resolveParams.displacementScale = simulationSettings.displacementScale;
    resolveParams.foamOptions = simulationSettings.foamEnabled ? OceanFoamOptionBitEnabled : 0u;
    if (foamHistoryResetPending) {
        resolveParams.foamOptions |= OceanFoamOptionBitResetHistory;
    }

    if (!UpdateCascades(commandList, frameThreadData, params, resolveParams)) {
        return false;
    }

    simulationTime = nextSimulationTime;
    return true;
}

GpuOcean::SurfaceQueryReadbackFrame *GpuOcean::DispatchSurfaceQueries(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanSurfaceQueryInput *queries, uint32_t queryCount, float surfaceHeight) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::DispatchSurfaceQueries");

    if (!queries || queryCount == 0 || queryCount > MaxSurfaceQueryCount) {
        return nullptr;
    }

    if (!cascadeOutputsReady || !fieldTextureArray || fieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return nullptr;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(sampleOceanSurfacePipeline);
    if (!pso || !EnsureSurfaceQueryResources()) {
        return nullptr;
    }

    // 이번 query dispatch 결과를 받을 유휴 readback frame 을 찾는다.
    SurfaceQueryReadbackFrame *readbackFrame = nullptr;
    uint32_t readbackFrameIndex = surfaceQueryReadbackFrameIndex;
    for (uint32_t attempt = 0; attempt < COUNT_OF(surfaceQueryReadbackFrames); ++attempt) {
        SurfaceQueryReadbackFrame &candidate = surfaceQueryReadbackFrames[readbackFrameIndex];
        if (candidate.buffer && candidate.state.load() == SurfaceQueryReadbackFrame::State::Idle) {
            readbackFrame = &candidate;
            break;
        }
        readbackFrameIndex = (readbackFrameIndex + 1u) % COUNT_OF(surfaceQueryReadbackFrames);
    }

    if (!readbackFrame) {
        return nullptr;
    }

    RHI::Buffer *queryInputBuffer = frameThreadData->AllocStructuredBuffer(sizeof(OceanSurfaceQueryInput), queryCount, queries);
    if (!queryInputBuffer) {
        return nullptr;
    }

    float cascadePatchSizes[4] = {};
    static_assert(CascadeCount <= COUNT_OF(cascadePatchSizes));
    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        cascadePatchSizes[cascadeIndex] = simulationSettings.cascades[cascadeIndex].patchSize;
    }

    OceanSurfaceQueryParams params = {};
    params.cascadePatchSizes = float4(cascadePatchSizes);
    params.surfaceHeight = surfaceHeight;
    params.displacementScale = simulationSettings.displacementScale;
    params.sampleTime = simulationTime;
    params.queryCount = queryCount;

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(params), &params);
    if (!constantBuffer) {
        return nullptr;
    }

    readbackFrame->queryCount = queryCount;

    // fence 값이 등록되기 전까지 다른 dispatch 가 이 readback frame 을 재사용하지 못하게 한다.
    readbackFrame->state.store(SurfaceQueryReadbackFrame::State::Resolving);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(surfaceQueryOutputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, fieldTextureArray);
    RHI::renderer->SetBuffer(commandList, 1, false, queryInputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, surfaceQueryOutputBuffer, surfaceQueryOutputUAV);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);
    RHI::renderer->Dispatch(commandList, BE1::DivCeil(queryCount, OceanSurfaceQueryBlockSize), 1, 1);

    // compute shader 가 생성한 결과를 CPU readback buffer 로 복사한다.
    const uint32_t resultSize = queryCount * sizeof(OceanSurfaceQueryResult);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(surfaceQueryOutputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::CopySrc));
    RHI::renderer->CopyBuffer(commandList, readbackFrame->buffer, 0, surfaceQueryOutputBuffer, 0, resultSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(surfaceQueryOutputBuffer, RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::Undefined));

    surfaceQueryReadbackFrameIndex = (readbackFrameIndex + 1u) % COUNT_OF(surfaceQueryReadbackFrames);
    return readbackFrame;
}

void GpuOcean::ConsumeCompletedSurfaceQueries() {
    assert(BE1::Engine::IsInMainThread());

    for (SurfaceQueryReadbackFrame &readbackFrame : surfaceQueryReadbackFrames) {
        if (readbackFrame.state.load() != SurfaceQueryReadbackFrame::State::Submitted) {
            continue;
        }

        const uint64_t fenceValue = readbackFrame.fenceValue.load();
        if (!RHI::renderer->IsFenceComplete(RHI::CommandQueueType::Graphics, fenceValue)) {
            continue;
        }

        const OceanSurfaceQueryResult *results = static_cast<const OceanSurfaceQueryResult *>(readbackFrame.buffer->mappedPtr);
        for (uint32_t resultIndex = 0; resultIndex < readbackFrame.queryCount; ++resultIndex) {
            const OceanSurfaceQueryResult &result = results[resultIndex];
            if (result.queryId < MaxSurfaceQueryCount) {
                surfaceQueryResultCache[result.queryId] = result;
                surfaceQueryResultValid[result.queryId] = true;
            }
        }

        // readback frame 을 재사용할 수 있도록 상태를 초기화한다.
        readbackFrame.queryCount = 0;
        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(SurfaceQueryReadbackFrame::State::Idle);
    }
}

bool GpuOcean::GetSurfaceQueryResult(uint32_t queryId, OceanSurfaceQueryResult &outResult) const {
    assert(BE1::Engine::IsInMainThread());

    if (queryId >= MaxSurfaceQueryCount || !surfaceQueryResultValid[queryId]) {
        return false;
    }

    outResult = surfaceQueryResultCache[queryId];
    return true;
}

bool GpuOcean::HasResources() const {
    return timeSpectrumBuffer && ifftWorkBuffer && spatialFieldsBuffer && initialSpectrumTextureArray && fieldTextureArray && foamHistoryTextureArray;
}

static RHI::Buffer *CreateOceanStructuredBuffer(const char *debugName, uint64_t elementCount) {
    const uint64_t bufferSize = elementCount * sizeof(float2);
    RHI::Buffer *buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, bufferSize, nullptr);
    if (!buffer) {
        BE_WARNLOG("GpuOcean::EnsureResources: failed to create %s\n", debugName);
        return nullptr;
    }

    RHI::renderer->SetName(buffer, debugName);

    if (!RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize).IsValid()) {
        BE_WARNLOG("GpuOcean::EnsureResources: failed to create %s SRV\n", debugName);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    if (!RHI::renderer->CreateView(buffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize).IsValid()) {
        BE_WARNLOG("GpuOcean::EnsureResources: failed to create %s UAV\n", debugName);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    return buffer;
}

static RHI::Texture *CreateOceanTexture(const char *debugName, uint32_t size, BE1::Image::Format format, uint32_t arraySize, RHI::TextureMipInitMode mipInitMode, RHI::GPUResourceState initialState) {
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->InitFromMemory(size, size, 1, arraySize, 1, format, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    const RHI::TextureType textureType = arraySize > 1 ? RHI::TextureType::Texture2DArray : RHI::TextureType::Texture2D;
    RHI::Texture *texture = RHI::renderer->CreateTexture(textureType, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, mipInitMode, {}, 1, initialState);
    if (!texture) {
        BE_WARNLOG("GpuOcean::EnsureResources: failed to create %s\n", debugName);
        return nullptr;
    }

    RHI::renderer->SetName(texture, debugName);
    return texture;
}

static bool CreateOceanMipViews(const char *debugName, RHI::Texture *texture, BE1::Array<RHI::ViewID> &outMipSRVs, BE1::Array<RHI::ViewID> &outMipUAVs) {
    const uint32_t mipLevelCount = texture->GetMipLevelCount();
    outMipSRVs.SetCount(mipLevelCount);
    outMipUAVs.SetCount(mipLevelCount);

    for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel) {
        outMipSRVs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::SRV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
        outMipUAVs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::UAV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
        if (!outMipSRVs[mipLevel].IsValid() || !outMipUAVs[mipLevel].IsValid()) {
            BE_WARNLOG("GpuOcean::EnsureResources: failed to create %s mip %u views\n", debugName, mipLevel);
            return false;
        }
    }
    return true;
}

bool GpuOcean::EnsureResources() {
    if (!cascadeResourcesDirty && HasResources()) {
        return true;
    }

    DestroyResources();
    cascadeResourcesDirty = true;

    const uint32_t spectrumSize = GpuFFT::GetGpuFFTSizeValue(simulationSettings.fftSize);
    if (spectrumSize == 0) {
        return false;
    }

    const uint64_t complexElementCount = uint64_t(spectrumSize) * uint64_t(spectrumSize) * OceanSpectrumComplexPlaneCount;
    timeSpectrumBuffer = CreateOceanStructuredBuffer("GpuOcean::timeSpectrumBuffer", complexElementCount);
    ifftWorkBuffer = CreateOceanStructuredBuffer("GpuOcean::ifftWorkBuffer", complexElementCount);
    spatialFieldsBuffer = CreateOceanStructuredBuffer("GpuOcean::spatialFieldsBuffer", complexElementCount);
    if (!timeSpectrumBuffer || !ifftWorkBuffer || !spatialFieldsBuffer) {
        DestroyResources();
        return false;
    }

    const BE1::Image::Format initialSpectrumFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    const BE1::Image::Format outputFieldFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    const BE1::Image::Format foamHistoryFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16_FLOAT);

    // 각 cascade 의 initial spectrum 을 동일한 index 의 slice 에 저장하며 graphics/compute SRV 상태로 유지한다.
    initialSpectrumTextureArray = CreateOceanTexture("GpuOcean::initialSpectrumTextureArray", spectrumSize, initialSpectrumFormat, CascadeCount, RHI::TextureMipInitMode::FromImage, RHI::GPUResourceState::ShaderResource);
    if (!initialSpectrumTextureArray) {
        DestroyResources();
        return false;
    }

    // 각 cascade 의 displacement 와 derivative 를 연속한 두 slice 에 저장한다.
    fieldTextureArray = CreateOceanTexture("GpuOcean::fieldTextureArray", spectrumSize, outputFieldFormat, FieldTextureSliceCount, RHI::TextureMipInitMode::AllocateEmpty, RHI::GPUResourceState::ShaderResource);
    if (!fieldTextureArray) {
        DestroyResources();
        return false;
    }

    // 각 cascade 의 foam history 를 동일한 index 의 slice 에 저장한다.
    foamHistoryTextureArray = CreateOceanTexture("GpuOcean::foamHistoryTextureArray", spectrumSize, foamHistoryFormat, CascadeCount, RHI::TextureMipInitMode::AllocateEmpty, RHI::GPUResourceState::ShaderResource);
    if (!foamHistoryTextureArray) {
        DestroyResources();
        return false;
    }

    if (!CreateOceanMipViews("fieldTextureArray", fieldTextureArray, fieldMipSRVs, fieldMipUAVs) ||
        !CreateOceanMipViews("foamHistoryTextureArray", foamHistoryTextureArray, foamMipSRVs, foamMipUAVs)) {
        DestroyResources();
        return false;
    }

    cascadeResourcesDirty = false;
    foamHistoryResetPending = true;
    return true;
}

void GpuOcean::DestroyResources() {
    if (initialSpectrumTextureArray) {
        RHI::renderer->DestroyTexture(initialSpectrumTextureArray);
        initialSpectrumTextureArray = nullptr;
    }
    if (fieldTextureArray) {
        RHI::renderer->DestroyTexture(fieldTextureArray);
        fieldTextureArray = nullptr;
    }
    if (foamHistoryTextureArray) {
        RHI::renderer->DestroyTexture(foamHistoryTextureArray);
        foamHistoryTextureArray = nullptr;
    }

    fieldMipSRVs.Clear();
    fieldMipUAVs.Clear();
    foamMipSRVs.Clear();
    foamMipUAVs.Clear();

    if (timeSpectrumBuffer) {
        RHI::renderer->DestroyBuffer(timeSpectrumBuffer);
        timeSpectrumBuffer = nullptr;
    }
    if (ifftWorkBuffer) {
        RHI::renderer->DestroyBuffer(ifftWorkBuffer);
        ifftWorkBuffer = nullptr;
    }
    if (spatialFieldsBuffer) {
        RHI::renderer->DestroyBuffer(spatialFieldsBuffer);
        spatialFieldsBuffer = nullptr;
    }

    initialSpectraReady = false;
    cascadeOutputsReady = false;
    foamHistoryResetPending = true;
    foamHistoryMayContainFoamMask = 0u;
    ResetCascadeUpdateSchedule();
}

bool GpuOcean::EnsureSurfaceQueryResources() {
    if (surfaceQueryOutputBuffer) {
        return true;
    }

    DestroySurfaceQueryResources();

    const uint32_t bufferSize = MaxSurfaceQueryCount * sizeof(OceanSurfaceQueryResult);
    surfaceQueryOutputBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::UnorderedAccess, bufferSize, nullptr);
    if (!surfaceQueryOutputBuffer) {
        BE_WARNLOG("GpuOcean::EnsureSurfaceQueryResources: failed to create output buffer\n");
        return false;
    }
    RHI::renderer->SetName(surfaceQueryOutputBuffer, "GpuOcean::surfaceQueryOutputBuffer");

    surfaceQueryOutputUAV = RHI::renderer->CreateView(surfaceQueryOutputBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(OceanSurfaceQueryResult), 0, bufferSize);
    if (!surfaceQueryOutputUAV.IsValid()) {
        BE_WARNLOG("GpuOcean::EnsureSurfaceQueryResources: failed to create output UAV\n");
        DestroySurfaceQueryResources();
        return false;
    }

    for (int i = 0; i < COUNT_OF(surfaceQueryReadbackFrames); ++i) {
        SurfaceQueryReadbackFrame &readbackFrame = surfaceQueryReadbackFrames[i];
        readbackFrame.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Readback, RHI::BufferFlag::None, bufferSize, nullptr);
        if (!readbackFrame.buffer) {
            BE_WARNLOG("GpuOcean::EnsureSurfaceQueryResources: failed to create readback buffer %i\n", i);
            DestroySurfaceQueryResources();
            return false;
        }

        RHI::renderer->SetName(readbackFrame.buffer, BE1::va("GpuOcean::surfaceQueryReadbackFrames[%i].buffer", i));
        readbackFrame.queryCount = 0;
        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(SurfaceQueryReadbackFrame::State::Idle);
    }

    surfaceQueryReadbackFrameIndex = 0;
    return true;
}

void GpuOcean::DestroySurfaceQueryResources() {
    for (SurfaceQueryReadbackFrame &readbackFrame : surfaceQueryReadbackFrames) {
        if (readbackFrame.buffer) {
            RHI::renderer->DestroyBuffer(readbackFrame.buffer);
            readbackFrame.buffer = nullptr;
        }
        readbackFrame.queryCount = 0;
        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(SurfaceQueryReadbackFrame::State::Idle);
    }

    if (surfaceQueryOutputBuffer) {
        RHI::renderer->DestroyBuffer(surfaceQueryOutputBuffer);
        surfaceQueryOutputBuffer = nullptr;
    }
    surfaceQueryOutputUAV = RHI::ViewID::InvalidID;
    surfaceQueryReadbackFrameIndex = 0;
}

void GpuOcean::ResetCascadeUpdateSchedule() {
    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        cascadeLastUpdateTimes[cascadeIndex] = 0.0f;
        cascadeNextUpdateTimes[cascadeIndex] = 0.0f;
        cascadeScheduledUpdateRates[cascadeIndex] = 0.0f;
    }
    cascadeUpdateInitializedMask = 0u;
}

static bool NormalizeWindDirection(float2 &windDirection) {
    const float lengthSquared = windDirection.x * windDirection.x + windDirection.y * windDirection.y;
    if (lengthSquared <= 0.0f) {
        return false;
    }

    const float inverseLength = BE1::Math::InvSqrt(lengthSquared);
    windDirection.x *= inverseLength;
    windDirection.y *= inverseLength;
    return true;
}

static void CopyInitialSpectrumCascadeParams(const GpuOcean::CascadeDesc (&cascadeDescs)[GpuOcean::CascadeCount], OceanInitialSpectrumCascadeParams (&cascadeParams)[OceanCascadeCount]) {
    static_assert(GpuOcean::CascadeCount == OceanCascadeCount);

    for (uint32_t cascadeIndex = 0; cascadeIndex < GpuOcean::CascadeCount; ++cascadeIndex) {
        const GpuOcean::CascadeDesc &cascadeDesc = cascadeDescs[cascadeIndex];
        OceanInitialSpectrumCascadeParams &params = cascadeParams[cascadeIndex];

        params.randomSeed = cascadeDesc.randomSeed;
        params.patchSize = cascadeDesc.patchSize;
        params.minWaveNumber = cascadeDesc.minWaveNumber;
        params.maxWaveNumber = cascadeDesc.maxWaveNumber;
    }
}

template <typename Params>
static bool DispatchGenerateInitialSpectra(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const SharedPtr<ShaderPipeline> &pipeline, const Params &params, RHI::Texture *outputTextureArray, RHI::ViewID outputUAV) {
    static_assert(GpuOcean::CascadeCount == OceanCascadeCount);
    static_assert((sizeof(Params) & 15u) == 0, "Initial spectrum parameters must match the HLSL constant-buffer layout");

    if (!outputTextureArray || outputTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready || !outputUAV.IsValid()) {
        return false;
    }

    const uint32_t spectrumWidth = outputTextureArray->GetWidth();
    const uint32_t spectrumHeight = outputTextureArray->GetHeight();
    if (spectrumWidth == 0 || spectrumWidth != spectrumHeight || (spectrumWidth & 1u) != 0) {
        return false;
    }

    RHI::TextureViewInfo outputViewInfo;
    if (!outputTextureArray->GetViewInfo(RHI::ViewType::UAV, outputUAV, outputViewInfo) ||
        outputViewInfo.firstSlice != 0 || outputViewInfo.sliceCount != OceanCascadeCount || outputViewInfo.firstMipLevel != 0) {
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(pipeline);
    if (!pso) {
        return false;
    }

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(params), &params);
    if (!constantBuffer) {
        return false;
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(outputTextureArray, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, true, outputTextureArray, outputUAV);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(spectrumWidth, OceanInitialSpectrumBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(spectrumHeight, OceanInitialSpectrumBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, GpuOcean::CascadeCount);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(outputTextureArray, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource));
    return true;
}

bool GpuOcean::GenerateInitialSpectraPhillips(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanPhillipsInitialSpectrumParams &commonParams) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::GenerateInitialSpectraPhillips");

    if (!EnsureResources() || initialSpectrumTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    initialSpectraReady = false;
    cascadeOutputsReady = false;

    OceanPhillipsInitialSpectrumParams params = commonParams;
    if (!NormalizeWindDirection(params.windDirection)) {
        return false;
    }

    CopyInitialSpectrumCascadeParams(simulationSettings.cascades, params.cascadeParams);

    if (!DispatchGenerateInitialSpectra(commandList, frameThreadData, generatePhillipsInitialSpectrumPipeline, params, initialSpectrumTextureArray, RHI::ViewID::PrimaryID)) {
        return false;
    }

    initialSpectraReady = true;
    return true;
}

bool GpuOcean::GenerateInitialSpectraTMA(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTMAInitialSpectrumParams &commonParams) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::GenerateInitialSpectraTMA");

    if (!EnsureResources() || initialSpectrumTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    initialSpectraReady = false;
    cascadeOutputsReady = false;

    OceanTMAInitialSpectrumParams params = commonParams;
    if (!NormalizeWindDirection(params.windDirection)) {
        return false;
    }

    CopyInitialSpectrumCascadeParams(simulationSettings.cascades, params.cascadeParams);

    const SharedPtr<ShaderPipeline> *pipeline;
    switch (simulationSettings.tma.directionalSpreadModel) {
    case DirectionalSpreadModel::PositiveCosineSquared:
        pipeline = &generateTMAPositiveCosineSquaredInitialSpectrumPipeline;
        break;
    case DirectionalSpreadModel::Hasselmann:
        pipeline = &generateTMAHasselmannInitialSpectrumPipeline;
        break;
    case DirectionalSpreadModel::DonelanBanner:
        pipeline = &generateTMADonelanBannerInitialSpectrumPipeline;
        break;
    case DirectionalSpreadModel::Mitsuyasu:
        pipeline = &generateTMAMitsuyasuInitialSpectrumPipeline;
        break;
    default:
        return false;
    }

    if (!DispatchGenerateInitialSpectra(commandList, frameThreadData, *pipeline, params, initialSpectrumTextureArray, RHI::ViewID::PrimaryID)) {
        return false;
    }

    initialSpectraReady = true;
    return true;
}

bool GpuOcean::GenerateTimeSpectrum(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTimeSpectrumParams &params) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::GenerateTimeSpectrum");

    if (!initialSpectrumTextureArray || initialSpectrumTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !timeSpectrumBuffer || timeSpectrumBuffer->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    const uint32_t spectrumWidth = initialSpectrumTextureArray->GetWidth();
    const uint32_t spectrumHeight = initialSpectrumTextureArray->GetHeight();
    if (spectrumWidth == 0 || spectrumWidth != spectrumHeight || (spectrumWidth & 1u) != 0) {
        return false;
    }

    RHI::TextureViewInfo initialSpectrumViewInfo;
    if (!initialSpectrumTextureArray->GetViewInfo(RHI::ViewType::SRV, RHI::ViewID::PrimaryID, initialSpectrumViewInfo) ||
        initialSpectrumViewInfo.firstSlice != 0 || initialSpectrumViewInfo.sliceCount != CascadeCount || initialSpectrumViewInfo.firstMipLevel != 0) {
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(generateTimeSpectrumPipeline);
    if (!pso) {
        return false;
    }

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(params), &params);
    if (!constantBuffer) {
        return false;
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(timeSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, initialSpectrumTextureArray);
    RHI::renderer->SetBuffer(commandList, 0, true, timeSpectrumBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(spectrumWidth >> 1u, OceanTimeSpectrumBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(spectrumHeight, OceanTimeSpectrumBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(timeSpectrumBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));
    return true;
}

bool GpuOcean::ResolveOceanFields(RHI::CommandList *commandList, const RHI::ConstantBuffer *resolveConstantBuffer) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::ResolveOceanFields");

    if (!resolveConstantBuffer ||
        !spatialFieldsBuffer || spatialFieldsBuffer->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !fieldTextureArray || fieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !foamHistoryTextureArray || foamHistoryTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    const uint32_t fieldWidth = fieldTextureArray->GetWidth();
    const uint32_t fieldHeight = fieldTextureArray->GetHeight();
    if (fieldWidth == 0 || fieldWidth != fieldHeight || foamHistoryTextureArray->GetWidth() != fieldWidth || foamHistoryTextureArray->GetHeight() != fieldHeight) {
        return false;
    }

    RHI::TextureViewInfo outputFieldViewInfo;
    if (!fieldTextureArray->GetViewInfo(RHI::ViewType::UAV, RHI::ViewID::PrimaryID, outputFieldViewInfo) ||
        outputFieldViewInfo.firstSlice != 0 || outputFieldViewInfo.sliceCount != FieldTextureSliceCount || outputFieldViewInfo.firstMipLevel != 0) {
        return false;
    }

    RHI::TextureViewInfo foamHistoryViewInfo;
    if (!foamHistoryTextureArray->GetViewInfo(RHI::ViewType::UAV, RHI::ViewID::PrimaryID, foamHistoryViewInfo) ||
        foamHistoryViewInfo.firstSlice != 0 || foamHistoryViewInfo.sliceCount != CascadeCount || foamHistoryViewInfo.firstMipLevel != 0) {
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(resolveOceanFieldsPipeline);
    if (!pso) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialFieldsBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(fieldTextureArray, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeTextureBarrier(foamHistoryTextureArray, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, spatialFieldsBuffer);
    RHI::renderer->SetTexture(commandList, 0, true, fieldTextureArray);
    RHI::renderer->SetTexture(commandList, 1, true, foamHistoryTextureArray);
    RHI::renderer->SetConstantBuffer(commandList, 0, resolveConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(fieldWidth, OceanResolveFieldsBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(fieldHeight, OceanResolveFieldsBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialFieldsBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeTextureBarrier(fieldTextureArray, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(foamHistoryTextureArray, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}

static void GenerateOceanCascadeMipRanges(RHI::CommandList *commandList, RHI::Texture *texture, const RHI::ViewID *mipSRVs, const RHI::ViewID *mipUAVs, uint32_t cascadeMask, uint32_t slicesPerCascade) {
    uint32_t cascadeIndex = 0;
    while (cascadeIndex < GpuOcean::CascadeCount) {
        if ((cascadeMask & (1u << cascadeIndex)) == 0u) {
            ++cascadeIndex;
            continue;
        }

        const uint32_t firstCascade = cascadeIndex;
        do {
            ++cascadeIndex;
        } while (cascadeIndex < GpuOcean::CascadeCount && (cascadeMask & (1u << cascadeIndex)) != 0u);

        const uint32_t cascadeRangeCount = cascadeIndex - firstCascade;
        RHI::renderer->GenerateMipChain(commandList, texture, mipSRVs, mipUAVs, false, slicesPerCascade * firstCascade, slicesPerCascade * cascadeRangeCount);
    }
}

bool GpuOcean::UpdateCascades(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTimeSpectrumParams &baseParams, const OceanResolveFieldsParams &resolveParams) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::UpdateCascades");

    if (!initialSpectraReady || !EnsureResources()) {
        return false;
    }

    const bool foamEnabled = (resolveParams.foamOptions & OceanFoamOptionBitEnabled) != 0u;
    const bool resetFoamHistory = foamEnabled && (resolveParams.foamOptions & OceanFoamOptionBitResetHistory) != 0u;
    const bool forceAllCascades = !cascadeOutputsReady || resetFoamHistory;

    cascadeOutputsReady = false;
    uint32_t updatedCascadeMask = 0u;
    uint32_t updatedFoamCascadeMask = 0u;
    if (resetFoamHistory) {
        foamHistoryMayContainFoamMask = 0u;
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        const CascadeSettings &cascadeSettings = simulationSettings.cascadeSettings[cascadeIndex];
        const float updatePeriod = 1.0f / cascadeSettings.updateRate;
        const bool updateRateChanged = cascadeScheduledUpdateRates[cascadeIndex] != cascadeSettings.updateRate;
        if (updateRateChanged) {
            cascadeScheduledUpdateRates[cascadeIndex] = cascadeSettings.updateRate;
            cascadeNextUpdateTimes[cascadeIndex] = cascadeLastUpdateTimes[cascadeIndex] + updatePeriod;
        }

        const float deadlineTolerance = BE1::Max(1e-6f, updatePeriod * 1e-4f);
        const bool updateDue = baseParams.time + deadlineTolerance >= cascadeNextUpdateTimes[cascadeIndex];
        if (!forceAllCascades && !updateDue) {
            continue;
        }

        const uint32_t cascadeBit = 1u << cascadeIndex;
        const bool hasPreviousUpdate = (cascadeUpdateInitializedMask & cascadeBit) != 0u;
        // 한 번 source 가 활성화된 cascade 는 generation rate 가 0 이 되어도 reset 전까지 history 감쇠를 계속한다.
        const bool updateFoamHistory = foamEnabled &&
            (resetFoamHistory || cascadeSettings.foamGenerationRate > 0.0f || (foamHistoryMayContainFoamMask & cascadeBit) != 0u);
        const float cascadeDeltaTime = resetFoamHistory || !hasPreviousUpdate
            ? resolveParams.deltaTime
            : BE1::Max(baseParams.time - cascadeLastUpdateTimes[cascadeIndex], 0.0f);

        OceanTimeSpectrumParams params = baseParams;

        // patchSize 는 각 cascade 의 patchSize 를 적용한다.
        params.patchSize = simulationSettings.cascades[cascadeIndex].patchSize;
        params.cascadeIndex = cascadeIndex;

        // 초기 스펙트럼을 시간에 따라 진화시키고 IFFT 입력 spectrum 을 생성한다.
        if (!GenerateTimeSpectrum(commandList, frameThreadData, params)) {
            return false;
        }

        // complex IFFT 로 주파수 영역 spectrum 을 공간 영역 field 로 변환한다.
        if (!renderSystem->GetGpuFFT().DispatchIFFTComplex2D(commandList, timeSpectrumBuffer, spatialFieldsBuffer, ifftWorkBuffer, simulationSettings.fftSize, OceanSpectrumComplexPlaneCount, GpuFFT::Complex2DDataLayout::Normal, GpuFFT::Complex2DDataLayout::Transposed)) {
            return false;
        }

        OceanResolveFieldsParams cascadeResolveParams = resolveParams;
        cascadeResolveParams.deltaTime = cascadeDeltaTime;
        if (updateFoamHistory) {
            cascadeResolveParams.foamThreshold = cascadeSettings.foamThreshold;
            cascadeResolveParams.foamGenerationRate = cascadeSettings.foamGenerationRate;
            cascadeResolveParams.foamDecayRate = BE1::Math::Ln2 / cascadeSettings.foamHalfLife;
        } else {
            cascadeResolveParams.foamOptions &= ~OceanFoamOptionBitEnabled;
        }
        cascadeResolveParams.cascadeIndex = cascadeIndex;
        RHI::ConstantBuffer *resolveConstantBuffer = frameThreadData->AllocConstant(sizeof(cascadeResolveParams), &cascadeResolveParams);
        if (!resolveConstantBuffer) {
            return false;
        }

        // 공간 영역 field 를 texture array 의 현재 cascade slice 로 변환한다.
        if (!ResolveOceanFields(commandList, resolveConstantBuffer)) {
            return false;
        }

        const float previousDeadline = cascadeNextUpdateTimes[cascadeIndex];
        cascadeLastUpdateTimes[cascadeIndex] = baseParams.time;
        if (forceAllCascades || !hasPreviousUpdate || updateRateChanged) {
            cascadeNextUpdateTimes[cascadeIndex] = baseParams.time + updatePeriod;
        } else {
            const float deadlineLateness = BE1::Max(baseParams.time - previousDeadline, 0.0f);
            const float elapsedPeriods = BE1::Math::Floor(deadlineLateness / updatePeriod) + 1.0f;
            cascadeNextUpdateTimes[cascadeIndex] = previousDeadline + elapsedPeriods * updatePeriod;
        }
        cascadeUpdateInitializedMask |= cascadeBit;
        updatedCascadeMask |= cascadeBit;
        if (updateFoamHistory) {
            updatedFoamCascadeMask |= cascadeBit;
            if (cascadeSettings.foamGenerationRate > 0.0f) {
                foamHistoryMayContainFoamMask |= cascadeBit;
            }
        }
    }

    if (updatedCascadeMask != 0u) {
        // 인접한 cascade 들을 하나의 texture array slice 범위로 묶어서 mip dispatch 횟수를 줄인다.
        GenerateOceanCascadeMipRanges(commandList, fieldTextureArray, fieldMipSRVs.Ptr(), fieldMipUAVs.Ptr(), updatedCascadeMask, 2u);

        if (updatedFoamCascadeMask != 0u) {
            GenerateOceanCascadeMipRanges(commandList, foamHistoryTextureArray, foamMipSRVs.Ptr(), foamMipUAVs.Ptr(), updatedFoamCascadeMask, 1u);
        }

        if (foamEnabled) {
            foamHistoryResetPending = false;
        }
    }

    cascadeOutputsReady = true;
    return true;
}
