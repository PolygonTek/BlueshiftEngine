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
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"
#include "Render/RenderInternal.h"
#include "Render/RenderFrameData.h"
#include "Shaders/ShaderInterop.h"

// 히스토그램 기반 luminance 계산: https://www.alextardif.com/HistogramLuminance.html
bool RenderBackend::DispatchLuminance() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchLuminance");

    const RenderFrameSettings &renderFrameSettings = frameData->GetRenderFrameSettings();
    if (!renderFrameSettings.toneMapping) {
        return false;
    }

    if (!frameLuminancePass1PSO || !frameLuminancePass2PSO) {
        return false;
    }

    const uint32_t inputW = currentVisCamera->renderWidth;
    const uint32_t inputH = currentVisCamera->renderHeight;
    if (inputW == 0 || inputH == 0) {
        return false;
    }

    RHI::Buffer *luminanceBuffer = currentVisCamera->luminanceBuffer;
    if (!luminanceBuffer) {
        return false;
    }

    const uint32_t sampleW = BE1::Max(inputW / 2, 1u);
    const uint32_t sampleH = BE1::Max(inputH / 2, 1u);

    const float lumLogMin = renderFrameSettings.toneMapLuminanceLogMin;
    const float lumLogMax = renderFrameSettings.toneMapLuminanceLogMax;
    const float lumLogRange = lumLogMax - lumLogMin;

    LuminancePass1Params luminancePass1Params = {};
    luminancePass1Params.sampleSize = uint2(sampleW, sampleH);
    luminancePass1Params.sampleSizeRcp = float2(1.0f / sampleW, 1.0f / sampleH);
    luminancePass1Params.luminanceLogMin = lumLogMin;
    luminancePass1Params.luminanceLogRangeRcp = 1.0f / lumLogRange;

    LuminancePass2Params luminancePass2Params = {};
    luminancePass2Params.luminanceLogMin = lumLogMin;
    luminancePass2Params.luminanceLogRange = lumLogRange;
    luminancePass2Params.luminancePixelCount = static_cast<float>(sampleW * sampleH);
    luminancePass2Params.luminanceAdaptationRate = renderFrameSettings.toneMapLuminanceAdaptationRate;
    luminancePass2Params.middleGray = renderFrameSettings.toneMapMiddleGray;
    luminancePass2Params.exposureCompensation = renderFrameSettings.toneMapExposureCompensation;
    const float minExposureEV = renderFrameSettings.toneMapMinExposureEV;
    const float maxExposureEV = renderFrameSettings.toneMapMaxExposureEV;
    luminancePass2Params.minExposure = BE1::Math::Pow(2.0f, minExposureEV);
    luminancePass2Params.maxExposure = BE1::Math::Pow(2.0f, maxExposureEV);

    RHI::Texture *inputTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;

    // Pass 1 : Log Luminance 히스토그램 계산
    {
        GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::LuminancePass1);

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute));

        RHI::renderer->SetPSO(mainCommandList, frameLuminancePass1PSO);
        RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
        RHI::renderer->SetBuffer(mainCommandList, 0, true, luminanceBuffer);
        RHI::renderer->SetConstants(mainCommandList, 1, &luminancePass1Params, sizeof(luminancePass1Params), 0);

        const uint32_t threadGroupCountX = BE1::DivCeil(luminancePass1Params.sampleSize.x, LuminanceBlockSize);
        const uint32_t threadGroupCountY = BE1::DivCeil(luminancePass1Params.sampleSize.y, LuminanceBlockSize);
        RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);
    }

    // Pass 2 : 평균 Luminance 를 계산하고, 보간된 exposure 도 계산한다.
    {
        GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::LuminancePass2);

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeMemoryBarrier(luminanceBuffer));

        RHI::renderer->SetPSO(mainCommandList, frameLuminancePass2PSO);
        RHI::renderer->SetBuffer(mainCommandList, 0, true, luminanceBuffer);
        RHI::renderer->SetConstantBuffer(mainCommandList, 0, frameConstantBuffer);
        RHI::renderer->SetConstants(mainCommandList, 1, &luminancePass2Params, sizeof(luminancePass2Params), 0);

        RHI::renderer->Dispatch(mainCommandList, 1, 1, 1);

        RHI::GPUBarrier endBarriers[] = {
            RHI::Renderer::MakeMemoryBarrier(luminanceBuffer),
            RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
        };
        RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));
    }

    return true;
}

