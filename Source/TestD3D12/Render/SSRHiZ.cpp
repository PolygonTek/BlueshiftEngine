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
#include "SSRHiZ.h"
#include "Render/Backend/RenderBackend.h"
#include "Render/RenderInternal.h"
#include "Render/GpuTiming.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipelineBootstrap.h"
#include "Shaders/ShaderInterop.h"

void SSRHiZ::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    genFirstMipShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssrHiZFirstMipShader", computeShaderModel, "SSR/GenSSRHiZFirstMip.hlsl", baseDir);
    genFirstMipMSAAShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssrHiZFirstMipMSAAShader", computeShaderModel, "SSR/GenSSRHiZFirstMipMSAA.hlsl", baseDir);
    genMipShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssrHiZMipShader", computeShaderModel, "SSR/GenSSRHiZMip.hlsl", baseDir);

    genFirstMipPipeline = shaderPipelineCache.GetComputePipeline(genFirstMipShaderTemplate, ShaderPermutationKey(), true);
    genFirstMipMSAAPipeline = shaderPipelineCache.GetComputePipeline(genFirstMipMSAAShaderTemplate, ShaderPermutationKey(), true);
    genMipPipeline = shaderPipelineCache.GetComputePipeline(genMipShaderTemplate, ShaderPermutationKey(), true);
}

void SSRHiZ::Shutdown() {
    genFirstMipShaderTemplate.reset();
    genFirstMipMSAAShaderTemplate.reset();
    genMipShaderTemplate.reset();

    genFirstMipPipeline.reset();
    genFirstMipMSAAPipeline.reset();
    genMipPipeline.reset();

    frameGenFirstMipPSO = nullptr;
    frameGenFirstMipMSAAPSO = nullptr;
    frameGenMipPSO = nullptr;
}

void SSRHiZ::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "SSRHiZ";

    bootstrap.TrackPipeline(groupName, genFirstMipPipeline);
    bootstrap.TrackPipeline(groupName, genFirstMipMSAAPipeline);
    bootstrap.TrackPipeline(groupName, genMipPipeline);
}

void SSRHiZ::UpdateFramePSOs() {
    frameGenFirstMipPSO = ShaderPipeline::GetReadyPSO(genFirstMipPipeline);
    frameGenFirstMipMSAAPSO = ShaderPipeline::GetReadyPSO(genFirstMipMSAAPipeline);
    frameGenMipPSO = ShaderPipeline::GetReadyPSO(genMipPipeline);
}

bool SSRHiZ::Dispatch(RenderBackend &backend) const {
    PROFILER_CPU_SCOPED_EVENT("SSRHiZ::Dispatch");

    RHI::CommandList *commandList = backend.GetCommandList();

    const FrameCamera *frameCamera = backend.GetFrameCamera();
    RHI::Texture *depthTexture = frameCamera->depthTexture;
    const RHI::ProjectionPolicy &projectionPolicy = frameCamera->projectionPolicy;
    const FrameSSRHiZ &ssrHiZ = frameCamera->ssrHiZ;
    const uint32_t mipCount = ssrHiZ.mipCount;

    if (mipCount == 0) {
        return false;
    }

    assert(ssrHiZ.validWidth == BE1::Max((depthTexture->GetWidth() + 1) >> 1, 1u));
    assert(ssrHiZ.validHeight == BE1::Max((depthTexture->GetHeight() + 1) >> 1, 1u));

    const bool useMSAA = depthTexture->GetSampleCount() > 1;

    RHI::PipelineState *genFirstMipPSO = useMSAA ? frameGenFirstMipMSAAPSO : frameGenFirstMipPSO;
    if (!genFirstMipPSO || (mipCount > 1 && !frameGenMipPSO)) {
        return false;
    }

    SSRHiZMipParams params = {};
    params.inputSize = uint2(depthTexture->GetWidth(), depthTexture->GetHeight()); // 입력 depth texture 크기
    params.outputSize = uint2(ssrHiZ.mipWidths[0], ssrHiZ.mipHeights[0]); // 출력 크기 (depth texture 의 절반 크기지만 POT 크기로 올림)
    params.validOutputSize = uint2(ssrHiZ.validWidth, ssrHiZ.validHeight); // 유효 출력 크기 (depth texture 의 절반 크기)
    params.clearDepth = projectionPolicy.GetDepthClearValue();
    params.reverseZ = projectionPolicy.reverseZ ? 1u : 0u;

    GpuTiming::ScopedTimer gpuScope(backend.GetGpuTiming(), commandList, GpuTiming::Tag::GenSSRHiZ);

    if (useMSAA) {
        DispatchFirstMipMSAA(commandList, *frameCamera, params);
    } else {
        DispatchFirstMip(commandList, *frameCamera, params);
    }

    if (mipCount > 1) {
        DispatchMips(commandList, ssrHiZ, params);
    }

    return true;
}

