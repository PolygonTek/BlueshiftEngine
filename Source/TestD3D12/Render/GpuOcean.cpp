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
#include "RenderSystem.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Shaders/ShaderInterop.h"

void GpuOcean::Settings::InitCascades(float largestPatchSize, float beta) {
    cascadeBeta = beta;

    const float highFrequencyBoundaryBin = beta * 0.5f * static_cast<float>(GpuFFT::GetGpuFFTSizeValue(fftSize));
    const float patchSizeRatio = BE1::Math::Pow(highFrequencyBoundaryBin, 1.0f / static_cast<float>(CascadeCount - 1));
    const float inversePatchSizeRatio = 1.0f / patchSizeRatio;

    float patchSize = largestPatchSize;
    float minWaveNumber = BE1::Math::TwoPi / patchSize;

    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        const bool isLastCascade = cascadeIndex == CascadeCount - 1;
        const float maxWaveNumber = isLastCascade ? BE1::Math::BigNumber : highFrequencyBoundaryBin * BE1::Math::TwoPi / patchSize;

        cascades[cascadeIndex] = { patchSize, minWaveNumber, maxWaveNumber, cascadeIndex };

        minWaveNumber = maxWaveNumber;
        patchSize *= inversePatchSizeRatio;
    }
}

void GpuOcean::Init() {
    simulationTime = 0.0f;
    pipelinesReady = false;
    pipelineBuildFailed = false;

    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel shaderModel = RHI::Renderer::MinimumShaderModel();
    constexpr bool asyncPipelineBuild = true;

    surfaceShaderTemplate = shaderTemplateManager.GetShaderTemplate("Source/TestD3D12/Shaders/Water/OceanSurface");
    if (!surfaceShaderTemplate) {
        BE_WARNLOG("GpuOcean::Init: failed to load ocean surface shader template\n");
    }
    CreateSurfaceGrid();

    generatePhillipsInitialSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generatePhillipsInitialOceanSpectrumShader", shaderModel, "Water/GenerateInitialSpectrum.hlsl", baseDir, "CSMain", "OCEAN_SPECTRUM_MODEL=0");
    generateTMAInitialSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generateTMAInitialOceanSpectrumShader", shaderModel, "Water/GenerateInitialSpectrum.hlsl", baseDir, "CSMain", "OCEAN_SPECTRUM_MODEL=1");
    generateTimeSpectrumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_generateOceanTimeSpectrumShader", shaderModel, "Water/GenerateTimeSpectrum.hlsl", baseDir, "CSMain");
    resolveOceanFieldsShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_resolveOceanFieldsShader", shaderModel, "Water/ResolveOceanFields.hlsl", baseDir, "CSMain");

    generatePhillipsInitialSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generatePhillipsInitialSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    generateTMAInitialSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generateTMAInitialSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    generateTimeSpectrumPipeline = shaderPipelineCache.GetComputePipeline(generateTimeSpectrumShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    resolveOceanFieldsPipeline = shaderPipelineCache.GetComputePipeline(resolveOceanFieldsShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
}

void GpuOcean::Shutdown() {
    DestroySurfaceGrid();
    DestroyResources();

    simulationTime = 0.0f;
    cascadeResourcesDirty = true;
    initialSpectraReady = false;
    cascadeOutputsReady = false;
    foamHistoryResetPending = true;
    pipelinesReady = false;
    pipelineBuildFailed = false;

    generatePhillipsInitialSpectrumPipeline.reset();
    generateTMAInitialSpectrumPipeline.reset();
    generateTimeSpectrumPipeline.reset();
    resolveOceanFieldsPipeline.reset();

    generatePhillipsInitialSpectrumShaderTemplate.reset();
    generateTMAInitialSpectrumShaderTemplate.reset();
    generateTimeSpectrumShaderTemplate.reset();
    resolveOceanFieldsShaderTemplate.reset();
    surfaceShaderTemplate.reset();
}

bool GpuOcean::CreateSurfaceGrid() {
    static_assert(CascadeCount == OceanCascadeCount);
    static_assert(SurfaceGridResolution == OceanSurfaceGridResolution);
    static_assert(SurfaceGridResolution * SurfaceGridResolution <= 1u << 16u);

    if (surfaceGridIndexBuffer) {
        return true;
    }

    BE1::Array<uint16_t> indices;
    indices.SetCount(SurfaceGridIndexCount);

    uint32_t indexOffset = 0;
    for (uint32_t y = 0; y < SurfaceGridResolution - 1; ++y) {
        for (uint32_t x = 0; x < SurfaceGridResolution - 1; ++x) {
            const uint32_t bottomLeft = y * SurfaceGridResolution + x;
            const uint32_t topLeft = bottomLeft + SurfaceGridResolution;

            indices[indexOffset++] = static_cast<uint16_t>(bottomLeft);
            indices[indexOffset++] = static_cast<uint16_t>(bottomLeft + 1);
            indices[indexOffset++] = static_cast<uint16_t>(topLeft);
            indices[indexOffset++] = static_cast<uint16_t>(topLeft);
            indices[indexOffset++] = static_cast<uint16_t>(bottomLeft + 1);
            indices[indexOffset++] = static_cast<uint16_t>(topLeft + 1);
        }
    }
    assert(indexOffset == SurfaceGridIndexCount);

    surfaceGridIndexBuffer = RHI::renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indices[0]), SurfaceGridIndexCount, indices.Ptr());
    if (!surfaceGridIndexBuffer) {
        BE_WARNLOG("GpuOcean::CreateSurfaceGrid: failed to create surface grid index buffer\n");
        return false;
    }

    RHI::renderer->SetName(surfaceGridIndexBuffer, "GpuOcean::surfaceGridIndexBuffer");
    return true;
}