bool RenderBackend::DispatchSurfaceOutputMSAAResolve() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSurfaceOutputMSAAResolve");

    assert(currentVisCamera->rtColorMSAATexture);
    assert(currentVisCamera->needNormalOutput || currentVisCamera->needVelocityOutput);

    RHI::PipelineState *resolvePSO = nullptr;
    if (currentVisCamera->needNormalOutput) {
        resolvePSO = currentVisCamera->needVelocityOutput ? frameSurfaceOutputMSAAResolveNormalVelocityPSO : frameSurfaceOutputMSAAResolveNormalPSO;
    } else {
        resolvePSO = frameSurfaceOutputMSAAResolveVelocityPSO;
    }

    if (!resolvePSO) {
        return false;
    }

    SurfaceOutputMSAAResolveParams resolveParams = {};
    resolveParams.outputSize = uint2(currentVisCamera->renderWidth, currentVisCamera->renderHeight);
    resolveParams.useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 0u : 1u;

    RHI::ConstantBuffer *resolveConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(SurfaceOutputMSAAResolveParams), &resolveParams);
    if (!resolveConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[5];
    uint32_t startBarrierCount = 0;
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->depthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);

    if (currentVisCamera->needNormalOutput) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->rtNormalMSAATexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->rtNormalTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }
    if (currentVisCamera->needVelocityOutput) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->rtVelocityMSAATexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->rtVelocityTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SurfaceOutputMSAAResolve);
    RHI::renderer->Barrier(mainCommandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, resolvePSO);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, resolveConstantBuffer);

    uint32_t srvSlot = 0;
    uint32_t uavSlot = 0;
    RHI::renderer->SetTexture(mainCommandList, srvSlot++, false, currentVisCamera->depthTexture);

    if (currentVisCamera->needNormalOutput) {
        RHI::renderer->SetTexture(mainCommandList, srvSlot++, false, currentVisCamera->rtNormalMSAATexture);
        RHI::renderer->SetTexture(mainCommandList, uavSlot++, true, currentVisCamera->rtNormalTexture);
    }
    if (currentVisCamera->needVelocityOutput) {
        RHI::renderer->SetTexture(mainCommandList, srvSlot++, false, currentVisCamera->rtVelocityMSAATexture);
        RHI::renderer->SetTexture(mainCommandList, uavSlot++, true, currentVisCamera->rtVelocityTexture);
    }

    const uint32_t threadGroupCountX = BE1::DivCeil(resolveParams.outputSize.x, SurfaceOutputMSAAResolveBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(resolveParams.outputSize.y, SurfaceOutputMSAAResolveBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[5];
    uint32_t endBarrierCount = 0;
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->depthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    if (currentVisCamera->needNormalOutput) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->rtNormalMSAATexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->rtNormalTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
    }
    if (currentVisCamera->needVelocityOutput) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->rtVelocityMSAATexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(currentVisCamera->rtVelocityTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
    }
    RHI::renderer->Barrier(mainCommandList, endBarriers, endBarrierCount);

    return true;
}

bool RenderBackend::DispatchCurrentDepthMSAAResolve() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchCurrentDepthMSAAResolve");

    assert(currentVisCamera->taaResolve);
    return DispatchDepthMSAAResolve(currentVisCamera->depthHistoryTextures[currentVisCamera->currentHistoryTextureIndex], RHI::GPUResourceState::ShaderResource);
}

