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
#include "RHI/RHI.h"
#include "Sampler.h"
#include "Render/Texture/TextureManager.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Font/FontManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/Animation/SkeletonManager.h"
#include "Render/Animation/AnimManager.h"

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

    shaderTemplateManager.Init();

    gpuFFT.Init();

    InitComputePipelines();

    convolutionBloomKernelGenerator.Init();

    materialManager.Init();

    fontManager.Init();

    skeletonManager.Init();

    animManager.Init();

    meshManager.Init();

    backEnd = new RenderBackend;
    backEnd->Init();

#ifdef USE_RENDER_THREAD
    InitRenderThread();
#endif
}

void RenderSystem::Shutdown() {
#ifdef USE_RENDER_THREAD
    ShutdownRenderThread();
#endif

    BE1::cmdSystem.RemoveCommand("screenshot");

    backEnd->Shutdown();
    SAFE_DELETE(backEnd);

    meshManager.Shutdown();

    animManager.Shutdown();

    skeletonManager.Shutdown();

    fontManager.Shutdown();

    materialManager.Shutdown();

    ReleaseComputePipelines();

    convolutionBloomKernelGenerator.Shutdown();

    gpuFFT.Shutdown();

    shaderPipelineCache.ClearAllPipelines();

    shaderTemplateManager.Shutdown();

    VertexInputLayoutFactory::Shutdown();

    textureManager.Shutdown();

    samplerManager.Shutdown();

    RHI::renderer->Shutdown();

    RHI::DestroyRenderer();
}

static SharedPtr<ShaderTemplate> CreateComputeShaderTemplate(const char *hashName, RHI::ShaderModel shaderModel, const char *filename, const char *baseDir, const char *baseDefine = nullptr) {
    BE1::Str shaderTemplateText;
    if (baseDefine && baseDefine[0]) {
        shaderTemplateText.sPrintf(
            "{\n"
            "    baseDefines {\n"
            "        %s\n"
            "    }\n"
            "    stages {\n"
            "        compute %s \"%s\" \"%s\"\n"
            "    }\n"
            "}\n",
            baseDefine, RHI::Renderer::ShaderModelName(shaderModel), filename, "CSMain");
    } else {
        shaderTemplateText.sPrintf(
            "{\n"
            "    stages {\n"
            "        compute %s \"%s\" \"%s\"\n"
            "    }\n"
            "}\n",
            RHI::Renderer::ShaderModelName(shaderModel), filename, "CSMain");
    }

    SharedPtr<ShaderTemplate> shaderTemplate = shaderTemplateManager.AllocShaderTemplate(hashName);
    if (!shaderTemplate->Create(shaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("RenderSystem::CreateComputeShaderTemplate: failed to create shader template '%s'", hashName);
    }
    return shaderTemplate;
}

static void EnsureComputePipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, SharedPtr<ShaderPipeline> &pipeline) {
    if (pipeline && pipeline->GetState() != ShaderPipeline::State::Empty) {
        return;
    }

    pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplate, ShaderPermutationKey());
}

