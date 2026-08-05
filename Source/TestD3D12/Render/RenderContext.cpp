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
#include "Render/Backend/RenderBackend.h"
#include "RenderCvars.h"
#include "RenderInternal.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Font/FontManager.h"

RenderContext *         RenderContext::activeContext = nullptr;

static uint32_t GetMainRTSampleCountFromCVar() {
    const int sampleCount = r_msaa.GetInteger();

    if (sampleCount >= 8) {
        return 8;
    }
    if (sampleCount >= 4) {
        return 4;
    }
    if (sampleCount >= 2) {
        return 2;
    }
    return 1;
}

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
    mainRTSampleCount = GetMainRTSampleCountFromCVar();
    this->baseClearColor = !RHI::Renderer::IsSRGBFormat(mainRTColorFormat) ? baseClearColor.SRGBToLinear() : baseClearColor;

    if (!CreateFrameCompositionTexture()) {
        BE_WARNLOG("RenderContext::Init: failed to create frame composition texture\n");
    }

    if (!EnsureCSMDepthTexture()) {
        BE_WARNLOG("RenderContext::Init: failed to create CSM depth texture\n");
    }

    InitPipelines();

    if (!textureStreamingFeedback.Init()) {
        BE_WARNLOG("RenderContext::Init: failed to initialize texture streaming feedback\n");
    }

    guiClipRectStack.Reserve(16);
    guiMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame2D);
    ResetGuiClipRect(backBufferWidth, backBufferHeight);
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

    ReleasePipelines();

    DestroyCSMDepthTexture();
    DestroyFrameCompositionTexture();

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
        renderSystem->WaitContextRenderCompleted(this);
    }

    // GPU 명령들이 완료될 때까지 기다린다.
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);
    RHI::renderer->Finish(RHI::CommandQueueType::Compute);

    // 스왑 체인의 크기 조정
    swapChain->Resize(width, height);

    if (!CreateFrameCompositionTexture()) {
        BE_WARNLOG("RenderContext::OnResize: failed to create frame composition texture\n");
    }

    ResetGuiClipRect(width, height);
}

bool RenderContext::CreateFrameCompositionTexture() {
    DestroyFrameCompositionTexture();

    const uint32_t width = swapChain->GetWidth();
    const uint32_t height = swapChain->GetHeight();
    if (width == 0 || height == 0) {
        return false;
    }

    BE1::Image::Format imageFormat = BE1::Image::Format::Unknown;
    bool isSRGB = false;
    if (!RHI::Renderer::RHIFormatToImageFormat(swapChain->GetRTVFormat(), &imageFormat, &isSRGB)) {
        return false;
    }

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->InitFromMemory(width, height, 1, 1, 1, imageFormat, isSRGB ? BE1::Image::GammaSpace::sRGB : BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    const RHI::ClearValue clearValue = RHI::ClearValue::Color(baseClearColor[0], baseClearColor[1], baseClearColor[2], baseClearColor[3]);
    frameCompositionTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource, image, RHI::TextureMipInitMode::FromImage, clearValue, 1, RHI::GPUResourceState::ShaderResource);
    if (!frameCompositionTexture) {
        return false;
    }

    RHI::renderer->SetName(frameCompositionTexture, "RenderContext::frameCompositionTexture");
    frameCompositionTextureNeedsClear = true;
    return true;
}

void RenderContext::DestroyFrameCompositionTexture() {
    if (frameCompositionTexture) {
        RHI::renderer->DestroyTexture(frameCompositionTexture, true);
        frameCompositionTexture = nullptr;
    }
    frameCompositionTextureNeedsClear = false;
}

bool RenderContext::EnsureCSMDepthTexture() {
    const uint32_t resolution = static_cast<uint32_t>(BE1::Max(r_csmResolution.GetInteger(), 1));
    if (csmDepthTexture && csmDepthTexture->GetWidth() == resolution && csmDepthTexture->GetHeight() == resolution) {
        return true;
    }

    return CreateCSMDepthTexture(resolution);
}