bool RenderBackend::DispatchDepthMSAAResolve(RHI::Texture *outputDepthTexture, RHI::GPUResourceState inputState, RHI::Texture *outputNormalTexture) {
    assert(currentVisCamera->depthTexture->GetSampleCount() > 1);

    RHI::Texture *inputDepthTexture = currentVisCamera->depthTexture;
    if (!outputDepthTexture) {
        return false;
    }
    assert(outputDepthTexture->GetSampleCount() == 1);
    assert(outputDepthTexture->GetWidth() == inputDepthTexture->GetWidth() && outputDepthTexture->GetHeight() == inputDepthTexture->GetHeight());

    const uint32_t outputW = outputDepthTexture->GetWidth();
    const uint32_t outputH = outputDepthTexture->GetHeight();
    if (outputW == 0 || outputH == 0) {
        return false;
    }

    RHI::Texture *inputNormalTexture = outputNormalTexture ? currentVisCamera->rtNormalMSAATexture : nullptr;
    if (outputNormalTexture) {
        if (!inputNormalTexture) {
            return false;
        }
        assert(inputNormalTexture->GetSampleCount() == inputDepthTexture->GetSampleCount());
        assert(inputNormalTexture->GetWidth() == outputW && inputNormalTexture->GetHeight() == outputH);
        assert(outputNormalTexture->GetSampleCount() == 1);
        assert(outputNormalTexture->GetWidth() == outputW && outputNormalTexture->GetHeight() == outputH);
    }

    RHI::PipelineState *depthMSAAResolvePSO = outputNormalTexture ? frameDepthNormalMSAAResolvePSO : frameDepthMSAAResolvePSO;
    if (!depthMSAAResolvePSO) {
        return false;
    }

    DepthMSAAResolveParams depthMSAAResolveParams = {};
    depthMSAAResolveParams.outputSize = uint2(outputW, outputH);
    depthMSAAResolveParams.useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 0u : 1u;

    RHI::ConstantBuffer *depthMSAAResolveConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(DepthMSAAResolveParams), &depthMSAAResolveParams);
    if (!depthMSAAResolveConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[4] = {
        RHI::Renderer::MakeTextureBarrier(inputDepthTexture, inputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    uint32_t startBarrierCount = 2;
    if (outputNormalTexture) {
        // The prepass leaves its MSAA normal attachment readable; SSAO consumes the resolved pair.
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(inputNormalTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(outputNormalTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::DepthMSAAResolve);

    RHI::renderer->Barrier(mainCommandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, depthMSAAResolvePSO);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, depthMSAAResolveConstantBuffer);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputDepthTexture);
    if (outputNormalTexture) {
        RHI::renderer->SetTexture(mainCommandList, 1, false, inputNormalTexture);
        RHI::renderer->SetTexture(mainCommandList, 1, true, outputNormalTexture);
    }

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, DepthMSAAResolveBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, DepthMSAAResolveBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[4] = {
        RHI::Renderer::MakeTextureBarrier(inputDepthTexture, RHI::GPUResourceState::ShaderResourceCompute, inputState),
        RHI::Renderer::MakeTextureBarrier(outputDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    uint32_t endBarrierCount = 2;
    if (outputNormalTexture) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(inputNormalTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(outputNormalTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
    }
    RHI::renderer->Barrier(mainCommandList, endBarriers, endBarrierCount);

    return true;
}

bool RenderBackend::DispatchTAAResolve() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchTAAResolve");

    if (!currentVisCamera->taaResolve || !frameTAAResolvePSO || !cameraConstantBuffer) {
        return false;
    }

    const uint32_t outputW = currentVisCamera->renderWidth;
    const uint32_t outputH = currentVisCamera->renderHeight;
    if (outputW == 0 || outputH == 0) {
        return false;
    }

    RHI::Texture *inputTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;
    RHI::Texture *velocityTexture = currentVisCamera->rtVelocityTexture;
    RHI::Texture *depthTexture = currentVisCamera->depthHistoryTextures[currentVisCamera->currentHistoryTextureIndex];
    const bool historyValid = currentVisCamera->historyValid;
    RHI::Texture *previousColorHistoryTexture = historyValid ? currentVisCamera->colorHistoryTextures[currentVisCamera->currentHistoryTextureIndex ^ 1] : inputTexture;
    RHI::Texture *previousDepthTexture = historyValid ? currentVisCamera->depthHistoryTextures[currentVisCamera->currentHistoryTextureIndex ^ 1] : depthTexture;
    RHI::Texture *outputColorHistoryTexture = currentVisCamera->colorHistoryTextures[currentVisCamera->currentHistoryTextureIndex];
    if (!inputTexture || !velocityTexture || !depthTexture || !previousColorHistoryTexture || !previousDepthTexture || !outputColorHistoryTexture) {
        return false;
    }

    const RenderFrameSettings &renderFrameSettings = frameData->GetRenderFrameSettings();
    TAAResolveParams taaResolveParams = {};
    taaResolveParams.outputSize = uint2(outputW, outputH);
    taaResolveParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    taaResolveParams.historyWeight = renderFrameSettings.taaHistoryWeight;
    taaResolveParams.historyValid = historyValid ? 1u : 0u;
    taaResolveParams.useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 0u : 1u;
    taaResolveParams.depthFallbackThreshold = renderFrameSettings.taaDepthFallbackThreshold;
    taaResolveParams.maxSubpixelCurrentWeight = renderFrameSettings.taaSubpixelWeight;
    taaResolveParams.luminanceWeight = renderFrameSettings.taaLuminanceWeight;
    taaResolveParams.colorBoxSigma = renderFrameSettings.taaColorBoxSigma;

    RHI::ConstantBuffer *taaResolveConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(TAAResolveParams), &taaResolveParams);
    if (!taaResolveConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[6];
    uint32_t startBarrierCount = 0;
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(velocityTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    if (historyValid) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(previousColorHistoryTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(previousDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    }
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(outputColorHistoryTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::TAAResolve);

    RHI::renderer->Barrier(mainCommandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, frameTAAResolvePSO);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, taaResolveConstantBuffer);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, velocityTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 3, false, previousColorHistoryTexture);
    RHI::renderer->SetTexture(mainCommandList, 4, false, previousDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputColorHistoryTexture);

    const uint32_t threadGroupCountX = BE1::DivCeil(taaResolveParams.outputSize.x, TAAResolveBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(taaResolveParams.outputSize.y, TAAResolveBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[6];
    uint32_t endBarrierCount = 0;
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(velocityTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);

    if (historyValid) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(previousColorHistoryTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(previousDepthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    }

    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(outputColorHistoryTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
    RHI::renderer->Barrier(mainCommandList, endBarriers, endBarrierCount);

    return true;
}

bool RenderBackend::DispatchBloom() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchBloom");

    const RenderBloomMode bloomMode = frameData->GetRenderFrameSettings().bloomMode;
    switch (bloomMode) {
    case RenderBloomMode::Pyramid:
        return DispatchPyramidBloom();
    case RenderBloomMode::Convolution:
        return DispatchConvolutionBloom();
    default:
        return false;
    }
}

bool RenderBackend::DispatchPyramidBloom() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchPyramidBloom");

    if (currentVisCamera->bloomMipCount < 2) {
        return false;
    }

    RHI::Texture *inputColorTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;
    RHI::Texture *bloomDownTexture = currentVisCamera->bloomDownTexture;
    RHI::Texture *bloomUpTexture = currentVisCamera->bloomUpTexture;

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute));

    const RenderFrameSettings &renderFrameSettings = frameData->GetRenderFrameSettings();
    const float bloomThreshold = renderFrameSettings.bloomThreshold;
    const float bloomSoftThreshold = renderFrameSettings.bloomSoftThreshold;
    const float pyramidBloomPrefilterQuality = renderFrameSettings.pyramidBloomPrefilterQuality;
    const float pyramidBloomScatter = renderFrameSettings.pyramidBloomScatter;
    const float pyramidBloomPrefilterRadius = renderFrameSettings.pyramidBloomPrefilterRadius;
    const float pyramidBloomDownsampleRadius = renderFrameSettings.pyramidBloomDownsampleRadius;
    const float pyramidBloomUpsampleRadius = renderFrameSettings.pyramidBloomUpsampleRadius;

    auto dispatchDownsamplePass = [this](GpuTiming::Tag timingTag, RHI::Texture *inputTexture, RHI::ViewID inputViewID, uint32_t inputWidth, uint32_t inputHeight, uint32_t outputMipLevel, float threshold, float softThreshold, float prefilterQuality, float radius) -> bool {
        const uint32_t outputWidth = currentVisCamera->bloomMipWidths[outputMipLevel];
        const uint32_t outputHeight = currentVisCamera->bloomMipHeights[outputMipLevel];
        if (inputWidth == 0 || inputHeight == 0 || outputWidth == 0 || outputHeight == 0) {
            return false;
        }

        BloomParams bloomParams = {};
        bloomParams.inputSize = uint2(inputWidth, inputHeight);
        bloomParams.outputSize = uint2(outputWidth, outputHeight);
        bloomParams.inputTexelSize = float2(1.0f / inputWidth, 1.0f / inputHeight);
        bloomParams.outputTexelSize = float2(1.0f / outputWidth, 1.0f / outputHeight);
        bloomParams.params0 = float4(threshold, softThreshold, prefilterQuality, radius);

        RHI::ConstantBuffer *bloomConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(BloomParams), &bloomParams);
        if (!bloomConstantBuffer) {
            return false;
        }

        GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, timingTag);

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(currentVisCamera->bloomDownTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel));

        RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture, inputViewID);
        RHI::renderer->SetTexture(mainCommandList, 0, true, currentVisCamera->bloomDownTexture, currentVisCamera->bloomDownMipUAVs[outputMipLevel]);
        RHI::renderer->SetConstantBuffer(mainCommandList, 0, bloomConstantBuffer);

        const uint32_t threadGroupCountX = BE1::DivCeil(outputWidth, BloomBlockSizeX);
        const uint32_t threadGroupCountY = BE1::DivCeil(outputHeight, BloomBlockSizeY);
        RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(currentVisCamera->bloomDownTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputMipLevel));

        return true;
    };

    // threshold 를 적용해 밝은 영역만 필터링하여 mip-0 에 blur downsample 한다.
    RHI::renderer->SetPSO(mainCommandList, frameBloomPrefilterPSO);
    if (!dispatchDownsamplePass(GpuTiming::Tag::BloomPrefilter, inputColorTexture, RHI::ViewID::PrimaryID, currentVisCamera->renderWidth, currentVisCamera->renderHeight, 0, bloomThreshold, bloomSoftThreshold, pyramidBloomPrefilterQuality, pyramidBloomPrefilterRadius)) {
        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));
        return false;
    }

    // 이후 mip 은 blur downsample 을 반복한다.
    RHI::renderer->SetPSO(mainCommandList, frameBloomDownsamplePSO);
    for (uint32_t mipLevel = 1; mipLevel < currentVisCamera->bloomMipCount; ++mipLevel) {
        const uint32_t inputMipLevel = mipLevel - 1;
        if (!dispatchDownsamplePass(GpuTiming::Tag::BloomDownsample, bloomDownTexture, currentVisCamera->bloomDownMipSRVs[inputMipLevel], currentVisCamera->bloomMipWidths[inputMipLevel], currentVisCamera->bloomMipHeights[inputMipLevel], mipLevel, 0.0f, 0.0f, 0.0f, pyramidBloomDownsampleRadius)) {
            RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));
            return false;
        }
    }

    RHI::renderer->SetPSO(mainCommandList, frameBloomUpsamplePSO);

    auto dispatchUpsamplePass = [this](uint32_t outputMipLevel, RHI::Texture *lowerTexture, RHI::ViewID lowerViewID, uint32_t lowerWidth, uint32_t lowerHeight, float weight, float radius) -> bool {
        const uint32_t outputWidth = currentVisCamera->bloomMipWidths[outputMipLevel];
        const uint32_t outputHeight = currentVisCamera->bloomMipHeights[outputMipLevel];
        if (lowerWidth == 0 || lowerHeight == 0 || outputWidth == 0 || outputHeight == 0) {
            return false;
        }

        BloomParams bloomParams = {};
        bloomParams.inputSize = uint2(lowerWidth, lowerHeight);
        bloomParams.outputSize = uint2(outputWidth, outputHeight);
        bloomParams.inputTexelSize = float2(1.0f / lowerWidth, 1.0f / lowerHeight);
        bloomParams.outputTexelSize = float2(1.0f / outputWidth, 1.0f / outputHeight);
        bloomParams.params0 = float4(0.0f, 0.0f, weight, radius);

        RHI::ConstantBuffer *bloomConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(BloomParams), &bloomParams);
        if (!bloomConstantBuffer) {
            return false;
        }

        GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::BloomUpsample);

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(currentVisCamera->bloomUpTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel));

        RHI::renderer->SetTexture(mainCommandList, 0, false, currentVisCamera->bloomDownTexture, currentVisCamera->bloomDownMipSRVs[outputMipLevel]);
        RHI::renderer->SetTexture(mainCommandList, 1, false, lowerTexture, lowerViewID);
        RHI::renderer->SetTexture(mainCommandList, 0, true, currentVisCamera->bloomUpTexture, currentVisCamera->bloomUpMipUAVs[outputMipLevel]);
        RHI::renderer->SetConstantBuffer(mainCommandList, 0, bloomConstantBuffer);

        const uint32_t threadGroupCountX = BE1::DivCeil(outputWidth, BloomBlockSizeX);
        const uint32_t threadGroupCountY = BE1::DivCeil(outputHeight, BloomBlockSizeY);
        RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(currentVisCamera->bloomUpTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputMipLevel));

        return true;
    };

    // 작은 mip 부터 큰 mip 으로 올라오면서 downsample 결과와 누적 bloom 을 합성한다.
    const uint32_t maxMipLevel = currentVisCamera->bloomMipCount - 1;
    for (uint32_t mipLevel = currentVisCamera->bloomMipCount - 1; mipLevel > 0; --mipLevel) {
        const uint32_t outputMipLevel = mipLevel - 1;
        RHI::Texture *lowerTexture = bloomDownTexture;
        RHI::ViewID lowerViewID = currentVisCamera->bloomDownMipSRVs[mipLevel];
        if (mipLevel < currentVisCamera->bloomMipCount - 1) {
            lowerTexture = bloomUpTexture;
            lowerViewID = currentVisCamera->bloomUpMipSRVs[mipLevel];
        }

        const float bloomWeight = BE1::Math::Pow(pyramidBloomScatter, static_cast<float>(maxMipLevel - mipLevel));

        if (!dispatchUpsamplePass(outputMipLevel, lowerTexture, lowerViewID, currentVisCamera->bloomMipWidths[mipLevel], currentVisCamera->bloomMipHeights[mipLevel], bloomWeight, pyramidBloomUpsampleRadius)) {
            RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));
            return false;
        }
    }

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));

    return true;
}

