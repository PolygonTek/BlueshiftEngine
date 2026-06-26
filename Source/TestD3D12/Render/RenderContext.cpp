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
#include "RenderInternal.h"
#include "ShaderTemplate.h"
#include "ShaderPipelineCache.h"

RenderContext *         RenderContext::activeContext = nullptr;

static constexpr int    GpuStatUpdateSampleCount = 16;

void RenderContext::Init(void *windowHandle, const BE1::Color4 &baseClearColor) {
    this->windowHandle = windowHandle;

    // 프레임 별로 사용할 프레임 데이터들을 초기화한다.
    // 프레임 데이터: 임시 메모리, 커맨드 리스트 풀, 디스크립터 힙, 다이나믹 버퍼와 그 디스크립터 풀
    for (int32_t frameIndex = 0; frameIndex < NumFrames; ++frameIndex) {
        frames[frameIndex].Init(frameIndex, NumFrames);
    }

    currentFrontendFrameIndex = 0;

    // 윈도우 크기 얻기
    // TODO: 윈도우 크기를 얻는 부분은 플랫폼 별로 분리하자.
    RECT rc;
    HWND hwnd = reinterpret_cast<HWND>(windowHandle);
    GetClientRect(hwnd, &rc);
    uint32_t backBufferWidth = rc.right;
    uint32_t backBufferHeight = rc.bottom;

    // 스왑 체인 (백버퍼) 생성
    RHI::Format swapChainFormat = RHI::Format::R8G8B8A8_UNORM;
    swapChain = RHI::renderer->CreateSwapChain(hwnd, backBufferWidth, backBufferHeight, swapChainFormat);

#ifdef ENABLE_IMGUI
    // ImGui 오버레이 객체 생성
    imguiOverlay = RHI::renderer->CreateImGuiOverlay(windowHandle, swapChain);
#endif

    mainRTColorFormat = RHI::Format::R16G16B16A16_FLOAT;
    this->baseClearColor = !RHI::Renderer::IsSRGBFormat(mainRTColorFormat) ? baseClearColor.SRGBToLinear() : baseClearColor;

    InitPSOs();

    textureStreamingFeedback.Init();

    guiMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame2D);
    guiMesh.SetClipRect(BE1::Rect(0, 0, backBufferWidth, backBufferHeight));
}

void RenderContext::Shutdown() {
    // GPU 명령들이 완료될 때까지 기다린다.
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);
    RHI::renderer->Finish(RHI::CommandQueueType::Compute);

#ifdef ENABLE_IMGUI
    for (RenderFrameData &frameData : frames) {
        frameData.ClearPendingImGuiOverlay();
    }

    // ImGui 오버레이 객체 파괴
    RHI::renderer->DestroyImGuiOverlay(imguiOverlay);
    imguiOverlay = nullptr;
#endif

    textureStreamingFeedback.Shutdown();

    ReleasePSOs();

    for (RenderFrameData &frameData : frames) {
        frameData.Shutdown();
    }

    RHI::renderer->DestroySwapChain(swapChain);
    swapChain = nullptr;
    windowHandle = nullptr;
}

void RenderContext::OnResize(int width, int height) {
    // 모든 프레임에 대해서 렌더 스레드가 렌더링을 완료할 때까지 기다린다.
    if (renderSystem->IsUsingRenderThread()) {
        for (RenderFrameData &frameData : frames) {
            renderSystem->WaitRenderCompleted(&frameData);
        }
    }

    // GPU 명령들이 완료될 때까지 기다린다.
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);
    RHI::renderer->Finish(RHI::CommandQueueType::Compute);

    // 스왑 체인의 크기 조정
    swapChain->Resize(width, height);

    guiMesh.SetClipRect(BE1::Rect(0, 0, width, height));
}

void RenderContext::GetMainRenderDest(RHI::RenderDest &renderDest) const {
    renderDest.renderTargetCount = 1;
    renderDest.depthStencilFormat = GetMainRTDepthFormat();
    renderDest.renderTargetFormats[0] = GetMainRTColorFormat();
    renderDest.sampleCount = GetMainRTSampleCount();
}

void RenderContext::GetSwapChainRenderDest(RHI::RenderDest &renderDest) const {
    renderDest = RHI::RenderDest();
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = swapChain->GetRTVFormat();
    renderDest.sampleCount = 1;
}

