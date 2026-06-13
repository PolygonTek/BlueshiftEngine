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
#include "RenderInternal.h"

void RenderBackend::DrawGenericSurfaces(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericSurfaces");

    assert(numDrawSurfs > 0);

#ifdef USE_RENDER_TASK
    // numDrawSurfs 가 많으면 태스크로 나눠서 그린다.
    if (numDrawSurfs > MaxInstancesPerDraw && BE1::Engine::taskManager->NumThreads() > 1) {
        DrawGenericSurfacesWithTask(drawSurfs, numDrawSurfs);
    } else {
        DrawGenericSurfacesWithoutTask(drawSurfs, numDrawSurfs);
    }
#else
    DrawGenericSurfacesWithoutTask(drawSurfs, numDrawSurfs);
#endif
}

void RenderBackend::DrawGenericSurfacesWithoutTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericSurfacesWithoutTask");

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

        const uint32_t instanceStartIndex = drawSurfIndex;
        drawSurfIndex = FindSurfaceInstanceBatchEnd(drawSurfs, numDrawSurfs, instanceStartIndex);

        DrawSurfaceInstances(frameData->GetThreadData(0), mainCommandList, &drawSurfs[instanceStartIndex], drawSurfIndex - instanceStartIndex, statDrawCount, statDrawTriangleCount, statInstanceTableAllocCount, statInstanceTableUploadBytes);
    }
}

void RenderBackend::DrawGenericSurfacesByTask(RenderBackend::DrawSurfsTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericSurfacesByTask");

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

        const uint32_t instanceStartIndex = drawSurfIndex;
        drawSurfIndex = FindSurfaceInstanceBatchEnd(taskDesc->drawSurfs, taskDesc->numDrawSurfs, instanceStartIndex);

        DrawSurfaceInstances(frameThreadData, commandList, &taskDesc->drawSurfs[instanceStartIndex], drawSurfIndex - instanceStartIndex, taskDesc->drawCount, taskDesc->drawTriangleCount, taskDesc->instanceTableAllocCount, taskDesc->instanceTableUploadBytes);
    }

    // 세컨더리 커맨드 리스트의 기록을 마친다.
    commandList->Close();

    // 세컨더리 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->commandList = commandList;
}

void RenderBackend::DrawGenericSurfacesByTaskFunction(void *data) {
    RenderBackend::DrawSurfsTaskDesc *taskDesc = reinterpret_cast<RenderBackend::DrawSurfsTaskDesc *>(data);
    renderSystem->GetBackend()->DrawGenericSurfacesByTask(taskDesc);
}

void RenderBackend::DrawGenericSurfacesWithTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericSurfacesWithTask");

    // numDrawSurfs 로 만들어 질 수 있는 최대 태스크 개수를 계산한다.
    const uint32_t maxTasks = BE1::Min(BE1::Engine::taskManager->NumThreads(), (numDrawSurfs + MaxInstancesPerDraw - 1) / MaxInstancesPerDraw);

    // 태스크 당 처리하는 목표 DrawSurfs 개수 (인스턴싱 경계에 걸리지 않도록 더 추가될 수 있다)
    const uint32_t targetDrawSurfsPerTask = (numDrawSurfs + maxTasks - 1) / maxTasks;

    uint32_t startIndex = 0;
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

        DrawSurfsTaskDesc &currentThreadDesc = drawSurfsTaskDescs.Alloc();
        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.drawSurfs = &drawSurfs[startIndex];
        currentThreadDesc.numDrawSurfs = endIndex - startIndex;
        currentThreadDesc.drawCount = 0;
        currentThreadDesc.drawTriangleCount = 0;
        currentThreadDesc.instanceTableAllocCount = 0;
        currentThreadDesc.instanceTableUploadBytes = 0;

        BE1::Engine::taskManager->AddTask(RenderBackend::DrawGenericSurfacesByTaskFunction, &currentThreadDesc, drawGroupId, false);

        startIndex = endIndex;
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