bool RenderBackend::DispatchConvolutionBloom() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchConvolutionBloom");

    if (currentVisCamera->renderWidth == 0 || currentVisCamera->renderHeight == 0 || currentVisCamera->fftBloomSize == 0) {
        return false;
    }

    // 커널 버퍼 업데이트가 필요한 경우에만 실행
    if (currentVisCamera->fftBloomKernelDirty) {
        if (!currentVisCamera->fftBloomKernelSourceTexture) {
            return false;
        }

        // 커널 이미지로 버퍼 준비 (축소 스케일, 회전)
        if (!DispatchConvolutionBloomKernelPrepare()) {
            return false;
        }

        // 커널 버퍼 FFT
        if (!DispatchConvolutionBloomFFT(currentVisCamera->fftBloomKernelSpatialBuffer, currentVisCamera->fftBloomKernelSpectrumBuffer)) {
            return false;
        }
    }

    // 화면 이미지에 bloom prefilter 를 적용하고, 축소 샘플링에 사용할 밉체인을 만든다.
    if (!DispatchConvolutionBloomScreenPrefilter()) {
        return false;
    }

    // 화면 이미지로 버퍼 준비 (축소 스케일)
    if (!DispatchConvolutionBloomScreenPrepare()) {
        return false;
    }

    // 화면 버퍼 FFT
    if (!DispatchConvolutionBloomFFT(currentVisCamera->fftBloomScreenSpatialBuffer, currentVisCamera->fftBloomScreenSpectrumBuffer)) {
        return false;
    }

    // 화면 스펙트럼과 커널 스펙트럼의 Hadamard 곱 (fftBloomWorkBuffer 에 출력)
    if (!DispatchConvolutionBloomMultiplySpectrum()) {
        return false;
    }

    // 결과를 IFFT
    if (!DispatchConvolutionBloomIFFT(currentVisCamera->fftBloomWorkBuffer, currentVisCamera->fftBloomScreenSpatialBuffer)) {
        return false;
    }

    // IFFT 결과 버퍼를 스크린 half-res 텍스쳐 (fftBloomConvolutionOutputTexture) 에 resolve
    if (!DispatchConvolutionBloomResolve()) {
        return false;
    }

    return true;
}

