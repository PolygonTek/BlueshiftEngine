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
#include "RenderBackEnd.h"
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

void RenderBackEnd::Init() {
    guiMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame2D);

#ifdef USE_RENDER_TASK
    drawGroupId = BE1::Engine::taskManager->CreateGroupId();
#endif
}

void RenderBackEnd::Shutdown() {
}

void RenderBackEnd::Execute(const void *data) {
    while (1) {
        RenderCommandId cmdId = *reinterpret_cast<const RenderCommandId *>(data);
        switch (cmdId) {
        case RenderCommandId::BeginContext:
            data = ExecuteBeginContext(data);
            continue;
        case RenderCommandId::DrawCamera:
            data = ExecuteDrawCamera(data);
            continue;
        case RenderCommandId::DrawPic:
            data = ExecuteDrawPic(data);
            continue;
        case RenderCommandId::ScreenShot:
            data = ExecuteScreenshot(data);
            continue;
        case RenderCommandId::SwapBuffers:
            data = ExecuteSwapBuffers(data);
            continue;
        case RenderCommandId::End:
            return;
        default:
            BE_ERRLOG("RenderBackEnd::Execute: invalid render command ID (%i)\n", cmdId);
            return;
        }
    }
}

const void *RenderBackEnd::ExecuteBeginContext(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::ExecuteBeginContext", 8);

    const BeginContextRenderCommand *cmd = reinterpret_cast<const BeginContextRenderCommand *>(data);

    currentContext = cmd->renderContext;

    // 프레임 데이터를 초기화하고, 이전 프레임에 대한 펜스를 기다린다.
    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    currentFrameData->BeginFrame();

    return (const void *)(cmd + 1);
}

const void *RenderBackEnd::ExecuteDrawCamera(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::ExecuteDrawCamera", 8);

    const DrawCameraRenderCommand *cmd = reinterpret_cast<const DrawCameraRenderCommand *>(data);

    currentVisCamera = cmd->visCamera;

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *frameThreadData = currentFrameData->GetThreadData(0);

    // 커맨드 리스트 풀에서 새로운 커맨드 리스트를 얻어온다.
    mainCommandList = frameThreadData->BeginCommandList(RHI::CommandQueueType::Graphics);

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    mainCommandList->Reset();

    // 뷰포트 & ScissorRect 의 초기값 설정
    RHI::renderer->SetViewport(mainCommandList, currentVisCamera->renderRect);
    RHI::renderer->SetScissorRect(mainCommandList, currentVisCamera->renderRect);

    RHI::ClearFlag clearFlags = RHI::ClearFlag::None;
    BE1::Color4 clearColor = BE1::Color4::black;
    float clearDepth = 1.0f;

    if (currentVisCamera->clearMethod == RenderCameraClearMethod::Color) {
        clearFlags |= (RHI::ClearFlag::Color | RHI::ClearFlag::Depth);
        clearColor = currentVisCamera->clearColor;
    } else if (currentVisCamera->clearMethod == RenderCameraClearMethod::DepthOnly || currentVisCamera->clearMethod == RenderCameraClearMethod::Skybox) {
        clearFlags |= RHI::ClearFlag::Depth;
    }

#if 1
    RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, currentContext->mainRTDepthTexture, clearColor, clearDepth, 0, clearFlags);
#else
    if (mainRTSampleCount > 1) {
        RHI::RenderPassImage renderPassImages[] = {
            RHI::RenderPassImage::Color(currentContext->mainRTColorMSAATexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::DepthStencil(currentContext->mainRTDepthTexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::ResolveColor(currentContext->mainRTColorTexture, 0, 0)
        };
        RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));
    } else {
        RHI::RenderPassImage renderPassImages[] = {
            RHI::RenderPassImage::Color(currentContext->mainRTColorTexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::DepthStencil(currentContext->mainRTDepthTexture, 0, RHI::RenderPassImage::LoadAction::Clear)
        };
        RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));
    }
#endif

    guiMesh.SetClipRect(currentVisCamera->renderRect);

    if (currentVisCamera->is2D) {
        DrawCamera2D();
    } else {
        DrawCamera3D();
    }

#if 1
    RHI::renderer->EndRenderPass(mainCommandList);
