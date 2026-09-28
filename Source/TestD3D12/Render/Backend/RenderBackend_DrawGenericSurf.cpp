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
#include "Render/RenderFrame.h"

void RenderBackend::DrawGenericSurfaces(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RHI::ConstantBuffer *viewConstantBuffer, const SurfaceInstanceBatch *hiZCullBatches, uint32_t hiZCullBatchCount) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericSurfaces");

    assert(numDrawSurfs > 0);
    assert(viewConstantBuffer);

#ifdef USE_RENDER_TASK
    // numDrawSurfs 가 많으면 태스크로 나눠서 그린다.
    if (numDrawSurfs > MaxInstancesPerDraw && frame->NumRenderTaskThreads() > 1) {
        DrawGenericSurfacesWithTask(drawSurfs, numDrawSurfs, viewConstantBuffer, hiZCullBatches, hiZCullBatchCount);
    } else {
        DrawGenericSurfacesWithoutTask(drawSurfs, numDrawSurfs, viewConstantBuffer, hiZCullBatches, hiZCullBatchCount);
    }
#else
    DrawGenericSurfacesWithoutTask(drawSurfs, numDrawSurfs, viewConstantBuffer, hiZCullBatches, hiZCullBatchCount);
#endif
}

void RenderBackend::DrawGenericSurfacesWithoutTask(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RHI::ConstantBuffer *viewConstantBuffer, const SurfaceInstanceBatch *hiZCullBatches, uint32_t hiZCullBatchCount) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericSurfacesWithoutTask");

    RHI::PipelineState *currentPSO = nullptr;
    const SubMesh *currentSubMesh = nullptr;
    const RHI::IndexBuffer *currentIndexBuffer = nullptr;
    VertexInputLayoutId currentInputLayoutId = VertexInputLayoutId::Count;

    uint32_t drawCount = 0;
    uint64_t drawTriangleCount = 0;
    uint32_t instanceTableAllocCount = 0;
    uint32_t instanceTableUploadBytes = 0;

    uint32_t drawSurfIndex = 0;
    uint32_t hiZCullBatchIndex = 0;

    while (drawSurfIndex < numDrawSurfs) {
        const RenderPassDrawSurf *drawSurf = drawSurfs[drawSurfIndex];
        if (!drawSurf->pso) {
            drawSurfIndex++;
            continue;
        }

        if (currentPSO != drawSurf->pso) {
            currentPSO = drawSurf->pso;

            RHI::renderer->SetPSO(mainCommandList, currentPSO);
            RHI::renderer->SetConstantBuffer(mainCommandList, 0, frameConstantBuffer);
            RHI::renderer->SetConstantBuffer(mainCommandList, 1, viewConstantBuffer);

            if (VertexInputLayoutFactory::IsSkinning(drawSurf->inputLayoutId)) {
                RHI::renderer->SetBuffer(mainCommandList, 1, false, currentFrameCamera->skinningJointBuffer);
            }
        }

        if (currentSubMesh != drawSurf->frameSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->frameSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;

            SetPassVertexBuffers(mainCommandList, drawSurf);
        }

        if (currentIndexBuffer != drawSurf->frameSurf->indexBuffer) {
            currentIndexBuffer = drawSurf->frameSurf->indexBuffer;
            RHI::renderer->SetIndexBuffer(mainCommandList, currentIndexBuffer);
        }

        const uint32_t instanceStartIndex = drawSurfIndex;
        drawSurfIndex = FindSurfaceInstanceBatchEnd(drawSurfs, numDrawSurfs, instanceStartIndex);
        const uint32_t instanceCount = drawSurfIndex - instanceStartIndex;

        const SurfaceInstanceBatch *preparedBatch = nullptr;
        if (hiZCullBatches && hiZCullBatchIndex < hiZCullBatchCount) {
            preparedBatch = &hiZCullBatches[hiZCullBatchIndex++];

            assert(preparedBatch->drawSurfIndex == instanceStartIndex);
            assert(preparedBatch->instanceCount == instanceCount);
        }

        if (preparedBatch && preparedBatch->valid) {
            DrawSurfaceInstanceBatch(mainCommandList, &drawSurfs[instanceStartIndex], *preparedBatch, drawCount, drawTriangleCount);
        } else {
            SurfaceInstanceBatch batch;
            if (PrepareSurfaceInstanceBatch(frame->GetThreadData(0), &drawSurfs[instanceStartIndex], instanceStartIndex, instanceCount, batch, instanceTableAllocCount, instanceTableUploadBytes)) {
                DrawSurfaceInstanceBatch(mainCommandList, &drawSurfs[instanceStartIndex], batch, drawCount, drawTriangleCount);
            }
        }
    }

    assert(hiZCullBatchIndex == hiZCullBatchCount);
    RecordDrawStats(drawCount, drawTriangleCount, instanceTableAllocCount, instanceTableUploadBytes);
}

