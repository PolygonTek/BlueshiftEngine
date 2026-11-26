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
#include "Profiler.h"
#include "Bloom.h"
#include "Render/Backend/RenderBackend.h"
#include "Render/RenderInternal.h"
#include "Render/GpuTiming.h"
#include "Shaders/ShaderInterop.h"

bool Bloom::DispatchPyramidBloom(RenderBackend &backend) const {
    PROFILER_CPU_SCOPED_EVENT("Bloom::DispatchPyramidBloom");

    RHI::CommandList *commandList = backend.GetCommandList();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    RHI::Texture *inputColorTexture = backend.GetPostProcessInputTexture();
    const FrameBloom &data = backend.GetFrameCamera()->bloom;
    const RenderFrameSettings &renderFrameSettings = backend.GetFrameSettings();

    if (data.bloomMipCount < 2) {
        return false;
    }

    RHI::Texture *bloomDownTexture = data.bloomDownTexture;
    RHI::Texture *bloomUpTexture = data.bloomUpTexture;

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute));

    const float bloomThreshold = renderFrameSettings.bloomThreshold;
    const float bloomSoftThreshold = renderFrameSettings.bloomSoftThreshold;
    const float pyramidBloomPrefilterQuality = renderFrameSettings.pyramidBloomPrefilterQuality;
    const float pyramidBloomScatter = renderFrameSettings.pyramidBloomScatter;
    const float pyramidBloomPrefilterRadius = renderFrameSettings.pyramidBloomPrefilterRadius;
    const float pyramidBloomDownsampleRadius = renderFrameSettings.pyramidBloomDownsampleRadius;
    const float pyramidBloomUpsampleRadius = renderFrameSettings.pyramidBloomUpsampleRadius;

    auto dispatchDownsamplePass = [commandList, &gpuTiming, &data](GpuTiming::Tag timingTag, RHI::Texture *inputTexture, RHI::ViewID inputViewID, uint32_t inputWidth, uint32_t inputHeight, uint32_t outputMipLevel, auto &bloomParams) -> bool {
        const uint32_t outputWidth = data.bloomMipWidths[outputMipLevel];
        const uint32_t outputHeight = data.bloomMipHeights[outputMipLevel];
        if (inputWidth == 0 || inputHeight == 0 || outputWidth == 0 || outputHeight == 0) {
            return false;
        }

        bloomParams.outputSize = uint2(outputWidth, outputHeight);
        bloomParams.inputTexelSize = float2(1.0f / inputWidth, 1.0f / inputHeight);
        bloomParams.outputTexelSize = float2(1.0f / outputWidth, 1.0f / outputHeight);

        GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, timingTag);

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(data.bloomDownTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel));

        RHI::renderer->SetTexture(commandList, 0, false, inputTexture, inputViewID);
        RHI::renderer->SetTexture(commandList, 0, true, data.bloomDownTexture, data.bloomDownMipUAVs[outputMipLevel]);
        RHI::renderer->SetConstants(commandList, 0, &bloomParams, sizeof(bloomParams), 0);

        const uint32_t threadGroupCountX = BE1::DivCeil(outputWidth, BloomBlockSizeX);
        const uint32_t threadGroupCountY = BE1::DivCeil(outputHeight, BloomBlockSizeY);
        RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(data.bloomDownTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputMipLevel));

        return true;
    };

    // threshold 를 적용해 밝은 영역만 필터링하여 mip-0 에 blur downsample 한다.
    BloomPrefilterParams prefilterParams = {};
    prefilterParams.threshold = bloomThreshold;
    prefilterParams.softThreshold = bloomSoftThreshold;
    prefilterParams.prefilterQuality = pyramidBloomPrefilterQuality;
    prefilterParams.radius = pyramidBloomPrefilterRadius;
    RHI::renderer->SetPSO(commandList, frameBloomPrefilterPSO);
    if (!dispatchDownsamplePass(GpuTiming::Tag::BloomPrefilter, inputColorTexture, RHI::ViewID::PrimaryID, data.renderWidth, data.renderHeight, 0, prefilterParams)) {
        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));
        return false;
    }

    // 이후 mip 은 blur downsample 을 반복한다.
    BloomDownsampleParams downsampleParams = {};
    downsampleParams.radius = pyramidBloomDownsampleRadius;
    RHI::renderer->SetPSO(commandList, frameBloomDownsamplePSO);
    for (uint32_t mipLevel = 1; mipLevel < data.bloomMipCount; ++mipLevel) {
        const uint32_t inputMipLevel = mipLevel - 1;
        if (!dispatchDownsamplePass(GpuTiming::Tag::BloomDownsample, bloomDownTexture, data.bloomDownMipSRVs[inputMipLevel], data.bloomMipWidths[inputMipLevel], data.bloomMipHeights[inputMipLevel], mipLevel, downsampleParams)) {
            RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));
            return false;
        }
    }

    RHI::renderer->SetPSO(commandList, frameBloomUpsamplePSO);

    auto dispatchUpsamplePass = [commandList, &gpuTiming, &data](uint32_t outputMipLevel, RHI::Texture *lowerTexture, RHI::ViewID lowerViewID, uint32_t lowerWidth, uint32_t lowerHeight, float weight, float radius) -> bool {
        const uint32_t outputWidth = data.bloomMipWidths[outputMipLevel];
        const uint32_t outputHeight = data.bloomMipHeights[outputMipLevel];
        if (lowerWidth == 0 || lowerHeight == 0 || outputWidth == 0 || outputHeight == 0) {
            return false;
        }

        BloomUpsampleParams bloomParams = {};
        bloomParams.outputSize = uint2(outputWidth, outputHeight);
        bloomParams.inputTexelSize = float2(1.0f / lowerWidth, 1.0f / lowerHeight);
        bloomParams.outputTexelSize = float2(1.0f / outputWidth, 1.0f / outputHeight);
        bloomParams.weight = weight;
        bloomParams.radius = radius;

        GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::BloomUpsample);

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(data.bloomUpTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel));

        RHI::renderer->SetTexture(commandList, 0, false, data.bloomDownTexture, data.bloomDownMipSRVs[outputMipLevel]);
        RHI::renderer->SetTexture(commandList, 1, false, lowerTexture, lowerViewID);
        RHI::renderer->SetTexture(commandList, 0, true, data.bloomUpTexture, data.bloomUpMipUAVs[outputMipLevel]);
        RHI::renderer->SetConstants(commandList, 0, &bloomParams, sizeof(bloomParams), 0);

        const uint32_t threadGroupCountX = BE1::DivCeil(outputWidth, BloomBlockSizeX);
        const uint32_t threadGroupCountY = BE1::DivCeil(outputHeight, BloomBlockSizeY);
        RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(data.bloomUpTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputMipLevel));

        return true;
    };

    // 작은 mip 부터 큰 mip 으로 올라오면서 downsample 결과와 누적 bloom 을 합성한다.
    const uint32_t maxMipLevel = data.bloomMipCount - 1;
    for (uint32_t mipLevel = data.bloomMipCount - 1; mipLevel > 0; --mipLevel) {
        const uint32_t outputMipLevel = mipLevel - 1;
        RHI::Texture *lowerTexture = bloomDownTexture;
        RHI::ViewID lowerViewID = data.bloomDownMipSRVs[mipLevel];
        if (mipLevel < data.bloomMipCount - 1) {
            lowerTexture = bloomUpTexture;
            lowerViewID = data.bloomUpMipSRVs[mipLevel];
        }

        const float bloomWeight = BE1::Math::Pow(pyramidBloomScatter, static_cast<float>(maxMipLevel - mipLevel));

        if (!dispatchUpsamplePass(outputMipLevel, lowerTexture, lowerViewID, data.bloomMipWidths[mipLevel], data.bloomMipHeights[mipLevel], bloomWeight, pyramidBloomUpsampleRadius)) {
            RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));
            return false;
        }
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(inputColorTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));

    return true;
}