bool RenderContext::CreateCSMDepthTexture(uint32_t resolution) {
    assert(resolution > 0);

    SharedPtr<BE1::Image> depthImage = MakeShared<BE1::Image>();
    depthImage->InitFromMemory(resolution, resolution, 1, CSMCascadeCount, 1, BE1::Image::Format::D24S8, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    const RHI::TextureFlag textureFlags = RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless;
    const RHI::ClearValue optimizedClearValue = RHI::ClearValue::DepthStencil(1.0f, 0);
    RHI::Texture *newTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2DArray, textureFlags, depthImage, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
    if (!newTexture) {
        return false;
    }

    RHI::ViewID newSRV = RHI::renderer->CreateView(newTexture, RHI::ViewType::SRV, RHI::Format::Auto, 0, CSMCascadeCount, 0, 1);
    RHI::ViewID newSliceDSVs[CSMCascadeCount] = {};
    bool viewsCreated = newSRV.IsValid();
    for (uint32_t cascadeIndex = 0; cascadeIndex < CSMCascadeCount && viewsCreated; ++cascadeIndex) {
        newSliceDSVs[cascadeIndex] = RHI::renderer->CreateView(newTexture, RHI::ViewType::DSV, RHI::Format::Auto, cascadeIndex, 1, 0, 1);
        viewsCreated = newSliceDSVs[cascadeIndex].IsValid();
    }

    if (!viewsCreated) {
        RHI::renderer->DestroyTexture(newTexture);
        return false;
    }

    RHI::renderer->SetName(newTexture, "RenderContext::csmDepthTexture");

    // 기존 텍스쳐를 파괴하고 새 텍스쳐와 뷰를 설정한다.
    DestroyCSMDepthTexture();

    csmDepthTexture = newTexture;
    csmDepthSRV = newSRV;
    for (uint32_t cascadeIndex = 0; cascadeIndex < CSMCascadeCount; ++cascadeIndex) {
        csmDepthSliceDSVs[cascadeIndex] = newSliceDSVs[cascadeIndex];
    }
    return true;
}

void RenderContext::DestroyCSMDepthTexture() {
    if (csmDepthTexture) {
        RHI::renderer->DestroyTexture(csmDepthTexture);
        csmDepthTexture = nullptr;
    }

    csmDepthSRV = RHI::ViewID::InvalidID;
    for (RHI::ViewID &sliceDSV : csmDepthSliceDSVs) {
        sliceDSV = RHI::ViewID::InvalidID;
    }
}

void RenderContext::GetMainRenderDest(RHI::RenderDest &renderDest, bool outputNormal, bool outputVelocity) const {
    renderDest = RHI::RenderDest();
    renderDest.renderTargetCount = 1;
    renderDest.depthStencilFormat = GetMainRTDepthFormat();
    renderDest.renderTargetFormats[0] = GetMainRTColorFormat();
    if (outputNormal) {
        renderDest.renderTargetFormats[renderDest.renderTargetCount++] = RHI::Format::R16G16_FLOAT;
    }
    if (outputVelocity) {
        renderDest.renderTargetFormats[renderDest.renderTargetCount++] = RHI::Format::R16G16_FLOAT;
    }
    renderDest.sampleCount = GetMainRTSampleCount();
}

void RenderContext::GetSwapChainRenderDest(RHI::RenderDest &renderDest) const {
    renderDest = RHI::RenderDest();
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = swapChain->GetRTVFormat();
    renderDest.sampleCount = 1;
}

void RenderContext::InitPipelines() {
    RHI::RenderDest renderDest;
    GetSwapChainRenderDest(renderDest);

    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.renderDest = renderDest;
    graphicsPipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Disabled;
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::Disabled;

    imagePipeline = shaderPipelineCache.GetGraphicsPipeline(shaderTemplateManager.fullscreenImageShaderTemplate, ShaderPermutationKey(), graphicsPipelineDesc);
}

void RenderContext::ReleasePipelines() {
    imagePipeline.reset();
}

void RenderContext::WaitAllFrameFences() {
    for (const RenderFrameData &frameData : frames) {
        frameData.WaitFrameFence();
    }
}

void RenderContext::UpdateGpuTagFrameTimes(const float tagFrameMsec[static_cast<int>(GpuTiming::Tag::Count)]) {
    static constexpr int GpuStatUpdateSampleCount = 16;

    for (int tagIndex = 0; tagIndex < static_cast<int>(GpuTiming::Tag::Count); ++tagIndex) {
        statGpuTagElapsedMsec[tagIndex] += tagFrameMsec[tagIndex];
        statGpuTagSampleCount[tagIndex]++;

        // GpuStatUpdateSampleCount 만큼 frame time 을 누적해서 평균을 계산한다.
        if (statGpuTagSampleCount[tagIndex] >= GpuStatUpdateSampleCount) {
            statGpuTagAverageMsec[tagIndex].store(statGpuTagElapsedMsec[tagIndex] / statGpuTagSampleCount[tagIndex]);

            statGpuTagElapsedMsec[tagIndex] = 0.0f;
            statGpuTagSampleCount[tagIndex] = 0;
        }
    }
}

float RenderContext::GetStatGpuTagAverageMsec(GpuTiming::Tag tag) const {
    const int tagIndex = static_cast<int>(tag);
    if (tagIndex < 0 || tagIndex >= static_cast<int>(GpuTiming::Tag::Count)) {
        return 0.0f;
    }
    return statGpuTagAverageMsec[tagIndex].load();
}

RenderContext::Stats RenderContext::GetStats() const {
    Stats snapshot;

    while (1) {
        const uint32_t sequence = statSequence.load();
        if (sequence & 1) {
            continue;
        }

        snapshot.visibleCameraCount = stats.visibleCameraCount.load();
        snapshot.visibleObjectCount = stats.visibleObjectCount.load();
        snapshot.visibleLightCount = stats.visibleLightCount.load();
        snapshot.visiblePointLightCount = stats.visiblePointLightCount.load();
        snapshot.visibleSpotLightCount = stats.visibleSpotLightCount.load();
        snapshot.visibleBoxDirLightCount = stats.visibleBoxDirLightCount.load();
        snapshot.drawCount = stats.drawCount.load();
        snapshot.drawTriangleCount = stats.drawTriangleCount.load();
        snapshot.instanceTableAllocCount = stats.instanceTableAllocCount.load();
        snapshot.instanceTableUploadBytes = stats.instanceTableUploadBytes.load();
        snapshot.lightCullingZBinCount = stats.lightCullingZBinCount.load();
        snapshot.lightCullingWordCount = stats.lightCullingWordCount.load();
        snapshot.lightCullingThreadGroupCount = stats.lightCullingThreadGroupCount.load();

        if (sequence == statSequence.load()) {
            return snapshot;
        }
    }
}

void RenderContext::SetStats(const Stats &snapshot) {
    // 홀수 sequence 는 백엔드가 통계 스냅숏을 갱신 중임을 나타낸다.
    statSequence.fetch_add(1);

    stats.visibleCameraCount.store(snapshot.visibleCameraCount);
    stats.visibleObjectCount.store(snapshot.visibleObjectCount);
    stats.visibleLightCount.store(snapshot.visibleLightCount);
    stats.visiblePointLightCount.store(snapshot.visiblePointLightCount);
    stats.visibleSpotLightCount.store(snapshot.visibleSpotLightCount);
    stats.visibleBoxDirLightCount.store(snapshot.visibleBoxDirLightCount);
    stats.drawCount.store(snapshot.drawCount);
    stats.drawTriangleCount.store(snapshot.drawTriangleCount);
    stats.instanceTableAllocCount.store(snapshot.instanceTableAllocCount);
    stats.instanceTableUploadBytes.store(snapshot.instanceTableUploadBytes);
    stats.lightCullingZBinCount.store(snapshot.lightCullingZBinCount);
    stats.lightCullingWordCount.store(snapshot.lightCullingWordCount);
    stats.lightCullingThreadGroupCount.store(snapshot.lightCullingThreadGroupCount);

    statSequence.fetch_add(1);
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
        renderSystem->WaitContextRenderCompleted(this);
    }

    WaitAllFrameFences();

    if (!textureStreamingFeedback.EnsureBufferCapacity(requiredCapacity)) {
        BE_WARNLOG("RenderContext::EnsureTextureStreamingFeedbackCapacity: failed to ensure texture streaming feedback capacity\n");
    }
}

