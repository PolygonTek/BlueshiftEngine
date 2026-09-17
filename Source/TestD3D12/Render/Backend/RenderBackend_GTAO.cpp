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
#include "RenderBackend.h"
#include "Render/RenderFrameData.h"
#include "Render/RenderInternal.h"
#include "Shaders/ShaderInterop.h"

bool RenderBackend::DispatchGTAO(RHI::GPUResourceState renderDepthState) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchGTAO");

    RHI::Texture *depthTexture = currentVisCamera->aoDepthTexture;
    RHI::Texture *normalTexture = currentVisCamera->rtNormalTexture;
    RHI::Texture *aoTexture = currentVisCamera->aoTexture;
    RHI::Texture *rawAOTexture = currentVisCamera->aoRawTexture;
    RHI::Texture *tempAOTexture = currentVisCamera->aoTempTexture;
    RHI::Texture *halfDepthTexture = currentVisCamera->aoHalfDepthTexture;
    RHI::Texture *prefilterDepthTexture = currentVisCamera->gtaoPrefilterDepthTexture;
    RHI::Texture *packedEdgesTexture = currentVisCamera->gtaoPackedEdgesTexture;
    const uint32_t mipCount = currentVisCamera->gtaoPrefilterDepthMipCount;

    if (!currentVisCamera->needDepthNormalOutput || !currentVisCamera->depthTexture || !depthTexture || !normalTexture ||
        !aoTexture || !rawAOTexture || !prefilterDepthTexture || !packedEdgesTexture || mipCount == 0 ||
        !frameGTAOPrefilterPSO || !frameGTAOMainPSO || !frameGTAODenoisePSO) {
        return false;
    }

    const uint32_t width = aoTexture->GetWidth();
    const uint32_t height = aoTexture->GetHeight();
    const uint32_t halfWidth = BE1::DivCeil(width, 2u);
    const uint32_t halfHeight = BE1::DivCeil(height, 2u);
    const auto matchesSize = [](const RHI::Texture *texture, uint32_t expectedWidth, uint32_t expectedHeight) {
        return texture && texture->GetWidth() == expectedWidth && texture->GetHeight() == expectedHeight;
    };

    if (!matchesSize(depthTexture, width, height) || !matchesSize(normalTexture, width, height) ||
        prefilterDepthTexture->GetWidth() != rawAOTexture->GetWidth() || prefilterDepthTexture->GetHeight() != rawAOTexture->GetHeight() ||
        !matchesSize(packedEdgesTexture, rawAOTexture->GetWidth(), rawAOTexture->GetHeight())) {
        return false;
    }

    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    const bool useHalfResolution = settings.aoHalfResolution && frameAOUpsamplePSO && matchesSize(rawAOTexture, halfWidth, halfHeight) && matchesSize(halfDepthTexture, halfWidth, halfHeight);
    const uint32_t processWidth = rawAOTexture->GetWidth();
    const uint32_t processHeight = rawAOTexture->GetHeight();
    const uint32_t denoisePassCount = BE1::Max(settings.gtaoDenoisePasses, 1u);
    const bool needTempTexture = useHalfResolution || denoisePassCount > 1;
    if ((useHalfResolution ? (processWidth != halfWidth || processHeight != halfHeight) : (processWidth != width || processHeight != height)) ||
        (needTempTexture && !matchesSize(tempAOTexture, processWidth, processHeight))) {
        return false;
    }

    const bool useMSAA = currentVisCamera->depthTexture->GetSampleCount() > 1;
    if (useMSAA) {
        RHI::Texture *normalMSAATexture = currentVisCamera->rtNormalMSAATexture;
        if (!frameDepthNormalMSAAResolvePSO || !matchesSize(normalMSAATexture, width, height) ||
            normalMSAATexture->GetSampleCount() != currentVisCamera->depthTexture->GetSampleCount() ||
            !DispatchDepthMSAAResolve(depthTexture, renderDepthState, normalTexture)) {
            return false;
        }
    }

    const RHI::GPUResourceState depthInputState = useMSAA ? RHI::GPUResourceState::ShaderResource : renderDepthState;
    DispatchGTAOPrefilterDepth(depthInputState, useHalfResolution);

    DispatchGTAOMain(useHalfResolution);

    RHI::GPUResourceState rawState = RHI::GPUResourceState::ShaderResourceCompute;
    RHI::GPUResourceState tempState = RHI::GPUResourceState::ShaderResource;
    RHI::Texture *denoiseSource = DispatchGTAODenoise(denoisePassCount, useHalfResolution, rawState, tempState);

    if (useHalfResolution) {
        DispatchAOUpsample(depthTexture, denoiseSource, halfDepthTexture, aoTexture, settings.aoRadius);
        if (denoiseSource == rawAOTexture) {
            rawState = RHI::GPUResourceState::ShaderResource;
        } else {
            tempState = RHI::GPUResourceState::ShaderResource;
        }
    }

    RHI::GPUBarrier finalBarriers[RenderGTAODepthMipMaxLevelCount + 7];
    uint32_t finalBarrierCount = 0;
    finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, depthInputState);
    finalBarriers[finalBarrierCount++] = RHI::Renderer::MakeTextureBarrier(normalTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
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
    RHI::renderer->Barrier(mainCommandList, finalBarriers, finalBarrierCount);

    return true;
}

