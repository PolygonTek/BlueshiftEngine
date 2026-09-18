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
    if (dfgSumGGXTexture == textureManager.defaultTexture) {
        BE_WARNLOG("RenderSystem::Init: failed to load the GGX DFG LUT texture\n");
        dfgSumGGXTexture.reset();
    }

    CreateColorGradingLUT();

    shaderTemplateManager.Init();

    shaderPipelineBootstrap.BeginCollection(96);

    gpuFFT.Init();
    gpuFFT.TrackBootstrapPipelines(shaderPipelineBootstrap);

    gpuOcean.Init();
    gpuOcean.TrackBootstrapPipelines(shaderPipelineBootstrap);

    gpuOceanSurface.Init(&gpuOcean);
    gpuOceanSurface.TrackBootstrapPipelines(shaderPipelineBootstrap);

    gpuSphericalHarmonics.Init();
    gpuSphericalHarmonics.TrackBootstrapPipelines(shaderPipelineBootstrap);

    screenSpaceAO.Init();
    screenSpaceAO.TrackBootstrapPipelines(shaderPipelineBootstrap);

    screenSpaceDepthPyramid.Init();
    screenSpaceDepthPyramid.TrackBootstrapPipelines(shaderPipelineBootstrap);

    bloom.Init();
    bloom.TrackBootstrapPipelines(shaderPipelineBootstrap);

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

#ifdef USE_RENDER_THREAD
    InitRenderThread();
#endif
}

void RenderSystem::Shutdown() {
    WaitAllRenderCompleted();
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);

    renderWorkQueue.CompleteAllAttachedWorks();
    renderWorkQueue.CancelPendingWorks();

#ifdef USE_RENDER_THREAD
    ShutdownRenderThread();
#endif

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

    bloom.Shutdown();

    screenSpaceDepthPyramid.Shutdown();

    screenSpaceAO.Shutdown();

    gpuSphericalHarmonics.Shutdown();

    gpuOceanSurface.Shutdown();

    gpuOcean.Shutdown();

    gpuFFT.Shutdown();

    shaderPipelineCache.ClearAllPipelines();

    shaderTemplateManager.Shutdown();

    VertexInputLayoutFactory::Shutdown();

    DestroyColorGradingLUT();

    dfgSumGGXTexture.reset();

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
    RenderContext *renderContext = new RenderContext;
    renderContexts.Append(renderContext);

    const BE1::Color4 baseClearColor = BE1::Color4::darkBlue;

    renderContext->Init(windowHandle, baseClearColor);

    if (isMain) {
        assert(!mainRenderContext);
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

void RenderSystem::SubmitFrame(RenderContext *renderContext, RenderFrameData *frameData) {
    assert(renderContext);
    assert(frameData);

    if (IsUsingRenderThread()) {
        BE1::ScopedWriteLock lock(frameSyncLock);

        // 주어진 프레임에 대한 (메인 스레드의) 업데이트가 완료되어,
        // (렌더 스레드의) 렌더링이 완료되기를 기다리는 상태로 변경.
        assert(frameData->frameSyncState == FrameSyncState::RenderCompleted);
        frameData->frameSyncState = FrameSyncState::UpdateCompleted;

        SubmittedFrame submittedFrame;
        submittedFrame.renderContext = renderContext;
        submittedFrame.frameData = frameData;
        submittedFrames.Append(submittedFrame);

        // 렌더 스레드에게 업데이트가 완료되었다고 신호를 보낸다.
        updateCompletedCondition.Signal();
    } else {
        // 이전에 사용이 완료된 리소스들을 해제한다.
        RHI::renderer->FreeRetiredResources(false);

        // 리소스 요청 처리
        RHI::renderer->ProcessResourceRequests(frameData->GetThreadData(0));

        // 현재 프레임 데이터로 렌더링 백엔드를 실행한다.
        backEnd->Execute(frameData);

        // 이번 프레임에 Release/Delete 를 요청한 리소스들을 retiredResources 목록에 추가한다.
        RHI::renderer->DrainReleaseRequestsToRetireList();
    }
}

void RenderSystem::WaitFrameRenderCompleted(const RenderFrameData *frameData) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitFrameRenderCompleted");

    assert(BE1::Engine::IsInMainThread());

    if (!IsUsingRenderThread()) {
        return;
    }

    {
        BE1::ScopedReadLock lock(frameSyncLock);

        // 주어진 프레임에 대한 (렌더 스레드의) 렌더링이 완료되기를 기다린다.
        renderCompletedCondition.Wait(lock, [frameData] {
            return frameData->frameSyncState == FrameSyncState::RenderCompleted;
        });
    }
}