void GpuOcean::DestroySurfaceGrid() {
    if (surfaceGridIndexBuffer) {
        RHI::renderer->DestroyIndexBuffer(surfaceGridIndexBuffer);
        surfaceGridIndexBuffer = nullptr;
    }
}

void GpuOcean::UpdatePipelineBuildState() {
    if (pipelinesReady || pipelineBuildFailed) {
        return;
    }

    const ShaderPipeline *pipelines[] = {
        generatePhillipsInitialSpectrumPipeline.get(),
        generateTMAInitialSpectrumPipeline.get(),
        generateTimeSpectrumPipeline.get(),
        resolveOceanFieldsPipeline.get()
    };

    bool allReady = true;

    for (const ShaderPipeline *pipeline : pipelines) {
        if (!pipeline) {
            pipelineBuildFailed = true;
            break;
        }

        switch (pipeline->GetState()) {
        case ShaderPipeline::State::Ready:
        case ShaderPipeline::State::Rebuilding:
            break;
        case ShaderPipeline::State::Building:
            allReady = false;
            break;
        case ShaderPipeline::State::Empty:
        case ShaderPipeline::State::Failed:
            pipelineBuildFailed = true;
            break;
        }

        if (pipelineBuildFailed) {
            break;
        }
    }

    if (pipelineBuildFailed) {
        BE_WARNLOG("GpuOcean::UpdatePipelineBuildState: pipeline build failed\n");
        return;
    }

    pipelinesReady = allReady;
}

bool GpuOcean::InitialSpectrumSettingsEqual(const Settings &lhs, const Settings &rhs) const {
    if (lhs.spectrumModel != rhs.spectrumModel || lhs.windDirection != rhs.windDirection || lhs.windSpeed != rhs.windSpeed ||
        lhs.gravity != rhs.gravity || lhs.smallWaveDamping != rhs.smallWaveDamping) {
        return false;
    }

    switch (lhs.spectrumModel) {
    case SpectrumModel::Phillips:
        return lhs.phillips == rhs.phillips;
    case SpectrumModel::TMA:
        return lhs.depth == rhs.depth && lhs.tma == rhs.tma;
    }
    return false;
}

