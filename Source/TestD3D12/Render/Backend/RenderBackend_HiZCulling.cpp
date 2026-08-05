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
#include "Render/RenderFrameData.h"
#include "Shaders/ShaderInterop.h"

static bool IsHiZCullRenderPass(RenderPassType passType) {
    return passType == RenderPassType::DepthPrepass || passType == RenderPassType::Opaque;
}

bool RenderBackend::PrepareHiZCulling() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::PrepareHiZCulling");

    hiZCullDrawBatches.SetCount(0, false);
    hiZCullPassRanges.SetCount(0, false);

    assert(currentVisCamera->hiZMipCount > 0 && currentVisCamera->hiZMipWidths[0] > 0 && currentVisCamera->hiZMipHeights[0] > 0);

    uint32_t totalBatchCount = 0;
    uint32_t totalInstanceCount = 0;

    // 전체 렌더패스들을 순회하면서 Hi-Z Culling 을 수행할 수 있는 DrawSurf 들을 찾아서 배치 개수, 인스턴스 개수를 계산한다.
    for (uint32_t passIndex = 0; passIndex < currentVisCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentVisCamera->renderPasses[passIndex];
        // 현재 Hi-Z Culling 은 DepthPrepass 와 Opaque Pass 에서만 수행한다.
        if (!IsHiZCullRenderPass(renderPass->type)) {
            continue;
        }

        uint32_t drawSurfIndex = 0;
        while (drawSurfIndex < renderPass->numDrawSurfs) {
            const RenderPassDrawSurf *drawSurf = renderPass->drawSurfs[drawSurfIndex];
            if (!drawSurf->pso) {
                drawSurfIndex++;
                continue;
            }

            const uint32_t instanceStartIndex = drawSurfIndex;
            drawSurfIndex = FindSurfaceInstanceBatchEnd(renderPass->drawSurfs, renderPass->numDrawSurfs, instanceStartIndex);

            totalBatchCount++;
            totalInstanceCount += drawSurfIndex - instanceStartIndex;
        }
    }

    // Hi-Z Culling 을 수행할 수 있는 배치가 하나도 없으면 false 를 반환한다.
    if (totalBatchCount == 0) {
        return false;
    }

    if (totalInstanceCount > RenderHiZCullMaxVisibleInstanceCount || totalBatchCount > RenderHiZCullMaxBatchCount) {
        BE_WARNLOG("RenderBackend::PrepareHiZCulling: HiZ cull batch capacity exceeded (instances %u/%u, batches %u/%u)\n", totalInstanceCount, static_cast<uint32_t>(RenderHiZCullMaxVisibleInstanceCount), totalBatchCount, static_cast<uint32_t>(RenderHiZCullMaxBatchCount));
        return false;
    }

    // 배치별로 DrawIndexedInstancedIndirect 를 위한 인자들을 기록할 업로드 버퍼를 할당한다.
    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);
    const uint32_t indirectArgsUploadSize = totalBatchCount * HiZCullIndirectArgSize;
    RHI::Buffer *indirectArgsUploadBuffer = frameThreadData->AllocRawBuffer(indirectArgsUploadSize, sizeof(uint32_t), nullptr);
    if (!indirectArgsUploadBuffer) {
        return false;
    }

    hiZCullDrawBatches.Reserve(totalBatchCount);

    hiZCullPassRanges.SetCount(currentVisCamera->numRenderPasses, false);
    for (uint32_t passIndex = 0; passIndex < currentVisCamera->numRenderPasses; ++passIndex) {
        hiZCullPassRanges[passIndex] = {};
    }

    uint32_t *indirectArgs = reinterpret_cast<uint32_t *>(indirectArgsUploadBuffer->mappedPtr);
    uint32_t instanceBaseOffset = 0;
    uint32_t batchIndex = 0;

    for (uint32_t passIndex = 0; passIndex < currentVisCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentVisCamera->renderPasses[passIndex];
        if (!IsHiZCullRenderPass(renderPass->type)) {
            continue;
        }

        HiZCullPassRange &passRange = hiZCullPassRanges[passIndex];
        passRange.firstBatchIndex = static_cast<uint32_t>(hiZCullDrawBatches.Count());

        uint32_t drawSurfIndex = 0;
        while (drawSurfIndex < renderPass->numDrawSurfs) {
            const RenderPassDrawSurf *drawSurf = renderPass->drawSurfs[drawSurfIndex];
            if (!drawSurf->pso) {
                drawSurfIndex++;
                continue;
            }

            const uint32_t instanceStartIndex = drawSurfIndex;
            drawSurfIndex = FindSurfaceInstanceBatchEnd(renderPass->drawSurfs, renderPass->numDrawSurfs, instanceStartIndex);
            const uint32_t instanceCount = drawSurfIndex - instanceStartIndex;
            const VisSurf *firstVisSurf = renderPass->drawSurfs[instanceStartIndex]->visSurf;

            // 배치 정보를 hiZCullDrawBatches 에 기록한다.
            SurfaceInstanceBatch &batch = hiZCullDrawBatches.Alloc();
            batch.drawSurfIndex = instanceStartIndex; // renderPass->drawSurfs 의 시작 index
            batch.instanceCount = instanceCount;
            batch.instanceToShaderSurfaceTable = currentVisCamera->hiZCullVisibleInstanceBuffer;
            batch.instanceBaseOffset = instanceBaseOffset;
            batch.indirectArgsBuffer = currentVisCamera->hiZCullIndirectArgsBuffer;
            batch.indirectArgsOffset = batchIndex * HiZCullIndirectArgSize;
            batch.valid = false;

            // indirectArgsUploadBuffer 에 배치별로 DrawIndexedInstancedIndirect 를 위한 인자들을 기록한다.
            uint32_t *batchIndirectArgs = indirectArgs + (batch.indirectArgsOffset / sizeof(uint32_t));
            batchIndirectArgs[0] = firstVisSurf->subMesh->numIndexes; // IndexCountPerInstance
            batchIndirectArgs[1] = 0; // InstanceCount
            batchIndirectArgs[2] = 0; // StartIndexLocation
            batchIndirectArgs[3] = 0; // BaseVertexLocation
            batchIndirectArgs[4] = 0; // StartInstanceLocation

            instanceBaseOffset += instanceCount;
            batchIndex++;
        }

        passRange.numBatches = static_cast<uint32_t>(hiZCullDrawBatches.Count()) - passRange.firstBatchIndex;
    }

    assert(batchIndex == totalBatchCount);
    assert(instanceBaseOffset == totalInstanceCount);

    RHI::Buffer *visibleInstanceBuffer = currentVisCamera->hiZCullVisibleInstanceBuffer;
    RHI::Buffer *indirectArgsBuffer = currentVisCamera->hiZCullIndirectArgsBuffer;

    RHI::GPUBarrier copyStartBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(indirectArgsBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::CopyDst)
    };

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::HiZCulling);

    RHI::renderer->Barrier(mainCommandList, copyStartBarriers, COUNT_OF(copyStartBarriers));

    // CPU 업로드 버퍼에 만든 indirectArgsUploadBuffer 를 GPU 디폴트 버퍼인 hiZCullIndirectArgsBuffer 에 복사한다.
    RHI::renderer->CopyBuffer(mainCommandList, indirectArgsBuffer, 0, indirectArgsUploadBuffer, 0, indirectArgsUploadSize);

    RHI::GPUBarrier dispatchStartBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(indirectArgsBuffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(visibleInstanceBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->hiZTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute)
    };
    RHI::renderer->Barrier(mainCommandList, dispatchStartBarriers, COUNT_OF(dispatchStartBarriers));

    // 각 배치 마다 인스턴스별 ShaderSurface 테이블을 GPU 에 업로드하고, DispatchHiZCullInstances 를 호출하여 Hi-Z Culling 을 수행한다.
    for (uint32_t passIndex = 0; passIndex < currentVisCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentVisCamera->renderPasses[passIndex];
        if (!IsHiZCullRenderPass(renderPass->type)) {
            continue;
        }

        const HiZCullPassRange &passRange = hiZCullPassRanges[passIndex];
        for (uint32_t i = 0; i < passRange.numBatches; ++i) {
            SurfaceInstanceBatch &batch = hiZCullDrawBatches[passRange.firstBatchIndex + i];
            RHI::Buffer *inputInstanceToShaderSurfaceTable = AllocInstanceToShaderSurfaceTable(frameThreadData, &renderPass->drawSurfs[batch.drawSurfIndex], batch.instanceCount);
            if (!inputInstanceToShaderSurfaceTable) {
                continue;
            }

            // Hi-Z culling input table upload 도 instance table 통계에 포함한다.
            RecordDrawStats(0, 0, 1, sizeof(uint32_t) * batch.instanceCount);

            batch.valid = DispatchHiZCullInstances(frameThreadData, mainCommandList, batch.instanceCount, inputInstanceToShaderSurfaceTable, batch.instanceBaseOffset, batch.indirectArgsOffset);
        }
    }

    RHI::GPUBarrier drawStartBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(visibleInstanceBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(indirectArgsBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::IndirectArgument),
        RHI::Renderer::MakeTextureBarrier(currentVisCamera->hiZTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, drawStartBarriers, COUNT_OF(drawStartBarriers));

    return true;
}