#else
    RHI::renderer->EndRenderPass(mainCommandList);

    RHI::renderer->BeginRenderPass(mainCommandList, swapChain, nullptr);
    RHI::renderer->SetPSO(mainCommandList, imagePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, mainRTColorTexture);
    RHI::renderer->Draw(mainCommandList, 3, 0);
    RHI::renderer->EndRenderPass(mainCommandList);
#endif

    // CommandList 에 기록을 마치고 실행
    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
}

const void *RenderBackEnd::ExecuteDrawPic(const void *data) {
    const DrawPicRenderCommand *cmd = reinterpret_cast<const DrawPicRenderCommand *>(data);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *frameThreadData = currentFrameData->GetThreadData(0);

    guiMesh.DrawPic(frameThreadData, cmd->x, cmd->y, cmd->w, cmd->h, cmd->s1, cmd->t1, cmd->s2, cmd->t2, cmd->texture, cmd->color);

    return (const void *)(cmd + 1);
}

const void *RenderBackEnd::ExecuteScreenshot(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::ExecuteScreenshot", 10);

    const ScreenShotRenderCommand *cmd = reinterpret_cast<const ScreenShotRenderCommand *>(data);

    // 캡쳐 영역 Rect
    BE1::Rect captureRect(cmd->x, cmd->y, cmd->width, cmd->height);

    // SwapChain 백버퍼를 캡쳐해서 저장할 빈 이미지 (메모리) 를 생성한다.
    BE1::Image screenImage;
    screenImage.Create2D(captureRect.w, captureRect.h, 1, BE1::Image::Format::B8G8R8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *frameThreadData = currentFrameData->GetThreadData(0);

    RHI::CommandList *commandList = frameThreadData->AllocGraphicsCommandList();
    commandList->Reset(true);

    // 백버퍼의 내용을 (필요하다면 지정된 포맷으로 컨버팅하여) screenImage 에 저장한다.
    RHI::renderer->ReadPixels(commandList, currentContext->GetSwapChain(), captureRect.x, captureRect.y, captureRect.w, captureRect.h, BE1::Image::Format::B8G8R8, screenImage.GetPixels());

    // 이제 이미지 파일로 저장한다.
    BE1::Str filename = cmd->filename;
    filename.DefaultFileExtension(".png");
    screenImage.Write(filename);

    return (const void *)(cmd + 1);
}

const void *RenderBackEnd::ExecuteSwapBuffers(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::ExecuteSwapBuffers", 10);

    const SwapBuffersRenderCommand *cmd = reinterpret_cast<const SwapBuffersRenderCommand *>(data);

    // 이번 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    currentFrameData->EndFrame();

    // 백버퍼를 전면버퍼와 교환한다.
    currentContext->GetSwapChain()->SwapBuffers(false);

    guiMesh.Clear();

    frameCount++;

    currentContext->currentFrameIndex = frameCount % COUNT_OF(currentContext->frames);

    return (const void *)(cmd + 1);
}

void RenderBackEnd::DrawCamera3D() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::DrawCamera3D", 9);

    constexpr uint32_t MaxActualDrawSurfs = 65536;
    const DrawSurf **actualDrawSurfs = (const DrawSurf **)_alloca(sizeof(DrawSurf *) * MaxActualDrawSurfs);
    uint32_t actualDrawSurfIndex = 0;
    uint32_t drawSurfIndex = 0;

    while (drawSurfIndex < currentVisCamera->numDrawSurfs) {
        const DrawSurf *drawSurf = currentVisCamera->drawSurfs[drawSurfIndex];

        if (!drawSurf->subMesh->vertexBuffer) {
            drawSurf->subMesh->UploadStaticDataToGPU();
        }

#ifdef USE_RENDER_INSTANCED
        if (BE1::HasFlag(drawSurf->flags, DrawSurf::Flag::UseInstancing)) {
            uint32_t instanceStartIndex = drawSurfIndex;

            while (drawSurfIndex < currentVisCamera->numDrawSurfs &&
                currentVisCamera->drawSurfs[drawSurfIndex]->subMesh == drawSurf->subMesh &&
                currentVisCamera->drawSurfs[drawSurfIndex]->texture == drawSurf->texture) {
                drawSurfIndex++;
            }

            uint32_t instanceCount = drawSurfIndex - instanceStartIndex;
            if (instanceCount > 1) {
                // 인스턴스 개수가 2 개 이상이어야 인스턴스드 렌더링을 수행한다.
                DrawInstancedSurface(&currentVisCamera->drawSurfs[instanceStartIndex], instanceCount);
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

void RenderBackEnd::DrawCamera2D() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::DrawCamera2D", 9);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(0);

    // GuiMesh 의 다이나믹 인덱스 버퍼를 업로드한다.
    guiMesh.CacheIndexes(currentFrameThreadData);

    for (int surfaceIndex = 0; surfaceIndex < guiMesh.NumSurfaces(); surfaceIndex++) {
        const GuiMesh::Surface *guiSurf = guiMesh.GetSurface(surfaceIndex);

        DrawGuiSurface(mainCommandList, guiSurf);
    }
}

void RenderBackEnd::DrawSurfaces(const DrawSurf **drawSurfs, uint32_t numDrawSurfs) {
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

void RenderBackEnd::DrawInstancedSurface(const DrawSurf **drawSurfs, uint32_t instanceCount) {
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

void RenderBackEnd::DrawSurfacesWithoutTask(const DrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::DrawSurfacesWithoutTask", 10);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(0);

    for (int drawSurfIndex = 0; drawSurfIndex < numDrawSurfs; ++drawSurfIndex) {
        const DrawSurf *drawSurf = drawSurfs[drawSurfIndex];

        DrawSurface(mainCommandList, drawSurf);
    }
}

#ifdef USE_RENDER_TASK
void RenderBackEnd::DrawSurfacesByTask(RenderBackEnd::DrawObjectTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::DrawSurfacesByTask", 10);

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

void RenderBackEnd::DrawSurfacesByTaskFunction(void *data) {
    RenderBackEnd::DrawObjectTaskDesc *taskDesc = reinterpret_cast<RenderBackEnd::DrawObjectTaskDesc *>(data);
    renderSystem->GetBackEnd()->DrawSurfacesByTask(taskDesc);
}

// drawSurfs 를 numTasks 만큼 task 로 나눠서 그린다.
void RenderBackEnd::DrawSurfacesWithTask(const DrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::DrawSurfacesWithTask", 10);

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
        BE1::Engine::taskManager->AddTask(RenderBackEnd::DrawSurfacesByTaskFunction, &currentThreadDesc, drawGroupId, false);

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

void RenderBackEnd::DrawSurface(RHI::CommandList *commandList, const DrawSurf *drawSurf) {
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

    RHI::renderer->SetPSO(commandList, currentContext->singlePSO);
    RHI::renderer->SetTexture(commandList, 0, false, drawSurf->texture->GetRHITexture());
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, drawSurf->subMesh->numIndexes, 0, 0);
}

void RenderBackEnd::DrawInstancedSurface(RHI::CommandList *commandList, const DrawSurf **instanceSurfs, int instanceCount) {
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

    RHI::renderer->SetPSO(commandList, currentContext->instancingPSO);
    RHI::renderer->SetTexture(commandList, 0, false, instanceSurfs[0]->texture->GetRHITexture());
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexedInstanced(commandList, instanceSurfs[0]->subMesh->numIndexes, instanceCount, 0, 0, 0);
}

void RenderBackEnd::DrawGuiSurface(RHI::CommandList *commandList, const GuiMesh::Surface *guiSurf) {
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();

    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(UnlitConstantData));
    if (!constantBuffer) {
        return;
    }

    UnlitConstantData *constantDataPtr = reinterpret_cast<UnlitConstantData *>(constantBuffer->writePtr);

    // 카메라의 View-Projection 행렬을 기록
    constantDataPtr->modelViewProjMatrix = currentVisCamera->viewProjMatrix;

    RHI::renderer->SetVertexBuffer(commandList, 0, guiSurf->vertexBuffer);
    RHI::renderer->SetIndexBuffer(commandList, guiSurf->indexBuffer);

    RHI::renderer->SetPSO(commandList, currentContext->singlePSO);
    RHI::renderer->SetTexture(commandList, 0, false, guiSurf->texture->GetRHITexture());
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, guiSurf->numIndexes, 0, 0);
}
