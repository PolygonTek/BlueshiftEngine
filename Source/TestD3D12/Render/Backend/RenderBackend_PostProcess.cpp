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
        // The prepass leaves its MSAA normal attachment readable; AO consumes the resolved pair.
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
    RHI::Texture *inputColorTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;
    return renderSystem->GetBloom().Dispatch(mainCommandList, frameData->GetThreadData(0), renderSystem->GetGpuFFT(), gpuTiming, inputColorTexture, currentVisCamera->bloom, frameData->GetRenderFrameSettings());
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

    RHI::Texture *bloomDirtMaskTexture = currentVisCamera->bloom.bloomDirtMaskTexture;
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
        bloomTexture = currentVisCamera->bloom.outputTexture;
        bloomViewID = currentVisCamera->bloom.outputViewID;
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