void RenderBackend::DrawGenericSurfacesByTask(RenderBackend::DrawSurfsTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericSurfacesByTask");

    // 세컨더리 커맨드 리스트를 시작한다.
    RHI::FrameThreadData *frameThreadData = frame->GetThreadData(taskDesc->threadIndex);
    RHI::CommandList *commandList = frameThreadData->BeginSecondaryCommandList();

    RHI::PipelineState *currentPSO = nullptr;
    const SubMesh *currentSubMesh = nullptr;
    const RHI::IndexBuffer *currentIndexBuffer = nullptr;
    VertexInputLayoutId currentInputLayoutId = VertexInputLayoutId::Count;

    uint32_t drawSurfIndex = 0;
    uint32_t hiZCullBatchIndex = 0;
    while (drawSurfIndex < taskDesc->numDrawSurfs) {
        const RenderPassDrawSurf *drawSurf = taskDesc->drawSurfs[drawSurfIndex];
        if (!drawSurf->pso) {
            drawSurfIndex++;
            continue;
        }

        if (currentPSO != drawSurf->pso) {
            currentPSO = drawSurf->pso;

            RHI::renderer->SetPSO(commandList, currentPSO);
            RHI::renderer->SetConstantBuffer(commandList, 0, frameConstantBuffer);
            RHI::renderer->SetConstantBuffer(commandList, 1, taskDesc->viewConstantBuffer);

            if (VertexInputLayoutFactory::IsSkinning(drawSurf->inputLayoutId)) {
                RHI::renderer->SetBuffer(commandList, 1, false, currentFrameCamera->skinningJointBuffer);
            }
        }

        if (currentSubMesh != drawSurf->frameSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->frameSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;

            SetPassVertexBuffers(commandList, drawSurf);
        }

        if (currentIndexBuffer != drawSurf->frameSurf->indexBuffer) {
            currentIndexBuffer = drawSurf->frameSurf->indexBuffer;
            RHI::renderer->SetIndexBuffer(commandList, currentIndexBuffer);
        }

        const uint32_t instanceStartIndex = drawSurfIndex;
        drawSurfIndex = FindSurfaceInstanceBatchEnd(taskDesc->drawSurfs, taskDesc->numDrawSurfs, instanceStartIndex);
        const uint32_t instanceCount = drawSurfIndex - instanceStartIndex;

        const SurfaceInstanceBatch *preparedBatch = nullptr;
        if (taskDesc->hiZCullBatches && hiZCullBatchIndex < taskDesc->numHiZCullBatches) {
            preparedBatch = &taskDesc->hiZCullBatches[hiZCullBatchIndex++];

            assert(preparedBatch->drawSurfIndex == taskDesc->firstDrawSurfIndex + instanceStartIndex);
            assert(preparedBatch->instanceCount == instanceCount);
        }

        if (preparedBatch && preparedBatch->valid) {
            DrawSurfaceInstanceBatch(commandList, &taskDesc->drawSurfs[instanceStartIndex], *preparedBatch, taskDesc->drawCount, taskDesc->drawTriangleCount);
        } else {
            SurfaceInstanceBatch batch;
            const uint32_t globalInstanceStartIndex = taskDesc->firstDrawSurfIndex + instanceStartIndex;
            if (PrepareSurfaceInstanceBatch(frameThreadData, &taskDesc->drawSurfs[instanceStartIndex], globalInstanceStartIndex, instanceCount, batch, taskDesc->instanceTableAllocCount, taskDesc->instanceTableUploadBytes)) {
                DrawSurfaceInstanceBatch(commandList, &taskDesc->drawSurfs[instanceStartIndex], batch, taskDesc->drawCount, taskDesc->drawTriangleCount);
            }
        }
    }

    assert(hiZCullBatchIndex == taskDesc->numHiZCullBatches);

    // 세컨더리 커맨드 리스트의 기록을 마친다.
    commandList->Close();

    // 세컨더리 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->commandList = commandList;
}

void RenderBackend::DrawGenericSurfacesByTaskFunction(void *data) {
    RenderBackend::DrawSurfsTaskDesc *taskDesc = reinterpret_cast<RenderBackend::DrawSurfsTaskDesc *>(data);
    renderSystem->GetBackend()->DrawGenericSurfacesByTask(taskDesc);
}

