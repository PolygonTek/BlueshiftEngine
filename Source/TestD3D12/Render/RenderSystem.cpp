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
#include "RenderWork/ConvolutionBloomKernelRenderWork.h"
#include "RenderContext.h"
#include "Render/Backend/RenderBackend.h"
#include "RenderCvars.h"
#include "RHI/RHI.h"
#include "SamplerManager.h"
#include "ColorGradingLUTGenerator.h"
#include "Render/Texture/TextureManager.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Font/FontManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/Animation/SkeletonManager.h"
#include "Render/Animation/AnimManager.h"

thread_local bool       RenderSystem::isRenderThread = false;

RenderSystem *          renderSystem = nullptr;

void RenderSystem::Init(void *mainWindowHandle) {
    BE1::cmdSystem.AddCommand("screenshot", Cmd_ScreenShot);

    if (!RHI::CreateRenderer()) {
        BE_FATALERROR("RenderSystem::Init: failed to create RHI renderer\n");
    }

    RHI::renderer->Init(mainWindowHandle);

    VertexInputLayoutFactory::Init();

    samplerManager.Init();

    textureManager.Init();

    dfgSumGGXTexture = textureManager.GetTexture("Data/EngineTextures/DFGSumGGX.dds", Texture::Flag::HighQuality | Texture::Flag::NoMipmaps | Texture::Flag::LinearColorSpace, false);
    if (dfgSumGGXTexture == textureManager.missingTexture) {
        BE_FATALERROR("RenderSystem::Init: failed to load 'Data/EngineTextures/DFGSumGGX.dds'\n");
    }

    InitSTBNTextures();

    CreateColorGradingLUT();

    UploadEngineTextures();

    shaderTemplateManager.Init();

    shaderPipelineBootstrap.BeginCollection(128);

    gpuFFT.Init();
    gpuFFT.TrackBootstrapPipelines(shaderPipelineBootstrap);

    gpuOcean.Init();
    gpuOcean.TrackBootstrapPipelines(shaderPipelineBootstrap);

    gpuOceanSurface.Init(&gpuOcean);
    gpuOceanSurface.TrackBootstrapPipelines(shaderPipelineBootstrap);

    gpuSphericalHarmonics.Init();
    gpuSphericalHarmonics.TrackBootstrapPipelines(shaderPipelineBootstrap);

    hiZ.Init();
    hiZ.TrackBootstrapPipelines(shaderPipelineBootstrap);

    screenSpaceAO.Init();
    screenSpaceAO.TrackBootstrapPipelines(shaderPipelineBootstrap);

    ssrHiZ.Init();
    ssrHiZ.TrackBootstrapPipelines(shaderPipelineBootstrap);

    ssrSceneColorHistory.Init();
    ssrSceneColorHistory.TrackBootstrapPipelines(shaderPipelineBootstrap);

    bloom.Init();
    bloom.TrackBootstrapPipelines(shaderPipelineBootstrap);

    postLuminance.Init();
    postLuminance.TrackBootstrapPipelines(shaderPipelineBootstrap);

    iblBaker.Init();
    iblBaker.TrackBootstrapPipelines(shaderPipelineBootstrap);

    probeBakeSystem.Init();

    InitComputePipelines();

    convolutionBloomKernelGenerator.Init();
    convolutionBloomKernelGenerator.TrackBootstrapPipelines(shaderPipelineBootstrap);

    materialManager.Init();

    fontManager.Init();

    skeletonManager.Init();

    animManager.Init();

    meshManager.Init();

    backEnd = new RenderBackend;
    backEnd->Init();

    shaderPipelineBootstrap.EndCollection();

    if (r_renderThread.GetBool()) {
        InitRenderThread();
    }
}

