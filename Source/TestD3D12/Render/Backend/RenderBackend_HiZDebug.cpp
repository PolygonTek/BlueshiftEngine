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
#include "Render/RenderSystem.h"
#include "Render/RenderFrameData.h"
#include "Render/RenderInternal.h"
#include "Shaders/ShaderInterop.h"

bool RenderBackend::DispatchHiZDebugPreview() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchHiZDebugPreview");

    // 실제 Hi-Z 리소스의 mip 수는 화면 크기에 따라 snapshot 의 전역 상한보다 작을 수 있다.
    const uint32_t mipLevel = BE1::Min(frameData->GetRenderFrameSettings().hiZDebugPreviewMipLevel, currentVisCamera->hiZMipCount - 1);
    const uint32_t inputWidth = currentVisCamera->hiZMipWidths[mipLevel];
    const uint32_t inputHeight = currentVisCamera->hiZMipHeights[mipLevel];
    const uint32_t outputWidth = currentVisCamera->hiZDebugPreviewTexture->GetWidth();
    const uint32_t outputHeight = currentVisCamera->hiZDebugPreviewTexture->GetHeight();

    if (inputWidth == 0 || inputHeight == 0 || outputWidth == 0 || outputHeight == 0) {
        return false;
    }

    HiZDebugPreviewParams hiZDebugPreviewParams = {};
    hiZDebugPreviewParams.inputSize = uint2(inputWidth, inputHeight);
    hiZDebugPreviewParams.outputSize = uint2(outputWidth, outputHeight);
    hiZDebugPreviewParams.projectionDepthParams = float2(currentVisCamera->projectionDepthA, currentVisCamera->projectionDepthB);
    hiZDebugPreviewParams.ndcDepthScaleBias = float2(currentVisCamera->ndcDepthScale, currentVisCamera->ndcDepthBias);
    hiZDebugPreviewParams.linearDepthScale = currentVisCamera->hiZDebugLinearDepthScale;
    hiZDebugPreviewParams.orthogonal = currentVisCamera->projectionOrthogonal ? 1u : 0u;

    RHI::ConstantBuffer *hiZDebugPreviewConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(HiZDebugPreviewParams), &hiZDebugPreviewParams);
    if (!hiZDebugPreviewConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->hiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute, 0, mipLevel),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->hiZDebugPreviewTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::HiZDebugPreview);

    RHI::renderer->Barrier(mainCommandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(mainCommandList, frameHiZDebugPreviewPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, currentVisCamera->hiZTexture, currentVisCamera->hiZMipSRVs[mipLevel]);
    RHI::renderer->SetTexture(mainCommandList, 0, true, currentVisCamera->hiZDebugPreviewTexture);
    RHI::renderer->SetConstantBuffer(mainCommandList, 0, hiZDebugPreviewConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputWidth, HiZDebugPreviewBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputHeight, HiZDebugPreviewBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->hiZTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, mipLevel),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->hiZDebugPreviewTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}
