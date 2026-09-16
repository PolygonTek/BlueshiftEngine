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

bool RenderBackend::DispatchSSAO(RHI::GPUResourceState renderDepthState) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSSAO");

    RHI::Texture *depthTexture = currentVisCamera->ssaoDepthTexture;
    RHI::Texture *ssaoTexture = currentVisCamera->ssaoTexture;
    if (!depthTexture || !ssaoTexture) {
        return false;
    }

    assert(depthTexture->GetSampleCount() == 1 && ssaoTexture->GetSampleCount() == 1);

    // 최종 AO 는 full-resolution 을 유지한다.
    assert(depthTexture->GetWidth() == ssaoTexture->GetWidth() && depthTexture->GetHeight() == ssaoTexture->GetHeight());

    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    RHI::Texture *ssaoRawTexture = currentVisCamera->ssaoRawTexture;
    RHI::Texture *ssaoTempTexture = currentVisCamera->ssaoFilterTexture;
    RHI::Texture *halfDepthTexture = currentVisCamera->ssaoHalfDepthTexture;

    const uint32_t width = ssaoTexture->GetWidth();
    const uint32_t height = ssaoTexture->GetHeight();
    const uint32_t halfWidth = BE1::DivCeil(width, 2u);
    const uint32_t halfHeight = BE1::DivCeil(height, 2u);

    const auto matchesSize = [](const RHI::Texture *texture, uint32_t width, uint32_t height) {
        return texture && texture->GetWidth() == width && texture->GetHeight() == height;
    };

    // Keep AO available while pipelines are compiling or allocation has failed.
    const bool useHalfResolution = settings.ssaoHalfResolution && frameSSAOHalfResolutionPSO && frameAOUpsamplePSO && matchesSize(ssaoRawTexture, halfWidth, halfHeight) && matchesSize(halfDepthTexture, halfWidth, halfHeight);
    const uint32_t aoWidth = useHalfResolution ? halfWidth : width;
    const uint32_t aoHeight = useHalfResolution ? halfHeight : height;

    // Full-resolution fallback 에서는 half-resolution 중간 텍스처를 필터에 사용하지 않는다.
    const bool useFilter = settings.ssaoBilateral && frameAOBilateralHorizontalPSO && frameAOBilateralVerticalPSO && matchesSize(ssaoRawTexture, aoWidth, aoHeight) && matchesSize(ssaoTempTexture, aoWidth, aoHeight);
    RHI::Texture *ssaoComputeOutputTexture = (useHalfResolution || useFilter) ? ssaoRawTexture : ssaoTexture;
    if (!useHalfResolution && !frameSSAOPSO) {
        return false;
    }

    // Lighting AO runs before opaque shading, so resolve the MSAA prepass depth first.
    const bool useMSAA = currentVisCamera->depthTexture->GetSampleCount() > 1;
    if (useMSAA && !DispatchDepthMSAAResolve(depthTexture, renderDepthState)) {
        return false;
    }

    const RHI::GPUResourceState depthInputState = useMSAA ? RHI::GPUResourceState::ShaderResource : renderDepthState;
    DispatchSSAOCompute(depthTexture, ssaoComputeOutputTexture, useHalfResolution ? halfDepthTexture : nullptr, depthInputState);

    if (useFilter) {
        DispatchAOBilateralFilter(useHalfResolution ? halfDepthTexture : depthTexture, ssaoRawTexture, ssaoTempTexture, useHalfResolution ? ssaoRawTexture : ssaoTexture, settings.ssaoRadius);
    }
    if (useHalfResolution) {
        DispatchAOUpsample(depthTexture, ssaoRawTexture, halfDepthTexture, ssaoTexture, settings.ssaoRadius);
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, depthInputState),
        RHI::Renderer::MakeTextureBarrier(ssaoTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

void RenderBackend::DispatchSSAOCompute(RHI::Texture *depthTexture, RHI::Texture *outputAOTexture, RHI::Texture *outputHalfDepthTexture, RHI::GPUResourceState depthInputState) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSSAOCompute");

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SSAO);

    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    const bool useHalfResolution = outputHalfDepthTexture != nullptr;

    SSAOParams params = {};
    params.inputSize = uint2(depthTexture->GetWidth(), depthTexture->GetHeight());
    params.inputSizeRcp = float2(1.0f / params.inputSize.x, 1.0f / params.inputSize.y);
    params.outputSize = uint2(outputAOTexture->GetWidth(), outputAOTexture->GetHeight());
    params.radius = settings.ssaoRadius;
    params.power = settings.ssaoPower;
    params.normalBias = params.radius * 0.01f;
    params.depthBias = params.radius * 0.004f;
    params.farPlaneDeviceDepth = currentVisCamera->projectionPolicy.GetDepthClearValue();

    RHI::GPUBarrier startBarriers[3] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, depthInputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputAOTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };

    if (useHalfResolution) {
        startBarriers[2] = RHI::Renderer::MakeTextureBarrier(outputHalfDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }

    RHI::renderer->Barrier(mainCommandList, startBarriers, useHalfResolution ? 3 : 2);

    RHI::renderer->SetPSO(mainCommandList, useHalfResolution ? frameSSAOHalfResolutionPSO : frameSSAOPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputAOTexture);
    if (useHalfResolution) {
        RHI::renderer->SetTexture(mainCommandList, 1, true, outputHalfDepthTexture);
    }
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(mainCommandList, BE1::DivCeil(params.outputSize.x, SSAOBlockSizeX), BE1::DivCeil(params.outputSize.y, SSAOBlockSizeY), 1);

    if (useHalfResolution) {
        const RHI::GPUBarrier depthBarrier = RHI::Renderer::MakeTextureBarrier(outputHalfDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
        RHI::renderer->Barrier(mainCommandList, &depthBarrier, 1);
    }
}

void RenderBackend::DispatchAOBilateralFilter(RHI::Texture *depthTexture, RHI::Texture *inputAOTexture, RHI::Texture *tempAOTexture, RHI::Texture *outputAOTexture, float radius) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchAOBilateralFilter");

    assert(depthTexture->GetSampleCount() == 1);
    assert(depthTexture->GetWidth() == outputAOTexture->GetWidth() && depthTexture->GetHeight() == outputAOTexture->GetHeight());

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::AOBilateralFilter);

    AOBilateralParams params = {};
    params.outputSize = uint2(outputAOTexture->GetWidth(), outputAOTexture->GetHeight());
    params.depthSigmaRcp = 1.0f / BE1::Max(radius * 0.1f, 1.0e-5f);
    params.farPlaneDeviceDepth = currentVisCamera->projectionPolicy.GetDepthClearValue();

    SeparableBlurDesc desc;
    desc.inputTexture = inputAOTexture;
    desc.tempTexture = tempAOTexture;
    desc.outputTexture = outputAOTexture;
    desc.depthTexture = depthTexture;
    desc.horizontalPSO = frameAOBilateralHorizontalPSO;
    desc.verticalPSO = frameAOBilateralVerticalPSO;
    desc.constants = &params;
    desc.constantsSize = sizeof(params);
    desc.viewConstantBuffer = cameraConstantBuffer;
    desc.blockSizeX = AOBilateralBlockSizeX;
    desc.blockSizeY = AOBilateralBlockSizeY;

    // Depth stays readable throughout all AO passes. Input AO enters as UAV;
    // scratch enters as SRV, and the caller transitions the final UAV for the next pass.
    desc.inputState = RHI::GPUResourceState::UnorderedAccess;

    DispatchSeparableBlur(desc);
}

