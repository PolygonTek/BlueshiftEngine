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

// 하나의 윈도우 (SwapChain) 와 연결된 렌더링 컨텍스트
class RenderContext {
    friend class RenderBackend;

public:
    RenderContext() = default;

    void                                Init(void *windowHandle, bool useRenderThread);
    void                                Shutdown();

    void                                BeginFrame();
    void                                EndFrame();

    RHI::SwapChain *                    GetSwapChain() const { return swapChain; }

    GuiMesh &                           GetGuiMesh() { return guiMesh; }

    uint32_t                            GetWidth() const { return swapChain->GetWidth(); }
    uint32_t                            GetHeight() const { return swapChain->GetHeight(); }

    void                                OnResize(int width, int height);

    BE1::Image::Format                  GetMainRTColorFormat() const { return mainRTColorFormat; }
    BE1::Image::Format                  GetMainRTDepthFormat() const { return mainRTDepthFormat; }
    uint32_t                            GetMainRTSampleCount() const { return mainRTSampleCount; }

    const RenderFrameData *             GetCurrentFrontendFrameData() const { return &frames[currentFrontendFrameIndex]; }
    RenderFrameData *                   GetCurrentFrontendFrameData() { return &frames[currentFrontendFrameIndex]; }

    const RenderFrameData *             GetCurrentBackendFrameData() const { return &frames[currentBackendFrameIndex]; }
    RenderFrameData *                   GetCurrentBackendFrameData() { return &frames[currentBackendFrameIndex]; }

                                        // 모든 렌더링 명령이 수행이 완료될 때 까지 대기한다.
    void                                WaitAllFrameFences();

                                        // 컨텍스트의 GUI 그리기 함수들
    void                                SetColor(const BE1::Color4 &color);
    void                                SetFont(Font *font);
    void                                SetTextScale(float scale);
    void                                SetTextLineSpacing(float lineSpacing);
    void                                SetTextShadowColor(const BE1::Color4 &shadowColor);
    void                                SetTextShadowOffset(float shadowOffsetX, float shadowOffsetY);
    void                                DrawPic(float x, float y, float w, float h, const Texture *texture);
    void                                DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Texture *texture);
    void                                DrawBar(float x, float y, float w, float h);
    void                                DrawRect(float x, float y, float w, float h);
    void                                DrawText(const BE1::Rect &rect, float x, float y, const char *text, DrawTextFlag flags);
    void                                DrawString(float x, float y, const BE1::Str &string, DrawTextFlag flags);

                                        // 렌더 스레드 사용 여부 리턴
    bool                                IsUsingRenderThread() const { return !!renderThread; }

    void                                WaitRenderCompleted(const RenderFrameData *frameData);
    void                                MarkUpdateCompleted(RenderFrameData *frameData);

    static RenderContext *              activeContext;

private:
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
    RHI::PipelineState *                unlitPSO = nullptr;
    RHI::PipelineState *                unlitAlphaBlendPSO = nullptr;
    RHI::PipelineState *                unlitInstancedPSO = nullptr;
    RHI::PipelineState *                unlitInstancedAlphaBlendPSO = nullptr;

#ifdef USE_RENDER_FRAME_RESOURCES
    static constexpr int                NumFrameResources = 2;
#else
    static constexpr int                NumFrameResources = 1;
#endif

    RenderFrameData                     frames[NumFrameResources] = {};
    uint32_t                            currentFrontendFrameIndex = 0;
    uint32_t                            currentBackendFrameIndex = 0;
    uint32_t                            frameCount = 0;

    BE1::PlatformSRWLock *              smpLock = nullptr;
    BE1::PlatformCondition *            renderCompletedCondition = nullptr;
    BE1::PlatformCondition *            updateCompletedCondition = nullptr;
    BE1::PlatformThread *               renderThread = nullptr;
    bool                                isStoppingRenderThread = false;

    BE1::Image::Format                  mainRTColorFormat = BE1::Image::Format::R8G8B8A8;
    BE1::Image::Format                  mainRTDepthFormat = BE1::Image::Format::D32_FLOAT;
    uint32_t                            mainRTSampleCount = 1;

    GuiMesh                             guiMesh;            ///< System GUI mesh

    BE1::Color4                         currentColor = BE1::Color4(1, 1, 1, 1);
    BE1::Color4                         currentTextShadowColor = BE1::Color4(0, 0, 0, 0);
    float                               currentTextScale = 1.0f;
    float                               currentTextLineSpacing = 2;
    BE1::Vec2                           currentTextShadowOffset = BE1::Vec2::zero;
    Font *                              currentFont = nullptr;
};
