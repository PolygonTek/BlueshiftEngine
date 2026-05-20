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

class RenderBackend;

/*
---------------------------------------------------------------------------------------------------

    RenderContext

    하나의 윈도우 (SwapChain) 에 대응되는 렌더링 객체

    - 스왑 체인
    - 화면 렌더 타겟 텍스쳐
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

public:
    RenderContext() = default;

    void                            Init(void *windowHandle);
    void                            Shutdown();

    void                            BeginFrame();
    void                            EndFrame();

    RHI::SwapChain *                GetSwapChain() const { return swapChain; }

    GuiMesh &                       GetGuiMesh() { return guiMesh; }

    uint32_t                        GetWidth() const { return swapChain->GetWidth(); }
    uint32_t                        GetHeight() const { return swapChain->GetHeight(); }

    void                            OnResize(int width, int height);

    void                            RequestScreenshot(const char *filename);

    RHI::Format                     GetMainRTColorFormat() const { return mainRTColorFormat; }
    RHI::Format                     GetMainRTDepthFormat() const { return mainRTDepthFormat; }
    uint32_t                        GetMainRTSampleCount() const { return mainRTSampleCount; }

    const RenderFrameData *         GetCurrentFrontendFrameData() const { return &frames[currentFrontendFrameIndex]; }
    RenderFrameData *               GetCurrentFrontendFrameData() { return &frames[currentFrontendFrameIndex]; }

                                    // 모든 렌더링 명령이 수행이 완료될 때까지 대기한다.
    void                            WaitAllFrameFences();

                                    // 컨텍스트의 GUI 그리기 함수들
    void                            SetColor(const BE1::Color4 &color);
    void                            SetFont(std::shared_ptr<Font> font);
    void                            SetTextScale(float scale);
    void                            SetTextLineSpacing(float lineSpacing);
    void                            SetTextShadowColor(const BE1::Color4 &shadowColor);
    void                            SetTextShadowOffset(float shadowOffsetX, float shadowOffsetY);
    void                            DrawPic(float x, float y, float w, float h, const std::shared_ptr<Texture> &texture);
    void                            DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const std::shared_ptr<Texture> &texture);
    void                            DrawBar(float x, float y, float w, float h);
    void                            DrawRect(float x, float y, float w, float h);
    void                            DrawText(const BE1::Rect &rect, float x, float y, const char *text, DrawTextFlag flags);
    void                            DrawString(float x, float y, const BE1::Str &string, DrawTextFlag flags);

    static RenderContext *          activeContext;

private:
    struct PendingScreenshot {
        char                        filename[BE1::MaxAbsolutePath] = {};
    };

    void                            CreateMainRenderTextures(uint32_t width, uint32_t height);
    void                            DestroyMainRenderTextures();
    void                            InitPSO();
    void                            InitFullScreenTrianglePSO();
    void                            FlushPendingScreenshots(RenderFrameData *frameData);

    RHI::SwapChain *                swapChain = nullptr;
    RHI::Texture *                  mainRTColorMSAATexture = nullptr;
    RHI::Texture *                  mainRTColorTexture = nullptr;
    RHI::Texture *                  mainRTDepthTexture = nullptr;
    RHI::PipelineState *            imagePSO = nullptr;
    RHI::PipelineState *            unlitPSO = nullptr;
    RHI::PipelineState *            unlitAlphaBlendPSO = nullptr;
    RHI::PipelineState *            unlitInstancedPSO = nullptr;
    RHI::PipelineState *            unlitInstancedAlphaBlendPSO = nullptr;

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
    RHI::Format                     mainRTDepthFormat = RHI::Format::D24_UNORM_S8_UINT;
    uint32_t                        mainRTSampleCount = 1;

    GuiMesh                         guiMesh;            ///< System GUI mesh

    BE1::Color4                     currentColor = BE1::Color4(1, 1, 1, 1);
    BE1::Color4                     currentTextShadowColor = BE1::Color4(0, 0, 0, 0);
    float                           currentTextScale = 1.0f;
    float                           currentTextLineSpacing = 2;
    BE1::Vec2                       currentTextShadowOffset = BE1::Vec2::zero;
    std::shared_ptr<Font>           currentFont;
};
