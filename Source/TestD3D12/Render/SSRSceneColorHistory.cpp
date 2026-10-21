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
#include "SSRSceneColorHistory.h"
#include "Render/Backend/RenderBackend.h"
#include "Render/RenderInternal.h"
#include "Render/GpuTiming.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipelineBootstrap.h"
#include "Shaders/ShaderInterop.h"

void SSRSceneColorHistory::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    genMipShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssrSceneColorHistoryMipShader", computeShaderModel, "SSR/GenSSRSceneColorHistoryMip.hlsl", baseDir);

    genMipDepthShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssrSceneColorHistoryMipDepthShader", computeShaderModel, "SSR/GenSSRSceneColorHistoryMip.hlsl", baseDir, "CSMain", "SSR_CAPTURE_FULL_RESOLUTION_DEPTH=1");
    genMipPipeline = shaderPipelineCache.GetComputePipeline(genMipShaderTemplate, ShaderPermutationKey(), true);
    genMipDepthPipeline = shaderPipelineCache.GetComputePipeline(genMipDepthShaderTemplate, ShaderPermutationKey(), true);
}

void SSRSceneColorHistory::Shutdown() {
    genMipShaderTemplate.reset();
    genMipDepthShaderTemplate.reset();

    genMipPipeline.reset();
    genMipDepthPipeline.reset();

    frameGenMipPSO = nullptr;
    frameGenMipDepthPSO = nullptr;
}

void SSRSceneColorHistory::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "SSRSceneColorHistory";

    bootstrap.TrackPipeline(groupName, genMipPipeline);
    bootstrap.TrackPipeline(groupName, genMipDepthPipeline);
}

void SSRSceneColorHistory::UpdateFramePSOs() {
    frameGenMipPSO = ShaderPipeline::GetReadyPSO(genMipPipeline);
    frameGenMipDepthPSO = ShaderPipeline::GetReadyPSO(genMipDepthPipeline);
}

