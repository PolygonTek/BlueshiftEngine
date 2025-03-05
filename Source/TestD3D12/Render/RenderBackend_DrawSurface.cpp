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
static constexpr uint32_t   MaxDrawCallsPerTask = 512;
static constexpr uint32_t   MaxInstancedDrawCount = 1024;

// 상수 버퍼는 16 바이트 정렬을 요구한다.
struct ALIGN_AS16 UnlitConstantData {
    BE1::Mat4       modelViewProjMatrix;
};

struct ALIGN_AS16 UnlitInstancedConstantData {
    BE1::Mat4       viewProjMatrix;
    BE1::Mat3x4     worldMatrix[MaxInstancedDrawCount];
};

void RenderBackend::DrawAllSurfaces(const DrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    constexpr uint32_t MaxActualDrawSurfs = 65536;
    const DrawSurf **actualDrawSurfs = (const DrawSurf **)_alloca(sizeof(DrawSurf *) * MaxActualDrawSurfs);
    uint32_t actualDrawSurfIndex = 0;
    uint32_t drawSurfIndex = 0;

    while (drawSurfIndex < numDrawSurfs) {
        const DrawSurf *drawSurf = drawSurfs[drawSurfIndex];

        if (!drawSurf->subMesh->vertexBuffer) {
            drawSurf->subMesh->UploadStaticDataToGPU();
        }

#ifdef USE_RENDER_INSTANCED
        if (BE1::HasFlag(drawSurf->flags, DrawSurf::Flag::UseInstancing)) {
            uint32_t instanceStartIndex = drawSurfIndex;

            while (drawSurfIndex < numDrawSurfs &&
                drawSurfs[drawSurfIndex]->subMesh == drawSurf->subMesh &&
                drawSurfs[drawSurfIndex]->texture == drawSurf->texture) {
                drawSurfIndex++;
            }

            uint32_t instanceCount = drawSurfIndex - instanceStartIndex;
            if (instanceCount > 1) {
                // 인스턴스 개수가 2 개 이상이어야 인스턴스드 렌더링을 수행한다.
                DrawInstancedSurface(&drawSurfs[instanceStartIndex], instanceCount);
                continue;
            } else {
                drawSurfIndex = instanceStartIndex;
            }
        }
#endif
        if (actualDrawSurfIndex >= MaxActualDrawSurfs) {
            DrawSurfaces(actualDrawSurfs, actualDrawSurfIndex);
            actualDrawSurfIndex = 0;
        }

        actualDrawSurfs[actualDrawSurfIndex++] = drawSurf;
        drawSurfIndex++;
    }

    if (actualDrawSurfIndex > 0) {
        DrawSurfaces(actualDrawSurfs, actualDrawSurfIndex);
    }
}

void RenderBackend::DrawSurfaces(const DrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    assert(numDrawSurfs > 0);

#ifdef USE_RENDER_TASK
    uint32_t numTasks = (uint32_t)BE1::Math::Ceil((float)numDrawSurfs / MaxDrawCallsPerTask);
    numTasks = BE1::Min(BE1::Engine::taskManager->NumThreads(), numTasks);

    if (numTasks > 1) {
        DrawSurfacesWithTask(drawSurfs, numDrawSurfs, numTasks);
    } else {
        DrawSurfacesWithoutTask(drawSurfs, numDrawSurfs);
    }
#else
    DrawSurfacesWithoutTask(drawSurfs, numDrawSurfs);
#endif
}

void RenderBackend::DrawInstancedSurface(const DrawSurf **drawSurfs, uint32_t instanceCount) {
    assert(instanceCount > 0);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(0);

    do {
        uint32_t currentInstanceCount = BE1::Min(instanceCount, MaxInstancedDrawCount);

        DrawInstancedSurface(mainCommandList, drawSurfs, currentInstanceCount);

        drawSurfs += currentInstanceCount;
        instanceCount -= currentInstanceCount;
    } while (instanceCount > 0);
}

void RenderBackend::DrawSurfacesWithoutTask(const DrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesWithoutTask", 10);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(0);

    for (int drawSurfIndex = 0; drawSurfIndex < numDrawSurfs; ++drawSurfIndex) {
        const DrawSurf *drawSurf = drawSurfs[drawSurfIndex];

        DrawSurface(mainCommandList, drawSurf);
    }
}

#ifdef USE_RENDER_TASK
void RenderBackend::DrawSurfacesByTask(RenderBackend::DrawObjectTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesByTask", 10);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(taskDesc->threadIndex);

    // Secondary CommandList 를 시작한다.
    RHI::CommandList *commandList = currentFrameThreadData->BeginSecondaryCommandList(mainCommandList);

    for (int drawSurfIndex = 0; drawSurfIndex < taskDesc->numDrawSurfs; ++drawSurfIndex) {
        const DrawSurf *drawSurf = taskDesc->drawSurfs[drawSurfIndex];

        DrawSurface(commandList, drawSurf);
    }

    // CommandList 기록을 마친다.
    commandList->Close();

    // 사용 중인 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->activeCommandList = commandList;
}

