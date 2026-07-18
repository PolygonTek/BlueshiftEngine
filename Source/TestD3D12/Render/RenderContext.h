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

#include "RHI/RHI.h"
#include "GpuTiming.h"
#include "RenderFrameData.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/Texture/TextureStreamingFeedback.h"
#include "Render/Mesh/GuiMesh.h"

class RenderBackend;

/*
---------------------------------------------------------------------------------------------------

    RenderContext

    하나의 윈도우 (SwapChain) 에 대응되는 렌더링 객체

    - 스왑 체인
    - 프레임 데이터 배열
        - 요소 하나는 한 프레임 렌더링에 필요한 데이터를 관리한다.
        - 프론트엔드 (메인 스레드) 는 프레임 데이터를 작성하고, RenderSystem 에 제출한다.
        - 백엔드 (렌더 스레드) 는 RenderSystem 이 관리하며, 제출된 프레임 데이터를 읽어서 처리한다.
        - 동시에 같은 프레임 데이터를 읽거나 쓰지 않도록 하기 위한 동기화도 RenderSystem 이 담당한다.

---------------------------------------------------------------------------------------------------
*/

class RenderContext {
    friend class RenderSystem;
    friend class RenderBackend;
    friend class GpuTiming;
    friend class RenderFrameData;

    template <typename T>
    using PlainStatValue = T;

    template <typename T>
    using AtomicStatValue = std::atomic<T>;

    template <template <typename> class Value>
    struct StatsT {
        Value<uint32_t>             visibleCameraCount = 0;
        Value<uint32_t>             visibleObjectCount = 0;
        Value<uint32_t>             visibleLightCount = 0;
        Value<uint32_t>             visiblePointLightCount = 0;
        Value<uint32_t>             visibleSpotLightCount = 0;
        Value<uint32_t>             visibleBoxDirLightCount = 0;
        Value<uint32_t>             drawCount = 0;
        Value<uint64_t>             drawTriangleCount = 0;
        Value<uint32_t>             instanceTableAllocCount = 0;
        Value<uint32_t>             instanceTableUploadBytes = 0;
        Value<uint32_t>             lightCullingZBinCount = 0;
        Value<uint32_t>             lightCullingWordCount = 0;
        Value<uint64_t>             lightCullingThreadGroupCount = 0;
    };

public:
    using Stats = StatsT<PlainStatValue>;

    RenderContext() = default;

    void                            Init(void *windowHandle, const BE1::Color4 &baseClearColor = BE1::Color4::black);
    void                            Shutdown();

    void                            BeginFrame(float frameTime, float elapsedTime, float elapsedPrevTime, bool statEnabled);
    void                            EndFrame();
    template <typename BuildFunc>
    void                            RenderImGui(const BuildFunc &buildFunc);

    void *                          GetWindowHandle() const { return windowHandle; }
    RHI::SwapChain *                GetSwapChain() const { return swapChain; }

    GuiMesh &                       GetGuiMesh() { return guiMesh; }

    uint32_t                        GetWidth() const { return swapChain->GetWidth(); }
    uint32_t                        GetHeight() const { return swapChain->GetHeight(); }
    const BE1::Color4 &             GetBaseClearColor() const { return baseClearColor; }

    float                           GetStatGpuAverageMsec() const { return GetStatGpuTagAverageMsec(GpuTiming::Tag::GpuFrame); }
    float                           GetStatGpuTagAverageMsec(GpuTiming::Tag tag) const;
    Stats                           GetStats() const;

    void                            OnResize(int width, int height);

    void                            RequestScreenshot(const char *filename);

    bool                            HandleImGuiWndProc(void *hwnd, uint32_t message, uintptr_t wParam, intptr_t lParam);
    bool                            ImGuiWantsCaptureMouse() const;
    bool                            ImGuiWantsCaptureKeyboard() const;

    RHI::Format                     GetMainRTColorFormat() const { return mainRTColorFormat; }
    RHI::Format                     GetMainRTDepthFormat() const { return mainRTDepthFormat; }
    uint32_t                        GetMainRTSampleCount() const { return mainRTSampleCount; }
    void                            GetMainRenderDest(RHI::RenderDest &renderDest, bool outputNormal, bool outputVelocity) const;
    void                            GetSwapChainRenderDest(RHI::RenderDest &renderDest) const;

    const RenderFrameData *         GetCurrentFrontendFrameData() const { return &frames[currentFrontendFrameIndex]; }
    RenderFrameData *               GetCurrentFrontendFrameData() { return &frames[currentFrontendFrameIndex]; }

                                    // 모든 렌더링 명령이 수행이 완료될 때까지 대기한다.
    void                            WaitAllFrameFences();

