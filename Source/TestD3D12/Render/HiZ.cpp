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
#include "HiZ.h"
#include "Render/RenderInternal.h"
#include "Render/GpuTiming.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipelineBootstrap.h"
#include "Shaders/ShaderInterop.h"

void HiZ::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    firstBatchShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genHiZMipFirstBatchShader", computeShaderModel, "HiZ/GenHiZMipFirstBatch.hlsl", baseDir);
    firstBatchMSAAShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genHiZMipFirstBatchMSAAShader", computeShaderModel, "HiZ/GenHiZMipFirstBatch.hlsl", baseDir, "CSMain", "GEN_HIZ_DEPTH_MSAA=1");
    mipBatchShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genHiZMipBatchShader", computeShaderModel, "HiZ/GenHiZMipBatch.hlsl", baseDir);
    oddDimShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genHiZMipOddDimShader", computeShaderModel, "HiZ/GenHiZMipOddDim.hlsl", baseDir);
    cullInstancesShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_hiZCullInstancesShader", computeShaderModel, "HiZ/HiZCullInstances.hlsl", baseDir);
    debugPreviewShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_hiZDebugPreviewShader", computeShaderModel, "HiZ/HiZDebugPreview.hlsl", baseDir);

    firstBatchPipeline = shaderPipelineCache.GetComputePipeline(firstBatchShaderTemplate, ShaderPermutationKey(), true);
    firstBatchMSAAPipeline = shaderPipelineCache.GetComputePipeline(firstBatchMSAAShaderTemplate, ShaderPermutationKey(), true);
    mipBatchPipeline = shaderPipelineCache.GetComputePipeline(mipBatchShaderTemplate, ShaderPermutationKey(), true);
    oddDimPipeline = shaderPipelineCache.GetComputePipeline(oddDimShaderTemplate, ShaderPermutationKey(), true);
    cullInstancesPipeline = shaderPipelineCache.GetComputePipeline(cullInstancesShaderTemplate, ShaderPermutationKey(), true);
    debugPreviewPipeline = shaderPipelineCache.GetComputePipeline(debugPreviewShaderTemplate, ShaderPermutationKey(), true);
}

void HiZ::Shutdown() {
    firstBatchShaderTemplate.reset();
    firstBatchMSAAShaderTemplate.reset();
    mipBatchShaderTemplate.reset();
    oddDimShaderTemplate.reset();
    cullInstancesShaderTemplate.reset();
    debugPreviewShaderTemplate.reset();

    firstBatchPipeline.reset();
    firstBatchMSAAPipeline.reset();
    mipBatchPipeline.reset();
    oddDimPipeline.reset();
    cullInstancesPipeline.reset();
    debugPreviewPipeline.reset();

    frameFirstBatchPSO = nullptr;
    frameFirstBatchMSAAPSO = nullptr;
    frameMipBatchPSO = nullptr;
    frameOddDimPSO = nullptr;
    frameCullInstancesPSO = nullptr;
    frameDebugPreviewPSO = nullptr;
}

void HiZ::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "HiZ";

    bootstrap.TrackPipeline(groupName, firstBatchPipeline);
    bootstrap.TrackPipeline(groupName, firstBatchMSAAPipeline);
    bootstrap.TrackPipeline(groupName, mipBatchPipeline);
    bootstrap.TrackPipeline(groupName, oddDimPipeline);
    bootstrap.TrackPipeline(groupName, cullInstancesPipeline);
    bootstrap.TrackPipeline(groupName, debugPreviewPipeline);
}

void HiZ::UpdateFramePSOs() {
    frameFirstBatchPSO = ShaderPipeline::GetReadyPSO(firstBatchPipeline);
    frameFirstBatchMSAAPSO = ShaderPipeline::GetReadyPSO(firstBatchMSAAPipeline);
    frameMipBatchPSO = ShaderPipeline::GetReadyPSO(mipBatchPipeline);
    frameOddDimPSO = ShaderPipeline::GetReadyPSO(oddDimPipeline);
    frameCullInstancesPSO = ShaderPipeline::GetReadyPSO(cullInstancesPipeline);
    frameDebugPreviewPSO = ShaderPipeline::GetReadyPSO(debugPreviewPipeline);
}