void RenderSystem::Shutdown() {
    WaitAllRenderCompleted();
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);

    renderWorkQueue.CompleteAllAttachedWorks();
    renderWorkQueue.CancelPendingWorks();

    if (IsUsingRenderThread()) {
        ShutdownRenderThread();
    }

    BE1::cmdSystem.RemoveCommand("screenshot");

    shaderPipelineBootstrap.Reset();

    backEnd->Shutdown();
    SAFE_DELETE(backEnd);

    meshManager.Shutdown();

    animManager.Shutdown();

    skeletonManager.Shutdown();

    fontManager.Shutdown();

    materialManager.Shutdown();

    ReleaseComputePipelines();

    convolutionBloomKernelGenerator.Shutdown();

    probeBakeSystem.Shutdown();

    iblBaker.Shutdown();

    postLuminance.Shutdown();

    bloom.Shutdown();

    ssrSceneColorHistory.Shutdown();

    ssrHiZ.Shutdown();

    screenSpaceAO.Shutdown();

    hiZ.Shutdown();

    gpuSphericalHarmonics.Shutdown();

    gpuOceanSurface.Shutdown();

    gpuOcean.Shutdown();

    gpuFFT.Shutdown();

    shaderPipelineCache.ClearAllPipelines();

    shaderTemplateManager.Shutdown();

    VertexInputLayoutFactory::Shutdown();

    DestroyColorGradingLUT();

    dfgSumGGXTexture.reset();
    stbnScalarTexture.reset();
    stbnVec1Texture.reset();
    stbnVec2Texture.reset();
    stbnVec3Texture.reset();
    stbnUnitVec1Texture.reset();
    stbnUnitVec2Texture.reset();
    stbnUnitVec3Texture.reset();
    stbnUnitVec3CosineTexture.reset();

    textureManager.Shutdown();

    samplerManager.Shutdown();

    RHI::renderer->Shutdown();

    RHI::DestroyRenderer();
}

void RenderSystem::BeginGameFrame() {
    gpuOceanSurface.ConsumeCompletedQueries();
}

void RenderSystem::UpdateFrame() {
    textureManager.UpdateFrame();

    if (colorGradingLUTDirty) {
        UpdateColorGradingLUT();
    }

    shaderTemplateManager.UpdateFrame();

    materialManager.UpdateFrame();

    meshManager.UpdateFrame();

    skeletonManager.UpdateFrame();

    animManager.UpdateFrame();

    fontManager.UpdateFrame();

    RHI::renderer->GetMemoryUsage(videoMemoryUsage);
    assert(videoMemoryUsage.budgetBytes > 0);

    for (RenderContext *renderContext : renderContexts) {
        renderContext->textureStreamingFeedback.ConsumeCompletedReadbacks([](const uint32_t *feedbackData, uint32_t capacity) {
            materialManager.ApplyStreamingFeedback(feedbackData, capacity);
        });
    }
}

RenderContext *RenderSystem::CreateRenderContext(void *windowHandle, bool isMain) {
    if (isMain && mainRenderContext) {
        BE_FATALERROR("RenderSystem::CreateRenderContext: main render context already exists");
        return nullptr;
    }

    const BE1::Color4 baseClearColor = BE1::Color4::darkBlue;

    RenderContext *renderContext = new RenderContext;
    renderContext->Init(windowHandle, baseClearColor);
    renderContexts.Append(renderContext);

    if (isMain) {
        mainRenderContext = renderContext;
    }

    return renderContext;
}

void RenderSystem::DestroyRenderContext(RenderContext *renderContext) {
    if (IsUsingRenderThread()) {
        WaitContextRenderCompleted(renderContext);
    }

    if (renderContext == mainRenderContext) {
        mainRenderContext = nullptr;
    }

    renderContext->Shutdown();

    renderWorkQueue.CompleteAttachedWorks(renderContext->frames, COUNT_OF(renderContext->frames));

    backEnd->DiscardGpuFrameTimings(renderContext);

    renderContexts.Remove(renderContext);
    delete renderContext;
}

