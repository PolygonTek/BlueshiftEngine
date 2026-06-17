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
#include "RenderCvars.h"
#include "RenderInternal.h"
#include "ShaderTemplate.h"
#include "ShaderPipelineCache.h"
#include "Shaders/ShaderInterop.h"

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

    // 렌더 타겟 텍스쳐 & 뎁스 텍스쳐 생성
    CreateMainRenderTextures(backBufferWidth, backBufferHeight);

    // Tonemapping 에 사용할 Luminance 버퍼 생성
    CreateLuminanceBuffer();

    InitFullScreenTrianglePSO();
    InitToneMappingPSO();

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

    imagePSO.reset();
    toneMappingPSO.reset();
    luminancePass1PSO.reset();
    luminancePass2PSO.reset();
    bloomPrefilterPSO.reset();
    bloomDownsamplePSO.reset();
    bloomUpsamplePSO.reset();
    fftBloomPrefilterPSO.reset();

    DestroyLuminanceBuffer();
    DestroyMainRenderTextures();

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

    // 렌더 타겟 텍스쳐 & 뎁스 텍스쳐 재생성
    DestroyMainRenderTextures();
    CreateMainRenderTextures(width, height);
}

void RenderContext::CreateMainRenderTextures(uint32_t width, uint32_t height) {
    BE1::Image::Format colorImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(mainRTColorFormat, &colorImageFormat, nullptr);

    // NOTE: colorImageFormat 이 sRGB 포맷이 아닌 Linear 포맷이라면, baseClearColor 도 같은 컬러 공간을 가져야 한다.
    const RHI::ClearValue colorOptimizedClearValue = RHI::ClearValue::Color(baseClearColor[0], baseClearColor[1], baseClearColor[2], baseClearColor[3]);
    const RHI::ClearValue depthStencilOptimizedClearValue = RHI::ClearValue::DepthStencil(1.0f, 0);

    SharedPtr<BE1::Image> colorImage = MakeShared<BE1::Image>();
    colorImage->InitFromMemory(width, height, 1, 1, 1, colorImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    mainRTColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, colorImage, false, colorOptimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
    RHI::renderer->SetName(mainRTColorTexture, "mainRTColorTexture");

    BE1::Image::Format postProcessColorImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(RHI::Format::R8G8B8A8_UNORM, &postProcessColorImageFormat, nullptr);

    SharedPtr<BE1::Image> postProcessColorImage = MakeShared<BE1::Image>();
    postProcessColorImage->InitFromMemory(width, height, 1, 1, 1, postProcessColorImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    postProcessColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, postProcessColorImage, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    RHI::renderer->SetName(postProcessColorTexture, "postProcessColorTexture");

    CreatePyramidBloomTextures(width, height);
    CreateFFTBloomTextures(width, height);

    if (mainRTSampleCount > 1) {
        mainRTColorMSAATexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource, colorImage, false, colorOptimizedClearValue, mainRTSampleCount, RHI::GPUResourceState::ShaderResource);
        RHI::renderer->SetName(mainRTColorMSAATexture, "mainRTColorMSAATexture");
    }

    BE1::Image::Format depthImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(mainRTDepthFormat, &depthImageFormat, nullptr);

    SharedPtr<BE1::Image> depthStencilImage = MakeShared<BE1::Image>();
    depthStencilImage->InitFromMemory(width, height, 1, 1, 1, depthImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    mainRTDepthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::DepthStencil, depthStencilImage, false, depthStencilOptimizedClearValue, mainRTSampleCount, RHI::GPUResourceState::DepthWrite);
    RHI::renderer->SetName(mainRTDepthTexture, "mainRTDepthTexture");
}

void RenderContext::DestroyMainRenderTextures() {
    DestroyFFTBloomTextures();
    DestroyPyramidBloomTextures();

    if (mainRTColorTexture) {
        RHI::renderer->DestroyTexture(mainRTColorTexture, true);
        mainRTColorTexture = nullptr;
    }
    if (postProcessColorTexture) {
        RHI::renderer->DestroyTexture(postProcessColorTexture, true);
        postProcessColorTexture = nullptr;
    }
    if (mainRTColorMSAATexture) {
        RHI::renderer->DestroyTexture(mainRTColorMSAATexture, true);
        mainRTColorMSAATexture = nullptr;
    }
    if (mainRTDepthTexture) {
        RHI::renderer->DestroyTexture(mainRTDepthTexture, true);
        mainRTDepthTexture = nullptr;
    }
}

void RenderContext::CreatePyramidBloomTextures(uint32_t width, uint32_t height) {
    DestroyPyramidBloomTextures();

    const BE1::Image::Format bloomImageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R11G11B10_FLOAT);
    const RHI::TextureFlag bloomTextureFlags = RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess | RHI::TextureFlag::SkipDefaultViews;
    const uint32_t maxLevelCount = static_cast<uint32_t>(BE1::Clamp(r_bloomMipCount.GetInteger(), 1, BloomMaxLevelCount));

    // bloom texture 의 mip 0 는 화면의 절반 크기로 시작
    uint32_t bloomWidth = width > 1 ? width >> 1 : 1;
    uint32_t bloomHeight = height > 1 ? height >> 1 : 1;
    uint32_t levelCount = 0;

    // 1x1 이 될 때까지 몇개의 bloom mip 이 필요한지 계산한다. (최대 BloomMaxLevelCount = 8)
    while (levelCount < maxLevelCount) {
        bloomMipWidths[levelCount] = bloomWidth;
        bloomMipHeights[levelCount] = bloomHeight;
        levelCount++;

        if (bloomWidth == 1 && bloomHeight == 1) {
            break;
        }

        bloomWidth = bloomWidth > 1 ? bloomWidth >> 1 : 1;
        bloomHeight = bloomHeight > 1 ? bloomHeight >> 1 : 1;
    }

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->InitFromMemory(bloomMipWidths[0], bloomMipHeights[0], 1, 1, levelCount, bloomImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    // bloom down texture 생성
    bloomDownTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, bloomTextureFlags, image, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomDownTexture) {
        BE_WARNLOG("RenderContext::CreatePyramidBloomTextures: failed to create bloom down texture\n");
        DestroyPyramidBloomTextures();
        return;
    }
    RHI::renderer->SetName(bloomDownTexture, "bloomDownTexture");

    // bloom up texture 생성
    bloomUpTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, bloomTextureFlags, image, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomUpTexture) {
        BE_WARNLOG("RenderContext::CreatePyramidBloomTextures: failed to create bloom up texture\n");
        DestroyPyramidBloomTextures();
        return;
    }
    RHI::renderer->SetName(bloomUpTexture, "bloomUpTexture");

    // mip 별 view 생성
    auto createBloomMipViews = [](RHI::Texture *texture, RHI::ViewID *srvViewIDs, RHI::ViewID *uavViewIDs, uint32_t levelCount, const char *debugName) -> bool {
        for (uint32_t mipLevel = 0; mipLevel < levelCount; ++mipLevel) {
            srvViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::SRV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!srvViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("RenderContext::CreatePyramidBloomTextures: failed to create %s mip %i SRV\n", debugName, mipLevel);
                return false;
            }

            uavViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::UAV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!uavViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("RenderContext::CreatePyramidBloomTextures: failed to create %s mip %i UAV\n", debugName, mipLevel);
                return false;
            }
        }

        return true;
    };

    if (!createBloomMipViews(bloomDownTexture, bloomDownMipSRVs, bloomDownMipUAVs, levelCount, "bloomDownTexture") ||
        !createBloomMipViews(bloomUpTexture, bloomUpMipSRVs, bloomUpMipUAVs, levelCount, "bloomUpTexture")) {
        DestroyPyramidBloomTextures();
        return;
    }

    bloomMipCount = levelCount;
}