bool GpuOcean::Update(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const Settings &newSettings, float deltaTime) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::Update");

    if (!newSettings.enabled) {
        return true;
    }

    if (static_cast<uint32_t>(newSettings.fftSize) >= static_cast<uint32_t>(GpuFFT::FFTSize::Count) ||
        !BE1::Math::IsFinite(newSettings.surfaceHeight) || !BE1::Math::IsFinite(newSettings.surfaceGridSpacing) || newSettings.surfaceGridSpacing <= 0.0f ||
        newSettings.surfaceGridTileCountPerAxis == 0 || newSettings.surfaceGridTileCountPerAxis > MaxSurfaceGridTileCountPerAxis ||
        !BE1::Math::IsFinite(newSettings.surfaceRoughness) || newSettings.surfaceRoughness < 0.02f || newSettings.surfaceRoughness > 1.0f ||
        !BE1::Math::IsFinite(newSettings.waveAmplitude) || newSettings.waveAmplitude < 0.0f ||
        !BE1::Math::IsFinite(newSettings.foamThreshold) || !BE1::Math::IsFinite(newSettings.foamGain) || newSettings.foamGain < 0.0f ||
        !BE1::Math::IsFinite(newSettings.foamDecayRate) || newSettings.foamDecayRate < 0.0f ||
        !BE1::Math::IsFinite(newSettings.foamAddRate) || newSettings.foamAddRate < 0.0f ||
        deltaTime < 0.0f || newSettings.timeScale < 0.0f || !newSettings.windDirection.IsFinite() || newSettings.windDirection.LengthSqr() <= 0.0f ||
        newSettings.windSpeed <= 0.0f || newSettings.gravity <= 0.0f || newSettings.depth <= 0.0f ||
        newSettings.smallWaveDamping < 0.0f || newSettings.choppiness < 0.0f) {
        return false;
    }

    switch (newSettings.spectrumModel) {
    case SpectrumModel::Phillips:
        if (newSettings.phillips.spectrumScale < 0.0f) {
            return false;
        }
        break;
    case SpectrumModel::TMA:
        if (newSettings.tma.spectrumScale < 0.0f || newSettings.tma.alpha <= 0.0f || newSettings.tma.peakOmega <= 0.0f) {
            return false;
        }
        break;
    default:
        return false;
    }

    for (const CascadeDesc &cascadeDesc : newSettings.cascades) {
        if (cascadeDesc.patchSize <= 0.0f || cascadeDesc.minWaveNumber < 0.0f || cascadeDesc.maxWaveNumber <= cascadeDesc.minWaveNumber) {
            return false;
        }
    }

    const bool fftSizeChanged = settings.fftSize != newSettings.fftSize;
    bool cascadeDescsChanged = false;
    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        if (settings.cascades[cascadeIndex] != newSettings.cascades[cascadeIndex]) {
            cascadeDescsChanged = true;
            break;
        }
    }

    const bool initialSpectrumSettingsChanged = !InitialSpectrumSettingsEqual(settings, newSettings);
    const bool foamSourceSettingsChanged = settings.waveAmplitude != newSettings.waveAmplitude || settings.choppiness != newSettings.choppiness ||
        settings.depth != newSettings.depth;
    if (fftSizeChanged) {
        cascadeResourcesDirty = true;
    }
    if (fftSizeChanged || cascadeDescsChanged || initialSpectrumSettingsChanged) {
        initialSpectraReady = false;
        cascadeOutputsReady = false;
    }
    if (fftSizeChanged || cascadeDescsChanged || initialSpectrumSettingsChanged || foamSourceSettingsChanged) {
        foamHistoryResetPending = true;
    }

    settings = newSettings;

    // 초기 스펙트럼 계수를 생성한다.
    if (!initialSpectraReady) {
        switch (newSettings.spectrumModel) {
        case SpectrumModel::Phillips: {
            OceanPhillipsInitialSpectrumParams params = {};
            params.spectrumScale = newSettings.phillips.spectrumScale;
            params.windDirection = float2(newSettings.windDirection.x, newSettings.windDirection.y);
            params.windSpeed = newSettings.windSpeed;
            params.gravity = newSettings.gravity;
            params.smallWaveDamping = newSettings.smallWaveDamping;
            params.oppositeWaveDamping = newSettings.phillips.oppositeWaveDamping;
            if (!GenerateInitialSpectraPhillips(commandList, frameThreadData, params)) {
                return false;
            }
            break;
        }
        case SpectrumModel::TMA: {
            OceanTMAInitialSpectrumParams params = {};
            params.spectrumScale = newSettings.tma.spectrumScale;
            params.windDirection = float2(newSettings.windDirection.x, newSettings.windDirection.y);
            params.windSpeed = newSettings.windSpeed;
            params.gravity = newSettings.gravity;
            params.smallWaveDamping = newSettings.smallWaveDamping;
            params.alpha = newSettings.tma.alpha;
            params.peakOmega = newSettings.tma.peakOmega;
            params.depth = newSettings.depth;
            params.swell = newSettings.tma.swell;
            if (!GenerateInitialSpectraTMA(commandList, frameThreadData, params)) {
                return false;
            }
            break;
        }
        }
    }

    const float simulationDeltaTime = deltaTime * newSettings.timeScale;
    const float nextSimulationTime = simulationTime + simulationDeltaTime;

    // 각 cascade 별로 초기 스펙트럼을 시간 진화시켜서 시간 스펙트럼을 생성하고,
    // 공간 필드를 계산하여 displacement 및 derivative 텍스처를 생성한다.
    OceanTimeSpectrumParams params = {};
    params.time = nextSimulationTime;
    params.gravity = newSettings.gravity;
    params.depth = newSettings.depth;
    params.choppiness = newSettings.choppiness;

    OceanResolveFieldsParams resolveParams = {};
    resolveParams.deltaTime = simulationDeltaTime;
    resolveParams.waveAmplitude = newSettings.waveAmplitude;
    resolveParams.foamThreshold = newSettings.foamThreshold;
    resolveParams.foamGain = newSettings.foamGain;
    resolveParams.foamDecayRate = newSettings.foamDecayRate;
    resolveParams.foamAddRate = newSettings.foamAddRate;
    resolveParams.resetFoamHistory = foamHistoryResetPending ? 1u : 0u;
    if (!UpdateCascades(commandList, frameThreadData, params, resolveParams)) {
        return false;
    }

    simulationTime = nextSimulationTime;
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

    const uint32_t spectrumSize = GpuFFT::GetGpuFFTSizeValue(settings.fftSize);
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

    const BE1::Image::Format initialSpectrumFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32G32B32A32_FLOAT);
    const BE1::Image::Format outputFieldFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    const BE1::Image::Format foamHistoryFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16_FLOAT);

    // 각 cascade 의 initial spectrum 을 동일한 index 의 slice 에 저장하며 compute SRV 상태로 유지한다.
    initialSpectrumTextureArray = CreateOceanTexture("GpuOcean::initialSpectrumTextureArray", spectrumSize, initialSpectrumFormat, CascadeCount, RHI::TextureMipInitMode::FromImage, RHI::GPUResourceState::ShaderResourceCompute);
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