void RenderContext::InitPSOs() {
    RHI::RenderDest renderDest;
    GetSwapChainRenderDest(renderDest);

    ShaderPipelineDesc pipelineDesc;
    pipelineDesc.renderDest = renderDest;
    pipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Never;
    pipelineDesc.blendStateType = RHI::BlendStateType::Disabled;
    imagePSO = shaderPipelineCache.GetGraphicsPipeline(shaderTemplateManager.fullscreenImageShaderTemplate, ShaderPermutationKey(), pipelineDesc, false);
}

void RenderContext::ReleasePSOs() {
    imagePSO.reset();
}

void RenderContext::WaitAllFrameFences() {
    for (const RenderFrameData &frameData : frames) {
        frameData.WaitFrameFence();
    }
}

void RenderContext::UpdateGpuFrameTime(float frameMsec) {
    statGpuElapsedMsec += frameMsec;
    statGpuSampleCount++;

    if (statGpuSampleCount >= GpuStatUpdateSampleCount) {
        statGpuAverageMsec.store(statGpuElapsedMsec / statGpuSampleCount);

        statGpuElapsedMsec = 0.0f;
        statGpuSampleCount = 0;
    }
}

void RenderContext::SetRenderStats(uint32_t renderObjectCount, uint32_t visibleObjectCount, uint32_t drawCount, uint64_t drawTriangleCount, uint32_t instanceTableAllocCount, uint32_t instanceTableUploadBytes) {
    statRenderObjectCount.store(renderObjectCount);
    statVisibleObjectCount.store(visibleObjectCount);
    statDrawCount.store(drawCount);
    statDrawTriangleCount.store(drawTriangleCount);
    statInstanceTableAllocCount.store(instanceTableAllocCount);
    statInstanceTableUploadBytes.store(instanceTableUploadBytes);
}

void RenderContext::RequestScreenshot(const char *filename) {
    if (!filename || !filename[0]) {
        return;
    }

    for (const PendingScreenshot &pendingScreenshot : pendingScreenshots) {
        if (!BE1::Str::Cmp(pendingScreenshot.filename, filename)) {
            return;
        }
    }

    PendingScreenshot &pendingScreenshot = pendingScreenshots.Alloc();
    BE1::Str::Copynz(pendingScreenshot.filename, filename, COUNT_OF(pendingScreenshot.filename));
}

void RenderContext::FlushPendingScreenshots(RenderFrameData *frameData) {
    for (const PendingScreenshot &pendingScreenshot : pendingScreenshots) {
        frameData->CmdScreenshot(0, 0, swapChain->GetWidth(), swapChain->GetHeight(), pendingScreenshot.filename);
    }

    pendingScreenshots.SetCount(0, false);
}

void RenderContext::EnsureTextureStreamingFeedbackCapacity(uint32_t requiredCapacity) {
    if (!textureStreamingFeedback.NeedsGrowCapacity(requiredCapacity)) {
        return;
    }

    if (renderSystem->IsUsingRenderThread()) {
        for (RenderFrameData &frameData : frames) {
            renderSystem->WaitRenderCompleted(&frameData);
        }
    }

    WaitAllFrameFences();

    textureStreamingFeedback.EnsureCapacity(requiredCapacity);
}

void RenderContext::BeginFrame(float frameTime, float elapsedTime, float elapsedPrevTime, bool statEnabled) {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::BeginFrame");

    assert(BE1::Engine::IsInMainThread());

    if (RenderContext::activeContext) {
        BE_ERRLOG("RenderContext::BeginFrame: Make sure to call EndFrame() before calling BeginFrame again\n");
        return;
    }

    currentFrontendFrameIndex = frameCount % COUNT_OF(frames);
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    if (renderSystem->IsUsingRenderThread()) {
        // 현재 프론트엔드 프레임의 렌더 스레드 작업이 끝날 때까지 기다린다.
        renderSystem->WaitRenderCompleted(frameData);
    }

    frameData->BeginFrame();
    frameData->SetFrameTiming(frameTime, elapsedTime, elapsedPrevTime, frameCount);
    frameData->SetStatEnabled(statEnabled);
    frameData->CmdBeginContext(this);

    RenderContext::activeContext = this;
}

void RenderContext::EndFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::EndFrame");

    assert(BE1::Engine::IsInMainThread());

    RenderContext::activeContext = nullptr;

    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    FlushPendingScreenshots(frameData);

#ifdef USE_MIPMAP_STREAMING
    EnsureTextureStreamingFeedbackCapacity(materialManager.GetStreamingMaterialCount());
#endif

    frameData->CmdSwapBuffers();
    frameData->EndFrame();

    frameCount++;

    renderSystem->SubmitFrame(this, frameData);
}