void RenderSystem::WaitContextRenderCompleted(const RenderContext *renderContext) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitContextRenderCompleted");

    assert(BE1::Engine::IsInMainThread());

    for (const RenderFrameData &frameData : renderContext->frames) {
        WaitFrameRenderCompleted(&frameData);
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

void RenderSystem::RequestScreenshot(RenderContext *renderContext, const char *filename) {
    if (!renderContext) {
        return;
    }

    renderContext->RequestScreenshot(filename);
}

void RenderSystem::EnqueueRenderWork(UniquePtr<RenderWork> work) {
    assert(BE1::Engine::IsInMainThread());

    renderWorkQueue.Enqueue(std::move(work));
}

void RenderSystem::CompleteRenderWorks(RenderFrameData *frameData) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::CompleteRenderWorks");

    assert(BE1::Engine::IsInMainThread());

    renderWorkQueue.CompleteAttachedWorks(frameData);
}

void RenderSystem::AttachPendingRenderWorks(RenderFrameData *frameData) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::AttachPendingRenderWorks");

    assert(BE1::Engine::IsInMainThread());

    static constexpr int MaxRenderWorksPerFrame = 1;
    renderWorkQueue.AttachPendingWorks(frameData, MaxRenderWorksPerFrame);
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

void RenderSystem::InitComputePipelines() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    genHiZMipFirstBatchShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genHiZMipFirstBatchShader", computeShaderModel, "HiZ/GenHiZMipFirstBatch.hlsl", baseDir);
    genHiZMipFirstBatchMSAAShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genHiZMipFirstBatchMSAAShader", computeShaderModel, "HiZ/GenHiZMipFirstBatch.hlsl", baseDir, "CSMain", "GEN_HIZ_DEPTH_MSAA=1");
    genHiZMipBatchShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genHiZMipBatchShader", computeShaderModel, "HiZ/GenHiZMipBatch.hlsl", baseDir);
    genHiZMipOddDimShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genHiZMipOddDimShader", computeShaderModel, "HiZ/GenHiZMipOddDim.hlsl", baseDir);
    hiZCullInstancesShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_hiZCullInstancesShader", computeShaderModel, "HiZ/HiZCullInstances.hlsl", baseDir);
    hiZDebugPreviewShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_hiZDebugPreviewShader", computeShaderModel, "HiZ/HiZDebugPreview.hlsl", baseDir);

    ssrTraceShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssrTraceShader", computeShaderModel, "Reflection/SSRTrace.hlsl", baseDir);

    lightCullingShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_lightCullingShader", computeShaderModel, "Lighting/LightCulling.hlsl", baseDir);
    depthMSAAResolveShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_depthMSAAResolveShader", computeShaderModel, "DepthMSAAResolve.hlsl", baseDir);
    depthNormalMSAAResolveShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_depthNormalMSAAResolveShader", computeShaderModel, "DepthMSAAResolve.hlsl", baseDir, "CSMain", "RESOLVE_MSAA_NORMAL=1");
    surfaceOutputMSAAResolveNormalShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_surfaceOutputMSAAResolveNormalShader", computeShaderModel, "SurfaceOutputMSAAResolve.hlsl", baseDir, "CSMain", "SURFACE_OUTPUT_MSAA_RESOLVE_MODE=1");
    surfaceOutputMSAAResolveVelocityShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_surfaceOutputMSAAResolveVelocityShader", computeShaderModel, "SurfaceOutputMSAAResolve.hlsl", baseDir, "CSMain", "SURFACE_OUTPUT_MSAA_RESOLVE_MODE=2");
    surfaceOutputMSAAResolveNormalVelocityShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_surfaceOutputMSAAResolveNormalVelocityShader", computeShaderModel, "SurfaceOutputMSAAResolve.hlsl", baseDir, "CSMain", "SURFACE_OUTPUT_MSAA_RESOLVE_MODE=3");

    finalPostProcessShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_finalPostProcessShader", computeShaderModel, "FinalPostProcess.hlsl", baseDir);
    taaResolveShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_taaResolveShader", computeShaderModel, "TAAResolve.hlsl", baseDir);
    luminancePass1ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_luminancePass1Shader", computeShaderModel, "ToneMapping/LuminancePass1.hlsl", baseDir);
    luminancePass2ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_luminancePass2Shader", computeShaderModel, "ToneMapping/LuminancePass2.hlsl", baseDir);

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
    EnsureComputePipeline(genHiZMipFirstBatchShaderTemplate, genHiZMipFirstBatchPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(genHiZMipFirstBatchMSAAShaderTemplate, genHiZMipFirstBatchMSAAPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(genHiZMipBatchShaderTemplate, genHiZMipBatchPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(genHiZMipOddDimShaderTemplate, genHiZMipOddDimPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(hiZCullInstancesShaderTemplate, hiZCullInstancesPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(hiZDebugPreviewShaderTemplate, hiZDebugPreviewPipeline, shaderPipelineBootstrap);

    EnsureComputePipeline(ssrTraceShaderTemplate, ssrTracePipeline, shaderPipelineBootstrap);

    EnsureComputePipeline(lightCullingShaderTemplate, lightCullingPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(depthMSAAResolveShaderTemplate, depthMSAAResolvePipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(depthNormalMSAAResolveShaderTemplate, depthNormalMSAAResolvePipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(surfaceOutputMSAAResolveNormalShaderTemplate, surfaceOutputMSAAResolveNormalPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(surfaceOutputMSAAResolveVelocityShaderTemplate, surfaceOutputMSAAResolveVelocityPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(surfaceOutputMSAAResolveNormalVelocityShaderTemplate, surfaceOutputMSAAResolveNormalVelocityPipeline, shaderPipelineBootstrap);

    EnsureComputePipeline(finalPostProcessShaderTemplate, finalPostProcessPipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(taaResolveShaderTemplate, taaResolvePipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(luminancePass1ShaderTemplate, luminancePass1Pipeline, shaderPipelineBootstrap);
    EnsureComputePipeline(luminancePass2ShaderTemplate, luminancePass2Pipeline, shaderPipelineBootstrap);
}

void RenderSystem::ReleaseComputePipelines() {
    genHiZMipFirstBatchPipeline.reset();
    genHiZMipFirstBatchMSAAPipeline.reset();
    genHiZMipBatchPipeline.reset();
    genHiZMipOddDimPipeline.reset();
    hiZCullInstancesPipeline.reset();
    hiZDebugPreviewPipeline.reset();

    ssrTracePipeline.reset();

    lightCullingPipeline.reset();
    depthMSAAResolvePipeline.reset();
    depthNormalMSAAResolvePipeline.reset();
    surfaceOutputMSAAResolveNormalPipeline.reset();
    surfaceOutputMSAAResolveVelocityPipeline.reset();
    surfaceOutputMSAAResolveNormalVelocityPipeline.reset();

    finalPostProcessPipeline.reset();
    taaResolvePipeline.reset();
    luminancePass1Pipeline.reset();
    luminancePass2Pipeline.reset();

    genHiZMipFirstBatchShaderTemplate.reset();
    genHiZMipFirstBatchMSAAShaderTemplate.reset();
    genHiZMipBatchShaderTemplate.reset();
    genHiZMipOddDimShaderTemplate.reset();
    hiZCullInstancesShaderTemplate.reset();
    hiZDebugPreviewShaderTemplate.reset();

    ssrTraceShaderTemplate.reset();

    depthMSAAResolveShaderTemplate.reset();
    depthNormalMSAAResolveShaderTemplate.reset();
    surfaceOutputMSAAResolveNormalShaderTemplate.reset();
    surfaceOutputMSAAResolveVelocityShaderTemplate.reset();
    surfaceOutputMSAAResolveNormalVelocityShaderTemplate.reset();

    finalPostProcessShaderTemplate.reset();
    taaResolveShaderTemplate.reset();
    luminancePass1ShaderTemplate.reset();
    luminancePass2ShaderTemplate.reset();
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

bool RenderSystem::CreateColorGradingLUT() {
    // cvars 로 settings 를 채우고, color grading 3D LUT image 를 생성한다.
    const ColorGradingLUTGenerator::Settings settings = MakeColorGradingLUTSettingsFromCVars();
    ColorGradingLUTGenerator::Generate(settings, colorGradingLUTImage);

    // CreateTexture 의 초기 업로드가 렌더 스레드에서 처리될 수 있으므로, 이후 갱신에 재사용할 CPU image 와 분리한다.
    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>(colorGradingLUTImage);

    // color grading 3D LUT texture 생성
    colorGradingLUTTexture = textureManager.AllocTexture("_Generated/ColorGradingLUT");
    if (!colorGradingLUTTexture->Create(RHI::TextureType::Texture3D, textureImage, Texture::Flag::HighQuality | Texture::Flag::NoMipmaps | Texture::Flag::Compute)) {
        BE_WARNLOG("RenderSystem::CreateColorGradingLUT: failed to create LUT texture\n");
        colorGradingLUTTexture.reset();
        colorGradingLUTImage.Clear();
        return false;
    }

    colorGradingLUTDirty = false;
    return true;
}

void RenderSystem::UpdateColorGradingLUT() {
    if (!colorGradingLUTTexture || !colorGradingLUTTexture->IsReadyForRendering()) {
        return;
    }

    const ColorGradingLUTGenerator::Settings settings = MakeColorGradingLUTSettingsFromCVars();
    ColorGradingLUTGenerator::Generate(settings, colorGradingLUTImage);

    if (!colorGradingLUTTexture->Update3D(0, 0, 0, 0,
        ColorGradingLUTGenerator::LUTSize,
        ColorGradingLUTGenerator::LUTSize,
        ColorGradingLUTGenerator::LUTSize,
        colorGradingLUTImage.GetFormat(),
        colorGradingLUTImage.GetPixels())) {
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
    submittedFrames.SetCount(0, false);
    submittedFrameReadIndex = 0;
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

    submittedFrames.SetCount(0, false);
    submittedFrameReadIndex = 0;
    isStoppingRenderThread = false;
}

void RenderSystem::MarkRenderCompleted(RenderFrameData *frameData) {
    assert(!IsUsingRenderThread() || RenderSystem::IsInRenderThread());

    BE1::ScopedWriteLock lock(frameSyncLock);

    // 주어진 프레임에 대한 (렌더 스레드의) 렌더링이 완료되어,
    // (메인 스레드의) 업데이트가 완료되기를 기다리는 상태로 변경.
    assert(frameData->frameSyncState == FrameSyncState::UpdateCompleted);
    frameData->frameSyncState = FrameSyncState::RenderCompleted;

    // 메인 스레드에게 렌더링이 완료되었다고 신호를 보낸다.
    renderCompletedCondition.Signal();
}

bool RenderSystem::WaitSubmittedFrame(SubmittedFrame &submittedFrame) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitSubmittedFrame");

    assert(!IsUsingRenderThread() || RenderSystem::IsInRenderThread());

    {
        BE1::ScopedWriteLock lock(frameSyncLock);

        // 제출된 프레임 데이터가 생기거나 렌더 스레드 종료 요청이 들어올 때까지 기다린다.
        updateCompletedCondition.Wait(lock, [this] {
            return submittedFrameReadIndex < submittedFrames.Count() || isStoppingRenderThread;
        });

        if (isStoppingRenderThread) {
            return false;
        }

        assert(submittedFrameReadIndex < submittedFrames.Count());

        submittedFrame = submittedFrames[submittedFrameReadIndex++];

        if (submittedFrameReadIndex == submittedFrames.Count()) {
            submittedFrames.SetCount(0, false);
            submittedFrameReadIndex = 0;
        }
    }
    return true;
}

unsigned int RenderSystem::RenderThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("RenderSystem::RenderThreadProc");

    isRenderThread = true;

    BE1::SIMD::SetDenormalFlushMode(true);

    RenderSystem *renderSystem = reinterpret_cast<RenderSystem *>(param);

    while (1) {
        PROFILER_CPU_SCOPED_EVENT("RenderThreadProcLoop");

        SubmittedFrame submittedFrame;

        // 제출된 프레임 데이터가 생길 때까지 기다린다.
        if (!renderSystem->WaitSubmittedFrame(submittedFrame)) {
            break;
        }

        // 이전에 사용이 완료된 리소스들을 해제한다.
        RHI::renderer->FreeRetiredResources(false);

        RenderFrameData *backendFrameData = submittedFrame.frameData;

        // 리소스 요청 처리
        RHI::renderer->ProcessResourceRequests(backendFrameData->GetThreadData(0));

        // 현재 백엔드 프레임 데이터로 렌더링 백엔드를 실행한다.
        renderSystem->backEnd->Execute(backendFrameData);

        // 이번 프레임에 Release/Delete 를 요청한 리소스들을 retiredResources 목록에 추가한다.
        RHI::renderer->DrainReleaseRequestsToRetireList();

        // 메인 스레드에게 렌더링이 완료되었음을 알린다.
        renderSystem->MarkRenderCompleted(backendFrameData);
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

    renderSystem->RequestScreenshot(renderContext, path);
}