RenderContext *RenderSystem::FindRenderContextByWindowHandle(void *windowHandle) const {
    for (RenderContext *renderContext : renderContexts) {
        if (renderContext->GetWindowHandle() == windowHandle) {
            return renderContext;
        }
    }
    return nullptr;
}

void RenderSystem::SubmitFrame(RenderFrame *frame) {
    assert(frame);

    if (IsUsingRenderThread()) {
        BE1::ScopedWriteLock lock(frameSyncLock);

        // 주어진 프레임에 대한 (메인 스레드의) 업데이트가 완료되어,
        // (렌더 스레드의) 렌더링이 완료되기를 기다리는 상태로 변경.
        assert(frame->frameSyncState == FrameSyncState::RenderCompleted);
        frame->frameSyncState = FrameSyncState::UpdateCompleted;

        submittedFrames.PushBack(frame);

        // 렌더 스레드에게 업데이트가 완료되었다고 신호를 보낸다.
        updateCompletedCondition.Signal();
        return;
    }

    // 현재 프레임 데이터로 렌더링 백엔드를 실행한다.
    backEnd->Execute(frame);
}

void RenderSystem::WaitFrameRenderCompleted(const RenderFrame *frame) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitFrameRenderCompleted");

    assert(BE1::Engine::IsInMainThread());

    if (!IsUsingRenderThread()) {
        return;
    }

    {
        BE1::ScopedReadLock lock(frameSyncLock);

        // 주어진 프레임에 대한 (렌더 스레드의) 렌더링이 완료되기를 기다린다.
        renderCompletedCondition.Wait(lock, [frame] {
            return frame->frameSyncState == FrameSyncState::RenderCompleted;
        });
    }
}

void RenderSystem::WaitContextRenderCompleted(const RenderContext *renderContext) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitContextRenderCompleted");

    assert(BE1::Engine::IsInMainThread());

    for (const RenderFrame &frame : renderContext->frames) {
        WaitFrameRenderCompleted(&frame);
    }
}

void RenderSystem::WaitAllRenderCompleted() {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitAllRenderCompleted");

    assert(BE1::Engine::IsInMainThread());

    if (!IsUsingRenderThread()) {
        return;
    }

    for (RenderContext *renderContext : renderContexts) {
        WaitContextRenderCompleted(renderContext);
    }
}

void RenderSystem::EnqueueRenderWork(UniquePtr<RenderWork> work) {
    assert(BE1::Engine::IsInMainThread());

    renderWorkQueue.Enqueue(std::move(work));
}

void RenderSystem::CompleteRenderWorks(RenderFrame *frame) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::CompleteRenderWorks");

    assert(BE1::Engine::IsInMainThread());

    renderWorkQueue.CompleteAttachedWorks(frame);
}

void RenderSystem::AttachPendingRenderWorks(RenderFrame *frame) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::AttachPendingRenderWorks");

    assert(BE1::Engine::IsInMainThread());

    static constexpr int MaxRenderWorksPerFrame = 1;
    renderWorkQueue.AttachPendingWorks(frame, MaxRenderWorksPerFrame);
}

uint64_t RenderSystem::RequestConvolutionBloomKernelGeneration(const SharedPtr<Texture> &apertureTexture, float apertureScale, float dispersionAmount) {
    assert(BE1::Engine::IsInMainThread());

    if (!apertureTexture) {
        return 0;
    }

    const uint64_t requestSerial = ++nextConvolutionBloomKernelGenerationSerial;
    UniquePtr<RenderWork> renderWork = MakeUnique<ConvolutionBloomKernelRenderWork>(requestSerial, apertureTexture, apertureScale, dispersionAmount);
    EnqueueRenderWork(std::move(renderWork));
    return requestSerial;
}

void RenderSystem::CompleteConvolutionBloomKernelGeneration(uint64_t requestSerial, bool succeeded) {
    assert(BE1::Engine::IsInMainThread());
    assert(requestSerial > completedConvolutionBloomKernelGenerationSerial);

    completedConvolutionBloomKernelGenerationSerial = requestSerial;
    completedConvolutionBloomKernelGenerationSucceeded = succeeded;
}