void RenderSystem::InitComputePipelines() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::ShaderModel::SM_6_6;

    genHiZMipFirstBatchShaderTemplate = CreateComputeShaderTemplate("_genHiZMipFirstBatchShader", computeShaderModel, "GenHiZMip/GenHiZMipFirstBatch.hlsl", baseDir);
    genHiZMipFirstBatchMSAAShaderTemplate = CreateComputeShaderTemplate("_genHiZMipFirstBatchMSAAShader", computeShaderModel, "GenHiZMip/GenHiZMipFirstBatch.hlsl", baseDir, "GEN_HIZ_DEPTH_MSAA=1");
    genHiZMipBatchShaderTemplate = CreateComputeShaderTemplate("_genHiZMipBatchShader", computeShaderModel, "GenHiZMip/GenHiZMipBatch.hlsl", baseDir);
    genHiZMipOddDimShaderTemplate = CreateComputeShaderTemplate("_genHiZMipOddDimShader", computeShaderModel, "GenHiZMip/GenHiZMipOddDim.hlsl", baseDir);
    hiZCullInstancesShaderTemplate = CreateComputeShaderTemplate("_hiZCullInstancesShader", computeShaderModel, "HiZCullInstances.hlsl", baseDir);
    hiZDebugPreviewShaderTemplate = CreateComputeShaderTemplate("_hiZDebugPreviewShader", computeShaderModel, "HiZDebugPreview.hlsl", baseDir);
    depthHistoryResolveShaderTemplate = CreateComputeShaderTemplate("_depthHistoryResolveShader", computeShaderModel, "DepthHistoryResolve.hlsl", baseDir);
    depthHistoryResolveMSAAShaderTemplate = CreateComputeShaderTemplate("_depthHistoryResolveMSAAShader", computeShaderModel, "DepthHistoryResolve.hlsl", baseDir, "DEPTH_HISTORY_RESOLVE_MSAA=1");

    toneMappingShaderTemplate = CreateComputeShaderTemplate("_toneMappingShader", computeShaderModel, "ToneMapping/ToneMapping.hlsl", baseDir);
    taaResolveShaderTemplate = CreateComputeShaderTemplate("_taaResolveShader", computeShaderModel, "TAAResolve.hlsl", baseDir);
    luminancePass1ShaderTemplate = CreateComputeShaderTemplate("_luminancePass1Shader", computeShaderModel, "ToneMapping/LuminancePass1.hlsl", baseDir);
    luminancePass2ShaderTemplate = CreateComputeShaderTemplate("_luminancePass2Shader", computeShaderModel, "ToneMapping/LuminancePass2.hlsl", baseDir);

    bloomPrefilterShaderTemplate = CreateComputeShaderTemplate("_bloomPrefilterShader", computeShaderModel, "Bloom/BloomPrefilter.hlsl", baseDir);
    bloomDownsampleShaderTemplate = CreateComputeShaderTemplate("_bloomDownsampleShader", computeShaderModel, "Bloom/BloomDownsample.hlsl", baseDir);
    bloomUpsampleShaderTemplate = CreateComputeShaderTemplate("_bloomUpsampleShader", computeShaderModel, "Bloom/BloomUpsample.hlsl", baseDir);

    convolutionBloomPrefilterShaderTemplate = CreateComputeShaderTemplate("_convolutionBloomPrefilterShader", computeShaderModel, "Bloom/ConvolutionBloomPrefilter.hlsl", baseDir);
    convolutionBloomKernelPrepareShaderTemplate = CreateComputeShaderTemplate("_convolutionBloomKernelPrepareShader", computeShaderModel, "Bloom/ConvolutionBloomKernelPrepare.hlsl", baseDir);
    convolutionBloomResolveShaderTemplate = CreateComputeShaderTemplate("_convolutionBloomResolveShader", computeShaderModel, "Bloom/ConvolutionBloomResolve.hlsl", baseDir);
    convolutionBloomAperturePrepareShaderTemplate = CreateComputeShaderTemplate("_convolutionBloomAperturePrepareShader", computeShaderModel, "Bloom/ConvolutionBloomAperturePrepare.hlsl", baseDir);
    convolutionBloomReduceApertureEnergyShaderTemplate = CreateComputeShaderTemplate("_convolutionBloomReduceApertureEnergyShader", computeShaderModel, "Bloom/ConvolutionBloomReduceApertureEnergy.hlsl", baseDir);
    convolutionBloomResolveDiffractionPSFShaderTemplate = CreateComputeShaderTemplate("_convolutionBloomResolveDiffractionPSFShader", computeShaderModel, "Bloom/ConvolutionBloomResolveDiffractionPSF.hlsl", baseDir);
    convolutionBloomApplyDispersionShaderTemplate = CreateComputeShaderTemplate("_convolutionBloomApplyDispersionShader", computeShaderModel, "Bloom/ConvolutionBloomApplyDispersion.hlsl", baseDir);

    EnsureComputePipelines();
}

