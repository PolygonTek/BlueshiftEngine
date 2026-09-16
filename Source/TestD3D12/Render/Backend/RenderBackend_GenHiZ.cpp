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
#include "Render/RenderInternal.h"
#include "Render/RenderFrameData.h"
#include "Shaders/ShaderInterop.h"

// width, height 가 1 이거나 둘 다 짝수이면 true 를 반환한다.
static bool CanFastReduceHiZMip(uint32_t width, uint32_t height) {
    return (width <= 1 || (width & 1) == 0) && (height <= 1 || (height & 1) == 0);
}

// inputMipLevel 부터 시작해서 maxReductionCount 만큼의 mip level 을 fast reduce 할 수 있는지 확인하고, fast reduce 가능한 mip level 의 개수를 반환한다.
static uint32_t CountFastHiZReductions(const VisCamera *visCamera, uint32_t inputMipLevel, uint32_t maxReductionCount) {
    uint32_t reductionCount = 0;
    while (reductionCount < maxReductionCount && inputMipLevel + reductionCount + 1 < visCamera->hiZMipCount) {
        const uint32_t mipLevel = inputMipLevel + reductionCount;
        if (!CanFastReduceHiZMip(visCamera->hiZMipWidths[mipLevel], visCamera->hiZMipHeights[mipLevel])) {
            break;
        }

        reductionCount++;
    }
    return reductionCount;
}

// (연속된) input mip dimension 이 짝수인 경우 inputMipLevel 부터 시작해서 outputMipCount 만큼의 mip level 을 fast reduce 한다.
static bool DispatchFastHiZReductionBatch(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const VisCamera *visCamera, RHI::Texture *hiZTexture, RHI::PipelineState *pipelineState, uint32_t useMinDepth, uint32_t inputMipLevel, uint32_t outputMipCount) {
    const uint32_t startOutputMipLevel = inputMipLevel + 1;
    const RHI::ViewID inputViewID = visCamera->hiZMipSRVs[inputMipLevel];

    const uint32_t inputWidth = visCamera->hiZMipWidths[inputMipLevel];
    const uint32_t inputHeight = visCamera->hiZMipHeights[inputMipLevel];
    const uint32_t outputWidth = visCamera->hiZMipWidths[startOutputMipLevel];
    const uint32_t outputHeight = visCamera->hiZMipHeights[startOutputMipLevel];

    HiZMipGenParams hiZMipGenParams = {};
    hiZMipGenParams.inputSize = uint2(inputWidth, inputHeight);
    hiZMipGenParams.inputSizeRcp = float2(1.0f / inputWidth, 1.0f / inputHeight);
    hiZMipGenParams.outputSize = uint2(outputWidth, outputHeight);
    hiZMipGenParams.outputMipCount = outputMipCount;
    hiZMipGenParams.useMinDepth = useMinDepth;

    RHI::ConstantBuffer *hiZMipGenConstantBuffer = frameThreadData->AllocConstant(sizeof(HiZMipGenParams), &hiZMipGenParams);
    if (!hiZMipGenConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[5];
    uint32_t startBarrierCount = 0;
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute, 0, inputMipLevel);
    for (uint32_t outputIndex = 0; outputIndex < outputMipCount; ++outputIndex) {
        const uint32_t outputMipLevel = startOutputMipLevel + outputIndex;
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel);
    }
    RHI::renderer->Barrier(commandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(commandList, pipelineState);
    RHI::renderer->SetTexture(commandList, 0, false, hiZTexture, inputViewID);

    for (uint32_t outputIndex = 0; outputIndex < 4; ++outputIndex) {
        const uint32_t mipLevel = outputIndex < outputMipCount ? startOutputMipLevel + outputIndex : startOutputMipLevel;
        RHI::renderer->SetTexture(commandList, outputIndex, true, hiZTexture, visCamera->hiZMipUAVs[mipLevel]);
    }

    RHI::renderer->SetConstantBuffer(commandList, 0, hiZMipGenConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputWidth, GenHiZMipBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputHeight, GenHiZMipBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[5];
    uint32_t endBarrierCount = 0;
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, inputMipLevel);
    for (uint32_t outputIndex = 0; outputIndex < outputMipCount; ++outputIndex) {
        const uint32_t outputMipLevel = startOutputMipLevel + outputIndex;
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputMipLevel);
    }
    RHI::renderer->Barrier(commandList, endBarriers, endBarrierCount);

    return true;
}