bool SSRSceneColorHistory::Dispatch(RenderBackend &backend) const {
    PROFILER_CPU_SCOPED_EVENT("SSRSceneColorHistory::Dispatch");

    RHI::CommandList *commandList = backend.GetCommandList();

    const FrameCamera *frameCamera = backend.GetFrameCamera();
    RHI::Texture *colorTexture = frameCamera->colorTexture;
    const bool useMSAA = colorTexture->GetSampleCount() > 1;

    // Opaque MSAA rendering already resolves into the ocean snapshot when it exists.
    // Both SSR paths finish reading the previous history before this capture runs.
    const bool hasOpaqueSnapshot = frameCamera->oceanSurfacePSO && frameCamera->snapshotColorTexture && frameCamera->snapshotDepthTexture;
    RHI::Texture *inputColorTexture = useMSAA ? (hasOpaqueSnapshot ? frameCamera->snapshotColorTexture : frameCamera->resolvedColorTexture) : colorTexture;
    const RHI::GPUResourceState inputColorState = useMSAA ? RHI::GPUResourceState::ShaderResource : RHI::GPUResourceState::RenderTarget;

    const FrameSSRSceneColorHistory &data = frameCamera->ssrSceneColorHistory;
    RHI::Texture *radianceTexture = data.radianceTexture;
    RHI::Texture *radiancePyramidTexture = data.radiancePyramidTexture;
    RHI::Texture *inputSSRHiZTexture = frameCamera->ssrHiZ.texture;
    RHI::Texture *depthExtremaTexture = data.depthExtremaTexture;
    RHI::Texture *fullResolutionDepthTexture = data.fullResolutionDepthTexture;
    RHI::Texture *inputDepthTexture = useMSAA ? frameCamera->resolvedDepthTexture : frameCamera->depthTexture;
    const RHI::GPUResourceState inputDepthState = useMSAA ? RHI::GPUResourceState::ShaderResource : RHI::GPUResourceState::DepthWrite;

    if (data.mipCount == 0 || !frameGenMipPSO || (fullResolutionDepthTexture && !frameGenMipDepthPSO)) {
        return false;
    }

    assert(inputColorTexture->GetSampleCount() == 1);
    assert(inputColorTexture->GetWidth() == radianceTexture->GetWidth() && inputColorTexture->GetHeight() == radianceTexture->GetHeight());
    assert(inputColorTexture->GetFormat() == radianceTexture->GetFormat());
    assert(data.mipWidths[0] == BE1::Max((radianceTexture->GetWidth() + 1u) >> 1, 1u));
    assert(data.mipHeights[0] == BE1::Max((radianceTexture->GetHeight() + 1u) >> 1, 1u));
    assert(depthExtremaTexture->GetWidth() == data.mipWidths[0] && depthExtremaTexture->GetHeight() == data.mipHeights[0]);
    assert(inputSSRHiZTexture->GetFormat() == depthExtremaTexture->GetFormat());
    assert(inputSSRHiZTexture->GetWidth() >= depthExtremaTexture->GetWidth() && inputSSRHiZTexture->GetHeight() >= depthExtremaTexture->GetHeight());

    SSRSceneColorHistoryMipParams params = {};
    params.inputSize = uint2(radianceTexture->GetWidth(), radianceTexture->GetHeight());
    params.outputSize = uint2(data.mipWidths[0], data.mipHeights[0]);
    params.inputTexelSize = float2(1.0f / params.inputSize.x, 1.0f / params.inputSize.y);
    params.outputTexelSize = float2(1.0f / params.outputSize.x, 1.0f / params.outputSize.y);
    params.compressInput = 1;

    GpuTiming::ScopedTimer gpuScope(backend.GetGpuTiming(), commandList, GpuTiming::Tag::UpdateSSRSceneColorHistory);

    const RHI::GPUBarrier captureStartBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputColorTexture, inputColorState, RHI::GPUResourceState::CopySrc),
        RHI::Renderer::MakeTextureBarrier(radianceTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::CopyDst),
        RHI::Renderer::MakeTextureBarrier(inputSSRHiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::CopySrc, 0, 0),
        RHI::Renderer::MakeTextureBarrier(depthExtremaTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::CopyDst)
    };
    RHI::renderer->Barrier(commandList, captureStartBarriers, COUNT_OF(captureStartBarriers));

    // 현재 color texture 를 radiance texture 에 복사한다.
    RHI::renderer->CopyTexture(commandList, radianceTexture, 0, 0, 0, 0, 0, inputColorTexture, 0, 0, 0, 0, 0, inputColorTexture->GetWidth(), inputColorTexture->GetHeight(), 1);

    // 현재 SSR Hi-Z texture (mip 0) 를 depth extrema texture 에 복사한다.
    RHI::renderer->CopyTexture(commandList, depthExtremaTexture, 0, 0, 0, 0, 0, inputSSRHiZTexture, 0, 0, 0, 0, 0, depthExtremaTexture->GetWidth(), depthExtremaTexture->GetHeight(), 1);

    RHI::GPUBarrier firstMipStartBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::CopySrc, inputColorState),
        RHI::Renderer::MakeTextureBarrier(radianceTexture, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(inputSSRHiZTexture, RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::ShaderResource, 0, 0),
        RHI::Renderer::MakeTextureBarrier(depthExtremaTexture, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(radiancePyramidTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, 0)
    };
    RHI::renderer->Barrier(commandList, firstMipStartBarriers, COUNT_OF(firstMipStartBarriers));

    // 이전 프레임에 사용할 full-resolution radiance history 를 직접 downsample 해서 전용 pyramid 의 물리적 mip 0 (논리적 mip 1) 을 생성한다.
    if (fullResolutionDepthTexture) {
        const RHI::GPUBarrier depthStartBarriers[] = {
            RHI::Renderer::MakeTextureBarrier(inputDepthTexture, inputDepthState, RHI::GPUResourceState::ShaderResource),
            RHI::Renderer::MakeTextureBarrier(fullResolutionDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
        };
        RHI::renderer->Barrier(commandList, depthStartBarriers, COUNT_OF(depthStartBarriers));
    }
    RHI::renderer->SetPSO(commandList, fullResolutionDepthTexture ? frameGenMipDepthPSO : frameGenMipPSO);
    if (fullResolutionDepthTexture) {
        RHI::renderer->SetTexture(commandList, 1, false, inputDepthTexture);
        RHI::renderer->SetTexture(commandList, 1, true, fullResolutionDepthTexture);
    }
    RHI::renderer->SetTexture(commandList, 0, false, radianceTexture);
    RHI::renderer->SetTexture(commandList, 0, true, radiancePyramidTexture, data.mipUAVs[0]);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

    uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRSceneColorHistoryBlockSizeX);
    uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRSceneColorHistoryBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(radiancePyramidTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, 0));

    if (fullResolutionDepthTexture) {
        const RHI::GPUBarrier depthEndBarriers[] = {
            RHI::Renderer::MakeTextureBarrier(inputDepthTexture, RHI::GPUResourceState::ShaderResource, inputDepthState),
            RHI::Renderer::MakeTextureBarrier(fullResolutionDepthTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
        };
        RHI::renderer->Barrier(commandList, depthEndBarriers, COUNT_OF(depthEndBarriers));
        RHI::renderer->SetPSO(commandList, frameGenMipPSO);
    }

    // 한번의 dispatch 로 하나의 mip 을 생성한다. 정규화된 footprint 를 사용해 홀수 크기의 가장자리 texel 도 축소 결과에 포함한다.
    params.compressInput = 0;
    for (uint32_t outputMipLevel = 1; outputMipLevel < data.mipCount; ++outputMipLevel) {
        const uint32_t inputMipLevel = outputMipLevel - 1;

        params.inputSize = uint2(data.mipWidths[inputMipLevel], data.mipHeights[inputMipLevel]);
        params.outputSize = uint2(data.mipWidths[outputMipLevel], data.mipHeights[outputMipLevel]);
        params.inputTexelSize = float2(1.0f / params.inputSize.x, 1.0f / params.inputSize.y);
        params.outputTexelSize = float2(1.0f / params.outputSize.x, 1.0f / params.outputSize.y);

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(radiancePyramidTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel));

        RHI::renderer->SetTexture(commandList, 0, false, radiancePyramidTexture, data.mipSRVs[inputMipLevel]);
        RHI::renderer->SetTexture(commandList, 0, true, radiancePyramidTexture, data.mipUAVs[outputMipLevel]);
        RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

        threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRSceneColorHistoryBlockSizeX);
        threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRSceneColorHistoryBlockSizeY);
        RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(radiancePyramidTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputMipLevel));
    }

    return true;
}
