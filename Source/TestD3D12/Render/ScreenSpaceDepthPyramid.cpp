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
#include "ScreenSpaceDepthPyramid.h"
#include "Render/RenderInternal.h"
#include "Render/GpuTiming.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipelineBootstrap.h"
#include "Shaders/ShaderInterop.h"

void ScreenSpaceDepthPyramid::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    firstMipShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_screenSpaceDepthPyramidFirstShader", computeShaderModel, "Reflection/GenerateScreenSpaceDepthPyramidFirst.hlsl", baseDir);
    firstMipMSAAShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_screenSpaceDepthPyramidFirstMSAAShader", computeShaderModel, "Reflection/GenerateScreenSpaceDepthPyramidFirstMSAA.hlsl", baseDir);
    reduceShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_screenSpaceDepthPyramidReduceShader", computeShaderModel, "Reflection/GenerateScreenSpaceDepthPyramidReduce.hlsl", baseDir);

    firstMipPipeline = shaderPipelineCache.GetComputePipeline(firstMipShaderTemplate, ShaderPermutationKey(), true);
    firstMipMSAAPipeline = shaderPipelineCache.GetComputePipeline(firstMipMSAAShaderTemplate, ShaderPermutationKey(), true);
    reducePipeline = shaderPipelineCache.GetComputePipeline(reduceShaderTemplate, ShaderPermutationKey(), true);
}

void ScreenSpaceDepthPyramid::Shutdown() {
    firstMipPipeline.reset();
    firstMipShaderTemplate.reset();
    frameFirstMipPSO = nullptr;
    firstMipMSAAPipeline.reset();
    firstMipMSAAShaderTemplate.reset();
    frameFirstMipMSAAPSO = nullptr;
    reducePipeline.reset();
    reduceShaderTemplate.reset();
    frameReducePSO = nullptr;
}

void ScreenSpaceDepthPyramid::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "ScreenSpaceDepthPyramid";

    bootstrap.TrackPipeline(groupName, firstMipPipeline);
    bootstrap.TrackPipeline(groupName, firstMipMSAAPipeline);
    bootstrap.TrackPipeline(groupName, reducePipeline);
}

void ScreenSpaceDepthPyramid::UpdateFramePSOs() {
    frameFirstMipPSO = ShaderPipeline::GetReadyPSO(firstMipPipeline);
    frameFirstMipMSAAPSO = ShaderPipeline::GetReadyPSO(firstMipMSAAPipeline);
    frameReducePSO = ShaderPipeline::GetReadyPSO(reducePipeline);
}