bool RenderBackend::DispatchConvolutionBloomKernelPrepare() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchConvolutionBloomKernelPrepare");

    if (currentVisCamera->fftBloomSize == 0) {
        return false;
    }

    RHI::Texture *sourceTexture = currentVisCamera->fftBloomKernelSourceTexture;
    RHI::Buffer *spatialBuffer = currentVisCamera->fftBloomKernelSpatialBuffer;
    const uint32_t inputW = sourceTexture->GetWidth();
    const uint32_t inputH = sourceTexture->GetHeight();
    const uint32_t outputW = currentVisCamera->fftBloomSize;
    const uint32_t outputH = currentVisCamera->fftBloomSize;
    if (inputW == 0 || inputH == 0 || outputW == 0 || outputH == 0) {
        return false;
    }

    float rotationSin;
    float rotationCos;
    BE1::Math::SinCos(DEG2RAD(currentVisCamera->fftBloomKernelRotation), rotationSin, rotationCos);

    // 회전과 non-uniform kernel scale 을 적용한 뒤 출력 texel 하나가 덮는 source kernel texel footprint 로 mip level 을 계산한다.
    const float mappedKernelScaleX = BE1::Max(currentVisCamera->fftBloomMappedKernelScale.x, 0.0001f);
    const float mappedKernelScaleY = BE1::Max(currentVisCamera->fftBloomMappedKernelScale.y, 0.0001f);
    const float sourceToOutputScaleX = static_cast<float>(inputW) / static_cast<float>(outputW);
    const float sourceToOutputScaleY = static_cast<float>(inputH) / static_cast<float>(outputH);
    const float sourceFootprintXX = sourceToOutputScaleX * rotationCos / mappedKernelScaleX;
    const float sourceFootprintXY = sourceToOutputScaleY * rotationSin / mappedKernelScaleX;
    const float sourceFootprintYX = sourceToOutputScaleX * rotationSin / mappedKernelScaleY;
    const float sourceFootprintYY = sourceToOutputScaleY * rotationCos / mappedKernelScaleY;
    const float sourceFootprintX = BE1::Math::Sqrt(sourceFootprintXX * sourceFootprintXX + sourceFootprintXY * sourceFootprintXY);
    const float sourceFootprintY = BE1::Math::Sqrt(sourceFootprintYX * sourceFootprintYX + sourceFootprintYY * sourceFootprintYY);
    const float kernelMipLevel = BE1::Max(BE1::Math::Log2(BE1::Max(sourceFootprintX, sourceFootprintY)), 0.0f);

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0 = float4(rotationCos, rotationSin, currentVisCamera->fftBloomMappedKernelScale.x, currentVisCamera->fftBloomMappedKernelScale.y);
    bloomParams.params1.x = kernelMipLevel;

    RHI::ConstantBuffer *bloomConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomKernelPrepare);

    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameConvolutionBloomKernelPreparePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, sourceTexture);
    RHI::renderer->SetBuffer(mainCommandList, 0, true, spatialBuffer);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool RenderBackend::DispatchConvolutionBloomScreenPrefilter() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchConvolutionBloomScreenPrefilter");

    RHI::Texture *inputTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;
    RHI::Texture *outputTexture = currentVisCamera->fftBloomConvolutionOutputTexture;
    const uint32_t inputW = inputTexture ? inputTexture->GetWidth() : 0;
    const uint32_t inputH = inputTexture ? inputTexture->GetHeight() : 0;
    const uint32_t outputW = currentVisCamera->fftBloomConvolutionOutputWidth;
    const uint32_t outputH = currentVisCamera->fftBloomConvolutionOutputHeight;
    const uint32_t mipLevelCount = currentVisCamera->fftBloomConvolutionOutputMipCount;
    if (inputW == 0 || inputH == 0 || outputW == 0 || outputH == 0 || mipLevelCount == 0 || !currentVisCamera->fftBloomConvolutionOutputMipUAVs[0].IsValid()) {
        return false;
    }

    const RenderFrameSettings &renderFrameSettings = frameData->GetRenderFrameSettings();
    const float bloomThreshold = renderFrameSettings.bloomThreshold;
    const float bloomSoftThreshold = renderFrameSettings.bloomSoftThreshold;
    const float convolutionBloomPrefilterRadius = renderFrameSettings.convolutionBloomPrefilterRadius;

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);

    // RotatedGrid5Tap 경로를 사용하여 작은 화면 영역의 고립된 고휘도 샘플을 Karis weight 로 억제한다.
    bloomParams.params0 = float4(bloomThreshold, bloomSoftThreshold, 0.0f, convolutionBloomPrefilterRadius);

    RHI::ConstantBuffer *bloomConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomScreenPrefilter);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, 0)
    };
    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameBloomPrefilterPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputTexture, currentVisCamera->fftBloomConvolutionOutputMipUAVs[0]);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, 0)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    if (mipLevelCount > 1) {
        RHI::renderer->GenerateMipChain(mainCommandList, outputTexture, currentVisCamera->fftBloomConvolutionOutputMipSRVs, currentVisCamera->fftBloomConvolutionOutputMipUAVs, false);
    }

    return true;
}

