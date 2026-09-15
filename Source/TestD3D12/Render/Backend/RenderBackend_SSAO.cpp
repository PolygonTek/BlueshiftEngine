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
    RHI::Texture *aoTexture = currentVisCamera->ssaoTexture;
    if (!frameSSAOPSO || !cameraConstantBuffer || !depthTexture || !aoTexture) {
        return false;
    }

    assert(depthTexture->GetSampleCount() == 1 && aoTexture->GetSampleCount() == 1);
    assert(depthTexture->GetWidth() == aoTexture->GetWidth() && depthTexture->GetHeight() == aoTexture->GetHeight());

    // Lighting AO runs before opaque shading, so resolve the MSAA prepass depth first.
    const bool useMSAA = currentVisCamera->depthTexture->GetSampleCount() > 1;
    if (useMSAA && !DispatchDepthMSAAResolve(depthTexture, renderDepthState)) {
        return false;
    }

    SSAOParams params = {};
    params.outputSize = uint2(aoTexture->GetWidth(), aoTexture->GetHeight());
    params.outputSizeRcp = float2(1.0f / params.outputSize.x, 1.0f / params.outputSize.y);
    params.radius = frameData->GetRenderFrameSettings().ssaoRadius;
    params.power = frameData->GetRenderFrameSettings().ssaoPower;
    params.normalBias = params.radius * 0.01f;
    params.depthBias = params.radius * 0.004f;
    params.farPlaneDeviceDepth = currentVisCamera->projectionPolicy.GetDepthClearValue();

    const RHI::GPUResourceState depthInputState = useMSAA ? RHI::GPUResourceState::ShaderResource : renderDepthState;
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, depthInputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(aoTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SSAO);
    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameSSAOPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, aoTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
    RHI::renderer->Dispatch(mainCommandList, BE1::DivCeil(params.outputSize.x, SSAOBlockSizeX), BE1::DivCeil(params.outputSize.y, SSAOBlockSizeY), 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, depthInputState),
        RHI::Renderer::MakeTextureBarrier(aoTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}