void RenderBackend::DispatchGTAOPrefilterDepth(RHI::GPUResourceState depthInputState, bool useHalfResolution) {
    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    RHI::Texture *depthTexture = currentVisCamera->aoDepthTexture;
    RHI::Texture *aoTexture = currentVisCamera->aoTexture;
    RHI::Texture *rawAOTexture = currentVisCamera->aoRawTexture;
    RHI::Texture *halfDepthTexture = currentVisCamera->aoHalfDepthTexture;
    RHI::Texture *prefilterDepthTexture = currentVisCamera->gtaoPrefilterDepthTexture;
    const uint32_t mipCount = currentVisCamera->gtaoPrefilterDepthMipCount;
    const uint32_t width = aoTexture->GetWidth();
    const uint32_t height = aoTexture->GetHeight();
    const uint32_t processWidth = rawAOTexture->GetWidth();
    const uint32_t processHeight = rawAOTexture->GetHeight();

    GTAOPrefilterParams prefilterParams = {};
    prefilterParams.inputSize = uint2(width, height);
    prefilterParams.outputSize = uint2(processWidth, processHeight);
    prefilterParams.resolutionScale = useHalfResolution ? 2u : 1u;
    prefilterParams.useMaxDepth = currentVisCamera->projectionPolicy.reverseZ ? 1u : 0u;
    prefilterParams.depthMipCount = mipCount;
    prefilterParams.radius = settings.aoRadius;
    prefilterParams.falloffRange = settings.gtaoFalloffRange;
    prefilterParams.radiusMultiplier = settings.gtaoRadiusMultiplier;
    prefilterParams.farPlaneDeviceDepth = currentVisCamera->projectionPolicy.GetDepthClearValue();

    RHI::Texture *selectedDepthOutputTexture = useHalfResolution ? halfDepthTexture : rawAOTexture;
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::GTAOPrefilter);

    RHI::GPUBarrier startBarriers[GTAODepthMipLevelCount + 2];
    uint32_t startBarrierCount = 0;
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, depthInputState, RHI::GPUResourceState::ShaderResourceCompute);
    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(prefilterDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, mipLevel);
    }
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(selectedDepthOutputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    RHI::renderer->Barrier(mainCommandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, frameGTAOPrefilterPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture);
    for (uint32_t slot = 0; slot < GTAODepthMipLevelCount; ++slot) {
        const uint32_t mipLevel = BE1::Min(slot, mipCount - 1u);
        RHI::renderer->SetTexture(mainCommandList, slot, true, prefilterDepthTexture, currentVisCamera->gtaoPrefilterDepthMipUAVs[mipLevel]);
    }
    RHI::renderer->SetTexture(mainCommandList, GTAODepthMipLevelCount, true, selectedDepthOutputTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &prefilterParams, sizeof(prefilterParams), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
    RHI::renderer->Dispatch(mainCommandList,
        BE1::DivCeil(processWidth, GTAOPrefilterBlockSizeX * 2u),
        BE1::DivCeil(processHeight, GTAOPrefilterBlockSizeY * 2u), 1);

    RHI::GPUBarrier endBarriers[GTAODepthMipLevelCount + 1];
    uint32_t endBarrierCount = 0;
    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(prefilterDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, 0, mipLevel);
    }
    if (useHalfResolution) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(selectedDepthOutputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
    }
    RHI::renderer->Barrier(mainCommandList, endBarriers, endBarrierCount);
}