bool RenderBackend::DispatchConvolutionBloomScreenPrepare() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchConvolutionBloomScreenPrepare");

    if (currentVisCamera->renderWidth == 0 || currentVisCamera->renderHeight == 0 || currentVisCamera->fftBloomSize == 0) {
        return false;
    }

    const uint32_t inputW = currentVisCamera->fftBloomConvolutionOutputWidth;
    const uint32_t inputH = currentVisCamera->fftBloomConvolutionOutputHeight;
    const uint32_t outputW = currentVisCamera->fftBloomSize;
    const uint32_t outputH = currentVisCamera->fftBloomSize;
    const uint32_t mipLevelCount = currentVisCamera->fftBloomConvolutionOutputMipCount;
    const float contentW = BE1::Max(currentVisCamera->fftBloomMappedContentRect.w, 1.0f);
    const float contentH = BE1::Max(currentVisCamera->fftBloomMappedContentRect.h, 1.0f);
    if (inputW == 0 || inputH == 0 || outputW == 0 || outputH == 0 || mipLevelCount == 0) {
        return false;
    }

    const float sourceTexelsPerOutputTexel = BE1::Max(static_cast<float>(inputW) / contentW, static_cast<float>(inputH) / contentH);
    const float screenMipLevel = BE1::Min(BE1::Max(BE1::Math::Log2(sourceTexelsPerOutputTexel), 0.0f), static_cast<float>(mipLevelCount - 1));

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0.z = screenMipLevel;
    bloomParams.params1 = float4(
        currentVisCamera->fftBloomMappedContentRect.x,
        currentVisCamera->fftBloomMappedContentRect.y,
        currentVisCamera->fftBloomMappedContentRect.w,
        currentVisCamera->fftBloomMappedContentRect.h);

    RHI::ConstantBuffer *bloomConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::Texture *inputTexture = currentVisCamera->fftBloomConvolutionOutputTexture;
    RHI::Buffer *outputBuffer = currentVisCamera->fftBloomScreenSpatialBuffer;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomScreenPrepare);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameConvolutionBloomScreenPreparePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
    RHI::renderer->SetBuffer(mainCommandList, 0, true, outputBuffer);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool RenderBackend::DispatchConvolutionBloomMultiplySpectrum() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchConvolutionBloomMultiplySpectrum");

    if (currentVisCamera->renderWidth == 0 || currentVisCamera->renderHeight == 0 || currentVisCamera->fftBloomSize == 0) {
        return false;
    }

    const uint32_t fftSize = currentVisCamera->fftBloomSize;
    constexpr uint32_t ColorPairCount = 2; // RG, B0

    // padding 때문에 줄어든 kernel footprint 의 면적만큼 커널 에너지를 보정한다.
    const float kernelAreaScale = BE1::Max(currentVisCamera->fftBloomMappedKernelScale.x * currentVisCamera->fftBloomMappedKernelScale.y, 0.000001f);
    const float kernelSampleCount = static_cast<float>(fftSize * fftSize) * kernelAreaScale;
    const float kernelNormalizationFactor = currentVisCamera->fftBloomKernelInverseAverageLuminance / kernelSampleCount;
    const float kernelGain = frameData->GetRenderFrameSettings().convolutionBloomKernelGain;
    const float kernelSpectrumScale = kernelGain * kernelNormalizationFactor;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomMultiplySpectrum);

    return renderSystem->gpuFFT.DispatchMultiplyPackedDualRealSpectrum2D(mainCommandList, frameData->GetThreadData(0),
        currentVisCamera->fftBloomKernelSpectrumBuffer,
        currentVisCamera->fftBloomScreenSpectrumBuffer,
        currentVisCamera->fftBloomWorkBuffer,
        fftSize,
        ColorPairCount,
        kernelSpectrumScale);
}

