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
#include "Render/Texture/Texture.h"
#include "Render/Texture/TextureManager.h"
#include "Shaders/ShaderInterop.h"
#include "Math/AABB.h"
#include "Math/Frustum.h"
#include "Math/OBB.h"

static constexpr uint32_t OceanSurfaceGridLodSteps[] = { 1u, 3u, 5u };
static_assert(COUNT_OF(OceanSurfaceGridLodSteps) == GpuOcean::SurfaceGridLodCount);
static_assert(GpuOcean::SurfaceGridLodCount == OceanSurfaceGridLodCount);
static_assert(GpuOcean::SurfaceGridLodCount == 3u);
static_assert((GpuOcean::SurfaceGridResolution - 1u) % OceanSurfaceGridLodSteps[GpuOcean::SurfaceGridLodCount - 1u] == 0u);

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

        // Patch 분할을 다시 계산할 때 사용자가 지정한 cascade 별 random seed 는 보존한다.
        CascadeDesc &cascadeDesc = cascades[cascadeIndex];
        cascadeDesc.patchSize = patchSize;
        cascadeDesc.minWaveNumber = minWaveNumber;
        cascadeDesc.maxWaveNumber = maxWaveNumber;

        minWaveNumber = maxWaveNumber;
        patchSize *= inversePatchSizeRatio;
    }
}