bool RenderContext::BeginImGui() {
#ifdef ENABLE_IMGUI
    return imguiOverlay && imguiOverlay->BeginFrameCapture();
#else
    return false;
#endif
}

void RenderContext::EndImGui() {
#ifdef ENABLE_IMGUI
    if (!imguiOverlay) {
        return;
    }
    void *imguiDrawData = imguiOverlay->EndFrameCapture();
    if (imguiDrawData) {
        // RenderBackend::ExecuteSwapBuffers 에서 렌더링하기 위해 캡쳐 데이터를 세팅한다.
        GetCurrentFrontendFrameData()->SetPendingImGuiOverlay(imguiOverlay, imguiDrawData);
    }
#endif
}

bool RenderContext::HandleImGuiWndProc(void *hwnd, uint32_t message, uintptr_t wParam, intptr_t lParam) {
#ifdef ENABLE_IMGUI
    if (imguiOverlay && hwnd == windowHandle) {
        return imguiOverlay->HandleWndProc(hwnd, message, wParam, lParam);
    }
#endif
    return false;
}

bool RenderContext::ImGuiWantsCaptureMouse() const {
#ifdef ENABLE_IMGUI
    return imguiOverlay && imguiOverlay->WantsCaptureMouse();
#else
    return false;
#endif
}

bool RenderContext::ImGuiWantsCaptureKeyboard() const {
#ifdef ENABLE_IMGUI
    return imguiOverlay && imguiOverlay->WantsCaptureKeyboard();
#else
    return false;
#endif
}

void RenderContext::DrawPic(float x, float y, float w, float h, const SharedPtr<Material> &material) {
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    frameData->RetainMaterial(material);
    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, currentColor.ToUInt32(), material ? material.get() : nullptr);
}

void RenderContext::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const SharedPtr<Material> &material) {
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    frameData->RetainMaterial(material);
    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, s1, t1, s2, t2, currentColor.ToUInt32(), material ? material.get() : nullptr);
}

void RenderContext::DrawBar(float x, float y, float w, float h) {
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, currentColor.ToUInt32(), materialManager.whiteUnlitMaterial.get());
}

void RenderContext::DrawRect(float x, float y, float w, float h) {
    if (w > 1) {
        DrawBar(x, y, w, 1);
        if (h > 1) {
            DrawBar(x, y + h - 1, w, 1);
        }
    }

    if (h > 2) {
        DrawBar(x, y + 1, 1, h - 2);
        if (w > 2) {
            DrawBar(x + w - 1, y + 1, 1, h - 2);
        }
    }
}

void RenderContext::SetColor(const BE1::Color4 &color) {
    currentColor = color;
}

void RenderContext::SetFont(SharedPtr<Font> font) {
    if (!font) {
        currentFont = fontManager.defaultFont;
        return;
    }
    currentFont = font;
}

void RenderContext::SetTextScale(float scale) {
    currentTextScale = scale;
}

void RenderContext::SetTextLineSpacing(float lineSpacing) {
    currentTextLineSpacing = lineSpacing;
}

void RenderContext::SetTextShadowColor(const BE1::Color4 &shadowColor) {
    currentTextShadowColor = shadowColor;
}

void RenderContext::SetTextShadowOffset(float shadowOffsetX, float shadowOffsetY) {
    currentTextShadowOffset.x = shadowOffsetX;
    currentTextShadowOffset.y = shadowOffsetY;
}

void RenderContext::DrawText(const BE1::Rect &rect, float x, float y, const char *text, DrawTextFlag flags) {
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    // 프레임 메모리에 텍스트 내용을 복사
    int size = BE1::Str::Length(text) + 1;
    char *frameTextPtr = (char *)frameData->MemAlloc(size);
    BE1::Str::Copynz(frameTextPtr, text, size);

    frameData->RetainFont(currentFont);

    guiMesh.DrawText2D(frameData->GetThreadData(0), rect, x, y, currentTextLineSpacing, currentTextScale, currentColor.ToUInt32(), currentTextShadowColor.ToUInt32(), currentTextShadowOffset, currentFont.get(), frameTextPtr, flags);
}

void RenderContext::DrawString(float x, float y, const BE1::Str &string, DrawTextFlag flags) {
    BE1::Rect textRect;
    textRect.x = 0;
    textRect.y = 0;
    textRect.w = GetWidth();
    textRect.h = GetHeight();

    DrawText(textRect, x, y, string, flags);
}