// width, height 가 1 이거나 둘 다 짝수이면 true 를 반환한다.
static bool CanFastReduceHiZMip(uint32_t width, uint32_t height) {
    return (width <= 1 || (width & 1) == 0) && (height <= 1 || (height & 1) == 0);
}

// inputMipLevel 부터 시작해서 maxReductionCount 만큼의 mip level 을 fast reduce 할 수 있는지 확인하고, fast reduce 가능한 mip level 의 개수를 반환한다.
static uint32_t CountFastHiZReductions(const FrameHiZ &data, uint32_t inputMipLevel, uint32_t maxReductionCount) {
    uint32_t reductionCount = 0;
    while (reductionCount < maxReductionCount && inputMipLevel + reductionCount + 1 < data.mipCount) {
        const uint32_t mipLevel = inputMipLevel + reductionCount;
        if (!CanFastReduceHiZMip(data.mipWidths[mipLevel], data.mipHeights[mipLevel])) {
            break;
        }

        reductionCount++;
    }
    return reductionCount;
}

// (연속된) input mip dimension 이 짝수인 경우 inputMipLevel 부터 시작해서 outputMipCount 만큼의 mip level 을 fast reduce 한다.
static bool DispatchFastHiZReductionBatch(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const FrameHiZ &data, RHI::Texture *hiZTexture, RHI::PipelineState *pipelineState, uint32_t useMinDepth, uint32_t inputMipLevel, uint32_t outputMipCount) {
    const uint32_t startOutputMipLevel = inputMipLevel + 1;
    const RHI::ViewID inputViewID = data.mipSRVs[inputMipLevel];

    const uint32_t inputWidth = data.mipWidths[inputMipLevel];
    const uint32_t inputHeight = data.mipHeights[inputMipLevel];
    const uint32_t outputWidth = data.mipWidths[startOutputMipLevel];
    const uint32_t outputHeight = data.mipHeights[startOutputMipLevel];

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
        RHI::renderer->SetTexture(commandList, outputIndex, true, hiZTexture, data.mipUAVs[mipLevel]);
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
static bool DispatchOddDimHiZReduction(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const FrameHiZ &data, RHI::Texture *hiZTexture, RHI::PipelineState *pipelineState, uint32_t useMinDepth, uint32_t inputMipLevel) {
    const uint32_t outputMipLevel = inputMipLevel + 1;
    const RHI::ViewID inputViewID = data.mipSRVs[inputMipLevel];

    const uint32_t inputWidth = data.mipWidths[inputMipLevel];
    const uint32_t inputHeight = data.mipHeights[inputMipLevel];
    const uint32_t outputWidth = data.mipWidths[outputMipLevel];
    const uint32_t outputHeight = data.mipHeights[outputMipLevel];

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
    RHI::renderer->SetTexture(commandList, 0, true, hiZTexture, data.mipUAVs[outputMipLevel]);
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

bool HiZ::DispatchGenerate(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, RHI::Texture *depthTexture, const RHI::ProjectionPolicy &projectionPolicy, const FrameHiZ &data) const {
    PROFILER_CPU_SCOPED_EVENT("HiZ::DispatchGenerate");

    RHI::Texture *hiZTexture = data.texture;
    const uint32_t hiZMipCount = data.mipCount;

    if (!depthTexture || !hiZTexture || hiZMipCount == 0) {
        return false;
    }

    const uint32_t useMinDepth = projectionPolicy.reverseZ ? 1 : 0;
    const uint32_t depthWidth = depthTexture->GetWidth();
    const uint32_t depthHeight = depthTexture->GetHeight();
    const uint32_t firstBatchMipCount = 1 + CountFastHiZReductions(data, 0, BE1::Min(hiZMipCount - 1, 3u));
    RHI::PipelineState *firstBatchPSO = depthTexture->GetSampleCount() > 1 ? frameFirstBatchMSAAPSO : frameFirstBatchPSO;
    if (!firstBatchPSO) {
        return false;
    }

    HiZMipGenParams hiZMipGenParams = {};
    hiZMipGenParams.inputSize = uint2(depthWidth, depthHeight);
    hiZMipGenParams.inputSizeRcp = float2(1.0f / depthWidth, 1.0f / depthHeight);
    hiZMipGenParams.outputSize = uint2(data.mipWidths[0], data.mipHeights[0]);
    hiZMipGenParams.outputMipCount = firstBatchMipCount;
    hiZMipGenParams.useMinDepth = useMinDepth;

    RHI::ConstantBuffer *hiZMipGenConstantBuffer = frameThreadData->AllocConstant(sizeof(HiZMipGenParams), &hiZMipGenParams);
    if (!hiZMipGenConstantBuffer) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::GenHiZ);

    // First batch: copy depth to mip 0, then generate consecutive fast 2x2 reductions while mip dimensions stay even.
    RHI::GPUBarrier startBarriers[5];
    uint32_t startBarrierCount = 0;
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    for (uint32_t outputIndex = 0; outputIndex < firstBatchMipCount; ++outputIndex) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, outputIndex);
    }
    RHI::renderer->Barrier(commandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(commandList, firstBatchPSO);
    RHI::renderer->SetTexture(commandList, 0, false, depthTexture, RHI::ViewID::PrimaryID);

    for (uint32_t outputIndex = 0; outputIndex < 4; ++outputIndex) {
        const uint32_t mipLevel = outputIndex < firstBatchMipCount ? outputIndex : 0;
        RHI::renderer->SetTexture(commandList, outputIndex, true, hiZTexture, data.mipUAVs[mipLevel]);
    }

    RHI::renderer->SetConstantBuffer(commandList, 0, hiZMipGenConstantBuffer);

    uint32_t threadGroupCountX = BE1::DivCeil(data.mipWidths[0], GenHiZMipBlockSizeX);
    uint32_t threadGroupCountY = BE1::DivCeil(data.mipHeights[0], GenHiZMipBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[5];
    uint32_t endBarrierCount = 0;
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(depthTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    for (uint32_t outputIndex = 0; outputIndex < firstBatchMipCount; ++outputIndex) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(hiZTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, outputIndex);
    }
    RHI::renderer->Barrier(commandList, endBarriers, endBarrierCount);

    // Remaining mips: use fast 2x2 batches for even dimensions, and odd-dim reductions for 2x3/3x2/3x3 coverage.
    uint32_t generatedMipLevel = firstBatchMipCount - 1;
    while (generatedMipLevel + 1 < hiZMipCount) {
        const uint32_t fastReductionCount = CountFastHiZReductions(data, generatedMipLevel, BE1::Min(hiZMipCount - generatedMipLevel - 1, 4u));
        if (fastReductionCount > 0) {
            if (!DispatchFastHiZReductionBatch(frameThreadData, commandList, data, hiZTexture, frameMipBatchPSO, useMinDepth, generatedMipLevel, fastReductionCount)) {
                return false;
            }

            generatedMipLevel += fastReductionCount;
        } else {
            if (!DispatchOddDimHiZReduction(frameThreadData, commandList, data, hiZTexture, frameOddDimPSO, useMinDepth, generatedMipLevel)) {
                return false;
            }

            generatedMipLevel++;
        }
    }

    return true;
}

bool HiZ::DispatchDebugPreview(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, uint32_t requestedMipLevel, const FrameHiZ &data) const {
    PROFILER_CPU_SCOPED_EVENT("HiZ::DispatchDebugPreview");

    if (!cameraConstantBuffer) {
        return false;
    }

    // 실제 Hi-Z 리소스의 mip 수는 화면 크기에 따라 snapshot 의 전역 상한보다 작을 수 있다.
    const uint32_t mipLevel = BE1::Min(requestedMipLevel, data.mipCount - 1);
    const uint32_t inputWidth = data.mipWidths[mipLevel];
    const uint32_t inputHeight = data.mipHeights[mipLevel];
    const uint32_t outputWidth = data.debugPreviewTexture->GetWidth();
    const uint32_t outputHeight = data.debugPreviewTexture->GetHeight();

    if (inputWidth == 0 || inputHeight == 0 || outputWidth == 0 || outputHeight == 0) {
        return false;
    }

    HiZDebugPreviewParams hiZDebugPreviewParams = {};
    hiZDebugPreviewParams.inputSize = uint2(inputWidth, inputHeight);
    hiZDebugPreviewParams.outputSize = uint2(outputWidth, outputHeight);
    hiZDebugPreviewParams.linearDepthScale = data.debugLinearDepthScale;

    RHI::ConstantBuffer *hiZDebugPreviewConstantBuffer = frameThreadData->AllocConstant(sizeof(HiZDebugPreviewParams), &hiZDebugPreviewParams);
    if (!hiZDebugPreviewConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(data.texture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute, 0, mipLevel),
        RHI::Renderer::MakeTextureBarrier(data.debugPreviewTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::HiZDebugPreview);

    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, frameDebugPreviewPSO);
    RHI::renderer->SetTexture(commandList, 0, false, data.texture, data.mipSRVs[mipLevel]);
    RHI::renderer->SetTexture(commandList, 0, true, data.debugPreviewTexture);
    RHI::renderer->SetConstantBuffer(commandList, 0, hiZDebugPreviewConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputWidth, HiZDebugPreviewBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputHeight, HiZDebugPreviewBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(data.texture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource, 0, mipLevel),
        RHI::Renderer::MakeTextureBarrier(data.debugPreviewTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool HiZ::DispatchCullInstances(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::ConstantBuffer *cameraConstantBuffer, uint32_t surfaceTableIndex, const RHI::ProjectionPolicy &projectionPolicy, const FrameHiZ &data, uint32_t instanceCount, RHI::Buffer *inputInstanceToShaderSurfaceTable, uint32_t instanceBaseOffset, uint32_t indirectArgsOffset) const {
    if (instanceCount == 0 || instanceCount > RenderHiZCullMaxInstanceCount) {
        return false;
    }

    HiZCullInstancesParams hiZCullParams = {};
    hiZCullParams.inputInstanceCount = instanceCount;
    hiZCullParams.surfaceTableIndex = surfaceTableIndex;
    hiZCullParams.hiZWidth = data.mipWidths[0];
    hiZCullParams.hiZHeight = data.mipHeights[0];
    hiZCullParams.hiZMipCount = data.mipCount;
    hiZCullParams.useMinDepth = projectionPolicy.reverseZ ? 1 : 0;
    hiZCullParams.instanceBaseOffset = instanceBaseOffset;
    hiZCullParams.indirectArgOffset = indirectArgsOffset;

    RHI::ConstantBuffer *hiZCullConstantBuffer = frameThreadData->AllocConstant(sizeof(HiZCullInstancesParams), &hiZCullParams);
    if (!hiZCullConstantBuffer) {
        return false;
    }

    RHI::Buffer *visibleInstanceBuffer = data.visibleInstanceBuffer;
    RHI::Buffer *indirectArgsBuffer = data.indirectArgsBuffer;

    RHI::renderer->SetPSO(commandList, frameCullInstancesPSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputInstanceToShaderSurfaceTable);
    RHI::renderer->SetTexture(commandList, 1, false, data.texture, RHI::ViewID::PrimaryID);
    RHI::renderer->SetBuffer(commandList, 0, true, visibleInstanceBuffer);
    RHI::renderer->SetBuffer(commandList, 1, true, indirectArgsBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, hiZCullConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(instanceCount, HiZCullThreadCount);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, 1, 1);

    return true;
}
