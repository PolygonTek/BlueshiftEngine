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

void RenderContext::Init(void *windowHandle, const BE1::Color4 &baseClearColor) {
    this->windowHandle = windowHandle;

    const RenderFrameSettings initialRenderFrameSettings = CaptureRenderFrameSettings();

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

    CreateSwapChain(backBufferWidth, backBufferHeight, initialRenderFrameSettings.hdrOutput);

    UpdateDisplayOutputSettings(initialRenderFrameSettings);

    mainRTColorFormat = RHI::Format::R16G16B16A16_FLOAT;
    mainRTSampleCount = initialRenderFrameSettings.mainRTSampleCount;
    this->baseClearColor = !RHI::Renderer::IsSRGBFormat(mainRTColorFormat) ? baseClearColor.SRGBToLinear() : baseClearColor;

    CreateFrameCompositionTexture();

#ifdef ENABLE_IMGUI
    // ImGui 는 최종 출력 변환 전의 프레임 합성 텍스처 포맷으로 렌더링한다.
    imguiOverlay = RHI::renderer->CreateImGuiOverlay(windowHandle, frameCompositionTexture->GetFormat());
#endif

    if (!EnsureCSMDepthTexture(initialRenderFrameSettings.csmResolution)) {
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

void RenderContext::CreateSwapChain(uint32_t width, uint32_t height, bool hdrOutputRequested) {
    assert(!swapChain);

    HWND hwnd = reinterpret_cast<HWND>(windowHandle);

    const RHI::SwapChain::ColorSpace requestedColorSpace = hdrOutputRequested ? RHI::renderer->GetHDRColorSpace() : RHI::SwapChain::ColorSpace::SRGB;
    swapChain = RHI::renderer->CreateSwapChain(hwnd, width, height, RHI::SwapChain::GetFormatForColorSpace(requestedColorSpace));

    if (requestedColorSpace != RHI::SwapChain::ColorSpace::SRGB) {
        // 실제 HDR 출력과 HDR color space 설정이 지원되는지 체크한다.
        const bool hdrOutputActive = swapChain->IsHDROutputAvailable() && swapChain->SetColorSpace(requestedColorSpace);
        if (hdrOutputActive) {
            BE_LOG("%s output enabled\n", RHI::SwapChain::GetColorSpaceName(requestedColorSpace));
        } else {
            BE_LOG("%s output unavailable; falling back to SDR\n", RHI::SwapChain::GetColorSpaceName(requestedColorSpace));
            RHI::renderer->DestroySwapChain(swapChain);
            swapChain = RHI::renderer->CreateSwapChain(hwnd, width, height, RHI::Format::R8G8B8A8_UNORM);
        }
    }
}

void RenderContext::UpdateDisplayOutputSettings(const RenderFrameSettings &renderFrameSettings) {
    static constexpr float DefaultHDRPeakLuminanceNits = 1000.0f;

    DisplayOutputSettings settings;
    settings.hdrOutputRequested = renderFrameSettings.hdrOutput;
    settings.paperWhiteNits = renderFrameSettings.hdrPaperWhiteNits;

    displayInfo = {};
    displayInfoAvailable = swapChain->GetDisplayInfo(displayInfo);
    if (displayInfoAvailable) {
        settings.hdrDisplayActive = displayInfo.hdrActive; // 현재 디스플레이의 HDR 출력 활성화 여부
        settings.minLuminanceNits = displayInfo.minLuminanceNits; // 디스플레이의 최소 휘도 (니트 단위)
        settings.maxFullFrameLuminanceNits = displayInfo.maxFullFrameLuminanceNits; // 디스플레이의 최대 전체 프레임 휘도 (니트 단위)
    }

    const float peakLuminanceOverrideNits = renderFrameSettings.hdrPeakNits;
    cachedHDRPeakNitsOverride = peakLuminanceOverrideNits;

    if (peakLuminanceOverrideNits > 0.0f) {
        // r_hdrPeakNits 으로 오버라이드된 최대 휘도
        settings.peakLuminanceNits = peakLuminanceOverrideNits;
        settings.maxDisplayLinearValue = settings.peakLuminanceNits / settings.paperWhiteNits;
    } else if (displayInfo.maxLinearOutputValue > 0.0f) {
        // 플랫폼이 기준 흰색에 상대적인 선형 출력 상한을 직접 제공하면 그 값을 우선 사용한다.
        settings.maxDisplayLinearValue = displayInfo.maxLinearOutputValue;
        settings.peakLuminanceNits = displayInfo.maxLuminanceNits > 0.0f ? displayInfo.maxLuminanceNits : settings.paperWhiteNits * settings.maxDisplayLinearValue;
    } else {
        // 디스플레이의 최대 휘도 (디스플레이의 최대 휘도를 알 수 없는 경우 기본값으로 1000 니트 사용)
        settings.peakLuminanceNits = displayInfo.maxLuminanceNits > 0.0f ? displayInfo.maxLuminanceNits : DefaultHDRPeakLuminanceNits;
        settings.maxDisplayLinearValue = settings.peakLuminanceNits / settings.paperWhiteNits;
    }

    settings.maxDisplayLinearValue = BE1::Max(settings.maxDisplayLinearValue, 1.0f);

    displayOutputSettings = settings;
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

    CreateFrameCompositionTexture();

    displayOutputDirty = true;

    ResetGuiClipRect(width, height);
}

void RenderContext::OnDisplayChange() {
    displayOutputDirty = true;
}

void RenderContext::RecreateDisplayOutput(bool hdrOutputRequested, const RenderFrameSettings &renderFrameSettings) {
    assert(!RenderContext::activeContext);

    if (renderSystem->IsUsingRenderThread()) {
        renderSystem->WaitContextRenderCompleted(this);
    }

    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);
    RHI::renderer->Finish(RHI::CommandQueueType::Compute);

    const uint32_t width = swapChain->GetWidth();
    const uint32_t height = swapChain->GetHeight();

#ifdef ENABLE_IMGUI
    for (RenderFrameData &frameData : frames) {
        frameData.ClearPendingImGuiOverlay();
    }
#endif

    ReleasePipelines();
    DestroyFrameCompositionTexture();

    RHI::renderer->DestroySwapChain(swapChain);
    swapChain = nullptr;

    CreateSwapChain(width, height, hdrOutputRequested);
    UpdateDisplayOutputSettings(renderFrameSettings);

    CreateFrameCompositionTexture();

#ifdef ENABLE_IMGUI
    if (imguiOverlay && !imguiOverlay->SetRenderTargetFormat(frameCompositionTexture->GetFormat())) {
        BE_FATALERROR("RenderContext::RecreateDisplayOutput: failed to update the ImGui render target format\n");
    }
#endif

    InitPipelines();
    ResetGuiClipRect(width, height);
}

void RenderContext::UpdateDisplayOutput(const RenderFrameSettings &renderFrameSettings) {
    static constexpr uint32_t DisplayOutputPollFrameInterval = 60;

    const bool hdrOutputRequested = renderFrameSettings.hdrOutput;
    const bool hdrOutputRequestChanged = displayOutputSettings.hdrOutputRequested != hdrOutputRequested;
    const bool hdrOutputSettingsChanged = hdrOutputRequestChanged ||
        displayOutputSettings.paperWhiteNits != renderFrameSettings.hdrPaperWhiteNits ||
        cachedHDRPeakNitsOverride != renderFrameSettings.hdrPeakNits;

    // 디스플레이 HDR 상태 변경 알림을 놓치지 않도록 저빈도로 현재 출력 정보를 다시 확인한다.
    const bool periodicDisplayOutputPoll = frameCount % DisplayOutputPollFrameInterval == 0;

    if (!displayOutputDirty && !hdrOutputSettingsChanged && !periodicDisplayOutputPoll) {
        return;
    }

    if (hdrOutputRequestChanged) {
        RecreateDisplayOutput(hdrOutputRequested, renderFrameSettings);
        displayOutputDirty = false;
        return;
    }

    if (!hdrOutputRequested) {
        UpdateDisplayOutputSettings(renderFrameSettings);
        displayOutputDirty = false;
        return;
    }

    const RHI::SwapChain::ColorSpace hdrColorSpace = RHI::renderer->GetHDRColorSpace();
    const RHI::SwapChain::ColorSpace currentColorSpace = swapChain->GetColorSpace();
    const bool hdrOutputActive = currentColorSpace == hdrColorSpace;
    const bool desiredHDROutput = swapChain->IsHDROutputAvailable();

    if (hdrOutputActive != desiredHDROutput) {
        RecreateDisplayOutput(desiredHDROutput, renderFrameSettings);
    } else {
        UpdateDisplayOutputSettings(renderFrameSettings);
    }

    displayOutputDirty = false;
}

void RenderContext::CreateFrameCompositionTexture() {
    DestroyFrameCompositionTexture();

    const uint32_t width = swapChain->GetWidth();
    const uint32_t height = swapChain->GetHeight();
    assert(width > 0 && height > 0);

    BE1::Image::Format imageFormat = BE1::Image::Format::Unknown;
    bool isSRGB = false;
    RHI::Renderer::RHIFormatToImageFormat(GetFrameCompositionColorFormat(), &imageFormat, &isSRGB);

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->InitFromMemory(width, height, 1, 1, 1, imageFormat, isSRGB ? BE1::Image::GammaSpace::sRGB : BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    const RHI::ClearValue clearValue = RHI::ClearValue::Color(baseClearColor[0], baseClearColor[1], baseClearColor[2], baseClearColor[3]);
    frameCompositionTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource, image, RHI::TextureMipInitMode::FromImage, clearValue, 1, RHI::GPUResourceState::ShaderResource);
    assert(frameCompositionTexture);

    RHI::renderer->SetName(frameCompositionTexture, "RenderContext::frameCompositionTexture");
    frameCompositionTextureNeedsClear = true;
}

RHI::Format RenderContext::GetFrameCompositionColorFormat() const {
    if (RHI::SwapChain::IsHDRColorSpace(swapChain->GetColorSpace())) {
        return RHI::Format::R16G16B16A16_FLOAT;
    }
    return swapChain->GetRTVFormat();
}

void RenderContext::DestroyFrameCompositionTexture() {
    if (frameCompositionTexture) {
        RHI::renderer->DestroyTexture(frameCompositionTexture, true);
        frameCompositionTexture = nullptr;
    }
    frameCompositionTextureNeedsClear = false;
}

bool RenderContext::EnsureCSMDepthTexture(uint32_t resolution) {
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

RHI::Format RenderContext::GetPostProcessColorFormat() const {
    if (RHI::SwapChain::IsHDRColorSpace(swapChain->GetColorSpace())) {
        // HDR 최종 포스트 프로세스는 1.0 을 초과하는 paper-white-relative 선형 값을 보존해야 한다.
        return RHI::Format::R16G16B16A16_FLOAT;
    }
    return RHI::Format::R8G8B8A8_UNORM;
}

void RenderContext::GetSwapChainRenderDest(RHI::RenderDest &renderDest) const {
    renderDest = RHI::RenderDest();
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = frameCompositionTexture->GetFormat();
    renderDest.sampleCount = 1;
}

void RenderContext::InitPipelines() {
    RHI::RenderDest renderDest;
    GetSwapChainRenderDest(renderDest);

    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.renderDest = renderDest;
    graphicsPipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Disabled;
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::Disabled;

    // Fullscreen triangle 은 frame composition 과 bootstrap 화면에 항상 필요하므로 동기적으로 준비한다.
    imagePipeline = shaderPipelineCache.GetGraphicsPipeline(shaderTemplateManager.fullscreenImageShaderTemplate, ShaderPermutationKey(), graphicsPipelineDesc, false);
    if (!ShaderPipeline::GetReadyPSO(imagePipeline)) {
        BE_FATALERROR("RenderContext::InitPipelines: failed to build image pipeline\n");
    }

    const SharedPtr<ShaderTemplate> *hdrOutputShaderTemplate = nullptr;
    switch (swapChain->GetColorSpace()) {
    case RHI::SwapChain::ColorSpace::BT2020PQ:
        hdrOutputShaderTemplate = &shaderTemplateManager.hdr10OutputShaderTemplate;
        break;
    case RHI::SwapChain::ColorSpace::DisplayP3LinearEDR:
        hdrOutputShaderTemplate = &shaderTemplateManager.displayP3EDROutputShaderTemplate;
        break;
    case RHI::SwapChain::ColorSpace::SRGB:
    default:
        break;
    }

    if (hdrOutputShaderTemplate) {
        graphicsPipelineDesc.renderDest.renderTargetFormats[0] = swapChain->GetRTVFormat();
        hdrOutputPipeline = shaderPipelineCache.GetGraphicsPipeline(*hdrOutputShaderTemplate, ShaderPermutationKey(), graphicsPipelineDesc, false);
        if (!ShaderPipeline::GetReadyPSO(hdrOutputPipeline)) {
            BE_FATALERROR("RenderContext::InitPipelines: failed to build HDR output pipeline\n");
        }
    }

    // bootstrap 화면의 DrawBar 가 첫 프레임부터 보이도록 white unlit GUI pipeline 을 미리 준비한다.
    GraphicsPipelineDesc bootstrapGuiPipelineDesc;
    bootstrapGuiPipelineDesc.inputLayout = &VertexInputLayoutFactory::Get(VertexInputLayoutId::XyzStColor);
    bootstrapGuiPipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Disabled;
    GetSwapChainRenderDest(bootstrapGuiPipelineDesc.renderDest);

    ShaderPipeline *bootstrapGuiPipeline = materialManager.whiteUnlitMaterial->GetGraphicsPipeline({}, bootstrapGuiPipelineDesc, false);
    if (!ShaderPipeline::GetReadyPSO(bootstrapGuiPipeline)) {
        BE_FATALERROR("RenderContext::InitPipelines: failed to build bootstrap GUI pipeline\n");
    }

    // bootstrap 텍스트가 첫 프레임부터 보이도록 필요한 glyph 와 font material pipeline 을 미리 준비한다.
    static constexpr char BootstrapTextGlyphs[] = "Building shader pipelines... 0123456789%Shader pipeline build failed";
    bool cachedGlyphs[128] = {};
    SharedPtr<Material> bootstrapFontMaterial;

    for (const unsigned char character : BootstrapTextGlyphs) {
        if (character == 0 || character >= COUNT_OF(cachedGlyphs) || cachedGlyphs[character]) {
            continue;
        }
        cachedGlyphs[character] = true;

        FontGlyph *glyph = fontManager.defaultFont->GetGlyph(static_cast<char32_t>(character));
        if (glyph && glyph->material && !bootstrapFontMaterial) {
            bootstrapFontMaterial = glyph->material;
        }
    }

    ShaderPipeline *bootstrapFontPipeline = bootstrapFontMaterial ? bootstrapFontMaterial->GetGraphicsPipeline({}, bootstrapGuiPipelineDesc, false) : nullptr;
    if (!ShaderPipeline::GetReadyPSO(bootstrapFontPipeline)) {
        BE_FATALERROR("RenderContext::InitPipelines: failed to build bootstrap font pipeline\n");
    }
}

void RenderContext::ReleasePipelines() {
    imagePipeline.reset();
    hdrOutputPipeline.reset();
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

    const RenderFrameSettings renderFrameSettings = CaptureRenderFrameSettings();
    UpdateDisplayOutput(renderFrameSettings);

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
    mainRTSampleCount = renderFrameSettings.mainRTSampleCount;

    if (renderSystem->IsUsingRenderThread()) {
        // 현재 프런트엔드 프레임의 렌더 스레드 작업이 끝날 때까지 기다린다.
        renderSystem->WaitFrameRenderCompleted(frameData);
    }

    frameData->BeginFrame(renderFrameSettings);

    // BeginFrame 에서 frame fence 완료를 기다렸으므로, 렌더 커맨드에 밀어 넣었던 render work 들을 안전하게 완료한다.
    renderSystem->CompleteRenderWorks(frameData);

    // CSM depth texture 가 없거나 해상도가 바뀌었으면 새로 생성한다.
    if (!EnsureCSMDepthTexture(renderFrameSettings.csmResolution)) {
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
