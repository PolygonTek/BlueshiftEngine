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
#include "Texture.h"
#include "ShaderTemplate.h"
#include "ShaderPipelineCache.h"
#include "VertexInputLayoutFactory.h"
#include "Material.h"
#include "Font.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "Anim.h"

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

    InitFFTPSOs();
    InitPostProcessPSOs();

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

    ReleasePostProcessPSOs();
    ReleaseFFTPSOs();

    shaderPipelineCache.ClearAll();

    shaderTemplateManager.Shutdown();

    VertexInputLayoutFactory::Shutdown();

    textureManager.Shutdown();

    samplerManager.Shutdown();

    RHI::renderer->Shutdown();

    RHI::DestroyRenderer();
}

void RenderSystem::InitFFTPSOs() {
    fftComplex256PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex256ShaderTemplate, ShaderPermutationKey(), false);
    fftComplex256StagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex256StageShaderTemplate, ShaderPermutationKey(), false);
    fftComplex512PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex512ShaderTemplate, ShaderPermutationKey(), false);
    fftComplex512StagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex512StageShaderTemplate, ShaderPermutationKey(), false);
    fftComplex1024PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex1024ShaderTemplate, ShaderPermutationKey(), false);
    fftComplex1024StagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex1024StageShaderTemplate, ShaderPermutationKey(), false);
    fftComplex1024LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex1024LastStageShaderTemplate, ShaderPermutationKey(), false);

    fftReal512FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftReal512FirstStageShaderTemplate, ShaderPermutationKey(), false);
    fftReal512LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftReal512LastStageShaderTemplate, ShaderPermutationKey(), false);
    fftReal1024FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftReal1024FirstStageShaderTemplate, ShaderPermutationKey(), false);
    fftReal1024LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftReal1024LastStageShaderTemplate, ShaderPermutationKey(), false);

    ifftReal512FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftReal512FirstStageShaderTemplate, ShaderPermutationKey(), false);
    ifftReal512LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftReal512LastStageShaderTemplate, ShaderPermutationKey(), false);
    ifftReal1024FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftReal1024FirstStageShaderTemplate, ShaderPermutationKey(), false);
    ifftReal1024LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftReal1024LastStageShaderTemplate, ShaderPermutationKey(), false);

    fftDualReal256FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal256FirstStageShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal256LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal256LastStageShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal512FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal512FirstStageShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal512LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal512LastStageShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal1024FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal1024FirstStageShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal1024LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal1024LastStageShaderTemplate, ShaderPermutationKey(), false);

    ifftDualReal256FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal256FirstStageShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal256LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal256LastStageShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal512FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal512FirstStageShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal512LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal512LastStageShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal1024FirstStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal1024FirstStageShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal1024LastStagePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal1024LastStageShaderTemplate, ShaderPermutationKey(), false);

    multiplyComplexSpectrum2DPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.multiplyComplexSpectrum2DShaderTemplate, ShaderPermutationKey(), false);
    multiplyPackedRealSpectrum2DPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.multiplyPackedRealSpectrum2DShaderTemplate, ShaderPermutationKey(), false);
    multiplyPackedDualRealSpectrum2DPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.multiplyPackedDualRealSpectrum2DShaderTemplate, ShaderPermutationKey(), false);

    transposeComplex256PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex256ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex256x512PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex256x512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512x256PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex512x256ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512x1024PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex512x1024ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex1024x512PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex1024x512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex1024PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex1024ShaderTemplate, ShaderPermutationKey(), false);
}

void RenderSystem::ReleaseFFTPSOs() {
    fftComplex256PSO.reset();
    fftComplex256StagePSO.reset();
    fftComplex512PSO.reset();
    fftComplex512StagePSO.reset();
    fftComplex1024PSO.reset();
    fftComplex1024StagePSO.reset();
    fftComplex1024LastStagePSO.reset();

    fftReal512FirstStagePSO.reset();
    fftReal512LastStagePSO.reset();
    fftReal1024FirstStagePSO.reset();
    fftReal1024LastStagePSO.reset();

    ifftReal512FirstStagePSO.reset();
    ifftReal512LastStagePSO.reset();
    ifftReal1024FirstStagePSO.reset();
    ifftReal1024LastStagePSO.reset();

    fftDualReal256FirstStagePSO.reset();
    fftDualReal256LastStagePSO.reset();
    fftDualReal512FirstStagePSO.reset();
    fftDualReal512LastStagePSO.reset();
    fftDualReal1024FirstStagePSO.reset();
    fftDualReal1024LastStagePSO.reset();

    ifftDualReal256FirstStagePSO.reset();
    ifftDualReal256LastStagePSO.reset();
    ifftDualReal512FirstStagePSO.reset();
    ifftDualReal512LastStagePSO.reset();
    ifftDualReal1024FirstStagePSO.reset();
    ifftDualReal1024LastStagePSO.reset();

    multiplyComplexSpectrum2DPSO.reset();
    multiplyPackedRealSpectrum2DPSO.reset();
    multiplyPackedDualRealSpectrum2DPSO.reset();

    transposeComplex256PSO.reset();
    transposeComplex256x512PSO.reset();
    transposeComplex512x256PSO.reset();
    transposeComplex512x1024PSO.reset();
    transposeComplex1024x512PSO.reset();
    transposeComplex512PSO.reset();
    transposeComplex1024PSO.reset();
}

void RenderSystem::InitPostProcessPSOs() {
    toneMappingPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.toneMappingShaderTemplate, ShaderPermutationKey(), false);
    luminancePass1PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.luminancePass1ShaderTemplate, ShaderPermutationKey(), false);
    luminancePass2PSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.luminancePass2ShaderTemplate, ShaderPermutationKey(), false);

    bloomPrefilterPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.bloomPrefilterShaderTemplate, ShaderPermutationKey(), false);
    bloomDownsamplePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.bloomDownsampleShaderTemplate, ShaderPermutationKey(), false);
    bloomUpsamplePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.bloomUpsampleShaderTemplate, ShaderPermutationKey(), false);

    convolutionBloomPrefilterPSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.convolutionBloomPrefilterShaderTemplate, ShaderPermutationKey(), false);
    convolutionBloomKernelPreparePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.convolutionBloomKernelPrepareShaderTemplate, ShaderPermutationKey(), false);
    convolutionBloomResolvePSO = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.convolutionBloomResolveShaderTemplate, ShaderPermutationKey(), false);
}

void RenderSystem::ReleasePostProcessPSOs() {
    toneMappingPSO.reset();
    luminancePass1PSO.reset();
    luminancePass2PSO.reset();

    bloomPrefilterPSO.reset();
    bloomDownsamplePSO.reset();
    bloomUpsamplePSO.reset();

    convolutionBloomPrefilterPSO.reset();
    convolutionBloomKernelPreparePSO.reset();
    convolutionBloomResolvePSO.reset();
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
