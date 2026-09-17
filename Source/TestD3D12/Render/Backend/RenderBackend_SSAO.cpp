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

    const bool useMSAA = currentVisCamera->depthTexture->GetSampleCount() > 1;
    RHI::Texture *normalTexture = currentVisCamera->rtNormalTexture;
    if (!currentVisCamera->needDepthNormalOutput || !matchesSize(normalTexture, width, height) || normalTexture->GetSampleCount() != 1) {
        return false;
    }

    if (useMSAA) {
        RHI::Texture *normalMSAATexture = currentVisCamera->rtNormalMSAATexture;
        // Never read the previous resolved normal while the paired resolve PSO is unavailable.
        if (!frameDepthNormalMSAAResolvePSO || !matchesSize(normalMSAATexture, width, height) || normalMSAATexture->GetSampleCount() != currentVisCamera->depthTexture->GetSampleCount()) {
            return false;
        }
    }

    // Keep full-resolution AO available while half-resolution resources are not ready.
    const bool useHalfResolution = settings.aoHalfResolution && frameSSAOHalfResolutionPSO && frameAOUpsamplePSO && matchesSize(ssaoRawTexture, halfWidth, halfHeight) && matchesSize(halfDepthTexture, halfWidth, halfHeight);
    const uint32_t aoWidth = useHalfResolution ? halfWidth : width;
    const uint32_t aoHeight = useHalfResolution ? halfHeight : height;

    // Full-resolution fallback 에서는 half-resolution 중간 텍스처를 필터에 사용하지 않는다.
    const bool useBlur = settings.aoBilateral && frameAOBilateralHorizontalPSO && frameAOBilateralVerticalPSO && matchesSize(ssaoRawTexture, aoWidth, aoHeight) && matchesSize(ssaoTempTexture, aoWidth, aoHeight);

    RHI::Texture *ssaoComputeOutputTexture = (useHalfResolution || useBlur) ? ssaoRawTexture : ssaoTexture;
    RHI::PipelineState *ssaoPSO = useHalfResolution ? frameSSAOHalfResolutionPSO : frameSSAOPSO;
    if (!ssaoPSO) {
        return false;
    }

    // Lighting AO runs before opaque shading; resolve prepass depth and its matching normal together.
    if (useMSAA && !DispatchDepthMSAAResolve(depthTexture, renderDepthState, normalTexture)) {
        return false;
    }

    const RHI::GPUResourceState depthInputState = useMSAA ? RHI::GPUResourceState::ShaderResource : renderDepthState;
    DispatchSSAOCompute(ssaoPSO, depthTexture, normalTexture, ssaoComputeOutputTexture, useHalfResolution ? halfDepthTexture : nullptr, depthInputState);

    if (useBlur) {
        DispatchAOBilateralFilter(useHalfResolution ? halfDepthTexture : depthTexture, ssaoRawTexture, ssaoTempTexture, useHalfResolution ? ssaoRawTexture : ssaoTexture, settings.aoRadius);
    }

    if (useHalfResolution) {
        DispatchAOUpsample(depthTexture, ssaoRawTexture, halfDepthTexture, ssaoTexture, settings.aoRadius);
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, depthInputState),
        RHI::Renderer::MakeTextureBarrier(ssaoTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

void RenderBackend::DispatchSSAOCompute(RHI::PipelineState *pso, RHI::Texture *depthTexture, RHI::Texture *normalTexture, RHI::Texture *outputAOTexture, RHI::Texture *outputHalfDepthTexture, RHI::GPUResourceState depthInputState) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSSAOCompute");

    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    const bool useHalfResolution = outputHalfDepthTexture != nullptr;

    SSAOParams params = {};
    params.inputSize = uint2(depthTexture->GetWidth(), depthTexture->GetHeight());
    params.inputSizeRcp = float2(1.0f / params.inputSize.x, 1.0f / params.inputSize.y);
    params.outputSize = uint2(outputAOTexture->GetWidth(), outputAOTexture->GetHeight());
    params.radius = settings.aoRadius;
    params.power = settings.aoPower;
    params.normalBias = params.radius * settings.ssaoNormalBiasScale;
    params.depthBias = params.radius * settings.ssaoDepthBiasScale;
    params.farPlaneDeviceDepth = currentVisCamera->projectionPolicy.GetDepthClearValue();

    if (settings.aoTemporalRotation) {
        // 8 프레임 동안 회전각을 고르게 분포시킨다.
        static constexpr float noisePhases[8] = {0.0f, 0.5f, 0.25f, 0.75f, 0.125f, 0.625f, 0.375f, 0.875f};
        params.noisePhase = noisePhases[frameData->GetFrameCount() % COUNT_OF(noisePhases)];
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SSAO);

    RHI::GPUBarrier startBarriers[4] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, depthInputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(normalTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputAOTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };

    uint32_t startBarrierCount = 3;
    if (useHalfResolution) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(outputHalfDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }

    RHI::renderer->Barrier(mainCommandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, pso);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, normalTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputAOTexture);
    if (useHalfResolution) {
        RHI::renderer->SetTexture(mainCommandList, 1, true, outputHalfDepthTexture);
    }
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(mainCommandList, BE1::DivCeil(params.outputSize.x, SSAOBlockSizeX), BE1::DivCeil(params.outputSize.y, SSAOBlockSizeY), 1);

    // Restore the state expected by opaque's later overwrite, or by the next frame without SSR.
    RHI::GPUBarrier endBarriers[2] = {
        RHI::Renderer::MakeTextureBarrier(normalTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
    };
    uint32_t endBarrierCount = 1;
    if (useHalfResolution) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(outputHalfDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
    }
    RHI::renderer->Barrier(mainCommandList, endBarriers, endBarrierCount);
}

void RenderBackend::DispatchAOBilateralFilter(RHI::Texture *depthTexture, RHI::Texture *inputAOTexture, RHI::Texture *tempAOTexture, RHI::Texture *outputAOTexture, float radius) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchAOBilateralFilter");

    assert(depthTexture->GetSampleCount() == 1);
    assert(depthTexture->GetWidth() == outputAOTexture->GetWidth() && depthTexture->GetHeight() == outputAOTexture->GetHeight());

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

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::AOBilateralFilter);

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