void GpuOcean::Init() {
    simulationTime = 0.0f;
    ResetCascadeUpdateSchedule();
    foamHistoryMayContainFoamMask = 0u;
    pipelinesReady = false;
    pipelineBuildFailed = false;

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

    waterDetailNormalTexture = textureManager.GetTexture("Data/EngineTextures/water_n.PNG", Texture::Flag::NormalMap | Texture::Flag::LinearColorSpace);
    if (waterDetailNormalTexture == textureManager.defaultTexture) {
        BE_WARNLOG("GpuOcean::Init: failed to load ocean detail normal texture\n");
        waterDetailNormalTexture.reset();
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
    ResetCascadeUpdateSchedule();
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
    foamDetailTexture.reset();
    waterDetailNormalTexture.reset();
}

static constexpr uint32_t GetOceanSurfaceGridLodIndexCount(uint32_t lodLevel) {
    const uint32_t quadCountPerAxis = (GpuOcean::SurfaceGridResolution - 1u) / OceanSurfaceGridLodSteps[lodLevel];
    return quadCountPerAxis * quadCountPerAxis * 6u;
}

bool GpuOcean::CreateSurfaceGrid() {
    static_assert(CascadeCount == OceanCascadeCount);
    static_assert(SurfaceGridResolution == OceanSurfaceGridResolution);
    static_assert(SurfaceGridResolution * SurfaceGridResolution <= 1u << 16u);

    if (surfaceGridIndexBuffer) {
        return true;
    }

    DestroySurfaceGrid();

    uint32_t totalIndexCount = 0;
    for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        surfaceGridLodRanges[lodLevel].firstIndex = totalIndexCount;
        surfaceGridLodRanges[lodLevel].indexCount = GetOceanSurfaceGridLodIndexCount(lodLevel);
        totalIndexCount += surfaceGridLodRanges[lodLevel].indexCount;
    }

    BE1::Array<uint16_t> indices;
    indices.SetCount(totalIndexCount);

    uint32_t indexOffset = 0;
    for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        const uint32_t gridStep = OceanSurfaceGridLodSteps[lodLevel];
        const SurfaceGridLodRange &lodRange = surfaceGridLodRanges[lodLevel];
        assert(indexOffset == lodRange.firstIndex);

        for (uint32_t y = 0; y < SurfaceGridResolution - 1; y += gridStep) {
            for (uint32_t x = 0; x < SurfaceGridResolution - 1; x += gridStep) {
                const uint32_t bottomLeft = y * SurfaceGridResolution + x;
                const uint32_t bottomRight = bottomLeft + gridStep;
                const uint32_t topLeft = bottomLeft + gridStep * SurfaceGridResolution;
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

    surfaceGridIndexBuffer = RHI::renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indices[0]), totalIndexCount, indices.Ptr());
    if (!surfaceGridIndexBuffer) {
        BE_WARNLOG("GpuOcean::CreateSurfaceGrid: failed to create packed surface grid index buffer\n");
        DestroySurfaceGrid();
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
    for (SurfaceGridLodRange &lodRange : surfaceGridLodRanges) {
        lodRange = {};
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

static void CalculateJONSWAPFetchParameters(float windSpeed, float gravity, float fetch, float &alpha, float &peakOmega) {
    // Hasselmann et al. 의 fetch-limited JONSWAP 관계식. windSpeed 는 해수면 위 10 m 풍속 U10 이다.
    const float dimensionlessFetch = gravity * fetch / (windSpeed * windSpeed);
    alpha = 0.076f * BE1::Math::Pow(dimensionlessFetch, -0.22f);
    peakOmega = BE1::Math::TwoPi * 3.5f * gravity / windSpeed * BE1::Math::Pow(dimensionlessFetch, -0.33f);
}

bool GpuOcean::Update(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const Settings &newSettings, float deltaTime) {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::Update");

    if (!newSettings.enabled) {
        return true;
    }

    if (newSettings.displacementScale < 0.0f ||
        deltaTime < 0.0f || newSettings.timeScale < 0.0f || newSettings.windDirection.LengthSqr() <= 0.0f ||
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
        if (newSettings.tma.spectrumScale < 0.0f || newSettings.tma.fetch <= 0.0f ||
            newSettings.tma.swell < 0.0f || newSettings.tma.swell > 1.0f || newSettings.tma.spread < 0.0f || newSettings.tma.spread > 1.0f) {
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

    for (const CascadeSettings &cascadeSettings : newSettings.cascadeSettings) {
        if (cascadeSettings.updateRate <= 0.0f || cascadeSettings.foamGenerationRate < 0.0f || cascadeSettings.foamHalfLife <= 0.0f) {
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
    const bool foamSourceSettingsChanged = settings.displacementScale != newSettings.displacementScale || settings.choppiness != newSettings.choppiness || settings.depth != newSettings.depth;
    const bool foamEnabledChanged = settings.foamEnabled != newSettings.foamEnabled;
    if (fftSizeChanged) {
        cascadeResourcesDirty = true;
    }
    if (fftSizeChanged || cascadeDescsChanged || initialSpectrumSettingsChanged) {
        initialSpectraReady = false;
        cascadeOutputsReady = false;
        ResetCascadeUpdateSchedule();
    }
    if (fftSizeChanged || cascadeDescsChanged || initialSpectrumSettingsChanged || foamSourceSettingsChanged || foamEnabledChanged) {
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
            CalculateJONSWAPFetchParameters(newSettings.windSpeed, newSettings.gravity, newSettings.tma.fetch, params.alpha, params.peakOmega);
            params.depth = newSettings.depth;
            params.swell = newSettings.tma.swell;
            params.spread = newSettings.tma.spread;
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
    resolveParams.displacementScale = newSettings.displacementScale;
    resolveParams.foamOptions = newSettings.foamEnabled ? OceanFoamOptionBitEnabled : 0u;
    if (foamHistoryResetPending) {
        resolveParams.foamOptions |= OceanFoamOptionBitResetHistory;
    }
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
    foamHistoryMayContainFoamMask = 0u;
    ResetCascadeUpdateSchedule();
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
    if (!NormalizeWindDirection(params.windDirection)) {
        return false;
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        const CascadeDesc &cascadeDesc = settings.cascades[cascadeIndex];

        OceanInitialSpectrumCascadeParams &cascadeParams = params.cascadeParams[cascadeIndex];
        cascadeParams.randomSeed = cascadeDesc.randomSeed;
        cascadeParams.patchSize = cascadeDesc.patchSize;
        cascadeParams.minWaveNumber = cascadeDesc.minWaveNumber;
        cascadeParams.maxWaveNumber = cascadeDesc.maxWaveNumber;
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
    if (!NormalizeWindDirection(params.windDirection)) {
        return false;
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        const CascadeDesc &cascadeDesc = settings.cascades[cascadeIndex];

        OceanInitialSpectrumCascadeParams &cascadeParams = params.cascadeParams[cascadeIndex];
        cascadeParams.randomSeed = cascadeDesc.randomSeed;
        cascadeParams.patchSize = cascadeDesc.patchSize;
        cascadeParams.minWaveNumber = cascadeDesc.minWaveNumber;
        cascadeParams.maxWaveNumber = cascadeDesc.maxWaveNumber;
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

    if (!initialSpectrumTextureArray || initialSpectrumTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready || !initialSpectrumSRV.IsValid() ||
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

    if (!resolveConstantBuffer ||
        !spatialFieldsBuffer || spatialFieldsBuffer->GetStatus() != RHI::GPUResourceStatus::Ready || !spatialFieldsSRV.IsValid() ||
        !outputFieldTextureArray || outputFieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready || !outputFieldUAV.IsValid() ||
        !foamHistoryTextureArray || foamHistoryTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready || !foamHistoryUAV.IsValid()) {
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
        const CascadeSettings &cascadeSettings = settings.cascadeSettings[cascadeIndex];
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
        if (!ResolveOceanFields(commandList, resolveConstantBuffer, cascadeIndex, spatialFieldsBuffer, fieldTextureArray, foamHistoryTextureArray)) {
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

bool GpuOcean::DrawSurface(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const RHI::PipelineState *pipelineState,
    const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer, const Settings &renderSettings, const SurfaceView &surfaceView, SurfaceDrawStats *drawStats) const {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::DrawSurface");

    if (drawStats) {
        *drawStats = {};
    }

    const bool hasPerspectiveFrustum = surfaceView.frustum != nullptr;
    const bool hasOrthographicBox = surfaceView.box != nullptr;
    const bool invalidCullVolume = renderSettings.surfaceFrustumCulling && hasPerspectiveFrustum == hasOrthographicBox;
    if (!pipelineState || !frameConstantBuffer || !cameraConstantBuffer || !surfaceGridIndexBuffer || !cascadeOutputsReady || invalidCullVolume) {
        return false;
    }

    if (!fieldTextureArray || fieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !foamHistoryTextureArray || foamHistoryTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    RHI::Texture *foamDetailMap = foamDetailTexture ? foamDetailTexture->GetReadyRHITexture() : nullptr;
    if (!foamDetailMap && textureManager.whiteTexture) {
        foamDetailMap = textureManager.whiteTexture->GetReadyRHITexture();
    }
    if (!foamDetailMap) {
        return false;
    }

    RHI::Texture *waterDetailNormalMap = waterDetailNormalTexture ? waterDetailNormalTexture->GetReadyRHITexture() : nullptr;
    if (!waterDetailNormalMap && textureManager.flatNormalTexture) {
        waterDetailNormalMap = textureManager.flatNormalTexture->GetReadyRHITexture();
    }
    if (!waterDetailNormalMap) {
        return false;
    }

    static constexpr uint32_t MaxSurfaceGridTileCount = MaxSurfaceGridTileCountPerAxis * MaxSurfaceGridTileCountPerAxis;
    const uint32_t tileCountPerAxis = renderSettings.surfaceGridTileCountPerAxis;
    if (tileCountPerAxis == 0 || tileCountPerAxis > MaxSurfaceGridTileCountPerAxis ||
        renderSettings.surfaceGridSpacing <= 0.0f || renderSettings.surfaceNormalMapSize <= 0.0f ||
        renderSettings.surfaceCascadeFadeScale <= 0.0f || renderSettings.windDirection.LengthSqr() <= 0.0f ||
        (renderSettings.foamEnabled && renderSettings.surfaceFoamDetailStrength > 0.0f && renderSettings.surfaceFoamDetailMapSize <= 0.0f)) {
        return false;
    }
    for (const CascadeDesc &cascadeDesc : renderSettings.cascades) {
        if (cascadeDesc.patchSize <= 0.0f) {
            return false;
        }
    }

    const float tileCenterOffset = static_cast<float>(tileCountPerAxis - 1u) * 0.5f;

    uint32_t tileLodLevels[MaxSurfaceGridTileCount] = {};
    uint32_t tileLodCounts[SurfaceGridLodCount] = {};
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
    const float gridSnapSpacing = renderSettings.surfaceGridSpacing * static_cast<float>(gridSnapStep);
    const BE1::Vec2 snappedCameraPosition(
        BE1::Math::Round(surfaceView.origin.x / gridSnapSpacing) * gridSnapSpacing,
        BE1::Math::Round(surfaceView.origin.y / gridSnapSpacing) * gridSnapSpacing);

    const float tileSize = static_cast<float>(SurfaceGridResolution - 1u) * renderSettings.surfaceGridSpacing;
    const float tileHalfSize = tileSize * 0.5f;
    BE1::Frustum expandedSurfaceFrustum;
    const BE1::Frustum *surfaceFrustum = surfaceView.frustum;
    if (renderSettings.surfaceFrustumCulling && hasPerspectiveFrustum && surfaceView.infiniteZFar) {
        expandedSurfaceFrustum = *surfaceView.frustum;

        // Infinite-far 투영에서 유한 camera far plane 때문에 실제로 투영 가능한 외곽 tile 이 제거되지 않도록 전체 grid 를 포함한다.
        const float gridHalfSize = static_cast<float>(tileCountPerAxis) * tileHalfSize;
        const float maxOffsetX = BE1::Math::Abs(snappedCameraPosition.x - surfaceView.origin.x) + gridHalfSize + renderSettings.surfaceMaxDisplacement;
        const float maxOffsetY = BE1::Math::Abs(snappedCameraPosition.y - surfaceView.origin.y) + gridHalfSize + renderSettings.surfaceMaxDisplacement;
        const float maxOffsetZ = BE1::Math::Abs(renderSettings.surfaceHeight - surfaceView.origin.z) + renderSettings.surfaceMaxDisplacement;
        const float requiredFarDistance = BE1::Math::Sqrt(maxOffsetX * maxOffsetX + maxOffsetY * maxOffsetY + maxOffsetZ * maxOffsetZ);
        if (requiredFarDistance > expandedSurfaceFrustum.GetFarDistance()) {
            expandedSurfaceFrustum.MoveFarDistance(requiredFarDistance);
        }
        surfaceFrustum = &expandedSurfaceFrustum;
    }

    bool tileVisible[MaxSurfaceGridTileCount] = {};
    uint32_t visibleTileCount = 0;
    uint32_t lodInstanceCounts[SurfaceGridLodCount] = {};
    if (renderSettings.surfaceFrustumCulling) {
        for (uint32_t tileY = 0; tileY < tileCountPerAxis; ++tileY) {
            for (uint32_t tileX = 0; tileX < tileCountPerAxis; ++tileX) {
                const uint32_t tileIndex = tileY * tileCountPerAxis + tileX;
                const float tilePositionX = static_cast<float>(tileX) - tileCenterOffset;
                const float tilePositionY = static_cast<float>(tileY) - tileCenterOffset;
                const BE1::Vec3 tileCenter(
                    snappedCameraPosition.x + tilePositionX * tileSize,
                    snappedCameraPosition.y + tilePositionY * tileSize,
                    renderSettings.surfaceHeight);

                // FFT field 의 최대 변위를 CPU 에서 알 수 없으므로 설정된 보수적 최대값으로 모든 축을 확장한다.
                const BE1::AABB tileBounds(
                    tileCenter - BE1::Vec3(tileHalfSize, tileHalfSize, 0.0f),
                    tileCenter + BE1::Vec3(tileHalfSize, tileHalfSize, 0.0f));
                const BE1::AABB expandedTileBounds = tileBounds.Expand(renderSettings.surfaceMaxDisplacement);
                const bool culled = hasPerspectiveFrustum
                    ? surfaceFrustum->CullAABB(expandedTileBounds)
                    : !surfaceView.box->IsIntersectAABB(expandedTileBounds);
                if (!culled) {
                    tileVisible[tileIndex] = true;
                    visibleTileCount++;
                    lodInstanceCounts[tileLodLevels[tileIndex]]++;
                }
            }
        }
    } else {
        visibleTileCount = tileCountPerAxis * tileCountPerAxis;
        for (uint32_t tileIndex = 0; tileIndex < visibleTileCount; ++tileIndex) {
            tileVisible[tileIndex] = true;
        }
        for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
            lodInstanceCounts[lodLevel] = tileLodCounts[lodLevel];
        }
    }

    if (visibleTileCount == 0u) {
        return true;
    }

    uint32_t lodInstanceOffsets[SurfaceGridLodCount] = {};
    uint32_t lodInstanceWriteOffsets[SurfaceGridLodCount] = {};
    for (uint32_t lodLevel = 1; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        lodInstanceOffsets[lodLevel] = lodInstanceOffsets[lodLevel - 1u] + lodInstanceCounts[lodLevel - 1u];
    }
    for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        lodInstanceWriteOffsets[lodLevel] = lodInstanceOffsets[lodLevel];
    }

    OceanSurfaceTileInstance tileInstances[MaxSurfaceGridTileCount] = {};
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

            OceanSurfaceTileInstance &tileInstance = tileInstances[lodInstanceWriteOffsets[lodLevel]++];
            tileInstance.tilePosition = float2(static_cast<float>(tileX) - tileCenterOffset, static_cast<float>(tileY) - tileCenterOffset);
            tileInstance.lodLevel = lodLevel;
            // left, right, bottom, top 순서로 이웃 LOD 를 8비트씩 패킹한다.
            tileInstance.packedNeighborLodLevels = leftLodLevel | (rightLodLevel << 8u) | (bottomLodLevel << 16u) | (topLodLevel << 24u);
        }
    }
    for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        assert(lodInstanceWriteOffsets[lodLevel] == lodInstanceOffsets[lodLevel] + lodInstanceCounts[lodLevel]);
    }

    RHI::Buffer *tileInstanceBuffer = frameThreadData->AllocStructuredBuffer(sizeof(OceanSurfaceTileInstance), visibleTileCount, tileInstances);
    if (!tileInstanceBuffer) {
        return false;
    }

    // 캐스캐이드 별 패치 사이즈 (L) 와 surfaceGridSpacing 에 맞는 필드 텍스쳐의 mip LOD 계산
    float cascadePatchSizes[4] = {};
    float cascadeVertexMipLevels[4] = {};
    const float fieldTextureResolution = static_cast<float>(fieldTextureArray->GetWidth());
    const float maxFieldMipLevel = static_cast<float>(fieldTextureArray->GetMipLevelCount() - 1u);
    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        const float patchSize = renderSettings.cascades[cascadeIndex].patchSize;
        cascadePatchSizes[cascadeIndex] = patchSize;
        cascadeVertexMipLevels[cascadeIndex] = BE1::Clamp(BE1::Math::Log2(renderSettings.surfaceGridSpacing * fieldTextureResolution / patchSize), 0.0f, maxFieldMipLevel);
    }

    OceanSurfaceParams params = {};
    params.cascadePatchSizes = float4(cascadePatchSizes);
    params.cascadeVertexMipLevels = float4(cascadeVertexMipLevels);
    params.gridSpacing = renderSettings.surfaceGridSpacing;
    params.surfaceHeight = renderSettings.surfaceHeight;
    params.roughness = renderSettings.surfaceRoughness;
    params.normalMapSize = renderSettings.surfaceNormalMapSize;
    params.normalStrength = renderSettings.surfaceNormalStrength;
    BE1::Vec2 windFlowDirection = -renderSettings.windDirection;
    windFlowDirection.Normalize();
    const float normalMapScrollSpeed = renderSettings.windSpeed * renderSettings.surfaceNormalScrollSpeedScale;
    const float normalMapUVOffsetScale = simulationTime * normalMapScrollSpeed / renderSettings.surfaceNormalMapSize;
    params.normalMapUVOffset = float2(windFlowDirection.x * normalMapUVOffsetScale, windFlowDirection.y * normalMapUVOffsetScale);
    const BE1::Color3 linearWaterColor = renderSettings.surfaceWaterColor.SRGBToLinear();
    params.waterColor = float3(linearWaterColor.r, linearWaterColor.g, linearWaterColor.b);
    params.fresnelF0 = renderSettings.surfaceFresnelF0;
    const BE1::Color3 linearFoamColor = renderSettings.surfaceFoamColor.SRGBToLinear();
    params.foamColor = float3(linearFoamColor.r, linearFoamColor.g, linearFoamColor.b);
    params.foamDetailMapSize = renderSettings.surfaceFoamDetailMapSize;
    params.foamDetailStrength = renderSettings.surfaceFoamDetailStrength;
    params.displacementScale = renderSettings.displacementScale;
    params.cascadeFadeScale = renderSettings.surfaceCascadeFadeScale;
    params.gridSnapSpacing = gridSnapSpacing;
    params.foamOptions = renderSettings.foamEnabled ? OceanFoamOptionBitEnabled : 0u;

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
    RHI::renderer->SetTexture(commandList, 2, false, foamDetailMap);
    RHI::renderer->SetTexture(commandList, 3, false, waterDetailNormalMap);
    RHI::renderer->SetBuffer(commandList, 4, false, tileInstanceBuffer);

    if (drawStats) {
        drawStats->instanceBufferAllocCount = 1;
        drawStats->instanceBufferUploadBytes = sizeof(OceanSurfaceTileInstance) * visibleTileCount;
    }

    for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        const uint32_t instanceCount = lodInstanceCounts[lodLevel];
        if (instanceCount == 0) {
            continue;
        }

        const SurfaceGridLodRange &lodRange = surfaceGridLodRanges[lodLevel];
        const uint32_t tileInstanceOffset = lodInstanceOffsets[lodLevel];
        RHI::renderer->SetConstants(commandList, 3, &tileInstanceOffset, sizeof(tileInstanceOffset), 0);
        RHI::renderer->DrawIndexedInstanced(commandList, lodRange.indexCount, instanceCount, lodRange.firstIndex, 0, 0);

        if (drawStats) {
            drawStats->drawCount++;
            drawStats->triangleCount += static_cast<uint64_t>(lodRange.indexCount / 3u) * instanceCount;
        }
    }
    return true;
}
