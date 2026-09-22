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
#include "Render/RenderFrame.h"
#include "Shaders/ShaderInterop.h"

bool RenderBackend::DispatchLuminance() {
    return renderSystem->GetPostLuminance().Dispatch(*this);
}

bool RenderBackend::DispatchMSAAResolveVelocity() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchMSAAResolveVelocity");

    assert(currentVisCamera->colorTexture->GetSampleCount() > 1);
    assert(currentVisCamera->needVelocityOutput);

    RHI::PipelineState *resolvePSO = frameMSAAResolveVelocityPSO;
    if (!resolvePSO) {
        return false;
    }

    MSAAResolveVelocityParams resolveParams = {};
    resolveParams.outputSize = uint2(currentVisCamera->renderWidth, currentVisCamera->renderHeight);
    resolveParams.useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 0u : 1u;

    RHI::ConstantBuffer *resolveConstantBuffer = frame->GetThreadData(0)->AllocConstant(sizeof(MSAAResolveVelocityParams), &resolveParams);
    if (!resolveConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->depthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->velocityTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->resolvedVelocityTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::MSAAResolveVelocity);
    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, resolvePSO);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, resolveConstantBuffer);

    RHI::renderer->SetTexture(mainCommandList, 0, false, currentVisCamera->depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, currentVisCamera->velocityTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, currentVisCamera->resolvedVelocityTexture);

    const uint32_t threadGroupCountX = BE1::DivCeil(resolveParams.outputSize.x, MSAAResolveVelocityBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(resolveParams.outputSize.y, MSAAResolveVelocityBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->depthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->velocityTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->resolvedVelocityTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool RenderBackend::DispatchMSAAResolveDepthHistory() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchMSAAResolveDepthHistory");

    assert(currentVisCamera->taaResolve);
    return DispatchMSAAResolveDepth(currentVisCamera->depthHistoryTextures[currentVisCamera->currentHistoryTextureIndex], RHI::GPUResourceState::ShaderResource);
}

bool RenderBackend::DispatchMSAAResolveDepth(RHI::Texture *outputDepthTexture, RHI::GPUResourceState inputState, RHI::Texture *outputNormalTexture) {
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

    RHI::Texture *inputNormalTexture = outputNormalTexture ? currentVisCamera->normalTexture : nullptr;
    if (outputNormalTexture) {
        if (!inputNormalTexture) {
            return false;
        }
        assert(inputNormalTexture->GetSampleCount() == inputDepthTexture->GetSampleCount());
        assert(inputNormalTexture->GetWidth() == outputW && inputNormalTexture->GetHeight() == outputH);
        assert(outputNormalTexture->GetSampleCount() == 1);
        assert(outputNormalTexture->GetWidth() == outputW && outputNormalTexture->GetHeight() == outputH);
    }

    RHI::PipelineState *msaaResolveDepthPSO = outputNormalTexture ? frameMSAAResolveDepthNormalPSO : frameMSAAResolveDepthPSO;
    if (!msaaResolveDepthPSO) {
        return false;
    }

    MSAAResolveDepthParams msaaResolveDepthParams = {};
    msaaResolveDepthParams.outputSize = uint2(outputW, outputH);
    msaaResolveDepthParams.useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 0u : 1u;

    RHI::ConstantBuffer *msaaResolveDepthConstantBuffer = frame->GetThreadData(0)->AllocConstant(sizeof(MSAAResolveDepthParams), &msaaResolveDepthParams);
    if (!msaaResolveDepthConstantBuffer) {
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
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::MSAAResolveDepth);

    RHI::renderer->Barrier(mainCommandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, msaaResolveDepthPSO);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, msaaResolveDepthConstantBuffer);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputDepthTexture);
    if (outputNormalTexture) {
        RHI::renderer->SetTexture(mainCommandList, 1, false, inputNormalTexture);
        RHI::renderer->SetTexture(mainCommandList, 1, true, outputNormalTexture);
    }

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, MSAAResolveDepthBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, MSAAResolveDepthBlockSizeY);
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

    assert(currentPostProcessInputTexture);
    RHI::Texture *inputTexture = currentPostProcessInputTexture;
    RHI::Texture *velocityTexture = currentVisCamera->resolvedVelocityTexture ? currentVisCamera->resolvedVelocityTexture : currentVisCamera->velocityTexture;
    RHI::Texture *depthTexture = currentVisCamera->depthHistoryTextures[currentVisCamera->currentHistoryTextureIndex];
    const bool historyValid = currentVisCamera->historyValid;
    RHI::Texture *previousColorHistoryTexture = historyValid ? currentVisCamera->colorHistoryTextures[currentVisCamera->currentHistoryTextureIndex ^ 1] : inputTexture;
    RHI::Texture *previousDepthTexture = historyValid ? currentVisCamera->depthHistoryTextures[currentVisCamera->currentHistoryTextureIndex ^ 1] : depthTexture;
    RHI::Texture *outputColorHistoryTexture = currentVisCamera->colorHistoryTextures[currentVisCamera->currentHistoryTextureIndex];
    if (!inputTexture || !velocityTexture || !depthTexture || !previousColorHistoryTexture || !previousDepthTexture || !outputColorHistoryTexture) {
        return false;
    }

    const RenderFrameSettings &renderFrameSettings = frame->GetRenderFrameSettings();
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

    RHI::ConstantBuffer *taaResolveConstantBuffer = frame->GetThreadData(0)->AllocConstant(sizeof(TAAResolveParams), &taaResolveParams);
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
    assert(currentPostProcessInputTexture);
    return renderSystem->GetBloom().Dispatch(*this);
}

bool RenderBackend::DispatchFinalPostProcess(bool useToneMapping, bool useBloom) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchFinalPostProcess");

    if (!frameFinalPostProcessPSO) {
        return false;
    }

    const RenderFrameSettings &renderFrameSettings = frame->GetRenderFrameSettings();

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

    RHI::ConstantBuffer *finalPostProcessConstantBuffer = frame->GetThreadData(0)->AllocConstant(sizeof(FinalPostProcessParams), &finalPostProcessParams);
    if (!finalPostProcessConstantBuffer) {
        return false;
    }

    RHI::Buffer *luminanceBuffer = currentVisCamera->luminanceBuffer;
    assert(currentPostProcessInputTexture);
    RHI::Texture *inputTexture = currentPostProcessInputTexture;
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
