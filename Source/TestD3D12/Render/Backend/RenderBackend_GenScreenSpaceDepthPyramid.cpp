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
#include "Render/RenderInternal.h"
#include "Shaders/ShaderInterop.h"

bool RenderBackend::DispatchGenScreenSpaceDepthPyramid() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchGenScreenSpaceDepthPyramid");

    RHI::Texture *depthTexture = currentVisCamera->depthTexture;
    RHI::Texture *depthPyramidTexture = currentVisCamera->screenSpaceDepthPyramidTexture;
    const uint32_t mipCount = currentVisCamera->screenSpaceDepthPyramidMipCount;

    if (!depthTexture || !depthPyramidTexture || mipCount == 0) {
        return false;
    }

    assert(currentVisCamera->screenSpaceDepthPyramidValidWidth == BE1::Max((depthTexture->GetWidth() + 1) >> 1, 1u));
    assert(currentVisCamera->screenSpaceDepthPyramidValidHeight == BE1::Max((depthTexture->GetHeight() + 1) >> 1, 1u));
    assert(currentVisCamera->screenSpaceDepthPyramidMipWidths[0] == depthPyramidTexture->GetWidth());
    assert(currentVisCamera->screenSpaceDepthPyramidMipHeights[0] == depthPyramidTexture->GetHeight());

    // MSAA depth 는 Texture2DMS 로 각 sample 을 직접 읽어 min/max 를 구해야 하므로, 단일 sample 용 Texture2D shader 와 PSO 를 분리한다.
    RHI::PipelineState *firstMipPSO = depthTexture->GetSampleCount() > 1 ? frameScreenSpaceDepthPyramidFirstMSAAPSO : frameScreenSpaceDepthPyramidFirstPSO;
    if (!firstMipPSO || (mipCount > 1 && !frameScreenSpaceDepthPyramidReducePSO)) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::GenScreenSpaceDepthPyramid);

    ScreenSpaceDepthPyramidParams params = {};
    params.inputSize = uint2(depthTexture->GetWidth(), depthTexture->GetHeight());
    params.outputSize = uint2(currentVisCamera->screenSpaceDepthPyramidMipWidths[0], currentVisCamera->screenSpaceDepthPyramidMipHeights[0]);
    params.validOutputSize = uint2(currentVisCamera->screenSpaceDepthPyramidValidWidth, currentVisCamera->screenSpaceDepthPyramidValidHeight);

    RHI::GPUBarrier firstMipStartBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, 0)
    };
    RHI::renderer->Barrier(mainCommandList, firstMipStartBarriers, COUNT_OF(firstMipStartBarriers));

    RHI::renderer->SetPSO(mainCommandList, firstMipPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture, RHI::ViewID::PrimaryID);
    RHI::renderer->SetTexture(mainCommandList, 0, true, depthPyramidTexture, currentVisCamera->screenSpaceDepthPyramidMipUAVs[0]);
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);

    uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, ScreenSpaceDepthPyramidBlockSizeX);
    uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, ScreenSpaceDepthPyramidBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier firstMipEndBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, 0, 0)
    };
    RHI::renderer->Barrier(mainCommandList, firstMipEndBarriers, COUNT_OF(firstMipEndBarriers));

    if (mipCount > 1) {
        RHI::renderer->SetPSO(mainCommandList, frameScreenSpaceDepthPyramidReducePSO);
    }

    // 한번의 dispatch 로 하나의 mip 을 생성한다.
    for (uint32_t outputMipLevel = 1; outputMipLevel < mipCount; ++outputMipLevel) {
        const uint32_t inputMipLevel = outputMipLevel - 1;

        params.inputSize = uint2(currentVisCamera->screenSpaceDepthPyramidMipWidths[inputMipLevel], currentVisCamera->screenSpaceDepthPyramidMipHeights[inputMipLevel]);
        params.outputSize = uint2(currentVisCamera->screenSpaceDepthPyramidMipWidths[outputMipLevel], currentVisCamera->screenSpaceDepthPyramidMipHeights[outputMipLevel]);
        params.validOutputSize = params.outputSize;

        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel));

        RHI::renderer->SetTexture(mainCommandList, 0, false, depthPyramidTexture, currentVisCamera->screenSpaceDepthPyramidMipSRVs[inputMipLevel]);
        RHI::renderer->SetTexture(mainCommandList, 0, true, depthPyramidTexture, currentVisCamera->screenSpaceDepthPyramidMipUAVs[outputMipLevel]);
        RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);

        threadGroupCountX = BE1::DivCeil(params.outputSize.x, ScreenSpaceDepthPyramidBlockSizeX);
        threadGroupCountY = BE1::DivCeil(params.outputSize.y, ScreenSpaceDepthPyramidBlockSizeY);
        RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

        // 다음 dispatch 가 방금 생성한 mip 을 바로 읽을 수 있게 한다.
        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, 0, outputMipLevel));
    }

    RHI::GPUBarrier finalBarriers[RenderScreenSpaceDepthPyramidMaxLevelCount];
    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        finalBarriers[mipLevel] = RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, mipLevel);
    }
    RHI::renderer->Barrier(mainCommandList, finalBarriers, mipCount);

    return true;
}
