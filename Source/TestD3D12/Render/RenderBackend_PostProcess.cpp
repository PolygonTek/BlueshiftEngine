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
#include "RenderCvars.h"
#include "RenderSystem.h"
#include "RenderInternal.h"
#include "Shaders/ShaderInterop.h"

// 히스토그램 기반 luminance 계산: https://www.alextardif.com/HistogramLuminance.html
bool RenderBackend::DispatchLuminance() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchLuminance");

    if (!r_toneMapping.GetBool()) {
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

    const uint32_t sampleW = BE1::Max(inputW / 2, 1u);
    const uint32_t sampleH = BE1::Max(inputH / 2, 1u);

    const float lumLogMin = r_toneMapLuminanceLogMin.GetFloat();
    const float lumLogMax = BE1::Max(r_toneMapLuminanceLogMax.GetFloat(), lumLogMin + 0.0001f);
    const float lumLogRange = lumLogMax - lumLogMin;

    RHI::Buffer *luminanceBuffer = currentVisCamera->luminanceBuffer;

    LuminancePass1Params luminancePass1Params = {};
    luminancePass1Params.sampleSize = uint2(sampleW, sampleH);
    luminancePass1Params.sampleSizeRcp = float2(1.0f / sampleW, 1.0f / sampleH);
    luminancePass1Params.luminanceLogMin = lumLogMin;
    luminancePass1Params.luminanceLogRangeRcp = 1.0f / lumLogRange;

    LuminancePass2Params luminancePass2Params = {};
    luminancePass2Params.luminanceLogMin = lumLogMin;
    luminancePass2Params.luminanceLogRange = lumLogRange;
    luminancePass2Params.luminancePixelCount = static_cast<float>(sampleW * sampleH);
    luminancePass2Params.luminanceAdaptationRate = BE1::Max(r_toneMapLuminanceAdaptationRate.GetFloat(), 0.0f);
    luminancePass2Params.middleGray = BE1::Max(r_toneMapMiddleGray.GetFloat(), 0.0f);

    RHI::Texture *inputTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;

    // Pass 1 : Log Luminance 히스토그램 계산
    {
        GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::LuminancePass1);

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute));

        RHI::renderer->SetPSO(mainCommandList, frameLuminancePass1PSO);
        RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
        RHI::renderer->SetBuffer(mainCommandList, 0, true, luminanceBuffer);
        RHI::renderer->SetConstants(mainCommandList, 1, &luminancePass1Params, sizeof(luminancePass1Params), 0);

        const uint32_t threadGroupCountX = (luminancePass1Params.sampleSize.x + LuminanceBlockSize - 1) / LuminanceBlockSize;
        const uint32_t threadGroupCountY = (luminancePass1Params.sampleSize.y + LuminanceBlockSize - 1) / LuminanceBlockSize;
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

