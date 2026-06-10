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
static constexpr uint32_t   MaxDrawSurfsPerTask = 1024;

// 상수 버퍼는 16 바이트 정렬을 요구한다.
struct ALIGN_AS16 UnlitConstantData {
    BE1::Mat4       modelViewProjMatrix;
    uint32_t        shaderMaterialIndex;
    uint32_t        padding[3];
};

struct ALIGN_AS16 UnlitInstancedConstantData {
    BE1::Mat4       viewProjMatrix;
    BE1::Mat3x4     worldMatrix[MaxDrawInstanceCount];
    uint32_t        shaderMaterialIndex;
    uint32_t        padding[3];
};

struct ALIGN_AS16 LitConstantData {
    BE1::Mat4       modelViewProjMatrix;
    uint32_t        shaderMaterialIndex;
    uint32_t        padding[3];
};

struct ALIGN_AS16 LitInstancedConstantData {
    BE1::Mat4       viewProjMatrix;
    BE1::Mat3x4     worldMatrix[MaxDrawInstanceCount];
    uint32_t        shaderMaterialIndex;
    uint32_t        padding[3];
};

void RenderBackend::SetPassVertexBuffers(RHI::CommandList *commandList, const PassDrawSurf *drawSurf) const {
    const SubMesh *subMesh = drawSurf->visSurf->subMesh;

    RHI::renderer->SetVertexBuffer(commandList, 0, subMesh->vertexBuffer);
    if (VertexInputLayoutFactory::IsSkinning(drawSurf->inputLayoutId)) {
        RHI::renderer->SetVertexBuffer(commandList, 1, subMesh->vertexWeightBuffer);
    }
}