void RenderBackend::DispatchAOUpsample(RHI::Texture *depthTexture, RHI::Texture *inputHalfAOTexture, RHI::Texture *inputHalfDepthTexture, RHI::Texture *outputAOTexture, float radius) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchAOUpsample");

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::AOUpsample);

    // 양쪽 depth는 compute SRV, half AO는 UAV, 최종 출력은 SRV 상태로 전달받는다.
    RHI::GPUBarrier upsampleBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputHalfAOTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputAOTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(mainCommandList, upsampleBarriers, COUNT_OF(upsampleBarriers));

    AOUpsampleParams upsampleParams = {};
    upsampleParams.inputSize = uint2(inputHalfAOTexture->GetWidth(), inputHalfAOTexture->GetHeight());
    upsampleParams.outputSize = uint2(outputAOTexture->GetWidth(), outputAOTexture->GetHeight());
    upsampleParams.depthSigmaRcp = 1.0f / BE1::Max(radius * 0.1f, 1.0e-5f);
    upsampleParams.farPlaneDeviceDepth = currentVisCamera->projectionPolicy.GetDepthClearValue();

    RHI::renderer->SetPSO(mainCommandList, frameAOUpsamplePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputHalfAOTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, inputHalfDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputAOTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &upsampleParams, sizeof(upsampleParams), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(mainCommandList, BE1::DivCeil(upsampleParams.outputSize.x, AOUpsampleBlockSizeX), BE1::DivCeil(upsampleParams.outputSize.y, AOUpsampleBlockSizeY), 1);

    RHI::GPUBarrier halfEndBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputHalfAOTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(inputHalfDepthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, halfEndBarriers, COUNT_OF(halfEndBarriers));
}
