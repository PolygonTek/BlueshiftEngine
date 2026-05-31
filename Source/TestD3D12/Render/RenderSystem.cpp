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
#include "RenderSystem.h"
#include "RenderContext.h"
#include "RenderBackend.h"
#include "D3D12Renderer/D3D12Renderer.h"
#include "Sampler.h"
#include "Texture.h"
#include "Font.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "Anim.h"

RenderSystem *          renderSystem = nullptr;

void RenderSystem::Init(void *mainWindowHandle) {
    BE1::cmdSystem.AddCommand("screenshot", Cmd_ScreenShot);

    RHI::renderer = new D3D12Renderer;
    RHI::renderer->Init(mainWindowHandle);

    samplerManager.Init();

    textureManager.Init();

    fontManager.Init();

    skeletonManager.Init();

    animManager.Init();

    meshManager.Init();

    backEnd = new RenderBackend;
    backEnd->Init();

#ifdef USE_RENDER_THREAD
    InitRenderThread();
#endif
}

void RenderSystem::Shutdown() {
#ifdef USE_RENDER_THREAD
    ShutdownRenderThread();
#endif

    BE1::cmdSystem.RemoveCommand("screenshot");

    backEnd->Shutdown();
    SAFE_DELETE(backEnd);

    meshManager.Shutdown();

    animManager.Shutdown();

    skeletonManager.Shutdown();

    fontManager.Shutdown();

    textureManager.Shutdown();

    samplerManager.Shutdown();

    RHI::renderer->Shutdown();
    SAFE_DELETE(RHI::renderer);
}

void RenderSystem::UpdateFrame() {
    RHI::renderer->GetMemoryUsage(videoMemoryUsage);
    assert(videoMemoryUsage.budgetBytes > 0);

    for (RenderContext *renderContext : renderContexts) {
        renderContext->textureStreamingFeedback.ConsumeCompletedReadbacks([](const uint32_t *feedbackData, uint32_t capacity) {
            textureManager.ApplyStreamingFeedback(feedbackData, capacity);
        });
    }
}

void RenderSystem::SubmitFrame(RenderContext *renderContext, RenderFrameData *frameData) {
    assert(renderContext);
    assert(frameData);

    if (IsUsingRenderThread()) {
        BE1::ScopedWriteLock lock(frameSyncLock);

        // 주어진 프레임에 대한 (메인 스레드의) 업데이트가 완료되어,
        // (렌더 스레드의) 렌더링이 완료되기를 기다리는 상태로 변경.
        assert(frameData->frameSyncState == FrameSyncState::RenderCompleted);
        frameData->frameSyncState = FrameSyncState::UpdateCompleted;

        SubmittedFrame submittedFrame;
        submittedFrame.renderContext = renderContext;
        submittedFrame.frameData = frameData;
        submittedFrames.Append(submittedFrame);

        // 렌더 스레드에게 업데이트가 완료되었다고 신호를 보낸다.
        BE1::PlatformCondition::Signal(updateCompletedCondition);
        return;
    }

    // 이전에 사용이 완료된 리소스들을 해제한다.
    RHI::renderer->FreeRetiredResources(false);

    // 리소스 요청 처리
    RHI::renderer->ProcessResourceRequests(frameData->GetThreadData(0));

    // 현재 프레임 데이터로 렌더링 백엔드를 실행한다.
    backEnd->Execute(frameData);

    // 이번 프레임에 Release/Delete 를 요청한 리소스들을 retiredResources 목록에 추가한다.
    RHI::renderer->DrainReleaseRequestsToRetireList();
}

void RenderSystem::InitRenderThread() {
    frameSyncLock = BE1::PlatformSRWLock::Create();
    renderCompletedCondition = BE1::PlatformCondition::Create();
    updateCompletedCondition = BE1::PlatformCondition::Create();
    submittedFrames.SetCount(0, false);
    submittedFrameReadIndex = 0;
    isStoppingRenderThread = false;

    // 렌더 스레드를 시작한다.
    renderThread = BE1::PlatformThread::Start(RenderSystem::RenderThreadProc, this);
}

void RenderSystem::ShutdownRenderThread() {
    {
        BE1::ScopedWriteLock lock(frameSyncLock);
        isStoppingRenderThread = true;

        // 렌더 스레드가 프레임 제출을 기다리는 중이라면 종료할 수 있도록 깨운다.
        BE1::PlatformCondition::Signal(updateCompletedCondition);
    }

    BE1::PlatformThread::Join(renderThread);
    renderThread = nullptr;

    BE1::PlatformCondition::Destroy(renderCompletedCondition);
    BE1::PlatformCondition::Destroy(updateCompletedCondition);
    BE1::PlatformSRWLock::Destroy(frameSyncLock);

    renderCompletedCondition = nullptr;
    updateCompletedCondition = nullptr;
    frameSyncLock = nullptr;
    submittedFrames.SetCount(0, false);
    submittedFrameReadIndex = 0;
    isStoppingRenderThread = false;
}

bool RenderSystem::WaitRenderCompleted(const RenderFrameData *frameData) {
    assert(BE1::Engine::IsInMainThread());

    if (!IsUsingRenderThread()) {
        return true;
    }

    {
        BE1::ScopedReadLock lock(frameSyncLock);

        // 주어진 프레임에 대한 (렌더 스레드의) 렌더링이 완료되기를 기다린다.
        BE1::PlatformCondition::Wait(renderCompletedCondition, frameSyncLock, false, [frameData] {
            return frameData->frameSyncState == FrameSyncState::RenderCompleted;
        });
    }
    return true;
}