void RenderSystem::EnsureComputePipelines() {
    EnsureComputePipeline(genHiZMipFirstBatchShaderTemplate, genHiZMipFirstBatchPipeline);
    EnsureComputePipeline(genHiZMipFirstBatchMSAAShaderTemplate, genHiZMipFirstBatchMSAAPipeline);
    EnsureComputePipeline(genHiZMipBatchShaderTemplate, genHiZMipBatchPipeline);
    EnsureComputePipeline(genHiZMipOddDimShaderTemplate, genHiZMipOddDimPipeline);
    EnsureComputePipeline(hiZCullInstancesShaderTemplate, hiZCullInstancesPipeline);
    EnsureComputePipeline(hiZDebugPreviewShaderTemplate, hiZDebugPreviewPipeline);
    EnsureComputePipeline(depthHistoryResolveShaderTemplate, depthHistoryResolvePipeline);
    EnsureComputePipeline(depthHistoryResolveMSAAShaderTemplate, depthHistoryResolveMSAAPipeline);

    EnsureComputePipeline(toneMappingShaderTemplate, toneMappingPipeline);
    EnsureComputePipeline(taaResolveShaderTemplate, taaResolvePipeline);
    EnsureComputePipeline(luminancePass1ShaderTemplate, luminancePass1Pipeline);
    EnsureComputePipeline(luminancePass2ShaderTemplate, luminancePass2Pipeline);

    EnsureComputePipeline(bloomPrefilterShaderTemplate, bloomPrefilterPipeline);
    EnsureComputePipeline(bloomDownsampleShaderTemplate, bloomDownsamplePipeline);
    EnsureComputePipeline(bloomUpsampleShaderTemplate, bloomUpsamplePipeline);

    EnsureComputePipeline(convolutionBloomPrefilterShaderTemplate, convolutionBloomPrefilterPipeline);
    EnsureComputePipeline(convolutionBloomKernelPrepareShaderTemplate, convolutionBloomKernelPreparePipeline);
    EnsureComputePipeline(convolutionBloomResolveShaderTemplate, convolutionBloomResolvePipeline);
    EnsureComputePipeline(convolutionBloomAperturePrepareShaderTemplate, convolutionBloomAperturePreparePipeline);
    EnsureComputePipeline(convolutionBloomReduceApertureEnergyShaderTemplate, convolutionBloomReduceApertureEnergyPipeline);
    EnsureComputePipeline(convolutionBloomResolveDiffractionPSFShaderTemplate, convolutionBloomResolveDiffractionPSFPipeline);
    EnsureComputePipeline(convolutionBloomApplyDispersionShaderTemplate, convolutionBloomApplyDispersionPipeline);
}

void RenderSystem::ReleaseComputePipelines() {
    genHiZMipFirstBatchPipeline.reset();
    genHiZMipFirstBatchMSAAPipeline.reset();
    genHiZMipBatchPipeline.reset();
    genHiZMipOddDimPipeline.reset();
    hiZCullInstancesPipeline.reset();
    hiZDebugPreviewPipeline.reset();
    depthHistoryResolvePipeline.reset();
    depthHistoryResolveMSAAPipeline.reset();

    toneMappingPipeline.reset();
    taaResolvePipeline.reset();
    luminancePass1Pipeline.reset();
    luminancePass2Pipeline.reset();

    bloomPrefilterPipeline.reset();
    bloomDownsamplePipeline.reset();
    bloomUpsamplePipeline.reset();

    convolutionBloomPrefilterPipeline.reset();
    convolutionBloomKernelPreparePipeline.reset();
    convolutionBloomResolvePipeline.reset();
    convolutionBloomAperturePreparePipeline.reset();
    convolutionBloomReduceApertureEnergyPipeline.reset();
    convolutionBloomResolveDiffractionPSFPipeline.reset();
    convolutionBloomApplyDispersionPipeline.reset();

    genHiZMipFirstBatchShaderTemplate.reset();
    genHiZMipFirstBatchMSAAShaderTemplate.reset();
    genHiZMipBatchShaderTemplate.reset();
    genHiZMipOddDimShaderTemplate.reset();
    hiZCullInstancesShaderTemplate.reset();
    hiZDebugPreviewShaderTemplate.reset();
    depthHistoryResolveShaderTemplate.reset();
    depthHistoryResolveMSAAShaderTemplate.reset();

    toneMappingShaderTemplate.reset();
    taaResolveShaderTemplate.reset();
    luminancePass1ShaderTemplate.reset();
    luminancePass2ShaderTemplate.reset();

    bloomPrefilterShaderTemplate.reset();
    bloomDownsampleShaderTemplate.reset();
    bloomUpsampleShaderTemplate.reset();

    convolutionBloomPrefilterShaderTemplate.reset();
    convolutionBloomKernelPrepareShaderTemplate.reset();
    convolutionBloomResolveShaderTemplate.reset();
    convolutionBloomAperturePrepareShaderTemplate.reset();
    convolutionBloomReduceApertureEnergyShaderTemplate.reset();
    convolutionBloomResolveDiffractionPSFShaderTemplate.reset();
    convolutionBloomApplyDispersionShaderTemplate.reset();
}