bool RenderContext::BeginFrame(const FrameOptions &options) {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::BeginFrame");

    assert(BE1::Engine::IsInMainThread());

    if (RenderContext::activeContext) {
        // BeginFrame/EndFrame 은 중첩할 수 없다.
        BE_ERRLOG("RenderContext::BeginFrame: a frame is already active\n");
        return false;
    }

    if (options.swapChainOutput) {
        // RenderCapture 는 활성 프레임 안에서 실행할 수 없으므로 필요한 radiance 캡처를 먼저 처리한다.
        renderSystem->PrepareProbeBakes(this);
        assert(!RenderContext::activeContext);
    }

    if (options.swapChainOutput) {
        ResetGuiClipRect(GetWidth(), GetHeight());
    }

    currentFrontendFrameIndex = frameCount % COUNT_OF(frames);
    RenderFrameData *frameData = GetCurrentFrontendFrameData();
    mainRTSampleCount = GetMainRTSampleCountFromCVar();

    if (renderSystem->IsUsingRenderThread()) {
        // 현재 프런트엔드 프레임의 렌더 스레드 작업이 끝날 때까지 기다린다.
        renderSystem->WaitFrameRenderCompleted(frameData);
    }

    frameData->BeginFrame();

    // BeginFrame 에서 frame fence 완료를 기다렸으므로, 렌더 커맨드에 밀어 넣었던 render work 들을 안전하게 완료한다.
    renderSystem->CompleteRenderWorks(frameData);

    // CSM depth texture 가 없거나 해상도가 바뀌었으면 새로 생성한다.
    if (!EnsureCSMDepthTexture()) {
        BE_WARNLOG("RenderContext::BeginFrame: failed to create CSM depth texture\n");
    }

    if (options.swapChainOutput && this == renderSystem->mainRenderContext) {
        // 대기 중인 render work 들을 렌더 커맨드에 (CmdBeginContext 전에) 밀어 넣는다.
        renderSystem->AttachPendingRenderWorks(frameData);
    }

    frameData->SetFrameTiming(options.frameTime, options.elapsedTime, options.elapsedPrevTime, frameCount);
    frameData->SetStatEnabled(options.statEnabled);
    frameData->CmdBeginContext(this, options.gpuTiming, options.textureStreamingFeedback, options.swapChainOutput);

    activeFrameOptions = options;

    RenderContext::activeContext = this;
    return true;
}

