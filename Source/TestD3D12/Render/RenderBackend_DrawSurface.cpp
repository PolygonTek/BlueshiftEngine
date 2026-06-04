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
static constexpr uint32_t   MaxDrawSurfsPerTask = 512;

// 상수 버퍼는 16 바이트 정렬을 요구한다.
struct ALIGN_AS16 UnlitConstantData {
    BE1::Mat4       modelViewProjMatrix;
    uint32_t        baseColorTextureIndex;
    uint32_t        padding[3];
};

struct ALIGN_AS16 UnlitInstancedConstantData {
    BE1::Mat4       viewProjMatrix;
    BE1::Mat3x4     worldMatrix[MaxDrawInstanceCount];
    uint32_t        baseColorTextureIndex;
    uint32_t        padding[3];
};

struct ALIGN_AS16 LitConstantData {
    BE1::Mat4       modelViewProjMatrix;
    uint32_t        baseColorTextureIndex;
    uint32_t        streamingTextureIndex;
    uint32_t        padding[2];
};

struct ALIGN_AS16 LitInstancedConstantData {
    BE1::Mat4       viewProjMatrix;
    BE1::Mat3x4     worldMatrix[MaxDrawInstanceCount];
    uint32_t        baseColorTextureIndex;
    uint32_t        streamingTextureIndex;
    uint32_t        padding[2];
};

void RenderBackend::DrawAllLitSurfaces(const DrawSurf **drawSurfs, uint32_t numDrawSurfs) {
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
                DrawLitSurfacesInstanced(&drawSurfs[instanceStartIndex], instanceCount);
                continue;
            } else {
                drawSurfIndex = instanceStartIndex;
            }
        }
#endif
        if (actualDrawSurfIndex >= MaxActualDrawSurfs) {
            DrawLitSurfaces(actualDrawSurfs, actualDrawSurfIndex);
            actualDrawSurfIndex = 0;
        }

        actualDrawSurfs[actualDrawSurfIndex++] = drawSurf;
        drawSurfIndex++;
    }

    if (actualDrawSurfIndex > 0) {
        DrawLitSurfaces(actualDrawSurfs, actualDrawSurfIndex);
    }
}