bool ScreenSpaceDepthPyramid::Dispatch(RHI::CommandList *commandList, GpuTiming &gpuTiming, RHI::Texture *depthTexture, RHI::Texture *normalMSAATexture, RHI::Texture *resolvedDepthTexture, RHI::Texture *resolvedNormalTexture, const RHI::ProjectionPolicy &projectionPolicy, const ScreenSpaceDepthPyramidFrameData &data) const {
    PROFILER_CPU_SCOPED_EVENT("ScreenSpaceDepthPyramid::Dispatch");

    RHI::Texture *depthPyramidTexture = data.texture;
    const uint32_t mipCount = data.mipCount;
    const bool useMSAA = depthTexture && depthTexture->GetSampleCount() > 1;

    if (!depthTexture || !depthPyramidTexture || mipCount == 0 ||
        (useMSAA && (!normalMSAATexture || !resolvedDepthTexture || !resolvedNormalTexture))) {
        return false;
    }

    assert(data.validWidth == BE1::Max((depthTexture->GetWidth() + 1) >> 1, 1u));
    assert(data.validHeight == BE1::Max((depthTexture->GetHeight() + 1) >> 1, 1u));

    // MSAA depth 는 Texture2DMS 로 각 sample 을 직접 읽어 min/max 를 구해야 하므로, 단일 sample 용 Texture2D shader 와 PSO 를 분리한다.
    // MSAA variant 는 같은 sample 순회를 재사용해 SSR 입력 depth/normal 도 함께 resolve 한다.
    RHI::PipelineState *firstMipPSO = useMSAA ? frameFirstMipMSAAPSO : frameFirstMipPSO;
    if (!firstMipPSO || (mipCount > 1 && !frameReducePSO)) {
        return false;
    }

    ScreenSpaceDepthPyramidParams params = {};
    params.inputSize = uint2(depthTexture->GetWidth(), depthTexture->GetHeight());
    params.outputSize = uint2(data.mipWidths[0], data.mipHeights[0]);
    params.validOutputSize = uint2(data.validWidth, data.validHeight);
    params.farPlaneDeviceDepth = projectionPolicy.GetDepthClearValue();
    params.useMinDepth = projectionPolicy.reverseZ ? 0u : 1u;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::GenScreenSpaceDepthPyramid);

    RHI::GPUBarrier firstMipStartBarriers[5];
    uint32_t firstMipStartBarrierCount = 0;
    firstMipStartBarriers[firstMipStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::ShaderResourceCompute);
    firstMipStartBarriers[firstMipStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, 0);
    if (useMSAA) {
        firstMipStartBarriers[firstMipStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(normalMSAATexture, RHI::GPUResourceState::RenderTarget, RHI::GPUResourceState::ShaderResourceCompute);
        firstMipStartBarriers[firstMipStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(resolvedDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
        firstMipStartBarriers[firstMipStartBarrierCount++] = RHI::Renderer::MakeTextureBarrier(resolvedNormalTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    }
    RHI::renderer->Barrier(commandList, firstMipStartBarriers, firstMipStartBarrierCount);

    RHI::renderer->SetPSO(commandList, firstMipPSO);
    RHI::renderer->SetTexture(commandList, 0, false, depthTexture, RHI::ViewID::PrimaryID);
    RHI::renderer->SetTexture(commandList, 0, true, depthPyramidTexture, data.mipUAVs[0]);
    if (useMSAA) {
        RHI::renderer->SetTexture(commandList, 1, false, normalMSAATexture);
        RHI::renderer->SetTexture(commandList, 1, true, resolvedDepthTexture);
        RHI::renderer->SetTexture(commandList, 2, true, resolvedNormalTexture);
    }
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

    uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, ScreenSpaceDepthPyramidBlockSizeX);
    uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, ScreenSpaceDepthPyramidBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier firstMipEndBarriers[5];
    uint32_t firstMipEndBarrierCount = 0;
    firstMipEndBarriers[firstMipEndBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::DepthWrite);
    firstMipEndBarriers[firstMipEndBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, 0, 0);
    if (useMSAA) {
        firstMipEndBarriers[firstMipEndBarrierCount++] = RHI::Renderer::MakeTextureBarrier(normalMSAATexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::RenderTarget);
        firstMipEndBarriers[firstMipEndBarrierCount++] = RHI::Renderer::MakeTextureBarrier(resolvedDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
        firstMipEndBarriers[firstMipEndBarrierCount++] = RHI::Renderer::MakeTextureBarrier(resolvedNormalTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
    }
    RHI::renderer->Barrier(commandList, firstMipEndBarriers, firstMipEndBarrierCount);

    if (mipCount > 1) {
        RHI::renderer->SetPSO(commandList, frameReducePSO);
    }

    // 한번의 dispatch 로 하나의 mip 을 생성한다.
    for (uint32_t outputMipLevel = 1; outputMipLevel < mipCount; ++outputMipLevel) {
        const uint32_t inputMipLevel = outputMipLevel - 1;

        params.inputSize = uint2(data.mipWidths[inputMipLevel], data.mipHeights[inputMipLevel]);
        params.outputSize = uint2(data.mipWidths[outputMipLevel], data.mipHeights[outputMipLevel]);
        params.validOutputSize = params.outputSize;

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel));

        RHI::renderer->SetTexture(commandList, 0, false, depthPyramidTexture, data.mipSRVs[inputMipLevel]);
        RHI::renderer->SetTexture(commandList, 0, true, depthPyramidTexture, data.mipUAVs[outputMipLevel]);
        RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

        threadGroupCountX = BE1::DivCeil(params.outputSize.x, ScreenSpaceDepthPyramidBlockSizeX);
        threadGroupCountY = BE1::DivCeil(params.outputSize.y, ScreenSpaceDepthPyramidBlockSizeY);
        RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

        // 다음 dispatch 가 방금 생성한 mip 을 바로 읽을 수 있게 한다.
        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, 0, outputMipLevel));
    }

    RHI::GPUBarrier finalBarriers[RenderScreenSpaceDepthPyramidMaxLevelCount];
    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        finalBarriers[mipLevel] = RHI::Renderer::MakeTextureBarrier(depthPyramidTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, mipLevel);
    }
    RHI::renderer->Barrier(commandList, finalBarriers, mipCount);

    return true;
}
