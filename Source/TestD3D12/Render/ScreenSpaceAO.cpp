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
#include "ScreenSpaceAO.h"
#include "Render/Backend/RenderBackend.h"
#include "Render/RenderInternal.h"
#include "Render/GpuTiming.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipelineBootstrap.h"
#include "Shaders/ShaderInterop.h"

static_assert(RenderGTAODepthMipMaxLevelCount == GTAODepthMipLevelCount, "GTAO depth mip counts must match");

void ScreenSpaceAO::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    aoUpsampleShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_aoUpsampleShader", computeShaderModel, "AO/AOUpsample.hlsl", baseDir);

    gtaoPrefilterShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoPrefilterShader", computeShaderModel, "AO/GTAOPrefilter.hlsl", baseDir);
    gtaoMainLowShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoMainLowShader", computeShaderModel, "AO/GTAOMain.hlsl", baseDir, "CSMain", "GTAO_SLICE_COUNT=1, GTAO_STEPS_PER_SLICE=3");
    gtaoMainMediumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoMainMediumShader", computeShaderModel, "AO/GTAOMain.hlsl", baseDir, "CSMain", "GTAO_SLICE_COUNT=2, GTAO_STEPS_PER_SLICE=3");
    gtaoMainHighShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoMainHighShader", computeShaderModel, "AO/GTAOMain.hlsl", baseDir, "CSMain", "GTAO_SLICE_COUNT=3, GTAO_STEPS_PER_SLICE=3");
    gtaoMainUltraShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoMainUltraShader", computeShaderModel, "AO/GTAOMain.hlsl", baseDir, "CSMain", "GTAO_SLICE_COUNT=9, GTAO_STEPS_PER_SLICE=3");
    gtaoDenoiseShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoDenoiseShader", computeShaderModel, "AO/GTAODenoise.hlsl", baseDir);
    gtaoDebugShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoDebugShader", computeShaderModel, "AO/GTAODebug.hlsl", baseDir);

    aoUpsamplePipeline = shaderPipelineCache.GetComputePipeline(aoUpsampleShaderTemplate, ShaderPermutationKey(), true);

    gtaoPrefilterPipeline = shaderPipelineCache.GetComputePipeline(gtaoPrefilterShaderTemplate, ShaderPermutationKey(), true);
    gtaoMainLowPipeline = shaderPipelineCache.GetComputePipeline(gtaoMainLowShaderTemplate, ShaderPermutationKey(), true);
    gtaoMainMediumPipeline = shaderPipelineCache.GetComputePipeline(gtaoMainMediumShaderTemplate, ShaderPermutationKey(), true);
    gtaoMainHighPipeline = shaderPipelineCache.GetComputePipeline(gtaoMainHighShaderTemplate, ShaderPermutationKey(), true);
    gtaoMainUltraPipeline = shaderPipelineCache.GetComputePipeline(gtaoMainUltraShaderTemplate, ShaderPermutationKey(), true);
    gtaoDenoisePipeline = shaderPipelineCache.GetComputePipeline(gtaoDenoiseShaderTemplate, ShaderPermutationKey(), true);
    gtaoDebugPipeline = shaderPipelineCache.GetComputePipeline(gtaoDebugShaderTemplate, ShaderPermutationKey(), true);
}

void ScreenSpaceAO::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "ScreenSpaceAO";

    bootstrap.TrackPipeline(groupName, aoUpsamplePipeline);
    bootstrap.TrackPipeline(groupName, gtaoPrefilterPipeline);
    bootstrap.TrackPipeline(groupName, gtaoMainLowPipeline);
    bootstrap.TrackPipeline(groupName, gtaoMainMediumPipeline);
    bootstrap.TrackPipeline(groupName, gtaoMainHighPipeline);
    bootstrap.TrackPipeline(groupName, gtaoMainUltraPipeline);
    bootstrap.TrackPipeline(groupName, gtaoDenoisePipeline);
    bootstrap.TrackPipeline(groupName, gtaoDebugPipeline);
}