                                    // 컨텍스트의 GUI 그리기 함수들
    void                            SetColor(const BE1::Color4 &color);
    void                            SetFont(SharedPtr<Font> font);
    void                            SetTextScale(float scale);
    void                            SetTextLineSpacing(float lineSpacing);
    void                            SetTextShadowColor(const BE1::Color4 &shadowColor);
    void                            SetTextShadowOffset(float shadowOffsetX, float shadowOffsetY);
    const BE1::Rect &               GetClipRect() const { assert(!guiClipRectStack.IsEmpty()); return guiClipRectStack.Last(); }
    void                            PushClipRect(const BE1::Rect &clipRect);
    void                            PopClipRect();
    void                            DrawPic(float x, float y, float w, float h, const SharedPtr<Material> &material);
    void                            DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const SharedPtr<Material> &material);
    void                            DrawSlicedPic(float x, float y, float w, float h, float imageWidth, float imageHeight, const GuiMesh::SliceBorder &border, const SharedPtr<Material> &material);
    void                            DrawStretchSlicedPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, float imageWidth, float imageHeight, const GuiMesh::SliceBorder &border, const SharedPtr<Material> &material);
    void                            DrawBar(float x, float y, float w, float h);
    void                            DrawRect(float x, float y, float w, float h);
    void                            DrawText(const BE1::Rect &rect, float x, float y, const char *text, DrawTextFlag flags);
    void                            DrawString(float x, float y, const BE1::Str &string, DrawTextFlag flags);

    static RenderContext *          activeContext;

private:
    using AtomicStats = StatsT<AtomicStatValue>;

    struct PendingScreenshot {
        char                        filename[BE1::MaxAbsolutePath] = {};
    };

    void                            InitPipelines();
    void                            ReleasePipelines();
    bool                            BeginImGui();
    void                            EndImGui();
    void                            FlushPendingScreenshots(RenderFrameData *frameData);
    void                            EnsureTextureStreamingFeedbackCapacity(uint32_t requiredCapacity);
    void                            ResetGuiClipRect(int width, int height);
    void                            UpdateGpuTagFrameTimes(const float tagFrameMsec[static_cast<int>(GpuTiming::Tag::Count)]);
    void                            SetStats(const Stats &stats);

    void *                          windowHandle = nullptr;
    RHI::SwapChain *                swapChain = nullptr;
    RHI::ImGuiOverlay *             imguiOverlay = nullptr;
    BE1::Color4                     baseClearColor = BE1::Color4::black;

#ifdef USE_RENDER_FRAME_BUFFERING
    static constexpr int            NumFrames = 2;
#else
    static constexpr int            NumFrames = 1;
#endif

    RenderFrameData                 frames[NumFrames] = {};
    uint32_t                        currentFrontendFrameIndex = 0;
    uint32_t                        frameCount = 0;
    BE1::Array<PendingScreenshot>   pendingScreenshots;

    RHI::Format                     mainRTColorFormat = RHI::Format::R8G8B8A8_UNORM;
    RHI::Format                     mainRTDepthFormat = RHI::Format::D32_FLOAT;
    uint32_t                        mainRTSampleCount = 1;

    SharedPtr<ShaderPipeline>       imagePipeline;

    TextureStreamingFeedback        textureStreamingFeedback;

    GuiMesh                         guiMesh;            ///< System GUI mesh
    BE1::Array<BE1::Rect>           guiClipRectStack;
    BE1::Color4                     currentColor = BE1::Color4(1, 1, 1, 1);
    BE1::Color4                     currentTextShadowColor = BE1::Color4(0, 0, 0, 0);
    float                           currentTextScale = 1.0f;
    float                           currentTextLineSpacing = 2;
    BE1::Vec2                       currentTextShadowOffset = BE1::Vec2::zero;
    SharedPtr<Font>                 currentFont;

    uint32_t                        gpuTimingSampleFrameCount = 0;
    float                           statGpuTagElapsedMsec[static_cast<int>(GpuTiming::Tag::Count)] = {};
    int                             statGpuTagSampleCount[static_cast<int>(GpuTiming::Tag::Count)] = {};
    std::atomic<float>              statGpuTagAverageMsec[static_cast<int>(GpuTiming::Tag::Count)] = {};

    AtomicStats                     stats;
    std::atomic<uint32_t>           statSequence = 0;
};

template <typename BuildFunc>
void RenderContext::RenderImGui(const BuildFunc &buildFunc) {
#ifdef ENABLE_IMGUI
    if (!BeginImGui()) {
        return;
    }

    buildFunc();

    EndImGui();
#else
    (void)buildFunc;
#endif
}