void RenderBackend::DispatchGTAOMain(bool useHalfResolution) {
    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    RHI::Texture *depthTexture = currentVisCamera->aoDepthTexture;
    RHI::Texture *normalTexture = currentVisCamera->rtNormalTexture;
    RHI::Texture *aoTexture = currentVisCamera->aoTexture;
    RHI::Texture *rawAOTexture = currentVisCamera->aoRawTexture;
    RHI::Texture *prefilterDepthTexture = currentVisCamera->gtaoPrefilterDepthTexture;
    RHI::Texture *packedEdgesTexture = currentVisCamera->gtaoPackedEdgesTexture;
    const uint32_t mipCount = currentVisCamera->gtaoPrefilterDepthMipCount;
    const uint32_t width = aoTexture->GetWidth();
    const uint32_t height = aoTexture->GetHeight();
    const uint32_t processWidth = rawAOTexture->GetWidth();
    const uint32_t processHeight = rawAOTexture->GetHeight();

    GTAOMainParams mainParams = {};
    mainParams.inputSize = uint2(width, height);
    mainParams.outputSize = uint2(processWidth, processHeight);
    mainParams.outputSizeRcp = float2(1.0f / processWidth, 1.0f / processHeight);
    mainParams.resolutionScale = useHalfResolution ? 2u : 1u;
    mainParams.noiseIndex = currentVisCamera->aoTemporalNoise ? frameData->GetFrameCount() & 63u : 0u;
    mainParams.depthMipCount = mipCount;
    mainParams.radius = settings.aoRadius;
    mainParams.falloffRange = settings.gtaoFalloffRange;
    mainParams.radiusMultiplier = settings.gtaoRadiusMultiplier;
    mainParams.sampleDistributionPower = settings.gtaoSampleDistributionPower;
    mainParams.thinOccluderCompensation = settings.gtaoThinOccluderCompensation;
    mainParams.depthMipSamplingOffset = settings.gtaoDepthMipSamplingOffset;
    mainParams.power = settings.gtaoPower;
    mainParams.farPlaneDeviceDepth = currentVisCamera->projectionPolicy.GetDepthClearValue();

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::GTAOMain);

    RHI::GPUBarrier mainStartBarriers[3];
    uint32_t mainStartBarrierCount = 0;
    mainStartBarriers[mainStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(normalTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    // Full-resolution raw AO is already a UAV because it served as the unused selected-depth binding.
    // Half-resolution raw AO has not participated in the prefilter and still needs its first transition.
    if (useHalfResolution) {
        mainStartBarriers[mainStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(rawAOTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }
    mainStartBarriers[mainStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(packedEdgesTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    RHI::renderer->Barrier(mainCommandList, mainStartBarriers, mainStartBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, frameGTAOMainPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, normalTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, prefilterDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, rawAOTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, true, packedEdgesTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &mainParams, sizeof(mainParams), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(mainCommandList, BE1::DivCeil(processWidth, GTAOMainBlockSizeX), BE1::DivCeil(processHeight, GTAOMainBlockSizeY), 1);

    RHI::GPUBarrier mainEndBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(rawAOTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(packedEdgesTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute)
    };
    RHI::renderer->Barrier(mainCommandList, mainEndBarriers, COUNT_OF(mainEndBarriers));
}

RHI::Texture *RenderBackend::DispatchGTAODenoise(uint32_t denoisePassCount, bool useHalfResolution, RHI::GPUResourceState &rawAOState, RHI::GPUResourceState &tempAOState) {
    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    RHI::Texture *aoTexture = currentVisCamera->aoTexture;
    RHI::Texture *rawAOTexture = currentVisCamera->aoRawTexture;
    RHI::Texture *tempAOTexture = currentVisCamera->aoTempTexture;
    RHI::Texture *packedEdgesTexture = currentVisCamera->gtaoPackedEdgesTexture;
    const uint32_t processWidth = rawAOTexture->GetWidth();
    const uint32_t processHeight = rawAOTexture->GetHeight();
    RHI::Texture *denoiseSource = rawAOTexture;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::GTAODenoise);

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

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(denoiseOutput, outputBeforeState, RHI::GPUResourceState::UnorderedAccess));
        if (outputState) {
            *outputState = RHI::GPUResourceState::UnorderedAccess;
        }

        GTAODenoiseParams denoiseParams = {};
        denoiseParams.outputSize = uint2(processWidth, processHeight);
        denoiseParams.outputSizeRcp = float2(1.0f / processWidth, 1.0f / processHeight);
        denoiseParams.blurBeta = settings.gtaoDenoisePasses == 0 ? 10000.0f : 1.2f;
        denoiseParams.finalPass = finalPass ? 1u : 0u;

        RHI::renderer->SetPSO(mainCommandList, frameGTAODenoisePSO);
        RHI::renderer->SetTexture(mainCommandList, 0, false, denoiseSource);
        RHI::renderer->SetTexture(mainCommandList, 1, false, packedEdgesTexture);
        RHI::renderer->SetTexture(mainCommandList, 0, true, denoiseOutput);
        RHI::renderer->SetConstants(mainCommandList, 0, &denoiseParams, sizeof(denoiseParams), 0);

        RHI::renderer->Dispatch(mainCommandList, BE1::DivCeil(processWidth, GTAODenoiseBlockSizeX * 2u), BE1::DivCeil(processHeight, GTAODenoiseBlockSizeY), 1);

        if (!finalPass) {
            RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(denoiseOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute));
            *outputState = RHI::GPUResourceState::ShaderResourceCompute;
        }
        denoiseSource = denoiseOutput;
    }

    return denoiseSource;
}

bool RenderBackend::DispatchGTAODebugPreview() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchGTAODebugPreview");

    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    if (!frameGTAODebugPSO || !currentVisCamera->aoTexture || settings.gtaoDebug <= 1) {
        return settings.gtaoDebug <= 1;
    }

    RHI::Texture *inputTexture = nullptr;
    RHI::ViewID inputViewID = RHI::ViewID::PrimaryID;
    uint32_t sourceType = 0;
    uint32_t mipLevel = 0;
    if (settings.gtaoDebug == 2) {
        inputTexture = currentVisCamera->aoRawTexture;
    } else if (settings.gtaoDebug == 3) {
        inputTexture = currentVisCamera->gtaoPackedEdgesTexture;
        sourceType = 1;
    } else {
        if (!currentVisCamera->gtaoPrefilterDepthTexture || currentVisCamera->gtaoPrefilterDepthMipCount == 0) {
            return false;
        }
        mipLevel = BE1::Min(settings.gtaoDebug - 4u, currentVisCamera->gtaoPrefilterDepthMipCount - 1u);
        inputTexture = currentVisCamera->gtaoPrefilterDepthTexture;
        inputViewID = currentVisCamera->gtaoPrefilterDepthMipSRVs[mipLevel];
        sourceType = 2;
    }

    if (!inputTexture) {
        return false;
    }

    GTAODebugParams params = {};
    params.inputSize = uint2(inputTexture->GetWidth() >> mipLevel, inputTexture->GetHeight() >> mipLevel);
    params.inputSize.x = BE1::Max(params.inputSize.x, 1u);
    params.inputSize.y = BE1::Max(params.inputSize.y, 1u);
    params.outputSize = uint2(currentVisCamera->aoTexture->GetWidth(), currentVisCamera->aoTexture->GetHeight());
    params.sourceType = sourceType;
    params.mipLevel = mipLevel;
    params.linearDepthScale = 1.0f / BE1::Max(settings.aoRadius * 16.0f, 1.0f);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute, 0, mipLevel),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->aoTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameGTAODebugPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture, inputViewID);
    RHI::renderer->SetTexture(mainCommandList, 0, true, currentVisCamera->aoTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);

    RHI::renderer->Dispatch(mainCommandList, BE1::DivCeil(params.outputSize.x, GTAOMainBlockSizeX), BE1::DivCeil(params.outputSize.y, GTAOMainBlockSizeY), 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, mipLevel),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->aoTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}