void ScreenSpaceAO::Shutdown() {
    aoUpsampleShaderTemplate.reset();
    gtaoPrefilterShaderTemplate.reset();
    gtaoMainLowShaderTemplate.reset();
    gtaoMainMediumShaderTemplate.reset();
    gtaoMainHighShaderTemplate.reset();
    gtaoMainUltraShaderTemplate.reset();
    gtaoDenoiseShaderTemplate.reset();
    gtaoDebugShaderTemplate.reset();

    aoUpsamplePipeline.reset();
    gtaoPrefilterPipeline.reset();
    gtaoMainLowPipeline.reset();
    gtaoMainMediumPipeline.reset();
    gtaoMainHighPipeline.reset();
    gtaoMainUltraPipeline.reset();
    gtaoDenoisePipeline.reset();
    gtaoDebugPipeline.reset();

    frameAOUpsamplePSO = nullptr;
    frameGTAOPrefilterPSO = nullptr;
    frameGTAOMainPSO = nullptr;
    frameGTAODenoisePSO = nullptr;
    frameGTAODebugPSO = nullptr;
}

void ScreenSpaceAO::UpdateFramePSOs(const RenderFrameSettings &settings) {
    frameAOUpsamplePSO = ShaderPipeline::GetReadyPSO(aoUpsamplePipeline);
    frameGTAOPrefilterPSO = ShaderPipeline::GetReadyPSO(gtaoPrefilterPipeline);

    const uint32_t gtaoQuality = BE1::Min(settings.gtaoQuality, GTAOQualityLevelCount - 1u);
    switch (gtaoQuality) {
    case 0:
        frameGTAOMainPSO = ShaderPipeline::GetReadyPSO(gtaoMainLowPipeline);
        break;
    case 1:
        frameGTAOMainPSO = ShaderPipeline::GetReadyPSO(gtaoMainMediumPipeline);
        break;
    case 2:
        frameGTAOMainPSO = ShaderPipeline::GetReadyPSO(gtaoMainHighPipeline);
        break;
    case 3:
        frameGTAOMainPSO = ShaderPipeline::GetReadyPSO(gtaoMainUltraPipeline);
        break;
    }
    frameGTAODenoisePSO = ShaderPipeline::GetReadyPSO(gtaoDenoisePipeline);
    frameGTAODebugPSO = ShaderPipeline::GetReadyPSO(gtaoDebugPipeline);
}

bool ScreenSpaceAO::HasRequiredResources(const FrameScreenSpaceAO &data, const RenderFrameSettings &settings) {
    const ScreenSpaceAOTextureSet &textureSet = data.textureSet;
    if (data.mode != RenderAOMode::GTAO || !data.depthTexture || !data.normalRoughnessTexture || !textureSet.outputTexture) {
        return false;
    }

    if (settings.aoHalfResolution && !textureSet.halfDepthTexture) {
        return false;
    }

    const bool needTempTexture = settings.aoHalfResolution || (settings.gtaoDebug != 2 && settings.gtaoDenoisePasses > 1);
    if (needTempTexture && !textureSet.tempTexture) {
        return false;
    }

    assert(!textureSet.gtaoPrefilterDepthTexture || textureSet.gtaoDepthMipCount > 0);
    return textureSet.rawTexture && textureSet.gtaoPrefilterDepthTexture && textureSet.gtaoPackedEdgesTexture;
}

bool ScreenSpaceAO::PrepareDispatchParams(const RenderFrameSettings &settings, DispatchParams &params) const {
    params = {};

    if (!frameGTAOPrefilterPSO || !frameGTAOMainPSO || !frameGTAODenoisePSO) {
        return false;
    }

    params.useHalfResolution = settings.aoHalfResolution;
    if (params.useHalfResolution && !frameAOUpsamplePSO) {
        return false;
    }

    // 디버그 표시용 원본 가시도를 보존한다. 노이즈 제거 시 두 텍스처에 번갈아 쓰면서 원본을 덮어쓸 수 있다.
    params.denoisePassCount = settings.gtaoDebug == 2 ? 1u : BE1::Max(settings.gtaoDenoisePasses, 1u);

    return true;
}