bool RenderBackend::DispatchDepthHistoryResolve() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchDepthHistoryResolve");

    if (!currentVisCamera->taaResolve) {
        return false;
    }

    RHI::Texture *inputDepthTexture = currentVisCamera->depthTexture;
    RHI::Texture *outputDepthTexture = currentVisCamera->depthHistoryTextures[currentVisCamera->currentHistoryTextureIndex];

    const uint32_t outputW = outputDepthTexture->GetWidth();
    const uint32_t outputH = outputDepthTexture->GetHeight();
    if (outputW == 0 || outputH == 0) {
        return false;
    }

    RHI::PipelineState *depthHistoryResolvePSO = inputDepthTexture->GetSampleCount() > 1 ? frameDepthHistoryResolveMSAAPSO : frameDepthHistoryResolvePSO;
    if (!depthHistoryResolvePSO) {
        return false;
    }

    DepthHistoryResolveParams depthHistoryResolveParams = {};
    depthHistoryResolveParams.outputSize = uint2(outputW, outputH);
    depthHistoryResolveParams.useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 0u : 1u;

    RHI::ConstantBuffer *depthHistoryResolveConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(DepthHistoryResolveParams), &depthHistoryResolveParams);
    if (!depthHistoryResolveConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::DepthHistoryResolve);

    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, depthHistoryResolvePSO);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, depthHistoryResolveConstantBuffer);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputDepthTexture);

    const uint32_t threadGroupCountX = (outputW + DepthHistoryResolveBlockSizeX - 1) / DepthHistoryResolveBlockSizeX;
    const uint32_t threadGroupCountY = (outputH + DepthHistoryResolveBlockSizeY - 1) / DepthHistoryResolveBlockSizeY;
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputDepthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(outputDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool RenderBackend::DispatchTAAResolve() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchTAAResolve");

    if (!currentVisCamera->taaResolve || !frameTAAResolvePSO) {
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
    RHI::Texture *previousColorHistoryTexture = currentVisCamera->colorHistoryTextures[currentVisCamera->currentHistoryTextureIndex ^ 1];
    RHI::Texture *previousDepthTexture = currentVisCamera->depthHistoryTextures[currentVisCamera->currentHistoryTextureIndex ^ 1];
    RHI::Texture *outputColorHistoryTexture = currentVisCamera->colorHistoryTextures[currentVisCamera->currentHistoryTextureIndex];
    if (!inputTexture || !velocityTexture || !depthTexture || !previousColorHistoryTexture || !previousDepthTexture || !outputColorHistoryTexture) {
        return false;
    }

    TAAResolveParams taaResolveParams = {};
    taaResolveParams.outputSize = uint2(outputW, outputH);
    taaResolveParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    taaResolveParams.historyWeight = r_taaHistoryWeight.GetFloat();
    taaResolveParams.historyValid = currentVisCamera->historyValid ? 1u : 0u;
    taaResolveParams.useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 0u : 1u;
    taaResolveParams.orthogonal = currentVisCamera->projectionOrthogonal ? 1u : 0u;
    taaResolveParams.projectionDepthParams = float2(currentVisCamera->projectionDepthA, currentVisCamera->projectionDepthB);
    taaResolveParams.ndcDepthScaleBias = float2(currentVisCamera->ndcDepthScale, currentVisCamera->ndcDepthBias);
    taaResolveParams.depthFallbackThreshold = r_taaDepthFallbackThreshold.GetFloat();
    taaResolveParams.maxSubpixelCurrentWeight = r_taaSubpixelWeight.GetFloat();
    taaResolveParams.luminanceWeight = r_taaLuminanceWeight.GetFloat();

    RHI::ConstantBuffer *taaResolveConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(TAAResolveParams), &taaResolveParams);
    if (!taaResolveConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(velocityTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(previousColorHistoryTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(previousDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputColorHistoryTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::TAAResolve);

    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameTAAResolvePSO);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, taaResolveConstantBuffer);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, velocityTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 3, false, previousColorHistoryTexture);
    RHI::renderer->SetTexture(mainCommandList, 4, false, previousDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputColorHistoryTexture);

    const uint32_t threadGroupCountX = (taaResolveParams.outputSize.x + TAAResolveBlockSizeX - 1) / TAAResolveBlockSizeX;
    const uint32_t threadGroupCountY = (taaResolveParams.outputSize.y + TAAResolveBlockSizeY - 1) / TAAResolveBlockSizeY;
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(velocityTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(previousColorHistoryTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(previousDepthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(outputColorHistoryTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool RenderBackend::DispatchBloom() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchBloom");

    const RenderBloomMode bloomMode = static_cast<RenderBloomMode>(BE1::Clamp(r_bloom.GetInteger(), static_cast<int>(RenderBloomMode::Off), static_cast<int>(RenderBloomMode::Convolution)));
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

    const float bloomThreshold = BE1::Max(r_bloomThreshold.GetFloat(), 0.0f);
    const float bloomSoftThreshold = BE1::Max(r_bloomSoftThreshold.GetFloat(), 0.0f);
    const float pyramidBloomPrefilterQuality = static_cast<float>(BE1::Clamp(r_pyramidBloomPrefilterQuality.GetInteger(), 0, 1));
    const float pyramidBloomScatter = BE1::Clamp(r_pyramidBloomScatter.GetFloat(), 0.0f, 1.0f);
    const float pyramidBloomPrefilterRadius = BE1::Max(r_pyramidBloomPrefilterRadius.GetFloat(), 0.0f);
    const float pyramidBloomDownsampleRadius = BE1::Max(r_pyramidBloomDownsampleRadius.GetFloat(), 0.0f);
    const float pyramidBloomUpsampleRadius = BE1::Max(r_pyramidBloomUpsampleRadius.GetFloat(), 0.0f);

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

        const uint32_t threadGroupCountX = (outputWidth + BloomBlockSizeX - 1) / BloomBlockSizeX;
        const uint32_t threadGroupCountY = (outputHeight + BloomBlockSizeY - 1) / BloomBlockSizeY;
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

        const uint32_t threadGroupCountX = (outputWidth + BloomBlockSizeX - 1) / BloomBlockSizeX;
        const uint32_t threadGroupCountY = (outputHeight + BloomBlockSizeY - 1) / BloomBlockSizeY;
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
        // 커널 버퍼 준비
        if (!DispatchConvolutionBloomKernelPrepare()) {
            return false;
        }
        // 커널 버퍼 FFT
        if (!DispatchConvolutionBloomFFT(currentVisCamera->fftBloomKernelSpatialBuffer, currentVisCamera->fftBloomKernelSpectrumBuffer)) {
            return false;
        }
    }

    // 블룸 프리필터된 화면 버퍼 준비
    if (!DispatchConvolutionBloomScreenPrefilter()) {
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

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0 = float4(rotationCos, rotationSin, currentVisCamera->fftBloomKernelSpatialScale.x, currentVisCamera->fftBloomKernelSpatialScale.y);

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

    const uint32_t threadGroupCountX = (outputW + BloomBlockSizeX - 1) / BloomBlockSizeX;
    const uint32_t threadGroupCountY = (outputH + BloomBlockSizeY - 1) / BloomBlockSizeY;
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool RenderBackend::DispatchConvolutionBloomScreenPrefilter() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchConvolutionBloomScreenPrefilter");

    if (currentVisCamera->renderWidth == 0 || currentVisCamera->renderHeight == 0 || currentVisCamera->fftBloomSize == 0) {
        return false;
    }

    const uint32_t inputW = currentVisCamera->renderWidth;
    const uint32_t inputH = currentVisCamera->renderHeight;
    const uint32_t outputW = currentVisCamera->fftBloomSize;
    const uint32_t outputH = currentVisCamera->fftBloomSize;

    const float bloomThreshold = BE1::Max(r_bloomThreshold.GetFloat(), 0.0f);
    const float bloomSoftThreshold = BE1::Max(r_bloomSoftThreshold.GetFloat(), 0.0f);

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0 = float4(bloomThreshold, bloomSoftThreshold, 0.0f, 0.0f);
    bloomParams.params1 = float4(
        currentVisCamera->fftBloomGuardedContentRect.x,
        currentVisCamera->fftBloomGuardedContentRect.y,
        currentVisCamera->fftBloomGuardedContentRect.w,
        currentVisCamera->fftBloomGuardedContentRect.h);

    RHI::ConstantBuffer *bloomConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::Texture *inputTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;
    RHI::Buffer *outputBuffer = currentVisCamera->fftBloomScreenSpatialBuffer;

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomScreenPrefilter);

    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameConvolutionBloomPrefilterPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
    RHI::renderer->SetBuffer(mainCommandList, 0, true, outputBuffer);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = (outputW + BloomBlockSizeX - 1) / BloomBlockSizeX;
    const uint32_t threadGroupCountY = (outputH + BloomBlockSizeY - 1) / BloomBlockSizeY;
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

    // content guard padding 때문에 줄어든 kernel footprint 의 면적만큼 커널 에너지를 보정한다.
    const float kernelAreaScale = BE1::Max(currentVisCamera->fftBloomKernelSpatialScale.x * currentVisCamera->fftBloomKernelSpatialScale.y, 0.000001f);

    // sourcePixelCount 를 곱해둔 factor 를 현재 FFT kernel 샘플 수로 나눈다.
    // 이렇게 해서 FFT size 에 독립적이 되도록 커널 에너지를 보정한다.
    const float kernelSampleCount = static_cast<float>(fftSize) * static_cast<float>(fftSize);
    const float kernelScale = BE1::Max(r_convolutionBloomKernelScale.GetFloat(), 0.0f) * BE1::Max(currentVisCamera->fftBloomKernelNormalizationFactor, 0.0f) / (BE1::Max(kernelSampleCount, 1.0f) * kernelAreaScale);

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomMultiplySpectrum);

    return renderSystem->gpuFFT.DispatchMultiplyPackedDualRealSpectrum2D(mainCommandList, frameData,
        currentVisCamera->fftBloomKernelSpectrumBuffer,
        currentVisCamera->fftBloomScreenSpectrumBuffer,
        currentVisCamera->fftBloomWorkBuffer,
        fftSize,
        ColorPairCount,
        kernelScale);
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
        currentVisCamera->fftBloomGuardedContentRect.x,
        currentVisCamera->fftBloomGuardedContentRect.y,
        currentVisCamera->fftBloomGuardedContentRect.w,
        currentVisCamera->fftBloomGuardedContentRect.h);

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

    const uint32_t threadGroupCountX = (outputW + BloomBlockSizeX - 1) / BloomBlockSizeX;
    const uint32_t threadGroupCountY = (outputH + BloomBlockSizeY - 1) / BloomBlockSizeY;
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

    return renderSystem->gpuFFT.DispatchFFTDualReal2D(mainCommandList, frameData, inputBuffer, outputBuffer, currentVisCamera->fftBloomWorkBuffer, fftSizeEnum, PlaneCount);
}

bool RenderBackend::DispatchConvolutionBloomIFFT(RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) {
    constexpr uint32_t PlaneCount = 2; // RG, B0

    GpuFFT::FFTSize fftSizeEnum;
    if (!GpuFFT::GetGpuFFTSize(currentVisCamera->fftBloomSize, fftSizeEnum)) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ConvolutionBloomIFFT);

    return renderSystem->gpuFFT.DispatchIFFTDualReal2D(mainCommandList, frameData, inputBuffer, outputBuffer, currentVisCamera->fftBloomScreenSpectrumBuffer, fftSizeEnum, PlaneCount);
}