void RenderContext::EndFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::EndFrame");

    assert(BE1::Engine::IsInMainThread());

    if (RenderContext::activeContext != this) {
        BE_ERRLOG("RenderContext::EndFrame: this context has no active frame\n");
        return;
    }

    RenderFrameData *frameData = GetCurrentFrontendFrameData();
    if (activeFrameOptions.swapChainOutput) {
        assert(guiClipRectStack.Count() == 1);
    } else {
        frameData->ClearPendingImGuiOverlay();
    }

#ifdef USE_MIPMAP_STREAMING
    if (activeFrameOptions.textureStreamingFeedback) {
        EnsureTextureStreamingFeedbackCapacity(materialManager.GetStreamingMaterialCount());
    }
#endif

    frameData->CmdFinalizeFrame(activeFrameOptions.swapChainOutput);

    if (activeFrameOptions.swapChainOutput) {
        FlushPendingScreenshots(frameData);

        frameData->CmdSwapBuffers();
    }

    frameData->EndFrame();

    RenderContext::activeContext = nullptr;
    activeFrameOptions = {};
    frameCount++;

    renderSystem->SubmitFrame(this, frameData);
}

bool RenderContext::BeginImGui() {
#ifdef ENABLE_IMGUI
    assert(BE1::Engine::IsInMainThread());

    return imguiOverlay && imguiOverlay->BeginFrameCapture();
#else
    return false;
#endif
}

void RenderContext::EndImGui() {
#ifdef ENABLE_IMGUI
    assert(BE1::Engine::IsInMainThread());

    if (!imguiOverlay) {
        return;
    }
    void *imguiDrawData = imguiOverlay->EndFrameCapture();
    if (imguiDrawData) {
        // RenderBackend::ExecuteFinalizeFrame 에서 렌더링하기 위해 캡쳐 데이터를 세팅한다.
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

void RenderContext::DrawSlicedPic(float x, float y, float w, float h, float imageWidth, float imageHeight, const GuiMesh::SliceBorder &border, const SharedPtr<Material> &material) {
    DrawStretchSlicedPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, imageWidth, imageHeight, border, material);
}

void RenderContext::DrawStretchSlicedPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, float imageWidth, float imageHeight, const GuiMesh::SliceBorder &border, const SharedPtr<Material> &material) {
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    frameData->RetainMaterial(material);
    guiMesh.DrawSlicedPic(frameData->GetThreadData(0), x, y, w, h, s1, t1, s2, t2, imageWidth, imageHeight, border, currentColor.ToUInt32(), material ? material.get() : nullptr);
}

void RenderContext::ResetGuiClipRect(int width, int height) {
    guiClipRectStack.SetCount(1, false);
    guiClipRectStack[0].Set(0, 0, width, height);
    guiMesh.SetClipRect(guiClipRectStack[0]);
}

void RenderContext::PushClipRect(const BE1::Rect &clipRect) {
    assert(!guiClipRectStack.IsEmpty());

    guiClipRectStack.Append(guiClipRectStack.Last().Intersect(clipRect));
    guiMesh.SetClipRect(guiClipRectStack.Last());
}

void RenderContext::PopClipRect() {
    assert(guiClipRectStack.Count() > 1);
    if (guiClipRectStack.Count() <= 1) {
        return;
    }

    guiClipRectStack.SetCount(guiClipRectStack.Count() - 1, false);
    guiMesh.SetClipRect(guiClipRectStack.Last());
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