template <typename Params>
static bool DispatchGenerateInitialSpectra(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const SharedPtr<ShaderPipeline> &pipeline, const Params &params, RHI::Texture *outputTextureArray, RHI::ViewID outputUAV) {
    static_assert(GpuOcean::CascadeCount == OceanCascadeCount);
    static_assert((sizeof(Params) & 15u) == 0, "Initial spectrum parameters must match the HLSL constant-buffer layout");

    if (!outputTextureArray || outputTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready || !outputUAV.IsValid() ||
        outputTextureArray->type != RHI::TextureType::Texture2DArray || outputTextureArray->GetArraySize() != OceanCascadeCount) {
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

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, true, outputTextureArray, outputUAV);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(spectrumWidth, OceanInitialSpectrumBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(spectrumHeight, OceanInitialSpectrumBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, GpuOcean::CascadeCount);
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
    if (params.spectrumScale < 0.0f || params.windSpeed <= 0.0f || params.gravity <= 0.0f || params.smallWaveDamping < 0.0f ||
        !NormalizeWindDirection(params.windDirection)) {
        return false;
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        const CascadeDesc &cascadeDesc = settings.cascades[cascadeIndex];

        OceanInitialSpectrumCascadeParams &cascadeParams = params.cascadeParams[cascadeIndex];
        cascadeParams.randomSeed = cascadeDesc.randomSeed;
        cascadeParams.patchSize = cascadeDesc.patchSize;
        cascadeParams.minWaveNumber = cascadeDesc.minWaveNumber;
        cascadeParams.maxWaveNumber = cascadeDesc.maxWaveNumber;

        if (cascadeParams.patchSize <= 0.0f || cascadeParams.minWaveNumber < 0.0f || cascadeParams.maxWaveNumber <= cascadeParams.minWaveNumber) {
            return false;
        }
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(initialSpectrumTextureArray, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));

    const bool generationSucceeded = DispatchGenerateInitialSpectra(commandList, frameThreadData, generatePhillipsInitialSpectrumPipeline, params, initialSpectrumTextureArray, RHI::ViewID::PrimaryID);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(initialSpectrumTextureArray, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute));
    if (!generationSucceeded) {
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
    if (params.spectrumScale < 0.0f || params.windSpeed <= 0.0f || params.gravity <= 0.0f || params.smallWaveDamping < 0.0f ||
        params.alpha <= 0.0f || params.peakOmega <= 0.0f || params.depth <= 0.0f || !NormalizeWindDirection(params.windDirection)) {
        return false;
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        const CascadeDesc &cascadeDesc = settings.cascades[cascadeIndex];

        OceanInitialSpectrumCascadeParams &cascadeParams = params.cascadeParams[cascadeIndex];
        cascadeParams.randomSeed = cascadeDesc.randomSeed;
        cascadeParams.patchSize = cascadeDesc.patchSize;
        cascadeParams.minWaveNumber = cascadeDesc.minWaveNumber;
        cascadeParams.maxWaveNumber = cascadeDesc.maxWaveNumber;

        if (cascadeParams.patchSize <= 0.0f || cascadeParams.minWaveNumber < 0.0f || cascadeParams.maxWaveNumber <= cascadeParams.minWaveNumber) {
            return false;
        }
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(initialSpectrumTextureArray, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));

    const bool generationSucceeded = DispatchGenerateInitialSpectra(commandList, frameThreadData, generateTMAInitialSpectrumPipeline, params, initialSpectrumTextureArray, RHI::ViewID::PrimaryID);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(initialSpectrumTextureArray, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute));
    if (!generationSucceeded) {
        return false;
    }

    initialSpectraReady = true;
    return true;
}