void RenderContext::DestroyPyramidBloomTextures() {
    if (bloomDownTexture) {
        RHI::renderer->DestroyTexture(bloomDownTexture, true);
        bloomDownTexture = nullptr;
    }
    if (bloomUpTexture) {
        RHI::renderer->DestroyTexture(bloomUpTexture, true);
        bloomUpTexture = nullptr;
    }

    for (uint32_t mipLevel = 0; mipLevel < BloomMaxLevelCount; ++mipLevel) {
        bloomDownMipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        bloomDownMipUAVs[mipLevel] = RHI::ViewID::InvalidID;
        bloomUpMipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        bloomUpMipUAVs[mipLevel] = RHI::ViewID::InvalidID;
        bloomMipWidths[mipLevel] = 0;
        bloomMipHeights[mipLevel] = 0;
    }

    bloomMipCount = 0;
}

void RenderContext::CreateFFTBloomTextures(uint32_t width, uint32_t height) {
    DestroyFFTBloomTextures();

    // FFT bloom 은 convolution 중간값을 다루므로 RGB-only packed 포맷보다 signed half float 작업 포맷을 사용한다.
    const BE1::Image::Format fftBloomImageFormat = BE1::Image::Format::R16G16B16A16_FLOAT;
    const RHI::TextureFlag fftBloomTextureFlags = RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess;

    // 화면의 절반 크기를 기준으로 하되, FFT pass 에서 쓰기 좋은 power-of-two 크기로 맞춘다.
    bloomFFTWidth = BE1::Math::RoundUpPowerOfTwo(width > 1 ? width >> 1 : 1);
    bloomFFTHeight = BE1::Math::RoundUpPowerOfTwo(height > 1 ? height >> 1 : 1);

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->InitFromMemory(bloomFFTWidth, bloomFFTHeight, 1, 1, 1, fftBloomImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    bloomFFTInputTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, fftBloomTextureFlags, image, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomFFTInputTexture) {
        BE_WARNLOG("RenderContext::CreateFFTBloomTextures: failed to create bloom FFT input texture\n");
        DestroyFFTBloomTextures();
        return;
    }
    RHI::renderer->SetName(bloomFFTInputTexture, "bloomFFTInputTexture");

    bloomFFTWorkTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, fftBloomTextureFlags, image, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomFFTWorkTexture) {
        BE_WARNLOG("RenderContext::CreateFFTBloomTextures: failed to create bloom FFT work texture\n");
        DestroyFFTBloomTextures();
        return;
    }
    RHI::renderer->SetName(bloomFFTWorkTexture, "bloomFFTWorkTexture");

    bloomFFTOutputTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, fftBloomTextureFlags, image, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomFFTOutputTexture) {
        BE_WARNLOG("RenderContext::CreateFFTBloomTextures: failed to create bloom FFT output texture\n");
        DestroyFFTBloomTextures();
        return;
    }
    RHI::renderer->SetName(bloomFFTOutputTexture, "bloomFFTOutputTexture");
}