bool RenderBackend::DispatchConvolutionBloomResolve() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchConvolutionBloomResolve");

    if (currentVisCamera->fftBloomSize == 0) {
        return false;
    }

    const uint32_t inputW = currentVisCamera->renderWidth;
    const uint32_t inputH = currentVisCamera->renderHeight;
    const uint32_t fftSize = currentVisCamera->fftBloomSize;
    const uint32_t outputW = currentVisCamera->fftBloomConvolutionOutputWidth;
    const uint32_t outputH = currentVisCamera->fftBloomConvolutionOutputHeight;
    if (inputW == 0 || inputH == 0 || fftSize == 0 || outputW == 0 || outputH == 0) {
        return false;
    }

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(fftSize, fftSize);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params1 = float4(
        currentVisCamera->fftBloomMappedContentRect.x,
        currentVisCamera->fftBloomMappedContentRect.y,
        currentVisCamera->fftBloomMappedContentRect.w,
        currentVisCamera->fftBloomMappedContentRect.h);

    RHI::ConstantBuffer *bloomConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::Buffer *spatialBuffer = currentVisCamera->fftBloomScreenSpatialBuffer;
    RHI::Texture *outputTexture = currentVisCamera->fftBloomConvolutionOutputTexture;

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomResolve);

    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameConvolutionBloomResolvePSO);
    RHI::renderer->SetBuffer(mainCommandList, 0, false, spatialBuffer);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputTexture);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool RenderBackend::DispatchConvolutionBloomFFT(RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) {
    constexpr uint32_t PlaneCount = 2; // RG, B0

    GpuFFT::FFTSize fftSizeEnum;
    if (!GpuFFT::GetGpuFFTSize(currentVisCamera->fftBloomSize, fftSizeEnum)) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomFFT);

    return renderSystem->gpuFFT.DispatchFFTDualReal2D(mainCommandList, inputBuffer, outputBuffer, currentVisCamera->fftBloomWorkBuffer, fftSizeEnum, PlaneCount);
}

bool RenderBackend::DispatchConvolutionBloomIFFT(RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) {
    constexpr uint32_t PlaneCount = 2; // RG, B0

    GpuFFT::FFTSize fftSizeEnum;
    if (!GpuFFT::GetGpuFFTSize(currentVisCamera->fftBloomSize, fftSizeEnum)) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomIFFT);

    return renderSystem->gpuFFT.DispatchIFFTDualReal2D(mainCommandList, inputBuffer, outputBuffer, currentVisCamera->fftBloomScreenSpectrumBuffer, fftSizeEnum, PlaneCount);
}