bool RenderSystem::GetConvolutionBloomKernelGenerationResult(uint64_t requestSerial, bool &succeeded) const {
    assert(BE1::Engine::IsInMainThread());

    // 요청 시리얼을 체크해서 이전에 요청한 것은 false 를 리턴하게 한다.
    if (requestSerial == 0 || completedConvolutionBloomKernelGenerationSerial != requestSerial) {
        return false;
    }

    succeeded = completedConvolutionBloomKernelGenerationSucceeded;
    return true;
}

void RenderSystem::PrepareProbeBakes(RenderContext *renderContext) {
    assert(BE1::Engine::IsInMainThread());

    if (renderContext != mainRenderContext) {
        return;
    }

    probeBakeSystem.Prepare(renderContext);
}

static SharedPtr<Texture> LoadSTBNTexture(const char *filename, BE1::Image::Format format) {
    SharedPtr<Texture> texture = textureManager.GetTexture(filename, Texture::Flag::HighQuality | Texture::Flag::NoMipmaps | Texture::Flag::LinearColorSpace, false);
    if (texture == textureManager.missingTexture) {
        BE_FATALERROR("RenderSystem::InitSTBNTextures: failed to load '%s'\n", filename);
    }

    if (texture->GetType() != RHI::TextureType::Texture2DArray ||
        texture->GetWidth() != 128 || texture->GetHeight() != 128 || texture->GetArraySize() != 64 ||
        texture->GetMipLevelCount() != 1 || texture->GetFormat() != format) {
        BE_FATALERROR("RenderSystem::InitSTBNTextures: unexpected texture format or dimensions in '%s'\n", filename);
    }
    return texture;
}

void RenderSystem::InitSTBNTextures() {
    stbnScalarTexture = LoadSTBNTexture("Data/EngineTextures/STBN/STBNScalar.dds", BE1::Image::Format::R8);
    stbnVec1Texture = LoadSTBNTexture("Data/EngineTextures/STBN/STBNVec1.dds", BE1::Image::Format::R8);
    stbnVec2Texture = LoadSTBNTexture("Data/EngineTextures/STBN/STBNVec2.dds", BE1::Image::Format::R8G8);
    stbnVec3Texture = LoadSTBNTexture("Data/EngineTextures/STBN/STBNVec3.dds", BE1::Image::Format::R8G8B8A8);
    stbnUnitVec1Texture = LoadSTBNTexture("Data/EngineTextures/STBN/STBNUnitVec1.dds", BE1::Image::Format::R8);
    stbnUnitVec2Texture = LoadSTBNTexture("Data/EngineTextures/STBN/STBNUnitVec2.dds", BE1::Image::Format::R8G8);
    stbnUnitVec3Texture = LoadSTBNTexture("Data/EngineTextures/STBN/STBNUnitVec3.dds", BE1::Image::Format::R8G8B8A8);
    stbnUnitVec3CosineTexture = LoadSTBNTexture("Data/EngineTextures/STBN/STBNUnitVec3Cosine.dds", BE1::Image::Format::R8G8B8A8);
}

void RenderSystem::UploadEngineTextures() {
    // 렌더 컨텍스트와 비동기 에셋 로딩을 시작하기 전에 필수 엔진 텍스처를 일괄 업로드한다.
    RHI::renderer->FlushResourceRequests();

    const Texture *engineTextures[] = {
        textureManager.missingTexture.get(),
        textureManager.blackTexture.get(),
        textureManager.whiteTexture.get(),
        textureManager.flatNormalTexture.get(),
        dfgSumGGXTexture.get(),
        colorGradingLUTTexture.get(),
        stbnScalarTexture.get(),
        stbnVec1Texture.get(),
        stbnVec2Texture.get(),
        stbnVec3Texture.get(),
        stbnUnitVec1Texture.get(),
        stbnUnitVec2Texture.get(),
        stbnUnitVec3Texture.get(),
        stbnUnitVec3CosineTexture.get()
    };

    for (const Texture *engineTexture : engineTextures) {
        const RHI::Texture *rhiTexture = engineTexture->GetRHITexture();
        if (!rhiTexture || rhiTexture->GetStatus() != RHI::GPUResourceStatus::Ready) {
            BE_FATALERROR("RenderSystem::UploadEngineTextures: failed to create or upload '%s'\n", engineTexture->GetHashName());
        }
    }
}