void RenderSystem::UpdateFrame() {
    textureManager.UpdateFrame();

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
        BE1::PlatformCondition::Signal(updateCompletedCondition);
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

void RenderSystem::InitRenderThread() {
    frameSyncLock = BE1::PlatformSRWLock::Create();
    renderCompletedCondition = BE1::PlatformCondition::Create();
    updateCompletedCondition = BE1::PlatformCondition::Create();
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
        BE1::PlatformCondition::Signal(updateCompletedCondition);
    }

    BE1::PlatformThread::Join(renderThread);
    renderThread = nullptr;

    BE1::PlatformCondition::Destroy(renderCompletedCondition);
    BE1::PlatformCondition::Destroy(updateCompletedCondition);
    BE1::PlatformSRWLock::Destroy(frameSyncLock);

    renderCompletedCondition = nullptr;
    updateCompletedCondition = nullptr;
    frameSyncLock = nullptr;
    submittedFrames.SetCount(0, false);
    submittedFrameReadIndex = 0;
    isStoppingRenderThread = false;
}

bool RenderSystem::WaitRenderCompleted(const RenderFrameData *frameData) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitRenderCompleted");

    assert(BE1::Engine::IsInMainThread());

    if (!IsUsingRenderThread()) {
        return true;
    }

    {
        BE1::ScopedReadLock lock(frameSyncLock);

        // 주어진 프레임에 대한 (렌더 스레드의) 렌더링이 완료되기를 기다린다.
        BE1::PlatformCondition::Wait(renderCompletedCondition, frameSyncLock, false, [frameData] {
            return frameData->frameSyncState == FrameSyncState::RenderCompleted;
        });
    }
    return true;
}

void RenderSystem::MarkRenderCompleted(RenderFrameData *frameData) {
    BE1::ScopedWriteLock lock(frameSyncLock);

    // 주어진 프레임에 대한 (렌더 스레드의) 렌더링이 완료되어,
    // (메인 스레드의) 업데이트가 완료되기를 기다리는 상태로 변경.
    assert(frameData->frameSyncState == FrameSyncState::UpdateCompleted);
    frameData->frameSyncState = FrameSyncState::RenderCompleted;

    // 메인 스레드에게 렌더링이 완료되었다고 신호를 보낸다.
    BE1::PlatformCondition::Signal(renderCompletedCondition);
}

bool RenderSystem::WaitSubmittedFrame(SubmittedFrame &submittedFrame) {
    PROFILER_CPU_SCOPED_EVENT("RenderSystem::WaitSubmittedFrame");

    {
        BE1::ScopedWriteLock lock(frameSyncLock);

        // 제출된 프레임 데이터가 생기거나 렌더 스레드 종료 요청이 들어올 때까지 기다린다.
        BE1::PlatformCondition::Wait(updateCompletedCondition, frameSyncLock, true, [this] {
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

RenderContext *RenderSystem::FindRenderContextByWindowHandle(void *windowHandle) const {
    for (RenderContext *renderContext : renderContexts) {
        if (renderContext->GetWindowHandle() == windowHandle) {
            return renderContext;
        }
    }
    return nullptr;
}

void RenderSystem::DestroyRenderContext(RenderContext *renderContext) {
    if (IsUsingRenderThread()) {
        for (RenderFrameData &frameData : renderContext->frames) {
            WaitRenderCompleted(&frameData);
        }
    }

    if (renderContext == mainRenderContext) {
        mainRenderContext = nullptr;
    }

    renderContext->Shutdown();

    backEnd->DiscardGpuFrameTimings(renderContext);

    renderContexts.Remove(renderContext);
    delete renderContext;
}

void RenderSystem::RequestScreenshot(RenderContext *renderContext, const char *filename) {
    if (!renderContext) {
        return;
    }

    renderContext->RequestScreenshot(filename);
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
