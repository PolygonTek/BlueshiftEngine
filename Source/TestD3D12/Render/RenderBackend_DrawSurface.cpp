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
#include "RenderSystem.h"
#include "RenderContext.h"
#include "RenderInternal.h"

// 태스크 당 처리할 최대 Draw Call 횟수
static constexpr uint32_t   MaxDrawSurfsPerTask = 2048;

static bool CanBatchSurfaceInstances(const PassDrawSurf *baseDrawSurf, const PassDrawSurf *drawSurf) {
    return drawSurf->pso &&
        drawSurf->pso == baseDrawSurf->pso &&
        drawSurf->visSurf->subMesh == baseDrawSurf->visSurf->subMesh &&
        drawSurf->inputLayoutId == baseDrawSurf->inputLayoutId;
}

void RenderBackend::SetPassVertexBuffers(RHI::CommandList *commandList, const PassDrawSurf *drawSurf) const {
    const SubMesh *subMesh = drawSurf->visSurf->subMesh;

    RHI::renderer->SetVertexBuffer(commandList, 0, subMesh->vertexBuffer);
    if (VertexInputLayoutFactory::IsSkinning(drawSurf->inputLayoutId)) {
        RHI::renderer->SetVertexBuffer(commandList, 1, subMesh->vertexWeightBuffer);
    }
}

void RenderBackend::DrawLitSurfaces(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    assert(numDrawSurfs > 0);

#ifdef USE_RENDER_TASK
    // numDrawSurfs 에 따라 numTasks 를 계산한다.
    uint32_t numTasks = (numDrawSurfs + MaxDrawSurfsPerTask - 1) / MaxDrawSurfsPerTask;
    numTasks = BE1::Min(BE1::Engine::taskManager->NumThreads(), numTasks);

    // numDrawSurfs 가 많으면 태스크로 나눠서 그린다.
    if (numTasks > 1) {
        DrawLitSurfacesWithTask(drawSurfs, numDrawSurfs, numTasks);
    } else {
        DrawLitSurfacesWithoutTask(drawSurfs, numDrawSurfs);
    }
#else
    DrawLitSurfacesWithoutTask(drawSurfs, numDrawSurfs);
#endif
}

void RenderBackend::DrawLitSurfacesWithoutTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesWithoutTask");

    RHI::PipelineState *currentPSO = nullptr;
    const SubMesh *currentSubMesh = nullptr;
    VertexInputLayoutId currentInputLayoutId = VertexInputLayoutId::Count;

    uint32_t drawSurfIndex = 0;
    while (drawSurfIndex < numDrawSurfs) {
        const PassDrawSurf *drawSurf = drawSurfs[drawSurfIndex];
        if (!drawSurf->pso) {
            drawSurfIndex++;
            continue;
        }

        if (currentPSO != drawSurf->pso) {
            currentPSO = drawSurf->pso;
            RHI::renderer->SetPSO(mainCommandList, currentPSO);
            RHI::renderer->SetConstantBuffer(mainCommandList, 0, frameConstantBuffer);
            RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
        }

        if (currentSubMesh != drawSurf->visSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->visSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;
            SetPassVertexBuffers(mainCommandList, drawSurf);
            RHI::renderer->SetIndexBuffer(mainCommandList, currentSubMesh->indexBuffer);
        }

        const uint32_t instanceStartIndex = drawSurfIndex++;
        while (drawSurfIndex < numDrawSurfs && CanBatchSurfaceInstances(drawSurfs[instanceStartIndex], drawSurfs[drawSurfIndex])) {
            drawSurfIndex++;
        }

        DrawSurfaceInstances(frameData->GetThreadData(0), mainCommandList, &drawSurfs[instanceStartIndex], drawSurfIndex - instanceStartIndex, statDrawCount, statDrawTriangleCount, statInstanceTableAllocCount, statInstanceTableUploadBytes);
    }
}

void RenderBackend::DrawLitSurfacesByTask(RenderBackend::DrawSurfsTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesByTask");

    // 세컨더리 커맨드 리스트를 시작한다.
    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(taskDesc->threadIndex);
    RHI::CommandList *commandList = frameThreadData->BeginSecondaryCommandList(mainCommandList);

    RHI::PipelineState *currentPSO = nullptr;
    const SubMesh *currentSubMesh = nullptr;
    VertexInputLayoutId currentInputLayoutId = VertexInputLayoutId::Count;

    uint32_t drawSurfIndex = 0;
    while (drawSurfIndex < taskDesc->numDrawSurfs) {
        const PassDrawSurf *drawSurf = taskDesc->drawSurfs[drawSurfIndex];
        if (!drawSurf->pso) {
            drawSurfIndex++;
            continue;
        }

        if (currentPSO != drawSurf->pso) {
            currentPSO = drawSurf->pso;
            RHI::renderer->SetPSO(commandList, currentPSO);
            RHI::renderer->SetConstantBuffer(commandList, 0, frameConstantBuffer);
            RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);
        }

        if (currentSubMesh != drawSurf->visSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->visSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;
            SetPassVertexBuffers(commandList, drawSurf);
            RHI::renderer->SetIndexBuffer(commandList, currentSubMesh->indexBuffer);
        }

        const uint32_t instanceStartIndex = drawSurfIndex++;
        while (drawSurfIndex < taskDesc->numDrawSurfs && CanBatchSurfaceInstances(taskDesc->drawSurfs[instanceStartIndex], taskDesc->drawSurfs[drawSurfIndex])) {
            drawSurfIndex++;
        }

        DrawSurfaceInstances(frameThreadData, commandList, &taskDesc->drawSurfs[instanceStartIndex], drawSurfIndex - instanceStartIndex, taskDesc->drawCount, taskDesc->drawTriangleCount, taskDesc->instanceTableAllocCount, taskDesc->instanceTableUploadBytes);
    }

    // 세컨더리 커맨드 리스트의 기록을 마친다.
    commandList->Close();

    // 세컨더리 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->commandList = commandList;
}

