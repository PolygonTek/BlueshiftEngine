// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
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

void RenderBackEnd::Init() {
#ifdef USE_TASK_MANAGER
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
        case RenderCommandId::ScreenShot:
            data = ExecuteScreenshot(data);
            continue;
        case RenderCommandId::SwapBuffers:
            data = ExecuteSwapBuffers(data);
            continue;
        case RenderCommandId::End:
            return;
        }
    }
}

const void *RenderBackEnd::ExecuteBeginContext(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::ExecuteBeginContext", 0);

    const BeginContextRenderCommand *cmd = reinterpret_cast<const BeginContextRenderCommand *>(data);

    currentContext = cmd->renderContext;

    return (const void *)(cmd + 1);
}

const void *RenderBackEnd::ExecuteDrawCamera(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::ExecuteDrawCamera", 0);

    const DrawCameraRenderCommand *cmd = reinterpret_cast<const DrawCameraRenderCommand *>(data);

    const VisCamera *visCamera = cmd->visCamera;

    // 프레임 데이터를 초기화하고, 이전 프레임에 대한 펜스를 기다린다.
    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    currentFrameData->BeginFrame();

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    RHI::FrameThreadData *frameThreadData = currentFrameData->GetThreadData(0);
    mainCommandList = frameThreadData->AllocGraphicsCommandList();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    mainCommandList->Reset();

    // 뷰포트 & ScissorRect 의 초기값 설정
    RHI::renderer->SetViewport(mainCommandList, visCamera->decl.renderRect);
    RHI::renderer->SetScissorRect(mainCommandList, visCamera->decl.renderRect);

#if 1
    RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, currentContext->mainRTDepthTexture, BE1::Color4::blue, 1.0f, 0, RHI::ClearFlag::Color | RHI::ClearFlag::Depth);
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

    uint32_t numVisObjects = visCamera->NumVisObjects();
    if (numVisObjects > 0) {
#ifdef USE_TASK_MANAGER
#ifdef USE_RENDEROBJECT_INSTANCING
        uint32_t numDrawCalls = (uint32_t)BE1::Math::Ceil((float)numVisObjects / 1024);
#else
        uint32_t numDrawCalls = numVisObjects;
#endif

        uint32_t numTasks = (uint32_t)BE1::Math::Ceil((float)numDrawCalls / MaxDrawCallsPerTask);
        numTasks = BE1::Min(BE1::Engine::taskManager->NumThreads(), numTasks);
        if (numTasks > 1) {
            DrawVisObjectsWithTask(visCamera, numTasks);
        } else {
            DrawVisObjectsWithoutTask(visCamera);
        }
#else
        DrawVisObjectsWithoutTask(visCamera);
#endif
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
    mainCommandList->CloseAndExecute(RHI::CommandQueueType::Graphics);

    // 이번 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    currentFrameData->EndFrame();

    return (const void *)(cmd + 1);
}

const void *RenderBackEnd::ExecuteScreenshot(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::ExecuteScreenshot", 0);

    const ScreenShotRenderCommand *cmd = reinterpret_cast<const ScreenShotRenderCommand *>(data);

    return (const void *)(cmd + 1);
}

const void *RenderBackEnd::ExecuteSwapBuffers(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::ExecuteSwapBuffers", 0);

    const SwapBuffersRenderCommand *cmd = reinterpret_cast<const SwapBuffersRenderCommand *>(data);

    // 백버퍼를 전면버퍼와 교환한다.
    currentContext->GetSwapChain()->SwapBuffers(false);

    frameCount++;

    // 이번 프레임을 위해 메인 스레드에서 할당했던 메모리를 해제한다.
    currentContext->GetCurrentFrameData()->EndFrameMemAllocs();

    currentContext->currentFrameIndex = frameCount % NumFrameResources;

    return (const void *)(cmd + 1);
}

// visCamera 에 등록된 특정 인덱스 범위의 visObjects 들을 그린다.
void RenderBackEnd::DrawVisObjects(RHI::CommandList *commandList, const VisCamera *visCamera, uint32_t startIndex, uint32_t endIndex) {
    PROFILER_SCOPED_EVENT(commandList, "RenderBackEnd::DrawVisObjects", 1);

    const RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    const VisObject *visObjects = currentFrameData->GetVisObjects();
    uint32_t index = startIndex;

    while (index <= endIndex) {
        const VisObject *currentVisObjectPtr = &visObjects[index];

#ifdef USE_RENDEROBJECT_INSTANCING
        uint32_t instanceCount = BE1::Min(1024u, endIndex - index + 1);
        if (instanceCount > 1) {
            VisObject::DrawInstanced(commandList, visCamera, currentVisObjectPtr, instanceCount);
            index += instanceCount;
        } else {
            VisObject::Draw(commandList, visCamera, &currentVisObjectPtr[0]);
            ++index;
        }
#else
        VisObject::Draw(commandList, visCamera, &currentVisObjectPtr[0]);
        ++index;
#endif
    }
}