void RenderSystem::MarkRenderCompleted(RenderFrameData *frameData) {
    BE1::ScopedWriteLock lock(frameSyncLock);

    // 주어진 프레임에 대한 (렌더 스레드의) 렌더링이 완료되어,
    // (메인 스레드의) 업데이트가 완료되기를 기다리는 상태로 변경.
    assert(frameData->frameSyncState == FrameSyncState::UpdateCompleted);
    frameData->frameSyncState = FrameSyncState::RenderCompleted;

    // 메인 스레드에게 렌더링이 완료되었다고 신호를 보낸다.
    BE1::PlatformCondition::Signal(renderCompletedCondition);
}

bool RenderSystem::WaitSubmittedFrame(SubmittedFrame &submittedFrame) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitSubmittedFrame");

    {
        BE1::ScopedWriteLock lock(frameSyncLock);

        // 제출된 프레임 데이터가 생기거나 렌더 스레드 종료 요청이 들어올 때까지 기다린다.
        BE1::PlatformCondition::Wait(updateCompletedCondition, frameSyncLock, true, [this] {
            return submittedFrameReadIndex < submittedFrames.Count() || isStoppingRenderThread;
        });

        if (isStoppingRenderThread) {
            return false;
        }

        assert(submittedFrameReadIndex < submittedFrames.Count());

        submittedFrame = submittedFrames[submittedFrameReadIndex++];

        if (submittedFrameReadIndex == submittedFrames.Count()) {
            submittedFrames.SetCount(0, false);
            submittedFrameReadIndex = 0;
        }
    }
    return true;
}

unsigned int RenderSystem::RenderThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("RenderSystem::RenderThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    RenderSystem *renderSystem = reinterpret_cast<RenderSystem *>(param);

    while (1) {
        PROFILER_CPU_SCOPED_EVENT("RenderThreadProcLoop");

        SubmittedFrame submittedFrame;

        // 제출된 프레임 데이터가 생길 때까지 기다린다.
        if (!renderSystem->WaitSubmittedFrame(submittedFrame)) {
            break;
        }

        // 이전에 사용이 완료된 리소스들을 해제한다.
        RHI::renderer->FreeRetiredResources(false);

        RenderFrameData *backendFrameData = submittedFrame.frameData;

        // 리소스 요청 처리
        RHI::renderer->ProcessResourceRequests(backendFrameData->GetThreadData(0));

        // 현재 백엔드 프레임 데이터로 렌더링 백엔드를 실행한다.
        renderSystem->backEnd->Execute(backendFrameData);

        // 이번 프레임에 Release/Delete 를 요청한 리소스들을 retiredResources 목록에 추가한다.
        RHI::renderer->DrainReleaseRequestsToRetireList();

        // 메인 스레드에게 렌더링이 완료되었음을 알린다.
        renderSystem->MarkRenderCompleted(backendFrameData);
    }

    return 0;
}

RenderContext *RenderSystem::CreateRenderContext(void *windowHandle, bool isMain) {
    RenderContext *renderContext = new RenderContext;
    renderContexts.Append(renderContext);

    renderContext->Init(windowHandle);

    if (isMain) {
        assert(!mainRenderContext);
        mainRenderContext = renderContext;
    }

    return renderContext;
}

void RenderSystem::DestroyRenderContext(RenderContext *renderContext) {
    if (IsUsingRenderThread()) {
        for (RenderFrameData &frameData : renderContext->frames) {
            WaitRenderCompleted(&frameData);
        }
    }

    if (renderContext == mainRenderContext) {
        mainRenderContext = nullptr;
    }

    renderContext->Shutdown();

    backEnd->DiscardGpuFrameTimings(renderContext);

    renderContexts.Remove(renderContext);
    delete renderContext;
}

void RenderSystem::RequestScreenshot(RenderContext *renderContext, const char *filename) {
    if (!renderContext) {
        return;
    }

    renderContext->RequestScreenshot(filename);
}

// mainRenderContext 에 대한 스크린샷을 찍는 Cmd 함수
void RenderSystem::Cmd_ScreenShot(const BE1::CmdArgs &args) {
    char path[1024];

    BE1::Str documentDir = BE1::fileSystem.GetUserDocumentDir();
    RenderContext *renderContext = renderSystem->mainRenderContext;

    if (!renderContext) {
        BE_WARNLOG("screenshot: main render context is not available\n");
        return;
    }

    if (args.Argc() > 1) {
        BE1::Str::snPrintf(path, sizeof(path), "%s/Screenshots/%s", documentDir.c_str(), args.Argv(1));
    } else {
        char filename[16];
        strcpy(filename, "shot000.png");

        int index;
        for (index = 0; index <= 999; index++) {
            filename[4] = '0' + index / 100;
            filename[5] = '0' + (index % 100) / 10;
            filename[6] = '0' + index % 10;
            BE1::Str::snPrintf(path, sizeof(path), "%s/Screenshots/%s", documentDir.c_str(), filename);
            if (!BE1::fileSystem.FileExists(path)) {
                break;
            }
        }

        if (index == 1000) {
            BE_WARNLOG("too many screenshots exist\n");
            return;
        }
    }

    renderSystem->RequestScreenshot(renderContext, path);
}