bool GpuOcean::GenerateTimeSpectrum(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTimeSpectrumParams &params, const RHI::Texture *initialSpectrumTextureArray, RHI::Buffer *outputSpectrumBuffer, RHI::ViewID initialSpectrumSRV, RHI::ViewID outputUAV) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::GenerateTimeSpectrum");

    if (params.gravity <= 0.0f || params.depth <= 0.0f || params.choppiness < 0.0f || params.patchSize <= 0.0f || params.cascadeIndex >= CascadeCount ||
        !initialSpectrumTextureArray || initialSpectrumTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready || !initialSpectrumSRV.IsValid() ||
        initialSpectrumTextureArray->type != RHI::TextureType::Texture2DArray || initialSpectrumTextureArray->GetArraySize() != CascadeCount ||
        !outputSpectrumBuffer || outputSpectrumBuffer->GetStatus() != RHI::GPUResourceStatus::Ready || !outputUAV.IsValid()) {
        return false;
    }

    const uint32_t spectrumWidth = initialSpectrumTextureArray->GetWidth();
    const uint32_t spectrumHeight = initialSpectrumTextureArray->GetHeight();
    if (spectrumWidth == 0 || spectrumWidth != spectrumHeight || (spectrumWidth & 1u) != 0) {
        return false;
    }

    RHI::TextureViewInfo initialSpectrumViewInfo;
    if (!initialSpectrumTextureArray->GetViewInfo(RHI::ViewType::SRV, initialSpectrumSRV, initialSpectrumViewInfo) ||
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

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, initialSpectrumTextureArray, initialSpectrumSRV);
    RHI::renderer->SetBuffer(commandList, 0, true, outputSpectrumBuffer, outputUAV);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(spectrumWidth >> 1u, OceanTimeSpectrumBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(spectrumHeight, OceanTimeSpectrumBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));
    return true;
}