bool RenderBackend::DispatchHiZCullInstances(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, uint32_t instanceCount, RHI::Buffer *inputInstanceToShaderSurfaceTable, uint32_t instanceBaseOffset, uint32_t indirectArgsOffset) {
    if (instanceCount == 0 || instanceCount > RenderHiZCullMaxInstanceCount) {
        return false;
    }

    HiZCullInstancesParams hiZCullParams = {};
    hiZCullParams.inputInstanceCount = instanceCount;
    hiZCullParams.surfaceTableIndex = frameData->GetShaderSurfaceTableBindlessIndex();
    hiZCullParams.hiZWidth = currentVisCamera->hiZMipWidths[0];
    hiZCullParams.hiZHeight = currentVisCamera->hiZMipHeights[0];
    hiZCullParams.hiZMipCount = currentVisCamera->hiZMipCount;
    hiZCullParams.useMinDepth = currentVisCamera->projectionPolicy.reverseZ ? 1 : 0;
    hiZCullParams.instanceBaseOffset = instanceBaseOffset;
    hiZCullParams.indirectArgOffset = indirectArgsOffset;

    RHI::ConstantBuffer *hiZCullConstantBuffer = frameThreadData->AllocConstant(sizeof(HiZCullInstancesParams), &hiZCullParams);
    if (!hiZCullConstantBuffer) {
        return false;
    }

    RHI::Buffer *visibleInstanceBuffer = currentVisCamera->hiZCullVisibleInstanceBuffer;
    RHI::Buffer *indirectArgsBuffer = currentVisCamera->hiZCullIndirectArgsBuffer;

    RHI::renderer->SetPSO(commandList, frameHiZCullInstancesPSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputInstanceToShaderSurfaceTable);
    RHI::renderer->SetTexture(commandList, 1, false, currentVisCamera->hiZTexture, RHI::ViewID::PrimaryID);
    RHI::renderer->SetBuffer(commandList, 0, true, visibleInstanceBuffer);
    RHI::renderer->SetBuffer(commandList, 1, true, indirectArgsBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, hiZCullConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(instanceCount, HiZCullThreadCount);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, 1, 1);

    return true;
}

void RenderBackend::FinishHiZCulling() {
    if (!currentVisCamera->hiZCullVisibleInstanceBuffer || !currentVisCamera->hiZCullIndirectArgsBuffer) {
        return;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(currentVisCamera->hiZCullVisibleInstanceBuffer, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(currentVisCamera->hiZCullIndirectArgsBuffer, RHI::GPUResourceState::IndirectArgument, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(mainCommandList, endBarriers, COUNT_OF(endBarriers));
}