void RenderBackend::DrawLitSurfacesByTaskFunction(void *data) {
    RenderBackend::DrawSurfsTaskDesc *taskDesc = reinterpret_cast<RenderBackend::DrawSurfsTaskDesc *>(data);
    renderSystem->GetBackend()->DrawLitSurfacesByTask(taskDesc);
}

// drawSurfs 를 numTasks 만큼 task 로 나눠서 그린다.
void RenderBackend::DrawLitSurfacesWithTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesWithTask");

    uint32_t numDrawSurfsPerTask = (numDrawSurfs + numTasks - 1) / numTasks;
    uint32_t startIndex = 0;
    int32_t threadIndex = 0;

    // 태스크 정보 초기화
    drawSurfsTaskDescs.Reserve(numTasks);
    drawSurfsTaskDescs.SetCount(0, false);

    // numTask 개수만큼 태스크를 추가한다.
    while (startIndex < numDrawSurfs) {
        DrawSurfsTaskDesc &currentThreadDesc = drawSurfsTaskDescs.Alloc();
        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.drawSurfs = &drawSurfs[startIndex];
        currentThreadDesc.numDrawSurfs = BE1::Min(startIndex + numDrawSurfsPerTask, numDrawSurfs) - startIndex;
        currentThreadDesc.drawCount = 0;
        currentThreadDesc.drawTriangleCount = 0;
        currentThreadDesc.instanceTableAllocCount = 0;
        currentThreadDesc.instanceTableUploadBytes = 0;

        BE1::Engine::taskManager->AddTask(RenderBackend::DrawLitSurfacesByTaskFunction, &currentThreadDesc, drawGroupId, false);

        startIndex += currentThreadDesc.numDrawSurfs;
    }

    // 모든 태스크가 끝날 때까지 기다린다.
    BE1::Engine::taskManager->WaitFinish(drawGroupId, true);

    // 메인 커맨드 리스트에 모든 태스크의 세컨더리 커맨드 리스트들을 기록한다.
    const int32_t threadCount = drawSurfsTaskDescs.Count();
    for (int32_t threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        RecordDrawStats(drawSurfsTaskDescs[threadIndex].drawCount, drawSurfsTaskDescs[threadIndex].drawTriangleCount, drawSurfsTaskDescs[threadIndex].instanceTableAllocCount, drawSurfsTaskDescs[threadIndex].instanceTableUploadBytes);

        mainCommandList->ExecuteSecondary(drawSurfsTaskDescs[threadIndex].commandList);
    }
}

RHI::Buffer *RenderBackend::AllocInstanceToShaderSurfaceTable(RHI::FrameThreadData *frameThreadData, const PassDrawSurf **drawSurfs, uint32_t instanceCount) const {
    RHI::Buffer *buffer = frameThreadData->AllocStructuredBuffer(sizeof(uint32_t), instanceCount, nullptr);
    if (!buffer) {
        return nullptr;
    }

    uint32_t *shaderSurfaceIndices = reinterpret_cast<uint32_t *>(buffer->mappedPtr);
    for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
        shaderSurfaceIndices[instanceIndex] = drawSurfs[instanceIndex]->visSurf->index;
    }

    return buffer;
}

void RenderBackend::DrawSurfaceInstances(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const PassDrawSurf **drawSurfs, uint32_t instanceCount, uint32_t &drawCount, uint64_t &drawTriangleCount, uint32_t &instanceTableAllocCount, uint32_t &instanceTableUploadBytes) {
    assert(instanceCount > 0);

    const VisSurf *firstVisSurf = drawSurfs[0]->visSurf;

    RHI::Buffer *instanceToShaderSurfaceTable = AllocInstanceToShaderSurfaceTable(frameThreadData, drawSurfs, instanceCount);
    if (!instanceToShaderSurfaceTable) {
        return;
    }

    RHI::renderer->SetBuffer(commandList, 0, false, instanceToShaderSurfaceTable);
    RHI::renderer->DrawIndexedInstanced(commandList, firstVisSurf->subMesh->numIndexes, instanceCount, 0, 0, 0);

    drawCount++;
    drawTriangleCount += firstVisSurf->subMesh->numIndexes / 3 * instanceCount;
    instanceTableAllocCount++;
    instanceTableUploadBytes += sizeof(uint32_t) * instanceCount;
}