bool GpuOcean::ResolveOceanFields(RHI::CommandList *commandList, const RHI::ConstantBuffer *resolveConstantBuffer, uint32_t cascadeIndex,
    RHI::Buffer *spatialFieldsBuffer, RHI::Texture *outputFieldTextureArray, RHI::Texture *foamHistoryTextureArray,
    RHI::ViewID spatialFieldsSRV, RHI::ViewID outputFieldUAV, RHI::ViewID foamHistoryUAV) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::ResolveOceanFields");

    if (!resolveConstantBuffer || cascadeIndex >= CascadeCount ||
        !spatialFieldsBuffer || spatialFieldsBuffer->GetStatus() != RHI::GPUResourceStatus::Ready || !spatialFieldsSRV.IsValid() ||
        !outputFieldTextureArray || outputFieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready || !outputFieldUAV.IsValid() ||
        !foamHistoryTextureArray || foamHistoryTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready || !foamHistoryUAV.IsValid() ||
        outputFieldTextureArray == foamHistoryTextureArray ||
        outputFieldTextureArray->type != RHI::TextureType::Texture2DArray || outputFieldTextureArray->GetArraySize() != FieldTextureSliceCount ||
        foamHistoryTextureArray->type != RHI::TextureType::Texture2DArray || foamHistoryTextureArray->GetArraySize() != CascadeCount) {
        return false;
    }

    const uint32_t fieldWidth = outputFieldTextureArray->GetWidth();
    const uint32_t fieldHeight = outputFieldTextureArray->GetHeight();
    if (fieldWidth == 0 || fieldWidth != fieldHeight ||
        foamHistoryTextureArray->GetWidth() != fieldWidth || foamHistoryTextureArray->GetHeight() != fieldHeight) {
        return false;
    }

    RHI::TextureViewInfo outputFieldViewInfo;
    if (!outputFieldTextureArray->GetViewInfo(RHI::ViewType::UAV, outputFieldUAV, outputFieldViewInfo) ||
        outputFieldViewInfo.firstSlice != 0 || outputFieldViewInfo.sliceCount != FieldTextureSliceCount || outputFieldViewInfo.firstMipLevel != 0) {
        return false;
    }

    RHI::TextureViewInfo foamHistoryViewInfo;
    if (!foamHistoryTextureArray->GetViewInfo(RHI::ViewType::UAV, foamHistoryUAV, foamHistoryViewInfo) ||
        foamHistoryViewInfo.firstSlice != 0 || foamHistoryViewInfo.sliceCount != CascadeCount || foamHistoryViewInfo.firstMipLevel != 0) {
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(resolveOceanFieldsPipeline);
    if (!pso) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialFieldsBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputFieldTextureArray, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeTextureBarrier(foamHistoryTextureArray, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, spatialFieldsBuffer, spatialFieldsSRV);
    RHI::renderer->SetTexture(commandList, 0, true, outputFieldTextureArray, outputFieldUAV);
    RHI::renderer->SetTexture(commandList, 1, true, foamHistoryTextureArray, foamHistoryUAV);
    RHI::renderer->SetConstantBuffer(commandList, 0, resolveConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(fieldWidth, OceanResolveFieldsBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(fieldHeight, OceanResolveFieldsBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialFieldsBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeTextureBarrier(outputFieldTextureArray, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(foamHistoryTextureArray, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}

bool GpuOcean::UpdateCascades(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTimeSpectrumParams &baseParams, const OceanResolveFieldsParams &resolveParams) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::UpdateCascades");

    if (!initialSpectraReady || !EnsureResources()) {
        return false;
    }

    cascadeOutputsReady = false;

    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        OceanTimeSpectrumParams params = baseParams;

        // patchSize 는 각 cascade 의 patchSize 를 적용한다.
        params.patchSize = settings.cascades[cascadeIndex].patchSize;
        params.cascadeIndex = cascadeIndex;

        // 초기 스펙트럼을 시간에 따라 진화시키고 IFFT 입력 spectrum 을 생성한다.
        if (!GenerateTimeSpectrum(commandList, frameThreadData, params, initialSpectrumTextureArray, timeSpectrumBuffer)) {
            return false;
        }

        // complex IFFT 로 주파수 영역 spectrum 을 공간 영역 field 로 변환한다.
        if (!renderSystem->GetGpuFFT().DispatchIFFTComplex2D(commandList, timeSpectrumBuffer, spatialFieldsBuffer, ifftWorkBuffer, settings.fftSize, OceanSpectrumComplexPlaneCount, GpuFFT::Complex2DDataLayout::Normal, GpuFFT::Complex2DDataLayout::Transposed)) {
            return false;
        }

        OceanResolveFieldsParams cascadeResolveParams = resolveParams;
        cascadeResolveParams.cascadeIndex = cascadeIndex;
        RHI::ConstantBuffer *resolveConstantBuffer = frameThreadData->AllocConstant(sizeof(cascadeResolveParams), &cascadeResolveParams);
        if (!resolveConstantBuffer) {
            return false;
        }

        // 공간 영역 field 를 texture array 의 현재 cascade slice 로 변환한다.
        if (!ResolveOceanFields(commandList, resolveConstantBuffer, cascadeIndex, spatialFieldsBuffer, fieldTextureArray, foamHistoryTextureArray)) {
            return false;
        }
    }

    // 모든 cascade 의 mip 0 이 준비된 뒤 surface filtering 에 사용할 하위 mip 들을 갱신한다.
    RHI::renderer->GenerateMipChain(commandList, fieldTextureArray, fieldMipSRVs.Ptr(), fieldMipUAVs.Ptr(), false);
    RHI::renderer->GenerateMipChain(commandList, foamHistoryTextureArray, foamMipSRVs.Ptr(), foamMipUAVs.Ptr(), false);

    cascadeOutputsReady = true;
    foamHistoryResetPending = false;
    return true;
}

bool GpuOcean::DrawSurface(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const RHI::PipelineState *pipelineState,
    const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer, const Settings &renderSettings) const {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::DrawSurface");

    if (!pipelineState || !frameConstantBuffer || !cameraConstantBuffer || !surfaceGridIndexBuffer || !cascadeOutputsReady) {
        return false;
    }

    if (!fieldTextureArray || fieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !foamHistoryTextureArray || foamHistoryTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    OceanSurfaceParams params = {};
    float cascadePatchSizes[4] = {};
    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        cascadePatchSizes[cascadeIndex] = renderSettings.cascades[cascadeIndex].patchSize;
    }
    params.cascadePatchSizes = float4(cascadePatchSizes);
    params.gridSpacing = renderSettings.surfaceGridSpacing;
    params.surfaceHeight = renderSettings.surfaceHeight;
    params.roughness = renderSettings.surfaceRoughness;
    params.waterColor = float3(0.005f, 0.045f, 0.065f);
    params.fresnelF0 = 0.02f;
    params.waveAmplitude = renderSettings.waveAmplitude;
    params.gridTileCountPerAxis = renderSettings.surfaceGridTileCountPerAxis;

    RHI::ConstantBuffer *surfaceConstantBuffer = frameThreadData->AllocConstant(sizeof(params), &params);
    if (!surfaceConstantBuffer) {
        return false;
    }

    RHI::renderer->SetPSO(commandList, pipelineState);
    RHI::renderer->SetIndexBuffer(commandList, surfaceGridIndexBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, frameConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 2, surfaceConstantBuffer);

    RHI::renderer->SetTexture(commandList, 0, false, fieldTextureArray);
    RHI::renderer->SetTexture(commandList, 1, false, foamHistoryTextureArray);

    RHI::renderer->DrawIndexedInstanced(commandList, SurfaceGridIndexCount, GetSurfaceGridInstanceCount(renderSettings), 0, 0, 0);
    return true;
}