// input mip dimension 이 홀수인 경우, 2x3/3x2/3x3 까지 커버 가능하도록 inputMipLevel 을 reduce 한다.
static bool DispatchOddDimHiZReduction(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const VisCamera *visCamera, RHI::Texture *hiZTexture, RHI::PipelineState *pipelineState, uint32_t useMinDepth, uint32_t inputMipLevel) {
    const uint32_t outputMipLevel = inputMipLevel + 1;
    const RHI::ViewID inputViewID = visCamera->hiZMipSRVs[inputMipLevel];

    const uint32_t inputWidth = visCamera->hiZMipWidths[inputMipLevel];
    const uint32_t inputHeight = visCamera->hiZMipHeights[inputMipLevel];
    const uint32_t outputWidth = visCamera->hiZMipWidths[outputMipLevel];
    const uint32_t outputHeight = visCamera->hiZMipHeights[outputMipLevel];

    HiZMipGenParams hiZMipGenParams = {};
    hiZMipGenParams.inputSize = uint2(inputWidth, inputHeight);
    hiZMipGenParams.inputSizeRcp = float2(1.0f / inputWidth, 1.0f / inputHeight);
    hiZMipGenParams.outputSize = uint2(outputWidth, outputHeight);
    hiZMipGenParams.outputMipCount = 1;
    hiZMipGenParams.useMinDepth = useMinDepth;

    RHI::ConstantBuffer *hiZMipGenConstantBuffer = frameThreadData->AllocConstant(sizeof(HiZMipGenParams), &hiZMipGenParams);
    if (!hiZMipGenConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute, 0, inputMipLevel),
        RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputMipLevel)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pipelineState);
    RHI::renderer->SetTexture(commandList, 0, false, hiZTexture, inputViewID);
    RHI::renderer->SetTexture(commandList, 0, true, hiZTexture, visCamera->hiZMipUAVs[outputMipLevel]);
    RHI::renderer->SetConstantBuffer(commandList, 0, hiZMipGenConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputWidth, GenHiZMipBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputHeight, GenHiZMipBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, inputMipLevel),
        RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputMipLevel)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool RenderBackend::DispatchGenHiZ() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchGenHiZ");

    RHI::Texture *depthTexture = currentVisCamera->depthTexture;
    RHI::Texture *hiZTexture = currentVisCamera->hiZTexture;
    const uint32_t hiZMipCount = currentVisCamera->hiZMipCount;

    if (!depthTexture || !hiZTexture || hiZMipCount == 0) {
        return false;
    }

    const uint32_t useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 1 : 0;
    const uint32_t depthWidth = depthTexture->GetWidth();
    const uint32_t depthHeight = depthTexture->GetHeight();
    const uint32_t firstBatchMipCount = 1 + CountFastHiZReductions(currentVisCamera, 0, BE1::Min(hiZMipCount - 1, 3u));
    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);
    RHI::PipelineState *firstBatchPSO = depthTexture->GetSampleCount() > 1 ? frameGenHiZMipFirstBatchMSAAPSO : frameGenHiZMipFirstBatchPSO;
    if (!firstBatchPSO) {
        return false;
    }

    HiZMipGenParams hiZMipGenParams = {};
    hiZMipGenParams.inputSize = uint2(depthWidth, depthHeight);
    hiZMipGenParams.inputSizeRcp = float2(1.0f / depthWidth, 1.0f / depthHeight);
    hiZMipGenParams.outputSize = uint2(currentVisCamera->hiZMipWidths[0], currentVisCamera->hiZMipHeights[0]);
    hiZMipGenParams.outputMipCount = firstBatchMipCount;
    hiZMipGenParams.useMinDepth = useMinDepth;

    RHI::ConstantBuffer *hiZMipGenConstantBuffer = frameThreadData->AllocConstant(sizeof(HiZMipGenParams), &hiZMipGenParams);
    if (!hiZMipGenConstantBuffer) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::GenHiZ);

    // First batch: copy depth to mip 0, then generate consecutive fast 2x2 reductions while mip dimensions stay even.
    RHI::GPUBarrier startBarriers[5];
    uint32_t startBarrierCount = 0;
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    for (uint32_t outputIndex = 0; outputIndex < firstBatchMipCount; ++outputIndex) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputIndex);
    }
    RHI::renderer->Barrier(mainCommandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, firstBatchPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture, RHI::ViewID::PrimaryID);

    for (uint32_t outputIndex = 0; outputIndex < 4; ++outputIndex) {
        const uint32_t mipLevel = outputIndex < firstBatchMipCount ? outputIndex : 0;
        RHI::renderer->SetTexture(mainCommandList, outputIndex, true, hiZTexture, currentVisCamera->hiZMipUAVs[mipLevel]);
    }

    RHI::renderer->SetConstantBuffer(mainCommandList, 0, hiZMipGenConstantBuffer);

    uint32_t threadGroupCountX = BE1::DivCeil(currentVisCamera->hiZMipWidths[0], GenHiZMipBlockSizeX);
    uint32_t threadGroupCountY = BE1::DivCeil(currentVisCamera->hiZMipHeights[0], GenHiZMipBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[5];
    uint32_t endBarrierCount = 0;
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    for (uint32_t outputIndex = 0; outputIndex < firstBatchMipCount; ++outputIndex) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputIndex);
    }
    RHI::renderer->Barrier(mainCommandList, endBarriers, endBarrierCount);

    // Remaining mips: use fast 2x2 batches for even dimensions, and odd-dim reductions for 2x3/3x2/3x3 coverage.
    uint32_t generatedMipLevel = firstBatchMipCount - 1;
    while (generatedMipLevel + 1 < hiZMipCount) {
        const uint32_t fastReductionCount = CountFastHiZReductions(currentVisCamera, generatedMipLevel, BE1::Min(hiZMipCount - generatedMipLevel - 1, 4u));
        if (fastReductionCount > 0) {
            if (!DispatchFastHiZReductionBatch(frameThreadData, mainCommandList, currentVisCamera, hiZTexture, frameGenHiZMipBatchPSO, useMinDepth, generatedMipLevel, fastReductionCount)) {
                return false;
            }

            generatedMipLevel += fastReductionCount;
        } else {
            if (!DispatchOddDimHiZReduction(frameThreadData, mainCommandList, currentVisCamera, hiZTexture, frameGenHiZMipOddDimPSO, useMinDepth, generatedMipLevel)) {
                return false;
            }

            generatedMipLevel++;
        }
    }

    return true;
}