void RenderBackend::DrawLitSurfaces(const DrawSurf **drawSurfs, uint32_t numDrawSurfs) {
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

const RHI::Texture *RenderBackend::ResolveDrawSurfTexture(const DrawSurf *drawSurf, bool useDefaultTexture) const {
    const RHI::Texture *rhiTexture = nullptr;
    if (drawSurf->texture) {
        rhiTexture = drawSurf->texture->GetRHITexture();
    }
    if (!rhiTexture || rhiTexture->GetStatus() != RHI::GPUResourceStatus::Ready) {
        rhiTexture = useDefaultTexture ? textureManager.defaultTexture->GetRHITexture() : nullptr;
    }
    return rhiTexture;
}

uint32_t RenderBackend::ResolveDrawSurfTextureBindlessIndex(const DrawSurf *drawSurf, bool useDefaultTexture, RHI::ViewType viewType, RHI::ViewID viewID) const {
    const RHI::Texture *texture = ResolveDrawSurfTexture(drawSurf, useDefaultTexture);
    if (!texture) {
        return 0;
    }
    return texture->GetBindlessIndex(viewType, viewID);
}

void RenderBackend::DrawLitSurfacesInstanced(const DrawSurf **drawSurfs, uint32_t instanceCount) {
    assert(instanceCount > 0);

    if (!currentPipelines.litInstancedPSO) {
        return;
    }

    RHI::renderer->SetPSO(mainCommandList, currentPipelines.litInstancedPSO);
    RHI::renderer->SetVertexBuffer(mainCommandList, 0, drawSurfs[0]->subMesh->vertexBuffer);
    RHI::renderer->SetIndexBuffer(mainCommandList, drawSurfs[0]->subMesh->indexBuffer);

    uint32_t feedbackBufferIndex = currentContext->textureStreamingFeedback.buffer->GetBindlessIndex(RHI::ViewType::UAV);
    RHI::renderer->SetConstants(mainCommandList, 1, &feedbackBufferIndex, sizeof(feedbackBufferIndex), 0);

    do {
        uint32_t currentInstanceCount = BE1::Min(instanceCount, MaxDrawInstanceCount);

        DrawLitSurfaceInstanced(frameData->GetThreadData(0), mainCommandList, drawSurfs, currentInstanceCount);

        drawSurfs += currentInstanceCount;
        instanceCount -= currentInstanceCount;
    } while (instanceCount > 0);
}

void RenderBackend::DrawLitSurfacesWithoutTask(const DrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesWithoutTask");

    if (!currentPipelines.litPSO) {
        return;
    }

    RHI::renderer->SetPSO(mainCommandList, currentPipelines.litPSO);

    uint32_t feedbackBufferIndex = currentContext->textureStreamingFeedback.buffer->GetBindlessIndex(RHI::ViewType::UAV);
    RHI::renderer->SetConstants(mainCommandList, 1, &feedbackBufferIndex, sizeof(feedbackBufferIndex), 0);

    const SubMesh *currentSubMesh = nullptr;

    for (int drawSurfIndex = 0; drawSurfIndex < numDrawSurfs; ++drawSurfIndex) {
        const DrawSurf *drawSurf = drawSurfs[drawSurfIndex];

        if (currentSubMesh != drawSurf->subMesh) {
            currentSubMesh = drawSurf->subMesh;
            RHI::renderer->SetVertexBuffer(mainCommandList, 0, currentSubMesh->vertexBuffer);
            RHI::renderer->SetIndexBuffer(mainCommandList, currentSubMesh->indexBuffer);
        }

        DrawLitSurface(frameData->GetThreadData(0), mainCommandList, drawSurf);
    }
}

void RenderBackend::DrawLitSurfacesByTask(RenderBackend::DrawSurfsTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSurfacesByTask");

    // 세컨더리 커맨드 리스트를 시작한다.
    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(taskDesc->threadIndex);
    RHI::CommandList *commandList = frameThreadData->BeginSecondaryCommandList(mainCommandList);

    if (!currentPipelines.litPSO) {
        commandList->Close();
        taskDesc->commandList = commandList;
        return;
    }

    RHI::renderer->SetPSO(commandList, currentPipelines.litPSO);

    uint32_t feedbackBufferIndex = currentContext->textureStreamingFeedback.buffer->GetBindlessIndex(RHI::ViewType::UAV);
    RHI::renderer->SetConstants(commandList, 1, &feedbackBufferIndex, sizeof(feedbackBufferIndex), 0);

    const SubMesh *currentSubMesh = nullptr;
    const RHI::Texture *currentTexture = nullptr;

    for (int drawSurfIndex = 0; drawSurfIndex < taskDesc->numDrawSurfs; ++drawSurfIndex) {
        const DrawSurf *drawSurf = taskDesc->drawSurfs[drawSurfIndex];

        if (currentSubMesh != drawSurf->subMesh) {
            currentSubMesh = drawSurf->subMesh;
            RHI::renderer->SetVertexBuffer(commandList, 0, currentSubMesh->vertexBuffer);
            RHI::renderer->SetIndexBuffer(commandList, currentSubMesh->indexBuffer);
        }

        DrawLitSurface(frameThreadData, commandList, drawSurf);
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
void RenderBackend::DrawLitSurfacesWithTask(const DrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t numTasks) {
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
        BE1::Engine::taskManager->AddTask(RenderBackend::DrawLitSurfacesByTaskFunction, &currentThreadDesc, drawGroupId, false);

        startIndex += currentThreadDesc.numDrawSurfs;
    }

    // 모든 태스크가 끝날 때까지 기다린다.
    BE1::Engine::taskManager->WaitFinish(drawGroupId, true);

    // 메인 커맨드 리스트에 모든 태스크의 세컨더리 커맨드 리스트들을 기록한다.
    for (int32_t threadIndex = 0; threadIndex < drawSurfsTaskDescs.Count(); ++threadIndex) {
        mainCommandList->ExecuteSecondary(drawSurfsTaskDescs[threadIndex].commandList);
    }
}

void RenderBackend::DrawLitSurface(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const DrawSurf *drawSurf) {
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(LitConstantData));
    if (!constantBuffer) {
        return;
    }

    LitConstantData *constantDataPtr = reinterpret_cast<LitConstantData *>(constantBuffer->mappedPtr);

    // 오브젝트의 MVP 행렬을 기록
    constantDataPtr->modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;

    // 오브젝트 텍스쳐의 바인드리스 인덱스 기록
    constantDataPtr->baseColorTextureIndex = ResolveDrawSurfTextureBindlessIndex(drawSurf, false, RHI::ViewType::SRV);

    // 오브젝트의 텍스처 인덱스를 기록
    constantDataPtr->streamingTextureIndex = drawSurf->texture->GetStreaming()->streamingListIndex;

    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, drawSurf->subMesh->numIndexes, 0, 0);
    RecordDrawStats(1, drawSurf->subMesh->numIndexes / 3);
}

void RenderBackend::DrawLitSurfaceInstanced(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const DrawSurf **instanceSurfs, int instanceCount) {
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(LitInstancedConstantData));
    if (!constantBuffer) {
        return;
    }

    LitInstancedConstantData *constantDataPtr = reinterpret_cast<LitInstancedConstantData *>(constantBuffer->mappedPtr);

    // 카메라의 뷰-프로젝션 행렬을 기록
    constantDataPtr->viewProjMatrix = currentVisCamera->viewProjMatrix;

    // 인스턴스 텍스쳐의 바인드리스 인덱스 기록
    constantDataPtr->baseColorTextureIndex = ResolveDrawSurfTextureBindlessIndex(instanceSurfs[0], false, RHI::ViewType::SRV);

    // 인스턴스의 텍스처 인덱스를 기록
    constantDataPtr->streamingTextureIndex = instanceSurfs[0]->texture->GetStreaming()->streamingListIndex;

    // 오브젝트 인스턴스들의 월드 행렬을 기록
    for (int i = 0; i < instanceCount; ++i) {
        constantDataPtr->worldMatrix[i] = instanceSurfs[i]->space->worldMatrix;
    }

    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexedInstanced(commandList, instanceSurfs[0]->subMesh->numIndexes, instanceCount, 0, 0, 0);
    RecordDrawStats(1, instanceSurfs[0]->subMesh->numIndexes / 3 * instanceCount);
}

void RenderBackend::DrawGuiSurface(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const DrawSurf *drawSurf) {
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(UnlitConstantData));
    if (!constantBuffer) {
        return;
    }

    UnlitConstantData *constantDataPtr = reinterpret_cast<UnlitConstantData *>(constantBuffer->mappedPtr);

    constantDataPtr->modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;

    constantDataPtr->baseColorTextureIndex = ResolveDrawSurfTextureBindlessIndex(drawSurf, false, RHI::ViewType::SRV);

    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, drawSurf->subMesh->numIndexes, 0, 0);
    RecordDrawStats(1, drawSurf->subMesh->numIndexes / 3);
}