void ScreenSpaceAO::Dispatch(RenderBackend &backend, const DispatchParams &params) const {
    PROFILER_CPU_SCOPED_EVENT("ScreenSpaceAO::Dispatch");

    RHI::CommandList *commandList = backend.GetCommandList();
    const FrameScreenSpaceAO &data = backend.GetFrameCamera()->ao;
    const RenderFrameSettings &settings = backend.GetFrameSettings();

    assert(data.mode == RenderAOMode::GTAO);

    RHI::Texture *depthTexture = data.depthTexture;
    RHI::Texture *normalRoughnessTexture = data.normalRoughnessTexture;
    RHI::Texture *aoTexture = data.textureSet.outputTexture;
    RHI::Texture *rawAOTexture = data.textureSet.rawTexture;
    RHI::Texture *tempAOTexture = data.textureSet.tempTexture;
    RHI::Texture *halfDepthTexture = data.textureSet.halfDepthTexture;
    RHI::Texture *prefilterDepthTexture = data.textureSet.gtaoPrefilterDepthTexture;
    RHI::Texture *packedEdgesTexture = data.textureSet.gtaoPackedEdgesTexture;
    const uint32_t mipCount = data.textureSet.gtaoDepthMipCount;
    const bool useHalfResolution = params.useHalfResolution;
    const uint32_t denoisePassCount = params.denoisePassCount;
    const RHI::GPUResourceState depthInputState = RHI::GPUResourceState::ShaderResource;

    DispatchGTAOPrefilterDepth(backend, depthInputState, useHalfResolution);

    DispatchGTAOMain(backend, useHalfResolution);

    RHI::GPUResourceState rawState = RHI::GPUResourceState::ShaderResourceCompute;
    RHI::GPUResourceState tempState = RHI::GPUResourceState::ShaderResource;
    RHI::Texture *denoiseSource = DispatchGTAODenoise(backend, denoisePassCount, useHalfResolution, rawState, tempState);

    if (useHalfResolution) {
        DispatchAOUpsample(backend, depthTexture, denoiseSource, halfDepthTexture, aoTexture, settings.aoRadius);

        if (denoiseSource == rawAOTexture) {
            rawState = RHI::GPUResourceState::ShaderResource;
        } else {
            tempState = RHI::GPUResourceState::ShaderResource;
        }
    }

    RHI::GPUBarrier finalBarriers[RenderGTAODepthMipMaxLevelCount + 7];
    uint32_t finalBarrierCount = 0;
    finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, depthInputState);
    finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(normalRoughnessTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(packedEdgesTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(aoTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
    if (rawState != RHI::GPUResourceState::ShaderResource) {
        finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(rawAOTexture, rawState, RHI::GPUResourceState::ShaderResource);
    }
    if (tempAOTexture && tempState != RHI::GPUResourceState::ShaderResource) {
        finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(tempAOTexture, tempState, RHI::GPUResourceState::ShaderResource);
    }
    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(prefilterDepthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, mipLevel);
    }
    RHI::renderer->Barrier(commandList, finalBarriers, finalBarrierCount);
}

void ScreenSpaceAO::DispatchGTAOPrefilterDepth(RenderBackend &backend, RHI::GPUResourceState depthInputState, bool useHalfResolution) const {
    RHI::CommandList *commandList = backend.GetCommandList();
    const RHI::ConstantBuffer *cameraConstantBuffer = backend.GetCameraConstantBuffer();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    const FrameScreenSpaceAO &data = backend.GetFrameCamera()->ao;
    const RenderFrameSettings &settings = backend.GetFrameSettings();

    RHI::Texture *depthTexture = data.depthTexture;
    RHI::Texture *aoTexture = data.textureSet.outputTexture;
    RHI::Texture *rawAOTexture = data.textureSet.rawTexture;
    RHI::Texture *halfDepthTexture = data.textureSet.halfDepthTexture;
    RHI::Texture *prefilterDepthTexture = data.textureSet.gtaoPrefilterDepthTexture;
    const uint32_t mipCount = data.textureSet.gtaoDepthMipCount;
    const uint32_t width = aoTexture->GetWidth();
    const uint32_t height = aoTexture->GetHeight();
    const uint32_t processWidth = rawAOTexture->GetWidth();
    const uint32_t processHeight = rawAOTexture->GetHeight();

    GTAOPrefilterParams prefilterParams = {};
    prefilterParams.inputSize = uint2(width, height);
    prefilterParams.outputSize = uint2(processWidth, processHeight);
    prefilterParams.resolutionScale = useHalfResolution ? 2u : 1u;
    prefilterParams.reverseZ = data.projectionPolicy.reverseZ ? 1u : 0u;
    prefilterParams.depthMipCount = mipCount;
    prefilterParams.radius = settings.aoRadius;
    prefilterParams.falloffRange = settings.gtaoFalloffRange;
    prefilterParams.radiusMultiplier = settings.gtaoRadiusMultiplier;
    prefilterParams.clearDepth = data.projectionPolicy.GetDepthClearValue();

    RHI::Texture *selectedDepthOutputTexture = useHalfResolution ? halfDepthTexture : rawAOTexture;
    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::GTAOPrefilter);

    RHI::GPUBarrier startBarriers[GTAODepthMipLevelCount + 2];
    uint32_t startBarrierCount = 0;
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, depthInputState, RHI::GPUResourceState::ShaderResourceCompute);
    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(prefilterDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, mipLevel);
    }
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(selectedDepthOutputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    RHI::renderer->Barrier(commandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(commandList, frameGTAOPrefilterPSO);
    RHI::renderer->SetTexture(commandList, 0, false, depthTexture);
    for (uint32_t slot = 0; slot < GTAODepthMipLevelCount; ++slot) {
        const uint32_t mipLevel = BE1::Min(slot, mipCount - 1u);
        RHI::renderer->SetTexture(commandList, slot, true, prefilterDepthTexture, data.textureSet.gtaoDepthMipUAVs[mipLevel]);
    }
    RHI::renderer->SetTexture(commandList, GTAODepthMipLevelCount, true, selectedDepthOutputTexture);
    RHI::renderer->SetConstants(commandList, 0, &prefilterParams, sizeof(prefilterParams), 0);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);
    RHI::renderer->Dispatch(commandList, BE1::DivCeil(processWidth, GTAOPrefilterBlockSizeX * 2u), BE1::DivCeil(processHeight, GTAOPrefilterBlockSizeY * 2u), 1);

    RHI::GPUBarrier endBarriers[GTAODepthMipLevelCount + 1];
    uint32_t endBarrierCount = 0;
    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(prefilterDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, 0, mipLevel);
    }
    if (useHalfResolution) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(selectedDepthOutputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
    }
    RHI::renderer->Barrier(commandList, endBarriers, endBarrierCount);
}

