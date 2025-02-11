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

#pragma once

#include "RHI.h"
#include "RenderFrameData.h"
#include "GuiMesh.h"

class RenderBackEnd;

// 하나의 윈도우 (SwapChain) 와 연결된 렌더링 컨텍스트
class RenderContext {
    friend class RenderBackEnd;

public:
    RenderContext() = default;

    void                                Init(void *windowHandle, bool useRenderThread);
    void                                Shutdown();

    void                                BeginFrame();
    void                                EndFrame();

    RHI::SwapChain *                    GetSwapChain() const { return swapChain; }

    uint32_t                            GetWidth() const { return swapChain->GetWidth(); }
    uint32_t                            GetHeight() const { return swapChain->GetHeight(); }

    void                                OnResize(int width, int height);

    BE1::Image::Format                  GetMainRTColorFormat() const { return mainRTColorFormat; }
    BE1::Image::Format                  GetMainRTDepthFormat() const { return mainRTDepthFormat; }
    uint32_t                            GetMainRTSampleCount() const { return mainRTSampleCount; }

    const RenderFrameData *             GetCurrentFrameData() const { return &frames[currentFrameIndex]; }
    RenderFrameData *                   GetCurrentFrameData() { return &frames[currentFrameIndex]; }

    void                                WaitAllFrameFences();

    bool                                IsUsingRenderThread() const { return !!renderThread; }

    void                                WaitRenderCompleted();
    void                                MarkUpdateCompleted();

    static RenderContext *              activeContext;

private:
    enum class FrameSyncState : uint8_t {
        WaitingForUpdateCompleted,      // (렌더 스레드가 렌더링이 완료되어) 메인 스레드의 다음 업데이트 작업이 완료되기를 기다리는 상태
        WaitingForRenderCompleted       // (메인 스레드가 업데이트가 완료되어) 렌더 스레드의 다음 렌더링 작업이 완료되기를 기다리는 상태
    };

    void                                CreateMainRenderTextures(uint32_t width, uint32_t height);
    void                                DestroyMainRenderTextures();
    void                                InitPSO();
    void                                InitFullScreenTrianglePSO();

    static unsigned int                 RenderThreadProc(void *param);

    void                                InitRenderThread();
    void                                ShutdownRenderThread();

    RHI::SwapChain *                    swapChain = nullptr;
    RHI::Texture *                      mainRTColorMSAATexture = nullptr;
    RHI::Texture *                      mainRTColorTexture = nullptr;
    RHI::Texture *                      mainRTDepthTexture = nullptr;
    RHI::PipelineState *                imagePSO = nullptr;
    RHI::PipelineState *                singlePSO = nullptr;
    RHI::PipelineState *                instancingPSO = nullptr;

#ifdef USE_RENDER_FRAME_RESOURCES
    static constexpr int                NumFrameResources = 2;
#else
    static constexpr int                NumFrameResources = 1;
#endif

    RenderFrameData                     frames[NumFrameResources] = {};
    uint32_t                            currentFrameIndex = 0;

    BE1::PlatformSRWLock *              smpLock = nullptr;
    BE1::PlatformCondition *            renderCompletedCondition = nullptr;
    BE1::PlatformCondition *            updateCompletedCondition = nullptr;
    BE1::PlatformThread *               renderThread = nullptr;
    bool                                isStoppingRenderThread = false;
    FrameSyncState                      frameSyncState = FrameSyncState::WaitingForUpdateCompleted;

    BE1::Image::Format                  mainRTColorFormat = BE1::Image::Format::R8G8B8A8;
    BE1::Image::Format                  mainRTDepthFormat = BE1::Image::Format::D32_FLOAT;
    uint32_t                            mainRTSampleCount = 1;
};
