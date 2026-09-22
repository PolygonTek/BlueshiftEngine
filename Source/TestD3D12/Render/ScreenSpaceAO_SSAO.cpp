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
#include "Shaders/ShaderInterop.h"

void ScreenSpaceAO::DispatchSSAO(RenderBackend &backend, const DispatchParams &params) const {
    PROFILER_CPU_SCOPED_EVENT("ScreenSpaceAO::DispatchSSAO");

    RHI::CommandList *commandList = backend.GetCommandList();
    const ScreenSpaceAOFrameData &data = backend.GetVisCamera()->ao;
    const RenderFrameSettings &settings = backend.GetFrameSettings();

    RHI::Texture *depthTexture = data.depthTexture;
    RHI::Texture *normalTexture = data.normalTexture;
    RHI::Texture *aoTexture = data.textures.outputTexture;
    RHI::Texture *aoRawTexture = data.textures.rawTexture;
    RHI::Texture *aoTempTexture = data.textures.tempTexture;
    RHI::Texture *halfDepthTexture = data.textures.halfDepthTexture;

    const bool useHalfResolution = params.useHalfResolution;
    const bool useBlur = params.useBlur;
    RHI::Texture *targetTexture = (useHalfResolution || useBlur) ? aoRawTexture : aoTexture;
    RHI::PipelineState *ssaoPSO = useHalfResolution ? frameSSAOHalfResolutionPSO : frameSSAOPSO;
    const RHI::GPUResourceState depthInputState = RHI::GPUResourceState::ShaderResource;

    DispatchSSAOCompute(backend, ssaoPSO, depthTexture, normalTexture, targetTexture, useHalfResolution ? halfDepthTexture : nullptr, depthInputState);

    if (useBlur) {
        DispatchAOBilateralBlur(backend, useHalfResolution ? halfDepthTexture : depthTexture, aoRawTexture, aoTempTexture, useHalfResolution ? aoRawTexture : aoTexture, settings.aoRadius);
    }

    if (useHalfResolution) {
        DispatchAOUpsample(backend, depthTexture, aoRawTexture, halfDepthTexture, aoTexture, settings.aoRadius);
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, depthInputState),
        RHI::Renderer::MakeTextureBarrier(aoTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
}

void ScreenSpaceAO::DispatchSSAOCompute(RenderBackend &backend, RHI::PipelineState *pso, RHI::Texture *depthTexture, RHI::Texture *normalTexture, RHI::Texture *outputAOTexture, RHI::Texture *outputHalfDepthTexture, RHI::GPUResourceState depthInputState) const {
    PROFILER_CPU_SCOPED_EVENT("ScreenSpaceAO::DispatchSSAOCompute");

    RHI::CommandList *commandList = backend.GetCommandList();
    const RHI::ConstantBuffer *cameraConstantBuffer = backend.GetCameraConstantBuffer();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    const uint32_t frameIndex = backend.GetFrameIndex();
    const ScreenSpaceAOFrameData &data = backend.GetVisCamera()->ao;
    const RenderFrameSettings &settings = backend.GetFrameSettings();

    const bool useHalfResolution = outputHalfDepthTexture != nullptr;

    SSAOParams params = {};
    params.inputSize = uint2(depthTexture->GetWidth(), depthTexture->GetHeight());
    params.inputSizeRcp = float2(1.0f / params.inputSize.x, 1.0f / params.inputSize.y);
    params.outputSize = uint2(outputAOTexture->GetWidth(), outputAOTexture->GetHeight());
    params.radius = settings.aoRadius;
    params.power = settings.ssaoPower;
    params.normalBias = params.radius * settings.ssaoNormalBiasScale;
    params.depthBias = params.radius * settings.ssaoDepthBiasScale;
    params.farPlaneDeviceDepth = data.projectionPolicy.GetDepthClearValue();

    if (data.temporalNoise) {
        // 8 프레임 동안 회전각을 고르게 분포시킨다.
        static constexpr float noisePhases[8] = {0.0f, 0.5f, 0.25f, 0.75f, 0.125f, 0.625f, 0.375f, 0.875f};
        params.noisePhase = noisePhases[frameIndex % COUNT_OF(noisePhases)];
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::SSAO);

    RHI::GPUBarrier startBarriers[4] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, depthInputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(normalTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputAOTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };

    uint32_t startBarrierCount = 3;
    if (useHalfResolution) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(outputHalfDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }

    RHI::renderer->Barrier(commandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, depthTexture);
    RHI::renderer->SetTexture(commandList, 1, false, normalTexture);
    RHI::renderer->SetTexture(commandList, 0, true, outputAOTexture);
    if (useHalfResolution) {
        RHI::renderer->SetTexture(commandList, 1, true, outputHalfDepthTexture);
    }
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(commandList, BE1::DivCeil(params.outputSize.x, SSAOBlockSizeX), BE1::DivCeil(params.outputSize.y, SSAOBlockSizeY), 1);

    // Restore the state expected by opaque's later overwrite, or by the next frame without SSR.
    RHI::GPUBarrier endBarriers[2] = {
        RHI::Renderer::MakeTextureBarrier(normalTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
    };
    uint32_t endBarrierCount = 1;
    if (useHalfResolution) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(outputHalfDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
    }
    RHI::renderer->Barrier(commandList, endBarriers, endBarrierCount);
}

void ScreenSpaceAO::DispatchAOBilateralBlur(RenderBackend &backend, RHI::Texture *depthTexture, RHI::Texture *inputAOTexture, RHI::Texture *tempAOTexture, RHI::Texture *outputAOTexture, float radius) const {
    PROFILER_CPU_SCOPED_EVENT("ScreenSpaceAO::DispatchAOBilateralBlur");

    RHI::CommandList *commandList = backend.GetCommandList();
    const RHI::ConstantBuffer *cameraConstantBuffer = backend.GetCameraConstantBuffer();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    const ScreenSpaceAOFrameData &data = backend.GetVisCamera()->ao;

    assert(depthTexture->GetSampleCount() == 1);
    assert(depthTexture->GetWidth() == outputAOTexture->GetWidth() && depthTexture->GetHeight() == outputAOTexture->GetHeight());

    AOBilateralParams params = {};
    params.outputSize = uint2(outputAOTexture->GetWidth(), outputAOTexture->GetHeight());
    params.depthSigmaRcp = 1.0f / BE1::Max(radius * 0.1f, 1.0e-5f);
    params.farPlaneDeviceDepth = data.projectionPolicy.GetDepthClearValue();

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::AOBilateralFilter);

    assert(inputAOTexture != tempAOTexture && tempAOTexture != outputAOTexture);
    assert(inputAOTexture->GetWidth() == outputAOTexture->GetWidth() && inputAOTexture->GetHeight() == outputAOTexture->GetHeight());
    assert(tempAOTexture->GetWidth() == outputAOTexture->GetWidth() && tempAOTexture->GetHeight() == outputAOTexture->GetHeight());

    const uint32_t groupCountX = BE1::DivCeil(params.outputSize.x, AOBilateralBlockSizeX);
    const uint32_t groupCountY = BE1::DivCeil(params.outputSize.y, AOBilateralBlockSizeY);

    // Horizontal blur
    RHI::GPUBarrier horizontalBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputAOTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(tempAOTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, horizontalBarriers, COUNT_OF(horizontalBarriers));

    RHI::renderer->SetPSO(commandList, frameAOBilateralHorizontalPSO);
    RHI::renderer->SetTexture(commandList, 0, false, inputAOTexture);
    RHI::renderer->SetTexture(commandList, 1, false, depthTexture);
    RHI::renderer->SetTexture(commandList, 0, true, tempAOTexture);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, 1);

    // Vertical blur
    RHI::GPUBarrier verticalBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(tempAOTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputAOTexture, inputAOTexture == outputAOTexture ? RHI::GPUResourceState::ShaderResourceCompute : RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, verticalBarriers, COUNT_OF(verticalBarriers));

    RHI::renderer->SetPSO(commandList, frameAOBilateralVerticalPSO);
    RHI::renderer->SetTexture(commandList, 0, false, tempAOTexture);
    RHI::renderer->SetTexture(commandList, 1, false, depthTexture);
    RHI::renderer->SetTexture(commandList, 0, true, outputAOTexture);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);

    RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(tempAOTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(inputAOTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
    };
    // 입력과 출력을 재사용하면 최종 결과는 UAV 상태로 호출부에 넘긴다.
    RHI::renderer->Barrier(commandList, endBarriers, inputAOTexture == outputAOTexture ? 1 : COUNT_OF(endBarriers));
}

void ScreenSpaceAO::DispatchAOUpsample(RenderBackend &backend, RHI::Texture *depthTexture, RHI::Texture *inputHalfAOTexture, RHI::Texture *inputHalfDepthTexture, RHI::Texture *outputAOTexture, float radius) const {
    PROFILER_CPU_SCOPED_EVENT("ScreenSpaceAO::DispatchAOUpsample");

    RHI::CommandList *commandList = backend.GetCommandList();
    const RHI::ConstantBuffer *cameraConstantBuffer = backend.GetCameraConstantBuffer();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    const ScreenSpaceAOFrameData &data = backend.GetVisCamera()->ao;

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
    upsampleParams.farPlaneDeviceDepth = data.projectionPolicy.GetDepthClearValue();

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