void RenderSystem::InitComputePipelines() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    msaaResolveDepthShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_msaaResolveDepthShader", computeShaderModel, "MSAA/MSAAResolveDepth.hlsl", baseDir);
    msaaResolveDepthNormalShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_msaaResolveDepthNormalShader", computeShaderModel, "MSAA/MSAAResolveDepthNormal.hlsl", baseDir);
    msaaResolveVelocityShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_msaaResolveVelocityShader", computeShaderModel, "MSAA/MSAAResolveVelocity.hlsl", baseDir);
    lightCullingShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_lightCullingShader", computeShaderModel, "Lighting/LightCulling.hlsl", baseDir);
    for (uint32_t resolutionMode = 0; resolutionMode < RenderFrameSettings::SSRResolutionModeCount; ++resolutionMode) {
        const BE1::Str defines = BE1::va("SSR_RESOLUTION_DIVISOR=%u", 1u << resolutionMode);
        ssrTraceShaderTemplates[resolutionMode] = shaderTemplateManager.CreateComputeShaderTemplate(BE1::va("_ssrTraceShader%u", resolutionMode), computeShaderModel, "SSR/SSRTrace.hlsl", baseDir, "CSMain", defines.c_str());
        ssrTemporalShaderTemplates[resolutionMode] = shaderTemplateManager.CreateComputeShaderTemplate(BE1::va("_ssrTemporalShader%u", resolutionMode), computeShaderModel, "SSR/SSRTemporal.hlsl", baseDir, "CSMain", defines.c_str());
        ssrUpsampleShaderTemplates[resolutionMode] = shaderTemplateManager.CreateComputeShaderTemplate(BE1::va("_ssrUpsampleShader%u", resolutionMode), computeShaderModel, "SSR/SSRUpsample.hlsl", baseDir, "CSMain", defines.c_str());
    }
    ssrRadianceResolveShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssrRadianceResolveShader", computeShaderModel, "SSR/SSRRadianceResolve.hlsl", baseDir);
    taaResolveShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_taaResolveShader", computeShaderModel, "TAAResolve.hlsl", baseDir);
    finalPostProcessShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_finalPostProcessShader", computeShaderModel, "FinalPostProcess.hlsl", baseDir);

    EnsureComputePipelines();
}

static void EnsureComputePipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, SharedPtr<ShaderPipeline> &pipeline, ShaderPipelineBootstrap &bootstrap) {
    static constexpr char groupName[] = "RenderSystem";

    if (!pipeline || pipeline->GetState() == ShaderPipeline::State::Empty) {
        pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplate, ShaderPermutationKey(), true);
    }
    bootstrap.TrackPipeline(groupName, pipeline);
}

