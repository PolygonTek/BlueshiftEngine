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
#include "RenderContext.h"
#include "RenderInternal.h"

void RenderBackend::Init() {
#ifdef USE_RENDER_TASK
    drawGroupId = BE1::Engine::taskManager->CreateGroupId();
#endif
}

void RenderBackend::Shutdown() {
}

void RenderBackend::Execute(const void *data) {
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
        default:
            BE_ERRLOG("RenderBackend::Execute: invalid render command ID (%i)\n", cmdId);
            return;
        }
    }
}

const void *RenderBackend::ExecuteBeginContext(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteBeginContext", 8);

    const BeginContextRenderCommand *cmd = reinterpret_cast<const BeginContextRenderCommand *>(data);

    currentContext = cmd->renderContext;

    // 프레임 데이터를 초기화하고, 이전 프레임에 대한 펜스를 기다린다.
    //RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    //currentFrameData->BeginFrame();

    return (const void *)(cmd + 1);
}

const void *RenderBackend::ExecuteDrawCamera(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteDrawCamera", 8);

    const DrawCameraRenderCommand *cmd = reinterpret_cast<const DrawCameraRenderCommand *>(data);

    currentVisCamera = cmd->visCamera;

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *frameThreadData = currentFrameData->GetThreadData(0);

    // 커맨드 리스트 풀에서 새로운 커맨드 리스트를 얻어온다.
    mainCommandList = frameThreadData->BeginCommandList(RHI::CommandQueueType::Graphics);

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

const void *RenderBackend::ExecuteScreenshot(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteScreenshot", 10);

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

const void *RenderBackend::ExecuteSwapBuffers(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteSwapBuffers", 10);

    const SwapBuffersRenderCommand *cmd = reinterpret_cast<const SwapBuffersRenderCommand *>(data);

    // 이번 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    currentFrameData->SetFenceValue(RHI::renderer->SignalFence(RHI::CommandQueueType::Graphics));

    // 백버퍼를 전면버퍼와 교환한다.
    currentContext->GetSwapChain()->SwapBuffers(false);

    frameCount++;

    currentContext->currentFrameIndex = frameCount % COUNT_OF(currentContext->frames);

    return (const void *)(cmd + 1);
}

void RenderBackend::DrawCamera3D() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera3D", 9);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(0);

    DrawAllSurfaces(currentVisCamera->drawSurfs, currentVisCamera->numDrawSurfs);
}

void RenderBackend::DrawCamera2D() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera2D", 9);

    RenderFrameData *currentFrameData = currentContext->GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(0);

    for (int drawSurfIndex = 0; drawSurfIndex < currentVisCamera->numDrawSurfs; ++drawSurfIndex) {
        const DrawSurf *drawSurf = currentVisCamera->drawSurfs[drawSurfIndex];

        DrawGuiSurface(mainCommandList, drawSurf);
    }
}