void ScreenSpaceAO::DispatchGTAOMain(RenderBackend &backend, bool useHalfResolution) const {
    RHI::CommandList *commandList = backend.GetCommandList();
    const RHI::ConstantBuffer *cameraConstantBuffer = backend.GetCameraConstantBuffer();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    const uint32_t frameIndex = backend.GetFrameIndex();
    const FrameScreenSpaceAO &data = backend.GetFrameCamera()->ao;
    const RenderFrameSettings &settings = backend.GetFrameSettings();

    RHI::Texture *depthTexture = data.depthTexture;
    RHI::Texture *normalRoughnessTexture = data.normalRoughnessTexture;
    RHI::Texture *aoTexture = data.textureSet.outputTexture;
    RHI::Texture *rawAOTexture = data.textureSet.rawTexture;
    RHI::Texture *prefilterDepthTexture = data.textureSet.gtaoPrefilterDepthTexture;
    RHI::Texture *packedEdgesTexture = data.textureSet.gtaoPackedEdgesTexture;
    const uint32_t mipCount = data.textureSet.gtaoDepthMipCount;
    const uint32_t width = aoTexture->GetWidth();
    const uint32_t height = aoTexture->GetHeight();
    const uint32_t processWidth = rawAOTexture->GetWidth();
    const uint32_t processHeight = rawAOTexture->GetHeight();

    GTAOMainParams mainParams = {};
    mainParams.inputSize = uint2(width, height);
    mainParams.outputSize = uint2(processWidth, processHeight);
    mainParams.outputSizeRcp = float2(1.0f / processWidth, 1.0f / processHeight);
    mainParams.resolutionScale = useHalfResolution ? 2u : 1u;
    mainParams.noiseIndex = data.temporalNoise ? frameIndex & 63u : 0u;
    mainParams.depthMipCount = mipCount;
    mainParams.radius = settings.aoRadius;
    mainParams.falloffRange = settings.gtaoFalloffRange;
    mainParams.radiusMultiplier = settings.gtaoRadiusMultiplier;
    mainParams.sampleDistributionPower = settings.gtaoSampleDistributionPower;
    mainParams.thinOccluderCompensation = settings.gtaoThinOccluderCompensation;
    mainParams.depthMipSamplingOffset = settings.gtaoDepthMipSamplingOffset;
    mainParams.power = settings.gtaoPower;
    mainParams.clearDepth = data.projectionPolicy.GetDepthClearValue();

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::GTAOMain);

    RHI::GPUBarrier mainStartBarriers[3];
    uint32_t mainStartBarrierCount = 0;
    mainStartBarriers[mainStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(normalRoughnessTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    // Full-resolution raw AO is already a UAV because it served as the unused selected-depth binding.
    // Half-resolution raw AO has not participated in the prefilter and still needs its first transition.
    if (useHalfResolution) {
        mainStartBarriers[mainStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(rawAOTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }
    mainStartBarriers[mainStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(packedEdgesTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    RHI::renderer->Barrier(commandList, mainStartBarriers, mainStartBarrierCount);

    RHI::renderer->SetPSO(commandList, frameGTAOMainPSO);
    RHI::renderer->SetTexture(commandList, 0, false, depthTexture);
    RHI::renderer->SetTexture(commandList, 1, false, normalRoughnessTexture);
    RHI::renderer->SetTexture(commandList, 2, false, prefilterDepthTexture);
    RHI::renderer->SetTexture(commandList, 0, true, rawAOTexture);
    RHI::renderer->SetTexture(commandList, 1, true, packedEdgesTexture);
    RHI::renderer->SetConstants(commandList, 0, &mainParams, sizeof(mainParams), 0);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(commandList, BE1::DivCeil(processWidth, GTAOMainBlockSizeX), BE1::DivCeil(processHeight, GTAOMainBlockSizeY), 1);

    RHI::GPUBarrier mainEndBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(rawAOTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(packedEdgesTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute)
    };
    RHI::renderer->Barrier(commandList, mainEndBarriers, COUNT_OF(mainEndBarriers));
}

RHI::Texture *ScreenSpaceAO::DispatchGTAODenoise(RenderBackend &backend, uint32_t denoisePassCount, bool useHalfResolution, RHI::GPUResourceState &rawAOState, RHI::GPUResourceState &tempAOState) const {
    RHI::CommandList *commandList = backend.GetCommandList();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    const FrameScreenSpaceAO &data = backend.GetFrameCamera()->ao;
    const RenderFrameSettings &settings = backend.GetFrameSettings();

    RHI::Texture *aoTexture = data.textureSet.outputTexture;
    RHI::Texture *rawAOTexture = data.textureSet.rawTexture;
    RHI::Texture *tempAOTexture = data.textureSet.tempTexture;
    RHI::Texture *packedEdgesTexture = data.textureSet.gtaoPackedEdgesTexture;
    const uint32_t processWidth = rawAOTexture->GetWidth();
    const uint32_t processHeight = rawAOTexture->GetHeight();
    RHI::Texture *denoiseSource = rawAOTexture;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::GTAODenoise);

    for (uint32_t passIndex = 0; passIndex < denoisePassCount; ++passIndex) {
        const bool finalPass = passIndex + 1 == denoisePassCount;
        RHI::Texture *denoiseOutput = nullptr;
        if (finalPass && !useHalfResolution) {
            denoiseOutput = aoTexture;
        } else {
            denoiseOutput = denoiseSource == rawAOTexture ? tempAOTexture : rawAOTexture;
        }

        RHI::GPUResourceState *outputState = nullptr;
        RHI::GPUResourceState outputBeforeState = RHI::GPUResourceState::ShaderResource;
        if (denoiseOutput == rawAOTexture) {
            outputState = &rawAOState;
            outputBeforeState = rawAOState;
        } else if (denoiseOutput == tempAOTexture) {
            outputState = &tempAOState;
            outputBeforeState = tempAOState;
        }

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(denoiseOutput, outputBeforeState, RHI::GPUResourceState::UnorderedAccess));
        if (outputState) {
            *outputState = RHI::GPUResourceState::UnorderedAccess;
        }

        GTAODenoiseParams denoiseParams = {};
        denoiseParams.outputSize = uint2(processWidth, processHeight);
        denoiseParams.outputSizeRcp = float2(1.0f / processWidth, 1.0f / processHeight);
        denoiseParams.blurBeta = settings.gtaoDenoisePasses == 0 ? 10000.0f : 1.2f;
        denoiseParams.finalPass = finalPass ? 1u : 0u;

        RHI::renderer->SetPSO(commandList, frameGTAODenoisePSO);
        RHI::renderer->SetTexture(commandList, 0, false, denoiseSource);
        RHI::renderer->SetTexture(commandList, 1, false, packedEdgesTexture);
        RHI::renderer->SetTexture(commandList, 0, true, denoiseOutput);
        RHI::renderer->SetConstants(commandList, 0, &denoiseParams, sizeof(denoiseParams), 0);

        RHI::renderer->Dispatch(commandList, BE1::DivCeil(processWidth, GTAODenoiseBlockSizeX * 2u), BE1::DivCeil(processHeight, GTAODenoiseBlockSizeY), 1);

        if (!finalPass) {
            RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(denoiseOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute));
            *outputState = RHI::GPUResourceState::ShaderResourceCompute;
        }
        denoiseSource = denoiseOutput;
    }

    return denoiseSource;
}

bool ScreenSpaceAO::DispatchDebugPreview(RenderBackend &backend) const {
    PROFILER_CPU_SCOPED_EVENT("ScreenSpaceAO::DispatchDebugPreview");

    RHI::CommandList *commandList = backend.GetCommandList();
    const FrameScreenSpaceAO &data = backend.GetFrameCamera()->ao;
    const RenderFrameSettings &settings = backend.GetFrameSettings();

    if (!frameGTAODebugPSO || !data.textureSet.outputTexture || settings.gtaoDebug <= 1) {
        return settings.gtaoDebug <= 1;
    }

    RHI::Texture *inputTexture = nullptr;
    RHI::ViewID inputViewID = RHI::ViewID::PrimaryID;
    uint32_t sourceType = 0;
    uint32_t mipLevel = 0;
    if (settings.gtaoDebug == 2) {
        inputTexture = data.textureSet.rawTexture;
    } else if (settings.gtaoDebug == 3) {
        inputTexture = data.textureSet.gtaoPackedEdgesTexture;
        sourceType = 1;
    } else {
        if (!data.textureSet.gtaoPrefilterDepthTexture || data.textureSet.gtaoDepthMipCount == 0) {
            return false;
        }
        mipLevel = BE1::Min(settings.gtaoDebug - 4u, data.textureSet.gtaoDepthMipCount - 1u);
        inputTexture = data.textureSet.gtaoPrefilterDepthTexture;
        inputViewID = data.textureSet.gtaoDepthMipSRVs[mipLevel];
        sourceType = 2;
    }

    if (!inputTexture) {
        return false;
    }

    GTAODebugParams params = {};
    params.inputSize = uint2(inputTexture->GetWidth() >> mipLevel, inputTexture->GetHeight() >> mipLevel);
    params.inputSize.x = BE1::Max(params.inputSize.x, 1u);
    params.inputSize.y = BE1::Max(params.inputSize.y, 1u);
    params.outputSize = uint2(data.textureSet.outputTexture->GetWidth(), data.textureSet.outputTexture->GetHeight());
    params.sourceType = sourceType;
    params.mipLevel = mipLevel;
    params.linearDepthScale = 1.0f / BE1::Max(settings.aoRadius * 16.0f, 1.0f);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute, 0, mipLevel),
        RHI::Renderer::MakeTextureBarrier(data.textureSet.outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, frameGTAODebugPSO);
    RHI::renderer->SetTexture(commandList, 0, false, inputTexture, inputViewID);
    RHI::renderer->SetTexture(commandList, 0, true, data.textureSet.outputTexture);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

    RHI::renderer->Dispatch(commandList, BE1::DivCeil(params.outputSize.x, GTAOMainBlockSizeX), BE1::DivCeil(params.outputSize.y, GTAOMainBlockSizeY), 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, mipLevel),
        RHI::Renderer::MakeTextureBarrier(data.textureSet.outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}

void ScreenSpaceAO::DispatchAOUpsample(RenderBackend &backend, RHI::Texture *depthTexture, RHI::Texture *inputHalfAOTexture, RHI::Texture *inputHalfDepthTexture, RHI::Texture *outputAOTexture, float radius) const {
    PROFILER_CPU_SCOPED_EVENT("ScreenSpaceAO::DispatchAOUpsample");

    RHI::CommandList *commandList = backend.GetCommandList();
    const RHI::ConstantBuffer *cameraConstantBuffer = backend.GetCameraConstantBuffer();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    const FrameScreenSpaceAO &data = backend.GetFrameCamera()->ao;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::AOUpsample);

    // 양쪽 depth 는 compute SRV, half AO 는 UAV, 최종 출력은 SRV 상태로 전달받는다.
    RHI::GPUBarrier upsampleBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputHalfAOTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputAOTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, upsampleBarriers, COUNT_OF(upsampleBarriers));

    AOUpsampleParams upsampleParams = {};
    upsampleParams.inputSize = uint2(inputHalfAOTexture->GetWidth(), inputHalfAOTexture->GetHeight());
    upsampleParams.outputSize = uint2(outputAOTexture->GetWidth(), outputAOTexture->GetHeight());
    upsampleParams.depthSigmaRcp = 1.0f / BE1::Max(radius * 0.1f, 1.0e-5f);
    upsampleParams.clearDepth = data.projectionPolicy.GetDepthClearValue();

    RHI::renderer->SetPSO(commandList, frameAOUpsamplePSO);
    RHI::renderer->SetTexture(commandList, 0, false, inputHalfAOTexture);
    RHI::renderer->SetTexture(commandList, 1, false, inputHalfDepthTexture);
    RHI::renderer->SetTexture(commandList, 2, false, depthTexture);
    RHI::renderer->SetTexture(commandList, 0, true, outputAOTexture);
    RHI::renderer->SetConstants(commandList, 0, &upsampleParams, sizeof(upsampleParams), 0);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(commandList, BE1::DivCeil(upsampleParams.outputSize.x, AOUpsampleBlockSizeX), BE1::DivCeil(upsampleParams.outputSize.y, AOUpsampleBlockSizeY), 1);

    RHI::GPUBarrier halfEndBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputHalfAOTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(inputHalfDepthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, halfEndBarriers, COUNT_OF(halfEndBarriers));
}