void RenderSystem::EnsureComputePipelines() {
    EnsureComputePipeline(msaaResolveDepthShaderTemplate, msaaResolveDepthPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(msaaResolveDepthNormalShaderTemplate, msaaResolveDepthNormalPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(msaaResolveVelocityShaderTemplate, msaaResolveVelocityPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(lightCullingShaderTemplate, lightCullingPipeline, shaderPipelineBootstrap);
    for (uint32_t resolutionMode = 0; resolutionMode < RenderFrameSettings::SSRResolutionModeCount; ++resolutionMode) {
        EnsureComputePipeline(ssrTraceShaderTemplates[resolutionMode], ssrTracePipelines[resolutionMode], shaderPipelineBootstrap);
        EnsureComputePipeline(ssrTemporalShaderTemplates[resolutionMode], ssrTemporalPipelines[resolutionMode], shaderPipelineBootstrap);
        EnsureComputePipeline(ssrUpsampleShaderTemplates[resolutionMode], ssrUpsamplePipelines[resolutionMode], shaderPipelineBootstrap);
    }
    EnsureComputePipeline(ssrRadianceResolveShaderTemplate, ssrRadianceResolvePipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(taaResolveShaderTemplate, taaResolvePipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(finalPostProcessShaderTemplate, finalPostProcessPipeline, shaderPipelineBootstrap);
}

void RenderSystem::ReleaseComputePipelines() {
    msaaResolveDepthPipeline.reset();
    msaaResolveDepthNormalPipeline.reset();
    msaaResolveVelocityPipeline.reset();
    lightCullingPipeline.reset();
    for (uint32_t resolutionMode = 0; resolutionMode < RenderFrameSettings::SSRResolutionModeCount; ++resolutionMode) {
        ssrTracePipelines[resolutionMode].reset();
        ssrTraceShaderTemplates[resolutionMode].reset();
        ssrTemporalPipelines[resolutionMode].reset();
        ssrTemporalShaderTemplates[resolutionMode].reset();
        ssrUpsamplePipelines[resolutionMode].reset();
        ssrUpsampleShaderTemplates[resolutionMode].reset();
    }
    ssrRadianceResolvePipeline.reset();
    taaResolvePipeline.reset();
    finalPostProcessPipeline.reset();

    msaaResolveDepthShaderTemplate.reset();
    msaaResolveDepthNormalShaderTemplate.reset();
    msaaResolveVelocityShaderTemplate.reset();
    lightCullingShaderTemplate.reset();
    ssrRadianceResolveShaderTemplate.reset();
    taaResolveShaderTemplate.reset();
    finalPostProcessShaderTemplate.reset();
}

static ColorGradingLUTGenerator::Settings MakeColorGradingLUTSettingsFromCVars() {
    ColorGradingLUTGenerator::Settings settings;
    settings.saturation = r_colorGradingSaturation.GetFloat();
    settings.contrast = r_colorGradingContrast.GetFloat();
    settings.gamma = r_colorGradingGamma.GetFloat();
    settings.gain = r_colorGradingGain.GetFloat();
    settings.tint.Set(r_colorGradingTintR.GetFloat(), r_colorGradingTintG.GetFloat(), r_colorGradingTintB.GetFloat());
    return settings;
}

void RenderSystem::CreateColorGradingLUT() {
    // cvars 로 settings 를 채우고, color grading 3D LUT image 를 생성한다.
    const ColorGradingLUTGenerator::Settings settings = MakeColorGradingLUTSettingsFromCVars();
    ColorGradingLUTGenerator::Generate(settings, colorGradingLUTImage);

    // 초기화 시 일괄 처리할 업로드 요청이 소유할 image 를 이후 갱신에 재사용할 CPU image 와 분리한다.
    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>(colorGradingLUTImage);

    // color grading 3D LUT texture 생성
    colorGradingLUTTexture = textureManager.AllocTexture("_Generated/ColorGradingLUT");
    if (!colorGradingLUTTexture->Create3D(std::move(textureImage), Texture::Flag::HighQuality | Texture::Flag::NoMipmaps | Texture::Flag::Compute)) {
        BE_FATALERROR("RenderSystem::CreateColorGradingLUT: failed to create LUT texture\n");
    }

    colorGradingLUTDirty = false;
}

void RenderSystem::UpdateColorGradingLUT() {
    const ColorGradingLUTGenerator::Settings settings = MakeColorGradingLUTSettingsFromCVars();
    ColorGradingLUTGenerator::Generate(settings, colorGradingLUTImage);

    if (!colorGradingLUTTexture->Update3D(0, 0, 0, 0,
        ColorGradingLUTGenerator::LUTSize, ColorGradingLUTGenerator::LUTSize, ColorGradingLUTGenerator::LUTSize,
        colorGradingLUTImage.GetFormat(), colorGradingLUTImage.GetPixels())) {
        BE_WARNLOG("RenderSystem::UpdateColorGradingLUT: failed to update LUT texture\n");
        return;
    }

    colorGradingLUTDirty = false;
}

void RenderSystem::DestroyColorGradingLUT() {
    colorGradingLUTTexture.reset();
    colorGradingLUTImage.Clear();
    colorGradingLUTDirty = false;
}

void RenderSystem::InitRenderThread() {
    submittedFrames.Reset();
    isStoppingRenderThread = false;

    // 렌더 스레드를 시작한다.
    renderThread = BE1::PlatformThread::Start(RenderSystem::RenderThreadProc, this);
}

void RenderSystem::ShutdownRenderThread() {
    {
        BE1::ScopedWriteLock lock(frameSyncLock);
        isStoppingRenderThread = true;

        // 렌더 스레드가 프레임 제출을 기다리는 중이라면 종료할 수 있도록 깨운다.
        updateCompletedCondition.Signal();
    }

    BE1::PlatformThread::Join(renderThread);
    renderThread = nullptr;

    submittedFrames.Reset();
    isStoppingRenderThread = false;
}

// 렌더 스레드에서 프레임이 제출될 때까지 기다리고, 제출되었다면 제출된 프레임을 리턴한다.
RenderFrame *RenderSystem::WaitSubmittedFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitSubmittedFrame");

    assert(RenderSystem::IsInRenderThread());

    RenderFrame *outFrame = nullptr;
    {
        BE1::ScopedWriteLock lock(frameSyncLock);

        // 제출된 프레임이 생기거나 렌더 스레드의 종료 요청이 들어올 때까지 기다린다.
        updateCompletedCondition.Wait(lock, [this] {
            return !submittedFrames.IsEmpty() || isStoppingRenderThread;
        });

        if (isStoppingRenderThread) {
            return nullptr;
        }

        // 가장 먼저 제출된 프레임을 꺼낸다.
        outFrame = submittedFrames.PopFront();
    }
    return outFrame;
}

void RenderSystem::MarkRenderCompleted(RenderFrame *frame) {
    assert(RenderSystem::IsInRenderThread());

    BE1::ScopedWriteLock lock(frameSyncLock);

    // 주어진 프레임에 대한 (렌더 스레드의) 렌더링이 완료되어,
    // (메인 스레드의) 업데이트가 완료되기를 기다리는 상태로 변경.
    assert(frame->frameSyncState == FrameSyncState::UpdateCompleted);
    frame->frameSyncState = FrameSyncState::RenderCompleted;

    // 메인 스레드에게 렌더링이 완료되었다고 신호를 보낸다.
    renderCompletedCondition.Signal();
}

unsigned int RenderSystem::RenderThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("RenderSystem::RenderThreadProc");

    isRenderThread = true;

    BE1::SIMD::SetDenormalFlushMode(true);

    RenderSystem *renderSystem = reinterpret_cast<RenderSystem *>(param);

    while (1) {
        PROFILER_CPU_SCOPED_EVENT("RenderThreadProcLoop");

        // 프론트엔드에서 프레임이 제출되기를 기다린다.
        RenderFrame *backendFrame = renderSystem->WaitSubmittedFrame();
        if (!backendFrame) {
            break;
        }

        // 제출된 프레임으로 렌더링 백엔드를 실행한다.
        renderSystem->backEnd->Execute(backendFrame);

        // 메인 스레드에게 렌더링이 완료되었음을 알린다.
        renderSystem->MarkRenderCompleted(backendFrame);
    }

    return 0;
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

    renderContext->RequestScreenshot(path);
}