void RenderBackend::DrawGenericSurfacesWithTask(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RHI::ConstantBuffer *viewConstantBuffer, const SurfaceInstanceBatch *hiZCullBatches, uint32_t hiZCullBatchCount) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericSurfacesWithTask");

    // numDrawSurfs 로 만들어 질 수 있는 최대 태스크 개수를 계산한다.
    const uint32_t maxTasks = BE1::Min(frame->NumRenderTaskThreads(), (numDrawSurfs + MaxInstancesPerDraw - 1) / MaxInstancesPerDraw);

    // 태스크 당 처리하는 목표 DrawSurfs 개수 (인스턴싱 경계에 걸리지 않도록 더 추가될 수 있다)
    const uint32_t targetDrawSurfsPerTask = (numDrawSurfs + maxTasks - 1) / maxTasks;

    uint32_t startIndex = 0;
    uint32_t hiZCullBatchCursor = 0;
    int32_t threadIndex = 0;

    // 태스크 정보 초기화
    drawSurfsTaskDescs.Reserve(maxTasks);
    drawSurfsTaskDescs.SetCount(0, false);

    // maxTasks 개수 이하로 태스크를 추가한다.
    while (startIndex < numDrawSurfs) {
        // 인스턴싱 batch 경계를 유지하므로 실제 생성되는 task 수는 maxTasks 보다 적을 수 있다.
        const uint32_t targetEndIndex = BE1::Min(startIndex + targetDrawSurfsPerTask, numDrawSurfs);
        uint32_t endIndex = startIndex;
        do {
            endIndex = FindSurfaceInstanceBatchEnd(drawSurfs, numDrawSurfs, endIndex);
        } while (endIndex < targetEndIndex);

        const uint32_t taskHiZCullBatchStart = hiZCullBatchCursor;
        if (hiZCullBatches) {
            while (hiZCullBatchCursor < hiZCullBatchCount && hiZCullBatches[hiZCullBatchCursor].drawSurfIndex < startIndex) {
                assert(false);
                hiZCullBatchCursor++;
            }
            while (hiZCullBatchCursor < hiZCullBatchCount && hiZCullBatches[hiZCullBatchCursor].drawSurfIndex < endIndex) {
                assert(hiZCullBatches[hiZCullBatchCursor].drawSurfIndex + hiZCullBatches[hiZCullBatchCursor].instanceCount <= endIndex);
                hiZCullBatchCursor++;
            }
        }

        DrawSurfsTaskDesc &currentThreadDesc = drawSurfsTaskDescs.Alloc();
        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.drawSurfs = &drawSurfs[startIndex];
        currentThreadDesc.numDrawSurfs = endIndex - startIndex;
        currentThreadDesc.viewConstantBuffer = viewConstantBuffer;
        currentThreadDesc.firstDrawSurfIndex = startIndex;
        currentThreadDesc.hiZCullBatches = nullptr;
        currentThreadDesc.numHiZCullBatches = hiZCullBatchCursor - taskHiZCullBatchStart;
        if (currentThreadDesc.numHiZCullBatches > 0) {
            currentThreadDesc.hiZCullBatches = &hiZCullBatches[taskHiZCullBatchStart];
        }
        currentThreadDesc.drawCount = 0;
        currentThreadDesc.drawTriangleCount = 0;
        currentThreadDesc.instanceTableAllocCount = 0;
        currentThreadDesc.instanceTableUploadBytes = 0;

        if (!BE1::Engine::taskManager->AddTask(RenderBackend::DrawGenericSurfacesByTaskFunction, &currentThreadDesc, drawGroupId, false)) {
            DrawGenericSurfacesByTask(&currentThreadDesc);
        }

        startIndex = endIndex;
    }

    assert(hiZCullBatchCursor == hiZCullBatchCount);

    // 모든 태스크가 끝날 때까지 기다린다.
    BE1::Engine::taskManager->WaitFinish(drawGroupId, true);

    // 메인 커맨드 리스트에 모든 태스크의 세컨더리 커맨드 리스트들을 기록한다.
    const int32_t threadCount = drawSurfsTaskDescs.Count();
    for (int32_t threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        RecordDrawStats(drawSurfsTaskDescs[threadIndex].drawCount, drawSurfsTaskDescs[threadIndex].drawTriangleCount, drawSurfsTaskDescs[threadIndex].instanceTableAllocCount, drawSurfsTaskDescs[threadIndex].instanceTableUploadBytes);

        mainCommandList->ExecuteSecondary(drawSurfsTaskDescs[threadIndex].commandList);
    }
}