void RenderBackend::DrawMainPass(const RenderPass *renderPass) {
    const PassDrawSurf **drawSurfs = renderPass->drawSurfs;
    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    const PassDrawSurf **actualDrawSurfs = (const PassDrawSurf **)_alloca(sizeof(PassDrawSurf *) * numDrawSurfs);
    uint32_t actualDrawSurfIndex = 0;
    uint32_t drawSurfIndex = 0;

    while (drawSurfIndex < numDrawSurfs) {
        const PassDrawSurf *drawSurf = drawSurfs[drawSurfIndex];
        const VisSurf *visSurf = drawSurf->visSurf;

        if (!visSurf->subMesh->vertexBuffer) {
            visSurf->subMesh->UploadStaticDataToGPU();
        }

#ifdef USE_RENDER_INSTANCED
        RHI::PipelineState *instancedPSO = drawSurf->instancedPSO;
        if (BE1::HasFlag(visSurf->flags, VisSurfFlag::UseInstancing) && instancedPSO) {
            uint32_t instanceStartIndex = drawSurfIndex;

            while (drawSurfIndex < numDrawSurfs) {
                const PassDrawSurf *nextDrawSurf = drawSurfs[drawSurfIndex];
                const VisSurf *nextVisSurf = nextDrawSurf->visSurf;

                if (!BE1::HasFlag(nextDrawSurf->visSurf->flags, VisSurfFlag::UseInstancing) ||
                    nextVisSurf->subMesh != visSurf->subMesh ||
                    nextVisSurf->shaderMaterialIndex != visSurf->shaderMaterialIndex ||
                    nextDrawSurf->inputLayoutId != drawSurf->inputLayoutId ||
                    nextDrawSurf->instancedPSO != instancedPSO) {
                    break;
                }

                drawSurfIndex++;
            }

            uint32_t instanceCount = drawSurfIndex - instanceStartIndex;
            if (instanceCount > 1) {
                // 인스턴스 개수가 2 개 이상이어야 인스턴스드 렌더링을 수행한다.
                DrawLitSurfacesInstanced(&drawSurfs[instanceStartIndex], instanceCount);
                continue;
            } else {
                drawSurfIndex = instanceStartIndex;
            }
        }
#endif
        actualDrawSurfs[actualDrawSurfIndex++] = drawSurf;
        drawSurfIndex++;
    }

    if (actualDrawSurfIndex > 0) {
        DrawLitSurfaces(actualDrawSurfs, actualDrawSurfIndex);
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

void RenderBackend::DrawLitSurfacesInstanced(const PassDrawSurf **drawSurfs, uint32_t instanceCount) {
    assert(instanceCount > 0);

    RHI::PipelineState *pso = drawSurfs[0]->instancedPSO;
    if (!pso) {
        return;
    }

    RHI::renderer->SetPSO(mainCommandList, pso);

    RHI::renderer->SetConstantBuffer(mainCommandList, 0, frameConstantBuffer);

    SetPassVertexBuffers(mainCommandList, drawSurfs[0]);
    RHI::renderer->SetIndexBuffer(mainCommandList, drawSurfs[0]->visSurf->subMesh->indexBuffer);

    do {
        uint32_t currentInstanceCount = BE1::Min(instanceCount, MaxDrawInstanceCount);

        DrawLitSurfaceInstanced(frameData->GetThreadData(0), mainCommandList, drawSurfs, currentInstanceCount, statDrawCount, statTriangleCount);

        drawSurfs += currentInstanceCount;
        instanceCount -= currentInstanceCount;
    } while (instanceCount > 0);
}

void RenderBackend::DrawLitSurfacesWithoutTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesWithoutTask");

    RHI::PipelineState *currentPSO = nullptr;
    const SubMesh *currentSubMesh = nullptr;
    VertexInputLayoutId currentInputLayoutId = VertexInputLayoutId::Count;

    for (int drawSurfIndex = 0; drawSurfIndex < numDrawSurfs; ++drawSurfIndex) {
        const PassDrawSurf *drawSurf = drawSurfs[drawSurfIndex];
        if (!drawSurf->basePSO) {
            continue;
        }

        if (currentPSO != drawSurf->basePSO) {
            currentPSO = drawSurf->basePSO;
            RHI::renderer->SetPSO(mainCommandList, currentPSO);
            RHI::renderer->SetConstantBuffer(mainCommandList, 0, frameConstantBuffer);
        }

        if (currentSubMesh != drawSurf->visSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->visSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;
            SetPassVertexBuffers(mainCommandList, drawSurf);
            RHI::renderer->SetIndexBuffer(mainCommandList, currentSubMesh->indexBuffer);
        }

        DrawLitSurface(frameData->GetThreadData(0), mainCommandList, drawSurf, statDrawCount, statTriangleCount);
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

    for (int drawSurfIndex = 0; drawSurfIndex < taskDesc->numDrawSurfs; ++drawSurfIndex) {
        const PassDrawSurf *drawSurf = taskDesc->drawSurfs[drawSurfIndex];
        if (!drawSurf->basePSO) {
            continue;
        }

        if (currentPSO != drawSurf->basePSO) {
            currentPSO = drawSurf->basePSO;
            RHI::renderer->SetPSO(commandList, currentPSO);
            RHI::renderer->SetConstantBuffer(commandList, 0, frameConstantBuffer);
        }

        if (currentSubMesh != drawSurf->visSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->visSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;
            SetPassVertexBuffers(commandList, drawSurf);
            RHI::renderer->SetIndexBuffer(commandList, currentSubMesh->indexBuffer);
        }

        DrawLitSurface(frameThreadData, commandList, drawSurf, taskDesc->drawCount, taskDesc->drawTriangleCount);
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

        BE1::Engine::taskManager->AddTask(RenderBackend::DrawLitSurfacesByTaskFunction, &currentThreadDesc, drawGroupId, false);

        startIndex += currentThreadDesc.numDrawSurfs;
    }

    // 모든 태스크가 끝날 때까지 기다린다.
    BE1::Engine::taskManager->WaitFinish(drawGroupId, true);

    // 메인 커맨드 리스트에 모든 태스크의 세컨더리 커맨드 리스트들을 기록한다.
    const int32_t threadCount = drawSurfsTaskDescs.Count();
    for (int32_t threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        RecordDrawStats(drawSurfsTaskDescs[threadIndex].drawCount, drawSurfsTaskDescs[threadIndex].drawTriangleCount);

        mainCommandList->ExecuteSecondary(drawSurfsTaskDescs[threadIndex].commandList);
    }
}

void RenderBackend::DrawLitSurface(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const PassDrawSurf *drawSurf, uint32_t &drawCount, uint64_t &drawTriangleCount) {
    const VisSurf *visSurf = drawSurf->visSurf;

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(LitConstantData));
    if (!constantBuffer) {
        return;
    }

    LitConstantData *constantDataPtr = reinterpret_cast<LitConstantData *>(constantBuffer->mappedPtr);

    // 오브젝트의 MVP 행렬을 기록
    constantDataPtr->modelViewProjMatrix = visSurf->space->modelViewProjMatrix;

    // 오브젝트가 사용할 shader material 인덱스 기록
    constantDataPtr->shaderMaterialIndex = visSurf->shaderMaterialIndex;

    RHI::renderer->SetConstantBuffer(commandList, 1, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, visSurf->subMesh->numIndexes, 0, 0);

    drawCount++;
    drawTriangleCount += visSurf->subMesh->numIndexes / 3;
}

void RenderBackend::DrawLitSurfaceInstanced(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const PassDrawSurf **instanceSurfs, int instanceCount, uint32_t &drawCount, uint64_t &drawTriangleCount) {
    const VisSurf *firstVisSurf = instanceSurfs[0]->visSurf;

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(LitInstancedConstantData));
    if (!constantBuffer) {
        return;
    }

    LitInstancedConstantData *constantDataPtr = reinterpret_cast<LitInstancedConstantData *>(constantBuffer->mappedPtr);

    // 카메라의 뷰-프로젝션 행렬을 기록
    constantDataPtr->viewProjMatrix = currentVisCamera->viewProjMatrix;

    // 인스턴스들이 공유하는 shader material 인덱스 기록
    constantDataPtr->shaderMaterialIndex = firstVisSurf->shaderMaterialIndex;

    // 오브젝트 인스턴스들의 월드 행렬을 기록
    for (int i = 0; i < instanceCount; ++i) {
        constantDataPtr->worldMatrix[i] = instanceSurfs[i]->visSurf->space->worldMatrix;
    }

    RHI::renderer->SetConstantBuffer(commandList, 1, constantBuffer);

    RHI::renderer->DrawIndexedInstanced(commandList, firstVisSurf->subMesh->numIndexes, instanceCount, 0, 0, 0);

    drawCount++;
    drawTriangleCount += firstVisSurf->subMesh->numIndexes / 3 * instanceCount;
}

void RenderBackend::DrawGuiSurface(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const PassDrawSurf *drawSurf, uint32_t &drawCount, uint64_t &drawTriangleCount) {
    const VisSurf *visSurf = drawSurf->visSurf;

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(UnlitConstantData));
    if (!constantBuffer) {
        return;
    }

    UnlitConstantData *constantDataPtr = reinterpret_cast<UnlitConstantData *>(constantBuffer->mappedPtr);

    constantDataPtr->modelViewProjMatrix = visSurf->space->modelViewProjMatrix;
    constantDataPtr->shaderMaterialIndex = visSurf->shaderMaterialIndex;

    RHI::renderer->SetConstantBuffer(commandList, 1, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, visSurf->subMesh->numIndexes, 0, 0);

    drawCount++;
    drawTriangleCount += visSurf->subMesh->numIndexes / 3;
}