bool RenderBackend::DispatchFinalPostProcess(bool useToneMapping, bool useBloom) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchFinalPostProcess");

    if (!frameFinalPostProcessPSO) {
        return false;
    }

    const RenderFrameSettings &renderFrameSettings = frameData->GetRenderFrameSettings();

    RHI::Texture *colorGradingLUTTexture = currentVisCamera->colorGradingLUTTexture;
    if (!colorGradingLUTTexture) {
        return false;
    }
    assert(colorGradingLUTTexture->GetStatus() == RHI::GPUResourceStatus::Ready);
    const bool useColorGrading = renderFrameSettings.colorGrading;

    RHI::Texture *bloomDirtMaskTexture = currentVisCamera->bloomDirtMaskTexture;
    const float bloomIntensity = useBloom ? renderFrameSettings.bloomIntensity : 0.0f;
    const float bloomDirtIntensity = renderFrameSettings.bloomDirtIntensity;
    const bool useBloomDirtMask = bloomIntensity > 0.0f && bloomDirtMaskTexture && bloomDirtIntensity > 0.0f;
    if (useBloomDirtMask) {
        assert(bloomDirtMaskTexture->GetStatus() == RHI::GPUResourceStatus::Ready);
    }

    const uint32_t outputW = currentVisCamera->renderWidth;
    const uint32_t outputH = currentVisCamera->renderHeight;
    if (outputW == 0 || outputH == 0) {
        return false;
    }

    FinalPostProcessParams finalPostProcessParams = {};
    finalPostProcessParams.outputSize = uint2(outputW, outputH);
    finalPostProcessParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    finalPostProcessParams.flags = useToneMapping ? FinalPostProcessFlagToneMapping : 0u;
    finalPostProcessParams.toneMappingOperator = renderFrameSettings.toneMappingOperator;
    const RenderContext::DisplayOutputSettings &displayOutputSettings = currentContext->GetDisplayOutputSettings();
    const bool hdrOutput = RHI::SwapChain::IsHDRColorSpace(currentContext->GetSwapChain()->GetColorSpace());
    finalPostProcessParams.paperWhiteNits = displayOutputSettings.paperWhiteNits;
    finalPostProcessParams.maxDisplayLinearValue = displayOutputSettings.maxDisplayLinearValue;
    finalPostProcessParams.bloomIntensity = bloomIntensity;
    finalPostProcessParams.bloomDirtIntensity = useBloomDirtMask ? bloomDirtIntensity : 0.0f;
    finalPostProcessParams.bloomDirtUvScale = float2(1.0f, 1.0f);
    if (useBloomDirtMask) {
        const float outputAspectRatio = static_cast<float>(outputW) / static_cast<float>(outputH);
        const float dirtAspectRatio = static_cast<float>(bloomDirtMaskTexture->GetWidth()) / static_cast<float>(bloomDirtMaskTexture->GetHeight());
        if (outputAspectRatio > dirtAspectRatio) {
            finalPostProcessParams.bloomDirtUvScale.y = dirtAspectRatio / outputAspectRatio;
        } else {
            finalPostProcessParams.bloomDirtUvScale.x = outputAspectRatio / dirtAspectRatio;
        }
    }
    finalPostProcessParams.chromaticAberration = renderFrameSettings.chromaticAberration;
    finalPostProcessParams.vignetteIntensity = renderFrameSettings.vignetteIntensity;
    finalPostProcessParams.lensDistortion = renderFrameSettings.lensDistortion;
    finalPostProcessParams.lensDistortionFitScale = 1.0f - BE1::Max(finalPostProcessParams.lensDistortion, 0.0f);
    const float outputWidth = static_cast<float>(outputW);
    const float outputHeight = static_cast<float>(outputH);
    finalPostProcessParams.lensDistortionRadiusSqScale = 4.0f / (outputWidth * outputWidth + outputHeight * outputHeight);

    // 선택한 색온도의 광원 아래에서 촬영된 white 가 6500K 기준 white 로 보이도록 RGB gain 을 계산한다.
    static const BE1::Color3 referenceWhite = BE1::Color3::FromColorTemperature(6500.0f);
    const BE1::Color3 sourceWhite = BE1::Color3::FromColorTemperature(renderFrameSettings.whiteBalanceTemperature);
    finalPostProcessParams.whiteBalance = float3(
        referenceWhite.r / BE1::Max(sourceWhite.r, referenceWhite.r * 0.01f),
        referenceWhite.g / BE1::Max(sourceWhite.g, referenceWhite.g * 0.01f),
        referenceWhite.b / BE1::Max(sourceWhite.b, referenceWhite.b * 0.01f));

    if (hdrOutput) {
        finalPostProcessParams.flags |= FinalPostProcessFlagHDROutput;
    }
    if (renderFrameSettings.dithering && !hdrOutput) {
        finalPostProcessParams.flags |= FinalPostProcessFlagDithering;
    }
    if (useColorGrading) {
        finalPostProcessParams.flags |= FinalPostProcessFlagColorGrading;
    }

    RHI::ConstantBuffer *finalPostProcessConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(FinalPostProcessParams), &finalPostProcessParams);
    if (!finalPostProcessConstantBuffer) {
        return false;
    }

    RHI::Buffer *luminanceBuffer = currentVisCamera->luminanceBuffer;
    RHI::Texture *inputTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;
    RHI::Texture *outputTexture = currentVisCamera->postProcessColorTexture;
    RHI::Texture *bloomTexture = inputTexture;
    RHI::ViewID bloomViewID = RHI::ViewID::PrimaryID;

    if (useBloom) {
        const bool useConvolutionBloom = renderFrameSettings.bloomMode == RenderBloomMode::Convolution;
        if (useConvolutionBloom && currentVisCamera->fftBloomConvolutionOutputTexture) {
            bloomTexture = currentVisCamera->fftBloomConvolutionOutputTexture;
        } else {
            bloomTexture = currentVisCamera->bloomUpTexture;
            bloomViewID = currentVisCamera->bloomUpMipSRVs[0];
        }
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(luminanceBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::FinalPostProcess);

    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameFinalPostProcessPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
    RHI::renderer->SetBuffer(mainCommandList, 1, false, luminanceBuffer);

    // useBloom 이 꺼져있으면, 더미 텍스쳐로 그냥 inputTexture 를 바인딩한다. (어차피 사용안함)
    RHI::renderer->SetTexture(mainCommandList, 2, false, bloomTexture, bloomViewID);
    RHI::renderer->SetTexture(mainCommandList, 3, false, colorGradingLUTTexture);
    RHI::renderer->SetTexture(mainCommandList, 4, false, useBloomDirtMask ? bloomDirtMaskTexture : inputTexture);

    RHI::renderer->SetTexture(mainCommandList, 0, true, outputTexture);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, finalPostProcessConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(finalPostProcessParams.outputSize.x, FinalPostProcessBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(finalPostProcessParams.outputSize.y, FinalPostProcessBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(luminanceBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}
