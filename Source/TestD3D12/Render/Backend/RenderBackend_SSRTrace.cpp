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
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"
#include "Shaders/ShaderInterop.h"

bool RenderBackend::DispatchSSRTrace() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSSRTrace");

    RHI::Texture *renderDepthTexture = currentFrameCamera->depthTexture;
    const bool useMSAA = renderDepthTexture && renderDepthTexture->GetSampleCount() > 1;
    RHI::Texture *depthTexture = useMSAA ? currentFrameCamera->resolvedDepthTexture : renderDepthTexture;
    RHI::Texture *normalTexture = useMSAA ? currentFrameCamera->resolvedNormalTexture : currentFrameCamera->normalTexture;
    RHI::Texture *depthPyramidTexture = currentFrameCamera->screenSpaceDepthPyramid.texture;
    RHI::Texture *traceTexture = currentFrameCamera->ssrTraceTexture;

    if (!depthTexture || !normalTexture || !depthPyramidTexture || !traceTexture || !cameraConstantBuffer || currentFrameCamera->screenSpaceDepthPyramid.mipCount == 0) {
        return false;
    }

    assert(depthTexture->GetWidth() == normalTexture->GetWidth() && depthTexture->GetHeight() == normalTexture->GetHeight());
    assert(depthTexture->GetSampleCount() == 1 && normalTexture->GetSampleCount() == 1);
    assert(traceTexture->GetWidth() == BE1::Max((depthTexture->GetWidth() + 1u) >> 1, 1u));
    assert(traceTexture->GetHeight() == BE1::Max((depthTexture->GetHeight() + 1u) >> 1, 1u));

    RHI::PipelineState *tracePSO = frameSSRTracePSO;
    if (!tracePSO) {
        return false;
    }

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();

    SSRTraceParams params = {};
    params.inputSize = uint2(depthTexture->GetWidth(), depthTexture->GetHeight());
    params.outputSize = uint2(traceTexture->GetWidth(), traceTexture->GetHeight());
    params.depthPyramidMipCount = currentFrameCamera->screenSpaceDepthPyramid.mipCount;
    params.maxIterations = frameSettings.ssrMaxIterations;
    params.binaryRefinementSteps = frameSettings.ssrBinaryRefinementSteps;
    params.useMinDepth = currentFrameCamera->projectionPolicy.reverseZ ? 0u : 1u;
    params.maxDistance = frameSettings.ssrMaxDistance;
    params.thickness = frameSettings.ssrThickness;
    params.normalBias = frameSettings.ssrNormalBias;

    const RHI::GPUResourceState depthInputState = useMSAA ? RHI::GPUResourceState::ShaderResource : RHI::GPUResourceState::DepthWrite;
    const RHI::GPUResourceState normalInputState = useMSAA ? RHI::GPUResourceState::ShaderResource : RHI::GPUResourceState::RenderTarget;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SSRTrace);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, depthInputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(normalTexture, normalInputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(traceTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, tracePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, normalTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, depthPyramidTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, traceTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRTraceBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRTraceBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, depthInputState),
        RHI::Renderer::MakeTextureBarrier(normalTexture, RHI::GPUResourceState::ShaderResourceCompute, normalInputState),
        RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(traceTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}