bool RenderBackend::DispatchToneMapping(bool useBloom) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchToneMapping");

    if (!r_toneMapping.GetBool()) {
        return false;
    }

    const uint32_t outputW = currentVisCamera->renderWidth;
    const uint32_t outputH = currentVisCamera->renderHeight;
    if (outputW == 0 || outputH == 0) {
        return false;
    }

    ToneMappingParams toneMappingParams = {};
    toneMappingParams.outputSize = uint2(outputW, outputH);
    toneMappingParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    toneMappingParams.exposureScale = 1.0f;
    toneMappingParams.toneMappingOperator = static_cast<uint32_t>(BE1::Clamp(r_toneMapOp.GetInteger(), static_cast<int>(ToneMappingOperatorLinear), static_cast<int>(ToneMappingOperatorFilmicGT)));
    toneMappingParams.bloomIntensity = useBloom ? BE1::Max(r_bloomIntensity.GetFloat(), 0.0f) : 0.0f;

    RHI::ConstantBuffer *toneMappingConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(ToneMappingParams), &toneMappingParams);
    if (!toneMappingConstantBuffer) {
        return false;
    }

    RHI::Buffer *luminanceBuffer = currentVisCamera->luminanceBuffer;
    RHI::Texture *inputTexture = currentPostProcessInputTexture ? currentPostProcessInputTexture : currentVisCamera->rtColorTexture;
    RHI::Texture *outputTexture = currentVisCamera->postProcessColorTexture;
    RHI::Texture *bloomTexture = inputTexture;
    RHI::ViewID bloomViewID = RHI::ViewID::PrimaryID;

    if (useBloom) {
        const RenderBloomMode bloomMode = static_cast<RenderBloomMode>(BE1::Clamp(r_bloom.GetInteger(), static_cast<int>(RenderBloomMode::Off), static_cast<int>(RenderBloomMode::Convolution)));
        const bool useConvolutionBloom = bloomMode == RenderBloomMode::Convolution;
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
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ToneMapping);

    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameToneMappingPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
    RHI::renderer->SetBuffer(mainCommandList, 1, false, luminanceBuffer);

    // useBloom 이 꺼져있으면, 더미 텍스쳐로 그냥 inputTexture 를 바인딩한다. (어차피 사용안함)
    RHI::renderer->SetTexture(mainCommandList, 2, false, bloomTexture, bloomViewID);

    RHI::renderer->SetTexture(mainCommandList, 0, true, outputTexture);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, toneMappingConstantBuffer);

    const uint32_t threadGroupCountX = (toneMappingParams.outputSize.x + ToneMappingBlockSizeX - 1) / ToneMappingBlockSizeX;
    const uint32_t threadGroupCountY = (toneMappingParams.outputSize.y + ToneMappingBlockSizeY - 1) / ToneMappingBlockSizeY;
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(luminanceBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}