void SSRHiZ::DispatchFirstMip(RHI::CommandList *commandList, const FrameCamera &frameCamera, const SSRHiZMipParams &params) const {
    const FrameSSRHiZ &ssrHiZ = frameCamera.ssrHiZ;
    RHI::Texture *ssrHiZTexture = ssrHiZ.texture;

    // SSRHiZ 는 surface prepass 직후 opaque pass 이전에 생성되므로 depth 는 ShaderResource 상태다.
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(ssrHiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, 0));

    RHI::renderer->SetPSO(commandList, frameGenFirstMipPSO);
    RHI::renderer->SetTexture(commandList, 0, false, frameCamera.depthTexture, RHI::ViewID::PrimaryID);
    RHI::renderer->SetTexture(commandList, 0, true, ssrHiZTexture, ssrHiZ.mipUAVs[0]);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

    const uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRHiZBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRHiZBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(ssrHiZTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, 0));
}

void SSRHiZ::DispatchFirstMipMSAA(RHI::CommandList *commandList, const FrameCamera &frameCamera, const SSRHiZMipParams &params) const {
    const FrameSSRHiZ &ssrHiZ = frameCamera.ssrHiZ;
    RHI::Texture *ssrHiZTexture = ssrHiZ.texture;
    RHI::Texture *resolvedDepthTexture = frameCamera.resolvedDepthTexture;
    RHI::Texture *resolvedNormalRoughnessTexture = frameCamera.resolvedNormalRoughnessTexture;
    RHI::Texture *resolvedVelocityTexture = frameCamera.resolvedVelocityTexture;

    // SSRHiZ 는 surface prepass 직후 opaque pass 이전에 생성되므로 depth 는 ShaderResource 상태다.
    RHI::GPUBarrier firstMipStartBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(ssrHiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, 0),
        RHI::Renderer::MakeTextureBarrier(resolvedDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeTextureBarrier(resolvedNormalRoughnessTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeTextureBarrier(resolvedVelocityTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, firstMipStartBarriers, COUNT_OF(firstMipStartBarriers));

    RHI::renderer->SetPSO(commandList, frameGenFirstMipMSAAPSO);
    RHI::renderer->SetTexture(commandList, 0, false, frameCamera.depthTexture, RHI::ViewID::PrimaryID);
    RHI::renderer->SetTexture(commandList, 1, false, frameCamera.normalRoughnessTexture);
    RHI::renderer->SetTexture(commandList, 2, false, frameCamera.velocityTexture);
    RHI::renderer->SetTexture(commandList, 0, true, ssrHiZTexture, ssrHiZ.mipUAVs[0]);
    RHI::renderer->SetTexture(commandList, 1, true, resolvedDepthTexture);
    RHI::renderer->SetTexture(commandList, 2, true, resolvedNormalRoughnessTexture);
    RHI::renderer->SetTexture(commandList, 3, true, resolvedVelocityTexture);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

    const uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRHiZBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRHiZBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier firstMipEndBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(ssrHiZTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, 0),
        RHI::Renderer::MakeTextureBarrier(resolvedDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(resolvedNormalRoughnessTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(resolvedVelocityTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, firstMipEndBarriers, COUNT_OF(firstMipEndBarriers));
}

void SSRHiZ::DispatchMips(RHI::CommandList *commandList, const FrameSSRHiZ &ssrHiZ, SSRHiZMipParams &params) const {
    RHI::Texture *ssrHiZTexture = ssrHiZ.texture;
    const uint32_t mipCount = ssrHiZ.mipCount;

    RHI::renderer->SetPSO(commandList, frameGenMipPSO);

    // 나머지 hi-Z mip 을 생성한다.
    for (uint32_t dstMipLevel = 1; dstMipLevel < mipCount; ++dstMipLevel) {
        const uint32_t srcMipLevel = dstMipLevel - 1;

        params.inputSize = uint2(ssrHiZ.mipWidths[srcMipLevel], ssrHiZ.mipHeights[srcMipLevel]);
        params.outputSize = uint2(ssrHiZ.mipWidths[dstMipLevel], ssrHiZ.mipHeights[dstMipLevel]);
        params.validOutputSize = params.outputSize;

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(ssrHiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, dstMipLevel));

        RHI::renderer->SetTexture(commandList, 0, false, ssrHiZTexture, ssrHiZ.mipSRVs[srcMipLevel]);
        RHI::renderer->SetTexture(commandList, 0, true, ssrHiZTexture, ssrHiZ.mipUAVs[dstMipLevel]);
        RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

        const uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRHiZBlockSizeX);
        const uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRHiZBlockSizeY);
        RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

        // 다음 dispatch 가 방금 생성한 hi-Z mip 을 바로 읽을 수 있게 한다.
        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(ssrHiZTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, dstMipLevel));
    }
}
