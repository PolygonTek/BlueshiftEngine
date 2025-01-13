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

#pragma once

#include "RHI.h"
#include "RenderContext.h"
#include "RenderFrameData.h"

enum class FrameSyncState : uint8_t {
    WaitingForUpdateCompleted,          // (렌더 스레드가 렌더링이 완료되어) 메인 스레드의 다음 업데이트 작업이 완료되기를 기다리는 상태
    WaitingForRenderCompleted           // (메인 스레드가 업데이트가 완료되어) 렌더 스레드의 다음 렌더링 작업이 완료되기를 기다리는 상태
};

class RenderContext {
public:
    RenderContext() = default;

    void                                Init(HWND hwnd);
    void                                Shutdown();

    void                                BeginFrame();
    void                                EndFrame();
    void                                RenderFrame();

    void                                SwapBuffers(bool vsync);
    RHI::SwapChain *                    GetSwapChain() const { return swapChain; }

    uint32_t                            GetWidth() const { return viewportRect.w; }
    uint32_t                            GetHeight() const { return viewportRect.h; }

    void                                OnResize(int width, int height);

    BE1::Image::Format::Enum            GetMainRTColorFormat() const { return mainRTColorFormat; }
    BE1::Image::Format::Enum            GetMainRTDepthFormat() const { return mainRTDepthFormat; }
    uint32_t                            GetMainRTSampleCount() const { return mainRTSampleCount; }

    const RenderFrameData *             GetCurrentFrameData() const { return &frameData[currentFrameIndex]; }
    RenderFrameData *                   GetCurrentFrameData() { return &frameData[currentFrameIndex]; }

    void                                WaitAllFrameFences();

    void                                WaitRenderCompleted();
    void                                MarkUpdateCompleted();

private:
    struct DrawObjectTaskDesc {
        RenderContext *                 renderContext = nullptr;
        int                             threadIndex = -1;
        int                             visObjectStartIndex = -1;
        int                             visObjectEndIndex = -1;
        RHI::CommandList *              activeCommandList = nullptr;
    };

    void                                CreateMainRenderTextures(uint32_t width, uint32_t height);
    void                                DestroyMainRenderTextures();
    void                                InitFullScreenTrianglePSO();

    static unsigned int                 RenderThreadProc(void *param);

    void                                InitRenderThread();
    void                                ShutdownRenderThread();

    void                                DrawVisObjects(int threadIndex, RHI::CommandList *commandList, int startIndex, int endIndex);
    void                                DrawVisObjectsWithoutTask();
#ifdef USE_RENDER_TASK
    void                                DrawVisObjectsWithTask(int numTasks);
    void                                DrawVisObjectsByTask(RenderContext::DrawObjectTaskDesc *taskDesc);
#endif
    static void                         DrawVisObjectsByTaskFunction(void *data);

    BE1::Image::Format::Enum            mainRTColorFormat = BE1::Image::Format::RGBA_8_8_8_8;
    BE1::Image::Format::Enum            mainRTDepthFormat = BE1::Image::Format::Depth_32F;
    uint32_t                            mainRTSampleCount = 1;

    BE1::Rect                           viewportRect;
    BE1::Rect                           scissorRect;
    RHI::SwapChain *                    swapChain = nullptr;
    RHI::Texture *                      mainRTColorMSAATexture = nullptr;
    RHI::Texture *                      mainRTColorTexture = nullptr;
    RHI::Texture *                      mainRTDepthTexture = nullptr;
    RHI::PipelineState *                imagePSO = nullptr;

    RHI::CommandList *                  mainCommandList = nullptr;
    UINT                                frameCount = 0;
    RenderFrameData                     frameData[NumFrameResources] = {};
    UINT                                currentFrameIndex = 0;

    BE1::PlatformSRWLock *              smpLock = nullptr;
    BE1::PlatformCondition *            renderCompletedCondition = nullptr;
    BE1::PlatformCondition *            updateCompletedCondition = nullptr;
    BE1::PlatformThread *               renderThread = nullptr;
    bool                                isStoppingRenderThread = false;
    int                                 renderFrameIndex = 1;
    FrameSyncState                      frameSyncState = FrameSyncState::WaitingForUpdateCompleted;

#ifdef USE_RENDER_TASK
    BE1::TaskManager                    renderTaskManager = BE1::TaskManager(MaxRenderTasks);
#endif

    BE1::Array<DrawObjectTaskDesc>      objectDrawingTaskDescs;
};