void RenderBackend::DrawSurfacesByTaskFunction(void *data) {
    RenderBackend::DrawObjectTaskDesc *taskDesc = reinterpret_cast<RenderBackend::DrawObjectTaskDesc *>(data);
    renderSystem->GetBackend()->DrawSurfacesByTask(taskDesc);
}

// drawSurfs 를 numTasks 만큼 task 로 나눠서 그린다.
void RenderBackend::DrawSurfacesWithTask(const DrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesWithTask", 10);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    uint32_t numDrawSurfsPerTasks = (uint32_t)BE1::Math::Ceil((float)numDrawSurfs / numTasks);
    uint32_t startIndex = 0;
    int threadIndex = 0;

    // 태스크 정보 초기화
    objectDrawingTaskDescs.Reserve(numTasks);
    objectDrawingTaskDescs.SetCount(0, false);

    // numTask 개수만큼 task 를 실행한다.
    while (startIndex < numDrawSurfs) {
        DrawObjectTaskDesc &currentThreadDesc = objectDrawingTaskDescs.Alloc();

        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.drawSurfs = &drawSurfs[startIndex];
        currentThreadDesc.numDrawSurfs = BE1::Min(startIndex + numDrawSurfsPerTasks, numDrawSurfs) - startIndex;
        BE1::Engine::taskManager->AddTask(RenderBackend::DrawSurfacesByTaskFunction, &currentThreadDesc, drawGroupId, false);

        startIndex += currentThreadDesc.numDrawSurfs;
    }

    BE1::Engine::taskManager->WaitFinish(drawGroupId, true);

    // Main CommandList 에 모든 태스크의 Secondary CommandList 들을 기록한다.
    int renderTaskCount = objectDrawingTaskDescs.Count();
    for (int threadIndex = 0; threadIndex < renderTaskCount; ++threadIndex) {
        const RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(threadIndex);

        objectDrawingTaskDescs[threadIndex].activeCommandList->ExecuteSecondary(mainCommandList, currentFrameThreadData);
    }
}
#endif // USE_RENDER_TASK

void RenderBackend::DrawSurface(RHI::CommandList *commandList, const DrawSurf *drawSurf) {
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(UnlitConstantData));
    if (!constantBuffer) {
        return;
    }

    UnlitConstantData *constantDataPtr = reinterpret_cast<UnlitConstantData *>(constantBuffer->writePtr);

    // 오브젝트의 MVP 행렬을 기록
    constantDataPtr->modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;

    RHI::renderer->SetVertexBuffer(commandList, 0, drawSurf->subMesh->vertexBuffer);
    RHI::renderer->SetIndexBuffer(commandList, drawSurf->subMesh->indexBuffer);

    RHI::renderer->SetPSO(commandList, currentContext->unlitPSO);
    RHI::renderer->SetTexture(commandList, 0, false, drawSurf->texture->GetRHITexture());
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, drawSurf->subMesh->numIndexes, 0, 0);
}

void RenderBackend::DrawInstancedSurface(RHI::CommandList *commandList, const DrawSurf **instanceSurfs, int instanceCount) {
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(UnlitInstancedConstantData));
    if (!constantBuffer) {
        return;
    }

    UnlitInstancedConstantData *constantDataPtr = reinterpret_cast<UnlitInstancedConstantData *>(constantBuffer->writePtr);

    // 카메라의 뷰-프로젝션 행렬을 기록
    constantDataPtr->viewProjMatrix = currentVisCamera->viewProjMatrix;

    // 오브젝트 인스턴스들의 월드 행렬을 기록
    for (int i = 0; i < instanceCount; ++i) {
        constantDataPtr->worldMatrix[i] = instanceSurfs[i]->space->worldMatrix;
    }

    RHI::renderer->SetVertexBuffer(commandList, 0, instanceSurfs[0]->subMesh->vertexBuffer);
    RHI::renderer->SetIndexBuffer(commandList, instanceSurfs[0]->subMesh->indexBuffer);

    RHI::renderer->SetPSO(commandList, currentContext->unlitInstancedPSO);
    RHI::renderer->SetTexture(commandList, 0, false, instanceSurfs[0]->texture->GetRHITexture());
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexedInstanced(commandList, instanceSurfs[0]->subMesh->numIndexes, instanceCount, 0, 0, 0);
}

void RenderBackend::DrawGuiSurface(RHI::CommandList *commandList, const DrawSurf *drawSurf) {
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(UnlitConstantData));
    if (!constantBuffer) {
        return;
    }

    UnlitConstantData *constantDataPtr = reinterpret_cast<UnlitConstantData *>(constantBuffer->writePtr);

    constantDataPtr->modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;

    RHI::renderer->SetVertexBuffer(commandList, 0, drawSurf->subMesh->vertexBuffer);
    RHI::renderer->SetIndexBuffer(commandList, drawSurf->subMesh->indexBuffer);

    RHI::renderer->SetPSO(commandList, currentContext->unlitAlphaBlendPSO);
    RHI::renderer->SetTexture(commandList, 0, false, drawSurf->texture->GetRHITexture());
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, drawSurf->subMesh->numIndexes, 0, 0);
}