// visCamera 에 등록된 전체 visObjects 들을 task 없이 한번에 그린다.
void RenderBackEnd::DrawVisObjectsWithoutTask(const VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::DrawVisObjectsWithoutTask", 2);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(0);

    // Secondary CommandList 를 시작한다.
    RHI::CommandList *commandList = currentFrameThreadData->BeginSecondaryCommandList(mainCommandList);

    // visObjects 들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(commandList, visCamera, visCamera->visObjectStartIndex, visCamera->visObjectEndIndex);

    // Secondary CommandList 를 닫고 메인 CommandList 에 등록한다.
    commandList->CloseAndExecuteSecondary(mainCommandList, currentFrameThreadData);
}

#ifdef USE_TASK_MANAGER
// taskDesc 에 담겨있는 정보를 기반으로 visCamera 에 등록된 visObjects 들을 그린다.
void RenderBackEnd::DrawVisObjectsByTask(RenderBackEnd::DrawObjectTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::DrawVisObjectsByTask", 3);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(taskDesc->threadIndex);

    // Secondary CommandList 를 시작한다.
    RHI::CommandList *commandList = currentFrameThreadData->BeginSecondaryCommandList(mainCommandList);

    // visObjects 들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(commandList, taskDesc->visCamera, taskDesc->visObjectStartIndex, taskDesc->visObjectEndIndex);

    // CommandList 기록을 마친다.
    commandList->Close();

    // 사용 중인 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->activeCommandList = commandList;
}

void RenderBackEnd::DrawVisObjectsByTaskFunction(void *data) {
    RenderBackEnd::DrawObjectTaskDesc *taskDesc = reinterpret_cast<RenderBackEnd::DrawObjectTaskDesc *>(data);
    renderSystem->GetBackEnd()->DrawVisObjectsByTask(taskDesc);
}

// visCamera 에 등록된 visObjects 들을 task 로 나눠서 그린다.
void RenderBackEnd::DrawVisObjectsWithTask(const VisCamera *visCamera, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackEnd::DrawVisObjectsWithTask", 4);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    uint32_t numVisObjects = visCamera->NumVisObjects();
    uint32_t numVisObjectsPerTasks = (uint32_t)BE1::Math::Ceil((float)numVisObjects / numTasks);
    uint32_t nextStartIndex = visCamera->visObjectStartIndex;
    int threadIndex = 0;

    // 태스크 정보 초기화
    objectDrawingTaskDescs.Reserve(numTasks);
    objectDrawingTaskDescs.SetCount(0, false);

    // 최대 쓰레드 개수만큼 task 를 실행한다.
    while (nextStartIndex <= visCamera->visObjectEndIndex) {
        DrawObjectTaskDesc &currentThreadDesc = objectDrawingTaskDescs.Alloc();

        currentThreadDesc.visCamera = visCamera;
        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.visObjectStartIndex = nextStartIndex;
        currentThreadDesc.visObjectEndIndex = BE1::Min(nextStartIndex + numVisObjectsPerTasks - 1, visCamera->visObjectEndIndex);
        BE1::Engine::taskManager->AddTask(RenderBackEnd::DrawVisObjectsByTaskFunction, &currentThreadDesc, drawGroupId, false);

        nextStartIndex = currentThreadDesc.visObjectEndIndex + 1;
    }

    BE1::Engine::taskManager->WaitFinish(drawGroupId, true);

    // Main CommandList 에 모든 태스크의 Secondary CommandList 들을 기록한다.
    int renderTaskCount = objectDrawingTaskDescs.Count();
    for (int threadIndex = 0; threadIndex < renderTaskCount; ++threadIndex) {
        const RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(threadIndex);

        objectDrawingTaskDescs[threadIndex].activeCommandList->ExecuteSecondary(mainCommandList, currentFrameThreadData);
    }
}
#endif // USE_TASK_MANAGER