void RenderContext::DestroyFFTBloomTextures() {
    if (bloomFFTInputTexture) {
        RHI::renderer->DestroyTexture(bloomFFTInputTexture, true);
        bloomFFTInputTexture = nullptr;
    }
    if (bloomFFTWorkTexture) {
        RHI::renderer->DestroyTexture(bloomFFTWorkTexture, true);
        bloomFFTWorkTexture = nullptr;
    }
    if (bloomFFTOutputTexture) {
        RHI::renderer->DestroyTexture(bloomFFTOutputTexture, true);
        bloomFFTOutputTexture = nullptr;
    }

    bloomFFTWidth = 0;
    bloomFFTHeight = 0;
}

void RenderContext::CreateLuminanceBuffer() {
    // Luminance buffer : exposure (float) + luminance (float) + histogram (uint32_t * 1024)
    static constexpr uint32_t LuminanceBufferSize = LuminanceBufferOffsetHistogram + LuminanceHistogramBinCount * sizeof(uint32_t);
    static constexpr uint32_t LuminanceBufferDwordCount = LuminanceBufferSize / sizeof(uint32_t);
    const uint32_t initialLuminanceBufferData[LuminanceBufferDwordCount] = {};

    luminanceBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, LuminanceBufferSize, initialLuminanceBufferData);
    if (!luminanceBuffer) {
        BE_WARNLOG("RenderContext::CreateLuminanceBuffer: failed to create luminance buffer\n");
        return;
    }

    RHI::renderer->SetName(luminanceBuffer, "luminanceBuffer");

    RHI::ViewID srvViewID = RHI::renderer->CreateView(luminanceBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, 0, 0, LuminanceBufferSize);
    if (!srvViewID.IsValid()) {
        BE_WARNLOG("RenderContext::CreateLuminanceBuffer: failed to create luminance SRV\n");
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
        return;
    }

    RHI::ViewID uavViewID = RHI::renderer->CreateView(luminanceBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, 0, 0, LuminanceBufferSize);
    if (!uavViewID.IsValid()) {
        BE_WARNLOG("RenderContext::CreateLuminanceBuffer: failed to create luminance UAV\n");
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
        return;
    }
}

void RenderContext::DestroyLuminanceBuffer() {
    if (luminanceBuffer) {
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
    }
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

void RenderContext::InitFullScreenTrianglePSO() {
    RHI::RenderDest renderDest;
    GetSwapChainRenderDest(renderDest);

    ShaderPipelineDesc pipelineDesc;
    pipelineDesc.renderDest = renderDest;
    pipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Never;
    pipelineDesc.blendStateType = RHI::BlendStateType::Disabled;
    imagePSO = shaderPipelineCache.GetGraphicsPipeline(shaderTemplateManager.fullscreenImageShaderTemplate, ShaderPermutationKey(), pipelineDesc, false);
}

void RenderContext::InitToneMappingPSO() {
    luminancePass1PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.luminancePass1ShaderTemplate, ShaderPermutationKey(), false);
    luminancePass2PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.luminancePass2ShaderTemplate, ShaderPermutationKey(), false);
    bloomPrefilterPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.bloomPrefilterShaderTemplate, ShaderPermutationKey(), false);
    bloomDownsamplePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.bloomDownsampleShaderTemplate, ShaderPermutationKey(), false);
    bloomUpsamplePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.bloomUpsampleShaderTemplate, ShaderPermutationKey(), false);
    fftBloomPrefilterPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftBloomPrefilterShaderTemplate, ShaderPermutationKey(), false);
    toneMappingPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.toneMappingShaderTemplate, ShaderPermutationKey(), false);
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
