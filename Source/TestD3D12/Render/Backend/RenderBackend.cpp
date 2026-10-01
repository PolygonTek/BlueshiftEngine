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
#include "RenderBackend.h"
#include "Image/BayerMatrix.h"
#include "Render/RenderWork/RenderWork.h"
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"
#include "Render/RenderInternal.h"
#include "Render/Texture/Texture.h"
#include "Render/Texture/TextureManager.h"
#include "Shaders/ShaderInterop.h"

RHI::FrameThreadData *RenderBackend::GetFrameThreadData() const {
    assert(frame);
    return frame->GetThreadData(0);
}

const RenderFrameSettings &RenderBackend::GetFrameSettings() const {
    assert(frame);
    return frame->GetRenderFrameSettings();
}

uint32_t RenderBackend::GetFrameIndex() const {
    assert(frame);
    return frame->GetFrameCount();
}

void RenderBackend::Init() {
#ifdef USE_RENDER_TASK
    drawGroupId = BE1::Engine::taskManager->CreateGroupId();
#endif

    InitDebugDraw();

    gpuTiming.Init();
}

void RenderBackend::Shutdown() {
    gpuTiming.Shutdown();
}

void RenderBackend::Execute(RenderFrame *frame) {
    // 이전에 사용이 완료된 리소스들을 해제한다.
    RHI::renderer->FreeRetiredResources(false);

    // 리소스 요청 처리
    RHI::renderer->ProcessResourceRequests(frame->GetThreadData(0));

    ExecuteFrame(frame);

    // 이번 프레임에 Release/Delete 를 요청한 리소스들을 retiredResources 목록에 추가한다.
    RHI::renderer->DrainReleaseRequestsToRetireList();
}

void RenderBackend::ExecuteFrame(RenderFrame *frame) {
    uint64_t renderUsec = 0;
    BE1::ScopedCpuTimer renderTimer(renderUsec, true, [this](uint64_t elapsedUsec) noexcept {
        UpdateRenderFrameTime(elapsedUsec);
    });

    // 백엔드가 처리할 프레임 데이터
    this->frame = frame;

    UpdateFramePSOs();

    frameConstantBuffer = nullptr;
    cameraConstantBuffer = nullptr;

    const void *data = frame->GetCommands()->buffer;
    bool finished = false;

    while (!finished) {
        RenderCommandId cmdId = *reinterpret_cast<const RenderCommandId *>(data);

        switch (cmdId) {
        case RenderCommandId::RenderWork:
            data = ExecuteRenderWork(data);
            continue;
        case RenderCommandId::BeginContext:
            data = ExecuteBeginContext(data);
            continue;
        case RenderCommandId::DrawCamera:
            data = ExecuteDrawCamera(data);
            continue;
        case RenderCommandId::ScreenShot:
            data = ExecuteScreenshot(data);
            continue;
        case RenderCommandId::FinalizeFrame:
            data = ExecuteFinalizeFrame(data);
            continue;
        case RenderCommandId::SwapBuffers:
            data = ExecuteSwapBuffers(data);
            continue;
        case RenderCommandId::End:
            finished = true;
            continue;
        default:
            BE_ERRLOG("RenderBackend::ExecuteFrame: invalid render command ID (%i)\n", cmdId);
            finished = true;
            continue;
        }
    }
}

void RenderBackend::UpdateRenderFrameTime(uint64_t frameUsec) {
    static constexpr int StatUpdateFrameCount = 60;

    statRenderElapsedUsec += frameUsec;
    statRenderFrameCount++;

    // 60 프레임마다 평균 렌더링 시간을 계산한다.
    if (statRenderFrameCount >= StatUpdateFrameCount) {
        statRenderAverageUsec.store((float)statRenderElapsedUsec / statRenderFrameCount);

        statRenderElapsedUsec = 0;
        statRenderFrameCount = 0;
    }
}

void RenderBackend::DiscardGpuFrameTimings(const RenderContext *renderContext) {
    gpuTiming.Discard(renderContext);
}

void RenderBackend::RecordDrawStats(uint32_t drawCount, uint64_t drawTriangleCount, uint32_t instanceTableAllocCount, uint32_t instanceTableUploadBytes) {
    if (!frame->IsStatEnabled()) {
        return;
    }

    statDrawCount += drawCount;
    statDrawTriangleCount += drawTriangleCount;
    statInstanceTableAllocCount += instanceTableAllocCount;
    statInstanceTableUploadBytes += instanceTableUploadBytes;
}

void RenderBackend::UpdateFrameStats() {
    RenderContext::Stats stats;
    stats.visibleCameraCount = frame->NumFrameCameras();
    stats.visibleObjectCount = frame->NumFrameObjects();
    stats.visibleLightCount = frame->NumFrameLights();
    stats.visiblePointLightCount = frame->NumFramePointLights();
    stats.visibleSpotLightCount = frame->NumFrameSpotLights();
    stats.visibleBoxDirLightCount = frame->NumFrameBoxDirLights();
    stats.drawCount = statDrawCount;
    stats.drawTriangleCount = statDrawTriangleCount;
    stats.instanceTableAllocCount = statInstanceTableAllocCount;
    stats.instanceTableUploadBytes = statInstanceTableUploadBytes;
    stats.shadowAtlasRenderedViewCount = statShadowAtlasRenderedViewCount;
    stats.lightCullingZBinCount = frame->NumLightCullingZBins();
    stats.lightCullingWordCount = frame->NumLightCullingWords();

    // 모든 렌더 태스크가 완료된 현재 프레임의 다이나믹 버퍼 사용량을 합산한다.
    for (uint32_t threadIndex = 0; threadIndex < frame->NumRenderTaskThreads(); ++threadIndex) {
        stats.dynamicBufferUsedBytes += frame->GetThreadData(static_cast<int32_t>(threadIndex))->GetUsedBytes();
    }

    const FrameCamera *frameCameras = frame->GetFrameCameras();
    for (uint32_t cameraIndex = 0; cameraIndex < frame->NumFrameCameras(); ++cameraIndex) {
        const FrameCamera &frameCamera = frameCameras[cameraIndex];
        if (frameCamera.frameShadowAtlas) {
            const FrameShadowAtlas &shadowAtlas = *frameCamera.frameShadowAtlas;
            stats.shadowAtlasCandidateCount += shadowAtlas.numCandidates;
            stats.shadowAtlasSkippedLightCount += shadowAtlas.numSkippedLights;
            stats.shadowAtlasResolutionDowngradeCount += shadowAtlas.numResolutionDowngrades;
            stats.shadowAtlasAllocationFailureCount += shadowAtlas.numAllocationFailures;

            // 갱신/재사용 대상 shadow view 를 집계한다. 갱신 대상의 shadow surface 는 view 별로, instancing 전에 센다.
            for (uint32_t shadowViewIndex = 0; shadowViewIndex < shadowAtlas.numShadowViews; ++shadowViewIndex) {
                const FrameLocalShadowView &view = shadowAtlas.shadowViews[shadowViewIndex];
                if (!shadowAtlas.shadows[view.shadowIndex].updateDepth) {
                    stats.shadowAtlasReusedViewCount++;
                    continue;
                }
                stats.shadowAtlasUpdateViewCount++;
                stats.shadowAtlasDrawSurfaceCount += view.renderPass.numDrawSurfs;
            }
        }

        if (frameCamera.lightCulling.wordsPerTile == 0) {
            continue;
        }

        const uint32_t threadGroupCountX = BE1::DivCeil(frameCamera.lightCulling.tileCountX, ShaderLightCullingThreadGroupSizeX);
        const uint32_t threadGroupCountY = BE1::DivCeil(frameCamera.lightCulling.tileCountY, ShaderLightCullingThreadGroupSizeY);

        stats.lightCullingThreadGroupCount += static_cast<uint64_t>(threadGroupCountX) * threadGroupCountY * frameCamera.lightCulling.wordsPerTile;
    }

    currentContext->SetStats(stats);
}

bool RenderBackend::UploadFrameConstants() {
    frameConstantBuffer = frame->GetThreadData(0)->AllocConstant(sizeof(FrameConstants));
    if (!frameConstantBuffer) {
        return false;
    }

    FrameConstants *constants = reinterpret_cast<FrameConstants *>(frameConstantBuffer->mappedPtr);
    constants->frameTime = frame->GetFrameTime();
    constants->elapsedTime = frame->GetElapsedTime();
    constants->elapsedPrevTime = frame->GetElapsedPrevTime();
    constants->frameCount = frame->GetFrameCount();
    constants->shaderMaterialTableIndex = frame->GetShaderMaterialTableBindlessIndex();
    constants->shaderPointLightTableIndex = frame->GetShaderPointLightTableBindlessIndex();
    constants->shaderSpotLightTableIndex = frame->GetShaderSpotLightTableBindlessIndex();
    constants->shaderBoxDirLightTableIndex = frame->GetShaderBoxDirLightTableBindlessIndex();
    constants->shaderLightRefTableIndex = frame->GetShaderLightRefTableBindlessIndex();
    constants->shaderLightZBinTableIndex = frame->GetShaderLightZBinTableBindlessIndex();
    constants->shaderLightTileWordTableIndex = frame->GetShaderLightTileWordTableBindlessIndex();
    constants->feedbackBufferIndex = textureStreamingFeedbackEnabled ? currentContext->textureStreamingFeedback.GetBindlessIndex() : 0;

    const RHI::Texture *dfgSumGGXTexture = renderSystem->dfgSumGGXTexture ? renderSystem->dfgSumGGXTexture->GetReadyRHITexture() : nullptr;
    constants->dfgSumGGXTextureIndex = dfgSumGGXTexture ? dfgSumGGXTexture->GetBindlessIndex(RHI::ViewType::SRV) : 0;
    const RHI::Texture *randomRotMatTexture = textureManager.randomRotMatTexture->GetRHITexture();
    constants->randomRotMatTextureIndex = randomRotMatTexture->GetBindlessIndex(RHI::ViewType::SRV);
    constants->padding1 = 0;
    constants->padding2 = 0;

    ShaderDirectionalLight &shaderDirectionalLight = constants->directionalLight;
    const FrameLight *primaryDirectionalLight = frame->GetPrimaryDirectionalLight();
    if (primaryDirectionalLight) {
        const BE1::Vec3 &direction = primaryDirectionalLight->axis[0];
        shaderDirectionalLight.direction = float3(direction.x, direction.y, direction.z);
        shaderDirectionalLight.flags = static_cast<uint32_t>(primaryDirectionalLight->flags);
        shaderDirectionalLight.lightColor = float3(primaryDirectionalLight->lightColor.r, primaryDirectionalLight->lightColor.g, primaryDirectionalLight->lightColor.b);
    } else {
        shaderDirectionalLight.direction = float3(0.0f, 0.0f, 0.0f);
        shaderDirectionalLight.flags = 0;
        shaderDirectionalLight.lightColor = float3(0.0f, 0.0f, 0.0f);
    }
    shaderDirectionalLight.padding0 = 0.0f;

    return true;
}

static void RemoveProjectionJitter(BE1::Mat4 &unjitteredViewProjMatrix, const BE1::Vec2 &jitterNDC) {
    if (jitterNDC.x == 0.0f && jitterNDC.y == 0.0f) {
        return;
    }

    for (int i = 0; i < 4; ++i) {
        unjitteredViewProjMatrix[0][i] -= unjitteredViewProjMatrix[3][i] * jitterNDC.x;
        unjitteredViewProjMatrix[1][i] -= unjitteredViewProjMatrix[3][i] * jitterNDC.y;
    }
}

bool RenderBackend::UploadCameraConstants() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::UploadCameraConstants");

    if (!currentFrameCamera) {
        return false;
    }

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();

    cameraConstantBuffer = frame->GetThreadData(0)->AllocConstant(sizeof(CameraConstants));
    if (!cameraConstantBuffer) {
        return false;
    }

    CameraConstants *constants = reinterpret_cast<CameraConstants *>(cameraConstantBuffer->mappedPtr);

    uint32_t flags = 0;
    if (currentFrameCamera->projectionOrthogonal) {
        flags |= ShaderCameraFlagProjectionOrthogonal;
    }
    if (frameSettings.lightCullingZBin) {
        flags |= ShaderCameraFlagUseLightZBin;
    }
    if (currentFrameCamera->directionalLightVisible) {
        flags |= ShaderCameraFlagUseDirectionalLight;
    }
    if (frameSettings.specularAA) {
        flags |= ShaderCameraFlagUseSpecularAA;
    }
    if (currentFrameCamera->applyAO) {
        flags |= ShaderCameraFlagUseAO;
    }

    ALIGN_AS32 BE1::Mat4 unjitteredViewProjMatrix = currentFrameCamera->viewProjMatrix;
    ALIGN_AS32 BE1::Mat4 prevUnjitteredViewProjMatrix = currentFrameCamera->prevViewProjMatrix;

    // jittered view projection matrix 에서 jittering 제거
    RemoveProjectionJitter(unjitteredViewProjMatrix, currentFrameCamera->jitterNDC);
    RemoveProjectionJitter(prevUnjitteredViewProjMatrix, currentFrameCamera->prevJitterNDC);

    BE1::simdProcessor->Memcpy(&constants->viewProjMatrix, &currentFrameCamera->viewProjMatrix, sizeof(constants->viewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->invViewProjMatrix, &currentFrameCamera->invViewProjMatrix, sizeof(constants->invViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->unjitteredViewProjMatrix, &unjitteredViewProjMatrix, sizeof(constants->unjitteredViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->prevViewProjMatrix, &currentFrameCamera->prevViewProjMatrix, sizeof(constants->prevViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->prevInvViewProjMatrix, &currentFrameCamera->prevInvViewProjMatrix, sizeof(constants->prevInvViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->prevUnjitteredViewProjMatrix, &prevUnjitteredViewProjMatrix, sizeof(constants->prevUnjitteredViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->hiZCullViewProjMatrix, &currentFrameCamera->hiZCullViewProjMatrix, sizeof(constants->hiZCullViewProjMatrix));

    for (uint32_t row = 0; row < 3; ++row) {
        const BE1::Vec4 &viewRow = currentFrameCamera->viewMatrix[row];
        constants->viewRotation.m[row][0] = viewRow.x;
        constants->viewRotation.m[row][1] = viewRow.y;
        constants->viewRotation.m[row][2] = viewRow.z;
        constants->viewRotation.m[row][3] = 0.0f;
    }

    constants->cameraOrigin = float3(currentFrameCamera->origin.x, currentFrameCamera->origin.y, currentFrameCamera->origin.z);
    constants->ndcFarZ = currentFrameCamera->projectionPolicy.GetNDCFarZ();
    constants->cameraForward = float3(currentFrameCamera->axis[0].x, currentFrameCamera->axis[0].y, currentFrameCamera->axis[0].z);
    constants->flags = flags;

    constants->ndcDepthScaleBias = float2(currentFrameCamera->ndcDepthScale, currentFrameCamera->ndcDepthBias);
    constants->projectionZ = float2(currentFrameCamera->projectionZ.x, currentFrameCamera->projectionZ.y);

    // Reconstruct directly in view space using the same jitter as the depth prepass.
    const BE1::Vec4 &projectionXY = currentFrameCamera->projectionXY;
    constants->projectionXY = float4(projectionXY.x, projectionXY.y, projectionXY.z + currentFrameCamera->jitterNDC.x, projectionXY.w + currentFrameCamera->jitterNDC.y);
    const float inverseProjectionScaleX = 1.0f / projectionXY.x;
    const float inverseProjectionScaleY = 1.0f / projectionXY.y;
    // Keep reconstruction in UV coordinates so passes can use their own resolution.
    constants->uvToView = float4(2.0f * inverseProjectionScaleX, -2.0f * inverseProjectionScaleY, (-1.0f - constants->projectionXY.z) * inverseProjectionScaleX, (1.0f - constants->projectionXY.w) * inverseProjectionScaleY);

    constants->surfaceTableIndex = frame->GetShaderSurfaceTableBindlessIndex();
    constants->numLocalLights = currentFrameCamera->numFrameLocalLights;
    constants->lightCullingLightRefTableOffset = currentFrameCamera->lightCulling.lightRefTableOffset;
    constants->lightCullingZBinTableOffset = currentFrameCamera->lightCulling.zBinTableOffset;
    constants->lightCullingTileWordTableOffset = currentFrameCamera->lightCulling.tileWordTableOffset;
    constants->lightCullingZBinScale = currentFrameCamera->lightCulling.zBinScale;
    constants->lightCullingZBinBias = currentFrameCamera->lightCulling.zBinBias;
    constants->lightCullingTileCountX = currentFrameCamera->lightCulling.tileCountX;
    constants->lightCullingTileCountY = currentFrameCamera->lightCulling.tileCountY;
    constants->lightCullingWordsPerTile = currentFrameCamera->lightCulling.wordsPerTile;
    constants->lightCullingDebugMode = frameSettings.lightCullingDebugMode;
    constants->padding0 = 0;

    const BE1::Mat3 &envLightAxis = currentFrameCamera->envLightAxis;
    constants->envLightAxis = float3x4(
        envLightAxis[0].x, envLightAxis[0].y, envLightAxis[0].z, 0.0f,
        envLightAxis[1].x, envLightAxis[1].y, envLightAxis[1].z, 0.0f,
        envLightAxis[2].x, envLightAxis[2].y, envLightAxis[2].z, 0.0f);
    constants->envLightColor = float3(currentFrameCamera->envLightColor.r, currentFrameCamera->envLightColor.g, currentFrameCamera->envLightColor.b);
    constants->envLightIntensity = currentFrameCamera->envLightIntensity;

    constants->envLightDiffuseIrradianceCubeMapIndex = currentFrameCamera->envLightDiffuseIrradianceCubeMap ? currentFrameCamera->envLightDiffuseIrradianceCubeMap->GetBindlessIndex(RHI::ViewType::SRV) : 0;
    constants->envLightSpecularCubeMapIndex = currentFrameCamera->envLightSpecularCubeMap ? currentFrameCamera->envLightSpecularCubeMap->GetBindlessIndex(RHI::ViewType::SRV) : 0;
    constants->envLightSpecularMipCount = currentFrameCamera->envLightSpecularMipCount;
    constants->aoTextureIndex = currentFrameCamera->applyAO ? currentFrameCamera->ao.textureSet.outputTexture->GetBindlessIndex(RHI::ViewType::SRV) : 0;

    // Shadow depth 기록을 마친 뒤 해당 카메라의 샘플링 데이터를 활성화한다.
    constants->csm = {};
    constants->shadowAtlas = {};

    return true;
}

void RenderBackend::UpdateFramePSOs() {
    renderSystem->GetHiZ().UpdateFramePSOs();
    renderSystem->GetScreenSpaceDepthPyramid().UpdateFramePSOs();
    frameSSRTracePSO = ShaderPipeline::GetReadyPSO(renderSystem->ssrTracePipeline);
    renderSystem->GetScreenSpaceAO().UpdateFramePSOs(frame->GetRenderFrameSettings());
    renderSystem->GetBloom().UpdateFramePSOs();
    renderSystem->GetPostLuminance().UpdateFramePSOs();

    frameMSAAResolveDepthPSO = ShaderPipeline::GetReadyPSO(renderSystem->msaaResolveDepthPipeline);
    frameMSAAResolveDepthNormalPSO = ShaderPipeline::GetReadyPSO(renderSystem->msaaResolveDepthNormalPipeline);
    frameMSAAResolveVelocityPSO = ShaderPipeline::GetReadyPSO(renderSystem->msaaResolveVelocityPipeline);
    frameLightCullingPSO = ShaderPipeline::GetReadyPSO(renderSystem->lightCullingPipeline);
    frameTAAResolvePSO = ShaderPipeline::GetReadyPSO(renderSystem->taaResolvePipeline);
    frameFinalPostProcessPSO = ShaderPipeline::GetReadyPSO(renderSystem->finalPostProcessPipeline);
}

const void *RenderBackend::ExecuteRenderWork(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteRenderWork");

    const RenderWorkRenderCommand *cmd = reinterpret_cast<const RenderWorkRenderCommand *>(data);
    RenderWork *renderWork = cmd->renderWork;
    if (!renderWork) {
        return (const void *)(cmd + 1);
    }

    RHI::FrameThreadData *frameThreadData = frame->GetThreadData(0);
    RHI::CommandList *commandList = frameThreadData->BeginCommandList(RHI::CommandQueueType::Graphics);

    // render work 의 커맨드 리스트를 기록한다.
    if (renderWork->RecordCommands(frameThreadData, commandList)) {
        commandList->CloseAndExecute();

        // 커맨드 리스트를 그래픽스 큐에 제출했음을 표시한다.
        renderWork->MarkCommandListSubmitted();
    } else {
        commandList->Close();
    }
    return (const void *)(cmd + 1);
}

const void *RenderBackend::ExecuteBeginContext(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteBeginContext");

    const BeginContextRenderCommand *cmd = reinterpret_cast<const BeginContextRenderCommand *>(data);

    currentContext = cmd->renderContext;
    frameImagePSO = cmd->imagePSO;
    frameHDROutputPSO = cmd->hdrOutputPSO;
    gpuTimingEnabled = cmd->gpuTiming;
    textureStreamingFeedbackEnabled = cmd->textureStreamingFeedback;
    swapChainOutputEnabled = cmd->swapChainOutput;
    oceanSurfaceQueryReadbackFrame = nullptr;

    statDrawCount = 0;
    statDrawTriangleCount = 0;
    statInstanceTableAllocCount = 0;
    statInstanceTableUploadBytes = 0;
    statShadowAtlasRenderedViewCount = 0;

    mainCommandList = frame->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

    if (gpuTimingEnabled) {
        gpuTiming.BeginFrame(mainCommandList, frame, currentContext);
    }

    const FrameOceanSurface *frameOceanSurface = frame->GetFrameOceanSurface();
    if (frameOceanSurface && swapChainOutputEnabled && currentContext == renderSystem->mainRenderContext) {
        renderSystem->gpuOcean.ApplySimulationSettings(frameOceanSurface->simulationSettings, frameOceanSurface->settingsRevision);

        // 메인 swap-chain 프레임마다 ocean simulation 을 한 프레임 진행한다.
        if (renderSystem->gpuOcean.Update(mainCommandList, frame->GetThreadData(0), frame->GetFrameTime())) {
            if (frameOceanSurface->surfaceQueryCount > 0) {
                oceanSurfaceQueryReadbackFrame = renderSystem->gpuOceanSurface.DispatchQueries(mainCommandList, frame->GetThreadData(0), frameOceanSurface->surfaceQueries, frameOceanSurface->surfaceQueryCount, frameOceanSurface->surfaceSettings.height);
            }
        }
    }

    if (swapChainOutputEnabled) {
        if (currentContext->frameCompositionTextureNeedsClear) {
            // 새로 생성되었거나 다시 활성화된 frameCompositionTexture 는 실제로 처음 사용할 때 한 번만 초기화한다.
            const BE1::Color4 &clearColor = currentContext->GetBaseClearColor();
            RHI::RenderPassImage outputImage = RHI::RenderPassImage::Color(
                currentContext->frameCompositionTexture, RHI::ViewID::PrimaryID,
                RHI::RenderPassImage::LoadAction::Clear, RHI::RenderPassImage::StoreAction::Store, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource)
                .WithClearValue(RHI::ClearValue::Color(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
            RHI::renderer->BeginRenderPass(mainCommandList, &outputImage, 1);
            RHI::renderer->EndRenderPass(mainCommandList);
            currentContext->frameCompositionTextureNeedsClear = false;
        }
    } else {
        // 스왑 체인 출력이 비활성화된 동안 합성 텍스처는 갱신되지 않는다.
        currentContext->frameCompositionTextureNeedsClear = true;
    }

#ifdef USE_MIPMAP_STREAMING
    if (textureStreamingFeedbackEnabled) {
        currentContext->textureStreamingFeedback.ClearBuffer(mainCommandList);
    }
#endif

    // 이번 프레임의 카메라들이 참조할 ShaderSurface 테이블 용량을 미리 확보한다.
    if (!frame->EnsureShaderSurfaceTableCapacity()) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to ensure shader surface table capacity\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 등록된 ShaderMaterial 테이블을 GPU 에 업로드한다.
    if (!frame->UploadShaderMaterialTable(frame->GetThreadData(0), mainCommandList)) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to upload shader material table\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 각 카메라 별로 참조할 local ShaderLight 테이블을 GPU 에 업로드한다.
    if (!frame->UploadShaderLightTable(frame->GetThreadData(0), mainCommandList)) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to upload shader light table\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 각 카메라 별로 visible light 참조 테이블과 z-bin 테이블을 GPU 에 업로드한다.
    if (!frame->UploadShaderLightCullingTable(frame->GetThreadData(0), mainCommandList)) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to upload shader light culling table\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 각 카메라의 visible light 화면 영역을 tile 단위 flat bit mask 로 변환한다.
    if (!DispatchLightCulling()) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to build shader light tile masks\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 이번 프레임에 사용할 상수 데이터들을 준비한다.
    UploadFrameConstants();

    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
}

static const RenderPass *FindRenderPass(const FrameCamera *frameCamera, RenderPassType passType) {
    for (uint32_t passIndex = 0; passIndex < frameCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &frameCamera->renderPasses[passIndex];
        if (renderPass->type == passType) {
            return renderPass;
        }
    }
    return nullptr;
}

static void InvalidateShadowAtlasUpdates(const FrameCamera *frameCamera) {
    const FrameShadowAtlas *shadowAtlas = frameCamera->frameShadowAtlas;
    if (!shadowAtlas) {
        return;
    }

    // 프런트엔드에서 border 를 갱신했어도 depth 기록을 실행하지 못했다면 이전 결과를 재사용할 수 없다.
    for (uint32_t shadowIndex = 0; shadowIndex < shadowAtlas->numShadows; ++shadowIndex) {
        const FrameLocalShadow &shadow = shadowAtlas->shadows[shadowIndex];
        if (shadow.updateDepth) {
            shadow.renderState->SetDepthValid(false);
        }
    }
}

const void *RenderBackend::ExecuteDrawCamera(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteDrawCamera");

    const DrawCameraRenderCommand *cmd = reinterpret_cast<const DrawCameraRenderCommand *>(data);

    currentFrameCamera = cmd->frameCamera;

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    if ((currentFrameCamera->is2D && frameSettings.skipDrawCamera2D) || (!currentFrameCamera->is2D && frameSettings.skipDrawCamera3D)) {
        InvalidateShadowAtlasUpdates(currentFrameCamera);
        return (const void *)(cmd + 1);
    }

    // 커맨드 리스트 풀에서 새로운 커맨드 리스트를 얻어온다.
    mainCommandList = frame->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

    // 현재 카메라에서 참조할 ShaderSurface 테이블을 GPU 에 업로드한다.
    if (!frame->UploadShaderSurfaceTable(frame->GetThreadData(0), mainCommandList, currentFrameCamera)) {
        BE_WARNLOG("RenderBackend::ExecuteDrawCamera: failed to upload shader surface table\n");
        InvalidateShadowAtlasUpdates(currentFrameCamera);
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 현재 카메라의 상수 데이터들을 준비한다.
    if (!UploadCameraConstants()) {
        BE_WARNLOG("RenderBackend::ExecuteDrawCamera: failed to upload camera constants\n");
        InvalidateShadowAtlasUpdates(currentFrameCamera);
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    if (!currentFrameCamera->is2D) {
        // Directional light 의 CSM depth 를 렌더링한다.
        DrawCSMPass();

        // Spot/Point light 의 shadow depth 를 shadow atlas 에 렌더링한다.
        DrawShadowAtlasPass();
    }

    BE1::Rect viewportRect = currentFrameCamera->outputRect;
    if (!currentFrameCamera->is2D) {
        viewportRect.Set(0, 0, currentFrameCamera->renderWidth, currentFrameCamera->renderHeight);
    }

    // Shadow 패스에서 사용한 viewport/scissor 를 메인 카메라 영역으로 설정한다.
    RHI::renderer->SetViewport(mainCommandList, viewportRect);
    RHI::renderer->SetScissorRect(mainCommandList, viewportRect);

    RHI::ClearFlag clearFlags = RHI::ClearFlag::None;
    BE1::Color4 clearColor = currentContext->baseClearColor;
    bool useExplicitClearColor = false;

    if (currentFrameCamera->clearMethod == RenderCamera::ClearMethod::Color) {
        clearFlags |= (RHI::ClearFlag::Color | RHI::ClearFlag::Depth);
        if (currentFrameCamera->clearColorMode == RenderCamera::ClearColorMode::Explicit) {
            clearColor = currentFrameCamera->clearColor;
            useExplicitClearColor = true;
        }
    } else if (currentFrameCamera->clearMethod == RenderCamera::ClearMethod::DepthOnly || currentFrameCamera->clearMethod == RenderCamera::ClearMethod::Skybox) {
        clearFlags |= RHI::ClearFlag::Depth;
    }

    if (currentFrameCamera->is2D) {
        DrawCamera2D(clearFlags, clearColor);
    } else {
        DrawCamera3D(clearFlags, clearColor, useExplicitClearColor);
    }

    // command list 에 기록을 마치고 실행
    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
}

void RenderBackend::DrawCamera2D(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera2D");

    if (!swapChainOutputEnabled) {
        BE_WARNLOG("RenderBackend::DrawCamera2D: swap chain output is disabled\n");
        return;
    }

    // 백버퍼에 바로 그리지 않고, frameCompositionTexture 에 그린다.
    RHI::Texture *frameCompositionTexture = currentContext->GetFrameCompositionTexture();
    const RHI::RenderPassImage::LoadAction loadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
    RHI::RenderPassImage outputImage = RHI::RenderPassImage::Color(
        frameCompositionTexture, RHI::ViewID::PrimaryID, loadAction, RHI::RenderPassImage::StoreAction::Store, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource)
        .WithClearValue(RHI::ClearValue::Color(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
    RHI::renderer->BeginRenderPass(mainCommandList, &outputImage, 1);

    for (uint32_t passIndex = 0; passIndex < currentFrameCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentFrameCamera->renderPasses[passIndex];
        if (renderPass->type == RenderPassType::Gui) {
            DrawGuiPass(renderPass);
        }
    }

    RHI::renderer->EndRenderPass(mainCommandList);
}

// AO 에서 사용할 non-MSAA depth/normal 텍스쳐를 준비한다.
bool RenderBackend::PrepareAOInputs() {
    RHI::Texture *depthTexture = currentFrameCamera->depthTexture;
    assert(depthTexture);

    if (depthTexture->GetSampleCount() == 1) {
        return true;
    }

    return DispatchMSAAResolveDepth(currentFrameCamera->ao.depthTexture, RHI::GPUResourceState::ShaderResource, currentFrameCamera->ao.normalTexture);
}

bool RenderBackend::DispatchGenHiZ() {
    return renderSystem->GetHiZ().DispatchGenerate(mainCommandList, frame->GetThreadData(0), gpuTiming, currentFrameCamera->depthTexture, currentFrameCamera->projectionPolicy, currentFrameCamera->hiZ);
}

bool RenderBackend::DispatchHiZDebugPreview() {
    return renderSystem->GetHiZ().DispatchDebugPreview(mainCommandList, frame->GetThreadData(0), cameraConstantBuffer, gpuTiming, frame->GetRenderFrameSettings().hiZDebugPreviewMipLevel, currentFrameCamera->hiZ);
}

bool RenderBackend::DispatchGenScreenSpaceDepthPyramid() {
    return renderSystem->GetScreenSpaceDepthPyramid().Dispatch(*this);
}

bool RenderBackend::DispatchScreenSpaceAO() {
    if (!currentFrameCamera->applyAO || !currentFrameCamera->needDepthNormalOutput) {
        return false;
    }

    ScreenSpaceAO &screenSpaceAO = renderSystem->GetScreenSpaceAO();
    const FrameScreenSpaceAO &ao = currentFrameCamera->ao;
    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    ScreenSpaceAO::DispatchParams params;
    if (!screenSpaceAO.PrepareDispatch(ao, frameSettings, params)) {
        return false;
    }

    if (!PrepareAOInputs()) {
        return false;
    }

    screenSpaceAO.Dispatch(*this, params);
    return true;
}

void RenderBackend::DrawCamera3D(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor, bool useExplicitClearColor) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera3D");

    bool hiZCullingPrepared = false;
    if (currentFrameCamera->hiZOcclusionCulling) {
        hiZCullingPrepared = PrepareHiZCulling();
    }

    const RHI::RenderPassImage::LoadAction depthLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
    // 불투명 surface 가 없는 프레임도 velocity 와 prepass normal 을 clear 해야 이전 프레임 값이 남지 않는다.
    const bool hasDepthPrepass = currentFrameCamera->needVelocityOutput || currentFrameCamera->needDepthNormalOutput || FindRenderPass(currentFrameCamera, RenderPassType::DepthPrepass) != nullptr;

    const bool useMSAA = currentFrameCamera->colorTexture->GetSampleCount() > 1;
    bool aoReady = false;

    // depth prepass
    if (hasDepthPrepass) {
        DrawDepthPrepass(depthLoadAction);

        aoReady = DispatchScreenSpaceAO();
    }

    // Disable AO if its commands were not recorded before opaque shading.
    // The camera command list has not been submitted yet.
    if (currentFrameCamera->applyAO && !aoReady) {
        CameraConstants *constants = reinterpret_cast<CameraConstants *>(cameraConstantBuffer->mappedPtr);
        constants->flags &= ~ShaderCameraFlagUseAO;
    }

    // Opaque 와 Skybox 를 렌더링한다.
    const RHI::RenderPassImage::LoadAction sceneDepthLoadAction = hasDepthPrepass ? RHI::RenderPassImage::LoadAction::Load : depthLoadAction;
    DrawScenePass(clearFlags, clearColor, useExplicitClearColor, sceneDepthLoadAction);

    // SSR trace 를 위한 screen-space depth pyramid 를 생성한다.
    bool ssrTraceReady = false;
    if (currentFrameCamera->generateScreenSpaceDepthPyramid) {
        if (DispatchGenScreenSpaceDepthPyramid()) {
            // SSR trace 를 수행한다.
            if (currentFrameCamera->traceSSR) {
                ssrTraceReady = DispatchSSRTrace();
            }
        }
    }

    // Ocean 이 현재 렌더 타겟을 읽는 feedback loop 없이 굴절할 수 있도록 Opaque scene color/depth 를 보존한다.
    CaptureSceneSnapshot();

    // Ocean, Transparent, Debug primitive 를 렌더링한다.
    DrawPostOpaquePass();

    // Velocity 는 shading 이 끝난 뒤 TAA 전에 resolve 한다.
    bool velocityReady = true;
    if (useMSAA && currentFrameCamera->needVelocityOutput) {
        velocityReady = DispatchMSAAResolveVelocity();
    }

    if (hiZCullingPrepared) {
        FinishHiZCulling();
    }

    if (currentFrameCamera->generateHiZ && !frame->GetRenderFrameSettings().hiZOcclusionCullingFreeze) {
        DispatchGenHiZ();
    }

    if (currentFrameCamera->hiZDebugPreview) {
        DispatchHiZDebugPreview();
    }

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    const RHI::Texture *outputOverrideTexture = nullptr;

    const bool debugGTAO = currentFrameCamera->ao.mode == RenderAOMode::GTAO && frameSettings.gtaoDebug != 0;

    if (debugGTAO && aoReady) {
        if (frameSettings.gtaoDebug > 1) {
            renderSystem->GetScreenSpaceAO().DispatchDebugPreview(*this);
        }
        outputOverrideTexture = currentFrameCamera->ao.textureSet.outputTexture;
    } else if (frameSettings.shadowDebugMode == ShaderShadowDebugModeVisibility) {
        outputOverrideTexture = useMSAA ? currentFrameCamera->resolvedColorTexture : currentFrameCamera->colorTexture;
    } else if (frameSettings.ssrDebug && ssrTraceReady) {
        outputOverrideTexture = currentFrameCamera->ssrTraceTexture;
    }

    PostProcessAndBlitToOutput(velocityReady, outputOverrideTexture);
}

void RenderBackend::DrawCSMPass() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCSMPass");

    const FrameCascadedShadow *cascadedShadow = currentFrameCamera->frameCascadedShadow;
    if (!cascadedShadow) {
        return;
    }
    assert(cascadedShadow->depthTexture);

    RHI::ConstantBuffer *shadowViewConstantBuffers[RenderFrameSettings::CSMCascadeCount] = {};
    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadedShadow->cascadeCount; cascadeIndex++) {
        const CSMCascade &cascade = cascadedShadow->cascades[cascadeIndex];
        assert(cascade.depthDSV.IsValid());

        if (cascade.renderPass.numDrawSurfs == 0) {
            continue;
        }

        RHI::ConstantBuffer *shadowViewConstantBuffer = frame->GetThreadData(0)->AllocConstant(sizeof(ShadowViewConstants));
        if (!shadowViewConstantBuffer) {
            BE_WARNLOG("RenderBackend::DrawCSMPass: failed to allocate cascade constants\n");
            return;
        }

        // 모든 상수를 resource transition 전에 준비하여 cascade 중간 실패를 방지한다.
        ShadowViewConstants *constants = reinterpret_cast<ShadowViewConstants *>(shadowViewConstantBuffer->mappedPtr);
        BE1::simdProcessor->Memcpy(&constants->viewProjMatrix, &cascade.viewProjMatrix, sizeof(constants->viewProjMatrix));
        constants->surfaceTableIndex = frame->GetShaderSurfaceTableBindlessIndex();
        constants->padding = uint3(0, 0, 0);

        shadowViewConstantBuffers[cascadeIndex] = shadowViewConstantBuffer;
    }

    const BE1::Rect shadowRect(0, 0, cascadedShadow->depthTexture->GetWidth(), cascadedShadow->depthTexture->GetHeight());
    RHI::renderer->SetViewport(mainCommandList, shadowRect);
    RHI::renderer->SetScissorRect(mainCommandList, shadowRect);

    // Texture2DArray 전체를 한 번만 DepthWrite 로 전환하여 cascade 사이의 SRV 왕복 barrier 를 없앤다.
    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(cascadedShadow->depthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::DepthWrite));

    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadedShadow->cascadeCount; cascadeIndex++) {
        const CSMCascade &cascade = cascadedShadow->cascades[cascadeIndex];

        // Caster 가 없어도 active slice 를 far depth 로 지워 이전 카메라/프레임의 그림자를 제거한다.
        RHI::RenderPassImage depthImage = RHI::RenderPassImage::DepthStencil(cascadedShadow->depthTexture, cascade.depthDSV,
            RHI::RenderPassImage::LoadAction::Clear, RHI::RenderPassImage::StoreAction::Store,
            RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::DepthWrite)
        .WithClearValue(RHI::ClearValue::DepthStencil(1.0f, 0));
        RHI::renderer->BeginRenderPass(mainCommandList, &depthImage, 1);

        DrawGenericPass(&cascade.renderPass, shadowViewConstantBuffers[cascadeIndex]);

        RHI::renderer->EndRenderPass(mainCommandList);
    }

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(cascadedShadow->depthTexture, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::ShaderResource));

    // 메인 카메라 CB 를 사용하는 GPU 명령은 아직 기록하지 않았다. Depth 기록 후에만 활성화한다.
    CameraConstants *cameraConstants = reinterpret_cast<CameraConstants *>(cameraConstantBuffer->mappedPtr);
    cameraConstants->csm = cascadedShadow->shaderData;
    cameraConstants->csm.enabled = 1;
}

void RenderBackend::DrawShadowAtlasPass() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawShadowAtlasPass");

    FrameShadowAtlas *shadowAtlas = currentFrameCamera->frameShadowAtlas;
    if (!shadowAtlas) {
        return;
    }

    if (shadowAtlas->numShadowViews == 0) {
        return;
    }

    assert(shadowAtlas->shadowTexture);
    assert(shadowAtlas->shadowDSV.IsValid());

    bool hasUpdates = false;
    bool allShadowsValid = true;

    // 갱신 대상은 depth 기록이 성공할 때까지 무효로 둔다. 유지 대상은 앞선 프레임의 최종 기록 결과를 다시 확인한다.
    for (uint32_t shadowIndex = 0; shadowIndex < shadowAtlas->numShadows; ++shadowIndex) {
        FrameLocalShadow &shadow = shadowAtlas->shadows[shadowIndex];

        if (shadow.updateDepth) {
            shadow.hasValidDepth = false;
            hasUpdates = true;
        } else {
            shadow.hasValidDepth = shadow.renderState->HasValidDepth();
            if (!shadow.hasValidDepth) {
                allShadowsValid = false;
            }
        }
    }

    RHI::FrameThreadData *frameThreadData = frame->GetThreadData(0);

    if (hasUpdates) {
        GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ShadowAtlas);

        // atlas 영역 clear 는 render pass 밖에서 DepthWrite 상태로 수행한다.
        RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(shadowAtlas->shadowTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::DepthWrite));

        ClearShadowAtlasTiles(*shadowAtlas);

        // 부분 clear 결과와 유지 영역을 보존하며, 갱신할 view 를 같은 pass 에서 그려 타일 사이의 SRV/DSV 전환을 생략한다.
        RHI::RenderPassImage depthImage = RHI::RenderPassImage::DepthStencil(shadowAtlas->shadowTexture, shadowAtlas->shadowDSV,
            RHI::RenderPassImage::LoadAction::Load, RHI::RenderPassImage::StoreAction::Store,
            RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::ShaderResource);
        RHI::renderer->BeginRenderPass(mainCommandList, &depthImage, 1);

        // 갱신 대상만 렌더링하고, 하나라도 실패하면 이번 카메라의 atlas 샘플링을 비활성화한다.
        if (!DrawShadowAtlasViews(*shadowAtlas, frameThreadData)) {
            allShadowsValid = false;
        }

        RHI::renderer->EndRenderPass(mainCommandList);
    }

    if (allShadowsValid) {
        // 갱신이 없는 프레임도 유지 중인 그림자를 조명에서 사용하도록 전체 shadow 테이블을 연결한다.
        CameraConstants *constants = reinterpret_cast<CameraConstants *>(cameraConstantBuffer->mappedPtr);
        if (!frame->UploadShaderShadowTable(frameThreadData, mainCommandList, *shadowAtlas, constants->shadowAtlas)) {
            BE_WARNLOG("RenderBackend::DrawShadowAtlasPass: failed to upload shadow tables\n");
        }
    }

    // 이번에 갱신한 shadow 의 depth 기록 결과만 공유 저장소에 반영한다.
    for (uint32_t shadowIndex = 0; shadowIndex < shadowAtlas->numShadows; ++shadowIndex) {
        const FrameLocalShadow &shadow = shadowAtlas->shadows[shadowIndex];
        if (shadow.updateDepth) {
            shadow.renderState->SetDepthValid(shadow.hasValidDepth);
        }
    }
}

void RenderBackend::ClearShadowAtlasTiles(const FrameShadowAtlas &shadowAtlas) {
    // 갱신할 타일만 지운다. shadow caster 가 없어도 타일 전체를 far depth 로 지운다.
    constexpr uint32_t rectBatchSize = 1024;
    BE1::Rect clearRects[rectBatchSize];

    for (uint32_t firstView = 0; firstView < shadowAtlas.numShadowViews;) {
        const uint32_t viewCount = BE1::Min(shadowAtlas.numShadowViews - firstView, rectBatchSize);
        uint32_t numClearRects = 0;

        for (uint32_t i = 0; i < viewCount; ++i) {
            const FrameLocalShadowView &view = shadowAtlas.shadowViews[firstView + i];

            if (shadowAtlas.shadows[view.shadowIndex].updateDepth) {
                const BE1::Rect &tileRect = view.tileRect;
                assert(tileRect.x >= 0 && tileRect.y >= 0 && tileRect.w > 0 && tileRect.h > 0);
                assert(tileRect.X2() <= shadowAtlas.size && tileRect.Y2() <= shadowAtlas.size);
                clearRects[numClearRects++] = tileRect;
            }
        }

        if (numClearRects > 0) {
            numClearRects = BE1::Rect::MergeAdjacentRects(clearRects, numClearRects);
            RHI::renderer->ClearDepthStencilRects(mainCommandList, shadowAtlas.shadowTexture, shadowAtlas.shadowDSV, RHI::ClearFlag::Depth, 1.0f, 0, clearRects, numClearRects);
        }

        firstView += viewCount;
    }
}

bool RenderBackend::DrawShadowAtlasViews(FrameShadowAtlas &shadowAtlas, RHI::FrameThreadData *frameThreadData) {
    for (uint32_t shadowIndex = 0; shadowIndex < shadowAtlas.numShadows; ++shadowIndex) {
        FrameLocalShadow &shadow = shadowAtlas.shadows[shadowIndex];
        if (!shadow.updateDepth) {
            continue;
        }

        assert(shadow.firstShadowView < shadowAtlas.numShadowViews);
        assert(shadow.numShadowViews <= shadowAtlas.numShadowViews - shadow.firstShadowView);

        for (uint32_t viewIndex = 0; viewIndex < shadow.numShadowViews; ++viewIndex) {
            const FrameLocalShadowView &shadowView = shadowAtlas.shadowViews[shadow.firstShadowView + viewIndex];
            if (shadowView.renderPass.numDrawSurfs == 0) {
                continue;
            }

            RHI::ConstantBuffer *shadowViewConstantBuffer = frameThreadData->AllocConstant(sizeof(ShadowViewConstants));
            if (!shadowViewConstantBuffer) {
                BE_WARNLOG("RenderBackend::DrawShadowAtlasViews: failed to allocate view constants\n");
                return false;
            }

            ShadowViewConstants *constants = reinterpret_cast<ShadowViewConstants *>(shadowViewConstantBuffer->mappedPtr);
            BE1::simdProcessor->Memcpy(&constants->viewProjMatrix, &shadowView.viewProjMatrix, sizeof(constants->viewProjMatrix));
            constants->surfaceTableIndex = frame->GetShaderSurfaceTableBindlessIndex();
            constants->padding = uint3(0, 0, 0);

            const BE1::Rect &tileRect = shadowView.tileRect;
            RHI::renderer->SetViewport(mainCommandList, tileRect);
            RHI::renderer->SetScissorRect(mainCommandList, tileRect);

            if (!DrawLocalShadowPass(&shadowView.renderPass, shadowViewConstantBuffer)) {
                return false;
            }

            if (frame->IsStatEnabled()) {
                statShadowAtlasRenderedViewCount++;
            }
        }

        // 모든 view 의 depth 기록이 성공했으므로 이번 프레임의 depth 를 사용할 수 있다.
        // shadow caster 가 없는 타일도 far depth 로 clear 되었으므로 유효하다.
        shadow.hasValidDepth = true;
    }

    return true;
}

bool RenderBackend::DrawLocalShadowPass(const RenderPass *renderPass, const RHI::ConstantBuffer *viewConstantBuffer) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawLocalShadowPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs == 0) {
        return true;
    }

    // 공용 draw 경로가 생략할 수 있는 PSO/instance 할당을 먼저 확인하여 불완전한 atlas 의 활성화를 방지한다.
    // 준비한 instance table 은 그대로 전달하므로 공용 draw 경로에서 다시 할당/업로드하지 않는다.
    localShadowDrawBatches.SetCount(0, false);
    uint32_t instanceTableAllocCount = 0;
    uint32_t instanceTableUploadBytes = 0;
    bool drawsReady = true;

    for (uint32_t drawSurfIndex = 0; drawSurfIndex < numDrawSurfs;) {
        if (!renderPass->drawSurfs[drawSurfIndex]->pso) {
            drawsReady = false;
            break;
        }

        const uint32_t batchEnd = FindSurfaceInstanceBatchEnd(renderPass->drawSurfs, numDrawSurfs, drawSurfIndex);
        SurfaceInstanceBatch &batch = localShadowDrawBatches.Alloc();
        if (!PrepareSurfaceInstanceBatch(frame->GetThreadData(0), &renderPass->drawSurfs[drawSurfIndex], drawSurfIndex, batchEnd - drawSurfIndex, batch, instanceTableAllocCount, instanceTableUploadBytes)) {
            drawsReady = false;
            break;
        }
        drawSurfIndex = batchEnd;
    }

    RecordDrawStats(0, 0, instanceTableAllocCount, instanceTableUploadBytes);

    if (!drawsReady) {
        return false;
    }

    DrawGenericSurfaces(renderPass->drawSurfs, numDrawSurfs, viewConstantBuffer, localShadowDrawBatches.Ptr(), localShadowDrawBatches.Count());
    return true;
}

void RenderBackend::DrawDepthPrepass(RHI::RenderPassImage::LoadAction depthLoadAction) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawDepthPrepass");

    RHI::RenderPassImage renderPassImages[3];
    uint32_t renderPassImageCount = 0;

    if (currentFrameCamera->needVelocityOutput) {
        renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::Color(
            currentFrameCamera->velocityTexture, RHI::ViewID::PrimaryID, RHI::RenderPassImage::LoadAction::Clear, RHI::RenderPassImage::StoreAction::Store,
            RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource).WithClearValue(RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f));
    }

    if (currentFrameCamera->needDepthNormalOutput) {
        const RHI::Texture *normalTexture = currentFrameCamera->normalTexture;
        assert(normalTexture->GetSampleCount() == currentFrameCamera->depthTexture->GetSampleCount());

        // AO consumes view normals before opaque writes SSR's shading normals to this attachment.
        renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::Color(
            normalTexture, RHI::ViewID::PrimaryID, RHI::RenderPassImage::LoadAction::Clear, RHI::RenderPassImage::StoreAction::Store,
            RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource).WithClearValue(RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f));
    }

    renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::DepthStencil(currentFrameCamera->depthTexture, RHI::ViewID::PrimaryID, depthLoadAction, RHI::RenderPassImage::StoreAction::Store,
        RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::ShaderResource)
        .WithClearValue(RHI::ClearValue::DepthStencil(currentFrameCamera->projectionPolicy.GetDepthClearValue(), 0));
    RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, renderPassImageCount);

    for (uint32_t passIndex = 0; passIndex < currentFrameCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentFrameCamera->renderPasses[passIndex];
        if (renderPass->type == RenderPassType::DepthPrepass) {
            DrawGenericPass(renderPass, cameraConstantBuffer);
        }
    }

    RHI::renderer->EndRenderPass(mainCommandList);
}

struct RenderPassImagesDesc {
    RHI::RenderPassImage::LoadAction        colorLoadAction;
    RHI::RenderPassImage::LoadAction        surfaceOutputLoadAction;
    RHI::RenderPassImage::LoadAction        depthLoadAction;
    RHI::GPUResourceState                   renderTargetBeforeState;
    RHI::GPUResourceState                   renderTargetAfterState;
    RHI::GPUResourceState                   depthBeforeState;
    RHI::GPUResourceState                   depthAfterState;
    const RHI::Texture *colorResolveTexture;
    const RHI::Texture *depthResolveTexture;
    RHI::RenderPassImage::DepthResolveMode  depthResolveMode;
    bool                                    outputVelocity;
};

// Camera3D render pass 에서 사용할 color, surface output, depth attachment 를 구성한다.
// 각 attachment 의 종료 상태를 다음 pass 의 시작 상태와 연결해 불필요한 resource transition 을 피하고,
// 지정된 resolve texture 로 MSAA color/depth 를 resolve 한다.
static uint32_t BuildCamera3DRenderPassImages(const FrameCamera *frameCamera, const RenderPassImagesDesc &desc, const RHI::ClearValue *clearColorValue, RHI::RenderPassImage renderPassImages[6]) {
    uint32_t renderPassImageCount = 0;

    const bool useMSAA = frameCamera->colorTexture->GetSampleCount() > 1;

    // color
    RHI::RenderPassImage colorImage = RHI::RenderPassImage::Color(
        frameCamera->colorTexture, RHI::ViewID::PrimaryID, desc.colorLoadAction, RHI::RenderPassImage::StoreAction::Store, desc.renderTargetBeforeState, desc.renderTargetAfterState);
    if (clearColorValue && desc.colorLoadAction == RHI::RenderPassImage::LoadAction::Clear) {
        colorImage = colorImage.WithClearValue(*clearColorValue);
    }
    renderPassImages[renderPassImageCount++] = colorImage;

    // normal
    if (frameCamera->needNormalOutput) {
        RHI::RenderPassImage normalImage = RHI::RenderPassImage::Color(
            frameCamera->normalTexture, RHI::ViewID::PrimaryID, desc.surfaceOutputLoadAction, RHI::RenderPassImage::StoreAction::Store, desc.renderTargetBeforeState, desc.renderTargetAfterState);
        if (desc.surfaceOutputLoadAction == RHI::RenderPassImage::LoadAction::Clear) {
            normalImage = normalImage.WithClearValue(RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f));
        }
        renderPassImages[renderPassImageCount++] = normalImage;
    }

    // velocity
    if (desc.outputVelocity) {
        // Prepass 의 velocity 를 보존하고 Ocean 등 후속 패스의 출력만 덧쓴다.
        renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::Color(frameCamera->velocityTexture, RHI::ViewID::PrimaryID,
            RHI::RenderPassImage::LoadAction::Load, RHI::RenderPassImage::StoreAction::Store,
            RHI::GPUResourceState::ShaderResource, desc.renderTargetAfterState);
    }

    // depth
    // A reused texture can retain the optimized clear value of a different projection policy.
    renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::DepthStencil(
        frameCamera->depthTexture, RHI::ViewID::PrimaryID, desc.depthLoadAction, RHI::RenderPassImage::StoreAction::Store, desc.depthBeforeState, RHI::GPUResourceState::DepthWrite, desc.depthAfterState)
        .WithClearValue(RHI::ClearValue::DepthStencil(frameCamera->projectionPolicy.GetDepthClearValue(), 0));

    // MSAA resolve
    if (useMSAA) {
        if (desc.colorResolveTexture) {
            renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::ResolveColor(desc.colorResolveTexture, RHI::ViewID::PrimaryID, 0);
        }
        if (desc.depthResolveTexture) {
            renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::ResolveDepth(desc.depthResolveTexture, RHI::ViewID::PrimaryID, desc.depthResolveMode);
        }
    }

    return renderPassImageCount;
}

void RenderBackend::DrawScenePass(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor, bool useExplicitClearColor, RHI::RenderPassImage::LoadAction depthLoadAction) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawScenePass");

    RHI::RenderPassImage::LoadAction colorLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
    if (currentFrameCamera->clearMethod == RenderCamera::ClearMethod::Skybox) {
        colorLoadAction = RHI::RenderPassImage::LoadAction::DontCare;
    }

    const bool captureSceneSnapshot = currentFrameCamera->oceanSurfacePSO && currentFrameCamera->snapshotColorTexture && currentFrameCamera->snapshotDepthTexture;
    const RHI::ClearValue clearColorValue = RHI::ClearValue::Color(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    const RenderPassImagesDesc passDesc = {
        colorLoadAction,
        RHI::RenderPassImage::LoadAction::Clear,
        depthLoadAction,
        RHI::GPUResourceState::ShaderResource,
        RHI::GPUResourceState::RenderTarget,
        RHI::GPUResourceState::ShaderResource,
        RHI::GPUResourceState::DepthWrite,
        captureSceneSnapshot ? currentFrameCamera->snapshotColorTexture : nullptr,
        captureSceneSnapshot ? currentFrameCamera->snapshotDepthTexture : nullptr,
        currentFrameCamera->projectionPolicy.reverseZ ? RHI::RenderPassImage::DepthResolveMode::Max : RHI::RenderPassImage::DepthResolveMode::Min,
        false
    };
    RHI::RenderPassImage renderPassImages[6];
    const uint32_t renderPassImageCount = BuildCamera3DRenderPassImages(currentFrameCamera, passDesc, useExplicitClearColor ? &clearColorValue : nullptr, renderPassImages);

    RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, renderPassImageCount);

    if (const RenderPass *opaquePass = FindRenderPass(currentFrameCamera, RenderPassType::Opaque)) {
        DrawGenericPass(opaquePass, cameraConstantBuffer);
    }

    if (const RenderPass *skyboxPass = FindRenderPass(currentFrameCamera, RenderPassType::Skybox)) {
        DrawSkyPass(skyboxPass);
    }

    RHI::renderer->EndRenderPass(mainCommandList);
}

void RenderBackend::CaptureSceneSnapshot() {
    RHI::Texture *snapshotColorTexture = currentFrameCamera->snapshotColorTexture;
    RHI::Texture *snapshotDepthTexture = currentFrameCamera->snapshotDepthTexture;

    if (!currentFrameCamera->oceanSurfacePSO || !snapshotColorTexture || !snapshotDepthTexture || currentFrameCamera->colorTexture->GetSampleCount() > 1) {
        return;
    }

    RHI::Texture *sourceColorTexture = currentFrameCamera->colorTexture;
    RHI::Texture *sourceDepthTexture = currentFrameCamera->depthTexture;
    assert(sourceColorTexture->GetSampleCount() == 1 && sourceDepthTexture->GetSampleCount() == 1);
    assert(sourceColorTexture->GetWidth() == sourceDepthTexture->GetWidth() && sourceColorTexture->GetHeight() == sourceDepthTexture->GetHeight());

    const bool downsampleSnapshot = snapshotColorTexture->GetWidth() != sourceColorTexture->GetWidth() || snapshotColorTexture->GetHeight() != sourceColorTexture->GetHeight();
    if (downsampleSnapshot) {
        assert(snapshotColorTexture->GetWidth() <= sourceColorTexture->GetWidth() && snapshotColorTexture->GetHeight() <= sourceColorTexture->GetHeight());
        assert(snapshotDepthTexture->GetWidth() == snapshotColorTexture->GetWidth() && snapshotDepthTexture->GetHeight() == snapshotColorTexture->GetHeight());
    } else {
        assert(snapshotDepthTexture->GetWidth() == sourceDepthTexture->GetWidth() && snapshotDepthTexture->GetHeight() == sourceDepthTexture->GetHeight());
    }

    if (downsampleSnapshot) {
        if (!currentFrameCamera->sceneSnapshotDownsamplePSO ||
            !snapshotColorTexture->HasValidView(RHI::ViewType::RTV) ||
            !snapshotDepthTexture->HasValidView(RHI::ViewType::RTV)) {
            return;
        }
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SceneSnapshot);

    RHI::GPUBarrier captureBarriers[4];
    uint32_t captureBarrierCount = 0;
    captureBarriers[captureBarrierCount++] = RHI::Renderer::MakeTextureBarrier(sourceColorTexture, RHI::GPUResourceState::RenderTarget, downsampleSnapshot ? RHI::GPUResourceState::ShaderResource : RHI::GPUResourceState::CopySrc);
    captureBarriers[captureBarrierCount++] = RHI::Renderer::MakeTextureBarrier(sourceDepthTexture, RHI::GPUResourceState::DepthWrite, downsampleSnapshot ? RHI::GPUResourceState::ShaderResource : RHI::GPUResourceState::CopySrc);
    if (!downsampleSnapshot) {
        captureBarriers[captureBarrierCount++] = RHI::Renderer::MakeTextureBarrier(snapshotColorTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::CopyDst);
        captureBarriers[captureBarrierCount++] = RHI::Renderer::MakeTextureBarrier(snapshotDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::CopyDst);
    }
    RHI::renderer->Barrier(mainCommandList, captureBarriers, captureBarrierCount);

    if (downsampleSnapshot) {
        const BE1::Rect snapshotRect(0, 0, snapshotColorTexture->GetWidth(), snapshotColorTexture->GetHeight());
        RHI::RenderPassImage snapshotImages[] = {
            RHI::RenderPassImage::Color(snapshotColorTexture, RHI::ViewID::PrimaryID,
                RHI::RenderPassImage::LoadAction::DontCare, RHI::RenderPassImage::StoreAction::Store, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource),
            RHI::RenderPassImage::Color(snapshotDepthTexture, RHI::ViewID::PrimaryID,
                RHI::RenderPassImage::LoadAction::DontCare, RHI::RenderPassImage::StoreAction::Store, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource)
        };
        RHI::renderer->BeginRenderPass(mainCommandList, snapshotImages, COUNT_OF(snapshotImages));
        RHI::renderer->SetViewport(mainCommandList, snapshotRect);
        RHI::renderer->SetScissorRect(mainCommandList, snapshotRect);
        RHI::renderer->SetPSO(mainCommandList, currentFrameCamera->sceneSnapshotDownsamplePSO);
        RHI::renderer->SetTexture(mainCommandList, 0, false, sourceColorTexture);
        RHI::renderer->SetTexture(mainCommandList, 1, false, sourceDepthTexture);
        RHI::renderer->Draw(mainCommandList, 3, 0);

        RecordDrawStats(1, 1, 0, 0);

        RHI::renderer->EndRenderPass(mainCommandList);

        const BE1::Rect sourceRect(0, 0, sourceColorTexture->GetWidth(), sourceColorTexture->GetHeight());
        RHI::renderer->SetViewport(mainCommandList, sourceRect);
        RHI::renderer->SetScissorRect(mainCommandList, sourceRect);
    } else {
        RHI::renderer->CopyTexture(mainCommandList, snapshotColorTexture, 0, 0, 0, 0, 0, sourceColorTexture, 0, 0, 0, 0, 0, sourceColorTexture->GetWidth(), sourceColorTexture->GetHeight(), 1);
        RHI::renderer->CopyTexture(mainCommandList, snapshotDepthTexture, 0, 0, 0, 0, 0, sourceDepthTexture, 0, 0, 0, 0, 0, sourceDepthTexture->GetWidth(), sourceDepthTexture->GetHeight(), 1);
    }

    RHI::GPUBarrier finishBarriers[4];
    uint32_t finishBarrierCount = 0;
    finishBarriers[finishBarrierCount++] = RHI::Renderer::MakeTextureBarrier(sourceColorTexture, downsampleSnapshot ? RHI::GPUResourceState::ShaderResource : RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::RenderTarget);
    finishBarriers[finishBarrierCount++] = RHI::Renderer::MakeTextureBarrier(sourceDepthTexture, downsampleSnapshot ? RHI::GPUResourceState::ShaderResource : RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::DepthWrite);
    if (!downsampleSnapshot) {
        finishBarriers[finishBarrierCount++] = RHI::Renderer::MakeTextureBarrier(snapshotColorTexture, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource);
        finishBarriers[finishBarrierCount++] = RHI::Renderer::MakeTextureBarrier(snapshotDepthTexture, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource);
    }
    RHI::renderer->Barrier(mainCommandList, finishBarriers, finishBarrierCount);
}

void RenderBackend::DrawPostOpaquePass() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawPostOpaquePass");

    const RenderPassImagesDesc passDesc = {
        RHI::RenderPassImage::LoadAction::Load,
        RHI::RenderPassImage::LoadAction::Load,
        RHI::RenderPassImage::LoadAction::Load,
        RHI::GPUResourceState::RenderTarget,
        RHI::GPUResourceState::ShaderResource,
        RHI::GPUResourceState::DepthWrite,
        RHI::GPUResourceState::ShaderResource,
        currentFrameCamera->resolvedColorTexture,
        nullptr,
        RHI::RenderPassImage::DepthResolveMode::None,
        currentFrameCamera->needVelocityOutput
    };
    RHI::RenderPassImage renderPassImages[6];
    const uint32_t renderPassImageCount = BuildCamera3DRenderPassImages(currentFrameCamera, passDesc, nullptr, renderPassImages);

    RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, renderPassImageCount);

    // GpuOcean surface 는 Transparent surface 보다 먼저 렌더링한다.
    DrawGpuOcean();

    if (const RenderPass *transparentPass = FindRenderPass(currentFrameCamera, RenderPassType::Transparent)) {
        DrawGenericPass(transparentPass, cameraConstantBuffer);
    }

    DrawDebugPrimitives();

    RHI::renderer->EndRenderPass(mainCommandList);
}

void RenderBackend::PostProcessAndBlitToOutput(bool velocityReady, const RHI::Texture *outputOverrideTexture) {
    currentPostProcessInputTexture = currentFrameCamera->resolvedColorTexture ? currentFrameCamera->resolvedColorTexture : currentFrameCamera->colorTexture;
    const RHI::Texture *finalColorTexture = currentPostProcessInputTexture;

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    if (outputOverrideTexture) {
        finalColorTexture = outputOverrideTexture;
    } else if (!frameSettings.skipPostProcess && !BE1::HasFlag(currentFrameCamera->flags, RenderCamera::Flag::SkipPostProcess)) {
        bool depthHistoryReady = true;
        if (currentFrameCamera->taaResolve && currentFrameCamera->depthTexture->GetSampleCount() > 1) {
            depthHistoryReady = DispatchMSAAResolveDepthHistory();
        }

        if (velocityReady && depthHistoryReady && DispatchTAAResolve()) {
            currentPostProcessInputTexture = currentFrameCamera->colorHistoryTextures[currentFrameCamera->currentHistoryTextureIndex];
            finalColorTexture = currentPostProcessInputTexture;
        }

        const bool useToneMapping = DispatchLuminance();
        const bool useBloom = DispatchBloom();

        if (DispatchFinalPostProcess(useToneMapping, useBloom)) {
            finalColorTexture = currentFrameCamera->postProcessColorTexture;
        }
    }

    if (currentFrameCamera->outputTexture) {
        if (finalColorTexture != currentFrameCamera->outputTexture) {
            BlitTextureToRenderTarget(finalColorTexture, currentFrameCamera->outputTexture, currentFrameCamera->outputViewID, currentFrameCamera->outputBeforeState, currentFrameCamera->outputAfterState, currentFrameCamera->outputRect, currentFrameCamera->outputBlitPSO);
        }
    } else if (swapChainOutputEnabled) {
        // 백버퍼에 바로 blit 하지 않고, frameCompositionTexture 에 먼저 blit 한다.
        RHI::Texture *frameCompositionTexture = currentContext->GetFrameCompositionTexture();
        BlitTextureToRenderTarget(finalColorTexture, frameCompositionTexture, RHI::ViewID::PrimaryID, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource, currentFrameCamera->outputRect, frameImagePSO);
    } else {
        BE_WARNLOG("RenderBackend::PostProcessAndBlitToOutput: no output render target was specified\n");
    }
    currentPostProcessInputTexture = nullptr;
}

void RenderBackend::BlitTextureToRenderTarget(const RHI::Texture *srcTexture, const RHI::Texture *renderTargetTexture, RHI::ViewID renderTargetViewID,
    RHI::GPUResourceState renderTargetBeforeState, RHI::GPUResourceState renderTargetAfterState, const BE1::Rect &destRect, RHI::PipelineState *imagePSO) {
    if (!srcTexture || !renderTargetTexture || !imagePSO) {
        return;
    }

    RHI::TextureViewInfo renderTargetViewInfo;
    if (!renderTargetTexture->GetViewInfo(RHI::ViewType::RTV, renderTargetViewID, renderTargetViewInfo)) {
        return;
    }

    const uint32_t w = renderTargetViewInfo.width;
    const uint32_t h = renderTargetViewInfo.height;
    if (w == 0 || h == 0 || destRect.w <= 0 || destRect.h <= 0) {
        return;
    }

    const int outputX0 = BE1::Max(destRect.x, 0);
    const int outputY0 = BE1::Max(destRect.y, 0);
    const int outputX1 = BE1::Min(destRect.x + destRect.w, static_cast<int>(w));
    const int outputY1 = BE1::Min(destRect.y + destRect.h, static_cast<int>(h));
    if (outputX1 <= outputX0 || outputY1 <= outputY0) {
        return;
    }

    const uint32_t outputW = static_cast<uint32_t>(outputX1 - outputX0);
    const uint32_t outputH = static_cast<uint32_t>(outputY1 - outputY0);

    FullscreenImageParams imageParams = {};
    imageParams.texCoordScale = float2(static_cast<float>(outputW) / destRect.w, static_cast<float>(outputH) / destRect.h);
    imageParams.texCoordOffset = float2(static_cast<float>(outputX0 - destRect.x) / destRect.w, static_cast<float>(outputY0 - destRect.y) / destRect.h);

    if (BE1::HasFlag(currentFrameCamera->flags, RenderCamera::Flag::FlipOutputTextureY)) {
        imageParams.texCoordScale.y = -imageParams.texCoordScale.y;
        imageParams.texCoordOffset.y = 1.0f - imageParams.texCoordOffset.y;
    }

    BE1::Rect outputRect(outputX0, outputY0, outputW, outputH);

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::BlitToOutput);

    // RTV 전체를 덮어쓰는 경우 이전 내용을 보존하지 않아 초기화되지 않은 렌더 타겟도 안전하게 사용할 수 있다.
    const bool overwriteWholeTarget = outputX0 == 0 && outputY0 == 0 && outputW == w && outputH == h;
    const RHI::RenderPassImage::LoadAction loadAction = overwriteWholeTarget ? RHI::RenderPassImage::LoadAction::DontCare : RHI::RenderPassImage::LoadAction::Load;
    RHI::RenderPassImage outputImage = RHI::RenderPassImage::Color(
        renderTargetTexture, renderTargetViewID, loadAction, RHI::RenderPassImage::StoreAction::Store, renderTargetBeforeState, renderTargetAfterState);
    RHI::renderer->BeginRenderPass(mainCommandList, &outputImage, 1);
    RHI::renderer->SetViewport(mainCommandList, outputRect);
    RHI::renderer->SetScissorRect(mainCommandList, outputRect);
    RHI::renderer->SetPSO(mainCommandList, imagePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, srcTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &imageParams, sizeof(imageParams), 0);
    RHI::renderer->Draw(mainCommandList, 3, 0);
    RecordDrawStats(1, 1, 0, 0);
    RHI::renderer->EndRenderPass(mainCommandList);
}

void RenderBackend::BlitTextureToSwapChain(const RHI::Texture *srcTexture, const BE1::Rect &destRect) {
    if (!srcTexture) {
        return;
    }

    const RHI::SwapChain::ColorSpace outputColorSpace = currentContext->GetSwapChain()->GetColorSpace();
    RHI::PipelineState *outputPSO = RHI::SwapChain::IsHDRColorSpace(outputColorSpace) ? frameHDROutputPSO : frameImagePSO;
    if (!outputPSO) {
        return;
    }

    const uint32_t w = currentContext->GetWidth();
    const uint32_t h = currentContext->GetHeight();
    if (w == 0 || h == 0) {
        return;
    }

    if (destRect.w <= 0 || destRect.h <= 0) {
        return;
    }

    const int outputX0 = BE1::Max(destRect.x, 0);
    const int outputY0 = BE1::Max(destRect.y, 0);
    const int outputX1 = BE1::Min(destRect.x + destRect.w, static_cast<int>(w));
    const int outputY1 = BE1::Min(destRect.y + destRect.h, static_cast<int>(h));
    if (outputX1 <= outputX0 || outputY1 <= outputY0) {
        return;
    }

    const uint32_t outputW = static_cast<uint32_t>(outputX1 - outputX0);
    const uint32_t outputH = static_cast<uint32_t>(outputY1 - outputY0);

    FullscreenImageParams imageParams = {};
    imageParams.texCoordScale = float2(static_cast<float>(outputW) / destRect.w, static_cast<float>(outputH) / destRect.h);
    imageParams.texCoordOffset = float2(static_cast<float>(outputX0 - destRect.x) / destRect.w, static_cast<float>(outputY0 - destRect.y) / destRect.h);

    BE1::Rect outputRect(outputX0, outputY0, outputW, outputH);

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::BlitToOutput);

    RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, nullptr);
    RHI::renderer->SetViewport(mainCommandList, outputRect);
    RHI::renderer->SetScissorRect(mainCommandList, outputRect);
    RHI::renderer->SetPSO(mainCommandList, outputPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, srcTexture);
    if (outputColorSpace == RHI::SwapChain::ColorSpace::BT2020PQ) {
        const RenderContext::DisplayOutputSettings &displayOutputSettings = currentContext->GetDisplayOutputSettings();
        HDR10OutputParams hdr10OutputParams = {};
        hdr10OutputParams.texCoordScale = imageParams.texCoordScale;
        hdr10OutputParams.texCoordOffset = imageParams.texCoordOffset;
        hdr10OutputParams.paperWhiteNits = displayOutputSettings.paperWhiteNits;
        hdr10OutputParams.maxDisplayLinearValue = displayOutputSettings.maxDisplayLinearValue;
        RHI::renderer->SetConstants(mainCommandList, 0, &hdr10OutputParams, sizeof(hdr10OutputParams), 0);
    } else if (outputColorSpace == RHI::SwapChain::ColorSpace::DisplayP3LinearEDR) {
        const RenderContext::DisplayOutputSettings &displayOutputSettings = currentContext->GetDisplayOutputSettings();
        DisplayP3EDROutputParams displayP3EDROutputParams = {};
        displayP3EDROutputParams.texCoordScale = imageParams.texCoordScale;
        displayP3EDROutputParams.texCoordOffset = imageParams.texCoordOffset;
        displayP3EDROutputParams.maxDisplayLinearValue = displayOutputSettings.maxDisplayLinearValue;
        RHI::renderer->SetConstants(mainCommandList, 0, &displayP3EDROutputParams, sizeof(displayP3EDROutputParams), 0);
    } else {
        RHI::renderer->SetConstants(mainCommandList, 0, &imageParams, sizeof(imageParams), 0);
    }
    RHI::renderer->Draw(mainCommandList, 3, 0);
    RecordDrawStats(1, 1, 0, 0);
    RHI::renderer->EndRenderPass(mainCommandList);
}

static void CopySDRScreenshotRegion(const BE1::Image &srcImage, const BE1::Rect &srcRect, BE1::Image &dstImage) {
    static constexpr int bpp = 3;
    const int srcPitch = srcImage.GetWidth() * bpp;
    const int dstPitch = dstImage.GetWidth() * bpp;
    const byte *src = srcImage.GetPixels() + srcRect.y * srcPitch + srcRect.x * bpp;
    byte *dst = dstImage.GetPixels();

    for (int y = 0; y < srcRect.h; ++y) {
        BE1::simdProcessor->Memcpy(dst, src, dstPitch);
        src += srcPitch;
        dst += dstPitch;
    }
}

static void ConvertHDRScreenshotRegionToSDR(const BE1::Image &srcImage, const BE1::Rect &srcRect, float maxDisplayLinearValue, BE1::Image &dstImage) {
    const float whiteLevel = BE1::Max(maxDisplayLinearValue, 1.0f);
    const float whiteLevelSquared = whiteLevel * whiteLevel;

    const BE1::half *src = reinterpret_cast<const BE1::half *>(srcImage.GetPixels());
    const int sourceWidth = srcImage.GetWidth();
    byte *dst = dstImage.GetPixels();

    for (int y = 0; y < srcRect.h; ++y) {
        const int srcY = srcRect.y + y;
        const BE1::half *srcPtr = src + (srcY * sourceWidth + srcRect.x) * 4;

        for (int x = 0; x < srcRect.w; ++x, srcPtr += 4, dst += 3) {
            BE1::Color3 linearColor;
            linearColor.r = BE1::Max(static_cast<float>(srcPtr[0]), 0.0f);
            linearColor.g = BE1::Max(static_cast<float>(srcPtr[1]), 0.0f);
            linearColor.b = BE1::Max(static_cast<float>(srcPtr[2]), 0.0f);

            // HDR color 의 luminance 를 extended Reinhard 커브로 SDR 범위에 톤 매핑하기 위해 luminanceScale 을 계산한다.
            const float luminance = BE1::Image::GetLuminance(linearColor);
            const float mappedLuminance = luminance * (1.0f + luminance / whiteLevelSquared) / (1.0f + luminance);
            const float luminanceScale = luminance > 0.0f ? mappedLuminance / luminance : 0.0f;

            // 원본 화면 좌표로 4x4 Bayer 패턴을 반복하여 8 비트 양자화 간격에 맞춘 dither 를 구한다.
            const float dither = BE1::BayerMatrix<4>::GetDitherOffset(srcRect.x + x, srcY, 1.0f / 255.0f);

            // sRGB 변환 후 dither 를 더해 8 비트 저장 시 나타나는 밴딩을 완화한다.
            const float sRGBR = BE1::Clamp01(BE1::Image::LinearToGamma(BE1::Clamp01(linearColor.r * luminanceScale)) + dither);
            const float sRGBG = BE1::Clamp01(BE1::Image::LinearToGamma(BE1::Clamp01(linearColor.g * luminanceScale)) + dither);
            const float sRGBB = BE1::Clamp01(BE1::Image::LinearToGamma(BE1::Clamp01(linearColor.b * luminanceScale)) + dither);

            dst[0] = BE1::Math::Ftob(sRGBB * 255.0f + 0.5f);
            dst[1] = BE1::Math::Ftob(sRGBG * 255.0f + 0.5f);
            dst[2] = BE1::Math::Ftob(sRGBR * 255.0f + 0.5f);
        }
    }
}

const void *RenderBackend::ExecuteScreenshot(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteScreenshot");

    const ScreenShotRenderCommand *cmd = reinterpret_cast<const ScreenShotRenderCommand *>(data);

    // 캡쳐 영역 Rect
    BE1::Rect captureRect(cmd->x, cmd->y, cmd->width, cmd->height);

    // 최종 합성 결과를 저장할 8-bit sRGB 이미지를 생성한다.
    BE1::Image screenImage;
    screenImage.Create2D(captureRect.w, captureRect.h, 1, BE1::Image::Format::B8G8R8, BE1::Image::GammaSpace::sRGB, nullptr);

    RHI::Texture *frameCompositionTexture = currentContext->GetFrameCompositionTexture();
    const int frameWidth = static_cast<int>(frameCompositionTexture->GetWidth());
    const int frameHeight = static_cast<int>(frameCompositionTexture->GetHeight());
    const bool hdrOutput = RHI::SwapChain::IsHDRColorSpace(currentContext->GetSwapChain()->GetColorSpace());
    if (hdrOutput) {
        // HDR 합성 텍스처는 paper-white-relative 선형 Rec.709 이다.
        // 디스플레이 피크를 white point 로 사용하는 extended Reinhard 커브로 SDR 범위에 압축한 뒤 sRGB 로 인코딩한다.
        BE1::Image compositionImage;
        compositionImage.Create2D(frameWidth, frameHeight, 1, BE1::Image::Format::R16G16B16A16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        RHI::renderer->GetTextureImage2D(frameCompositionTexture, 0, compositionImage.GetFormat(), compositionImage.GetPixels());

        ConvertHDRScreenshotRegionToSDR(compositionImage, captureRect, currentContext->GetDisplayOutputSettings().maxDisplayLinearValue, screenImage);
    } else {
        // SDR 합성 텍스처에는 최종 sRGB 값이 저장되어 있으므로 색 변환 없이 캡쳐 영역만 복사한다.
        BE1::Image compositionImage;
        compositionImage.Create2D(frameWidth, frameHeight, 1, BE1::Image::Format::B8G8R8, BE1::Image::GammaSpace::sRGB, nullptr);
        RHI::renderer->GetTextureImage2D(frameCompositionTexture, 0, compositionImage.GetFormat(), compositionImage.GetPixels());

        CopySDRScreenshotRegion(compositionImage, captureRect, screenImage);
    }

    // 이제 이미지 파일로 저장한다.
    BE1::Str filename = cmd->filename;
    filename.DefaultFileExtension(".png");
    screenImage.Write(filename);

    return (const void *)(cmd + 1);
}

const void *RenderBackend::ExecuteFinalizeFrame(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteFinalizeFrame");

    const FinalizeFrameRenderCommand *cmd = reinterpret_cast<const FinalizeFrameRenderCommand *>(data);
    const bool renderImGuiOverlay = cmd->renderImGuiOverlay && frame->HasPendingImGuiOverlay();
    const bool finishGpuTiming = gpuTimingEnabled && gpuTiming.IsFrameActive();
    RHI::Texture *frameCompositionTexture = swapChainOutputEnabled ? currentContext->GetFrameCompositionTexture() : nullptr;

    bool needsCommandList = renderImGuiOverlay || finishGpuTiming || frameCompositionTexture;
#ifdef USE_MIPMAP_STREAMING
    needsCommandList |= textureStreamingFeedbackEnabled;
    TextureStreamingFeedback::ReadbackFrame *textureStreamingReadbackFrame = nullptr;
#endif

    if (needsCommandList) {
        mainCommandList = frame->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

#ifdef USE_MIPMAP_STREAMING
        if (textureStreamingFeedbackEnabled) {
            textureStreamingReadbackFrame = currentContext->textureStreamingFeedback.Resolve(mainCommandList);
        }
#endif

        if (renderImGuiOverlay) {
            GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ImGuiOverlay);

            BE1::Rect viewportRect(0, 0, currentContext->GetWidth(), currentContext->GetHeight());

            // 뷰포트 & ScissorRect 의 초기값 설정
            RHI::renderer->SetViewport(mainCommandList, viewportRect);
            RHI::renderer->SetScissorRect(mainCommandList, viewportRect);

            RHI::RenderPassImage outputImage = RHI::RenderPassImage::Color(
                frameCompositionTexture, RHI::ViewID::PrimaryID, RHI::RenderPassImage::LoadAction::Load,
                RHI::RenderPassImage::StoreAction::Store, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource);
            RHI::renderer->BeginRenderPass(mainCommandList, &outputImage, 1);

            frame->RenderPendingImGuiOverlay(mainCommandList);

            RHI::renderer->EndRenderPass(mainCommandList);
        }

        if (swapChainOutputEnabled) {
            BE1::Rect outputRect(0, 0, currentContext->GetWidth(), currentContext->GetHeight());
            BlitTextureToSwapChain(frameCompositionTexture, outputRect);
        }

        if (finishGpuTiming) {
            gpuTiming.EndFrame(mainCommandList);
        }

        mainCommandList->CloseAndExecute();
    }

    // 이번 백엔드 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    uint64_t fenceValue = RHI::renderer->SignalFence(RHI::CommandQueueType::Graphics);
    frame->SetFrameFenceValue(fenceValue);

    if (gpuTimingEnabled) {
        gpuTiming.SetFrameFence(fenceValue);
    }

    if (frame->IsStatEnabled()) {
        UpdateFrameStats();
    }

#ifdef USE_MIPMAP_STREAMING
    if (textureStreamingReadbackFrame) {
        textureStreamingReadbackFrame->SetReadbackFenceValue(fenceValue);
    }
#endif

    if (oceanSurfaceQueryReadbackFrame) {
        oceanSurfaceQueryReadbackFrame->SetReadbackFenceValue(fenceValue);
        oceanSurfaceQueryReadbackFrame = nullptr;
    }

    return (const void *)(cmd + 1);
}

const void *RenderBackend::ExecuteSwapBuffers(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteSwapBuffers");

    const SwapBuffersRenderCommand *cmd = reinterpret_cast<const SwapBuffersRenderCommand *>(data);

    // 백버퍼를 전면버퍼와 교환한다.
    currentContext->GetSwapChain()->SwapBuffers(frame->GetRenderFrameSettings().vsync);

    PROFILER_FRAME_MARK();

    frameCount++;

    return (const void *)(cmd + 1);
}

void RenderBackend::SetPassVertexBuffers(RHI::CommandList *commandList, const RenderPassDrawSurf *drawSurf) const {
    const SubMesh *subMesh = drawSurf->frameSurf->subMesh;
    RHI::renderer->SetVertexBuffer(commandList, 0, subMesh->vertexBuffer);

    if (VertexInputLayoutFactory::IsSkinning(drawSurf->inputLayoutId)) {
        RHI::renderer->SetVertexBuffer(commandList, 1, subMesh->vertexWeightBuffer);
    }
}

static bool IsHiZCullRenderPass(RenderPassType passType) {
    return passType == RenderPassType::DepthPrepass || passType == RenderPassType::Opaque;
}

static GpuTiming::Tag GpuTagForRenderPass(RenderPassType passType) {
    switch (passType) {
    case RenderPassType::Shadow:
        return GpuTiming::Tag::ShadowPass;
    case RenderPassType::DepthPrepass:
        return GpuTiming::Tag::DepthPrepass;
    case RenderPassType::Opaque:
        return GpuTiming::Tag::OpaquePass;
    case RenderPassType::Skybox:
        return GpuTiming::Tag::SkyPass;
    case RenderPassType::Transparent:
        return GpuTiming::Tag::TransparentPass;
    case RenderPassType::Gui:
        return GpuTiming::Tag::GuiPass;
    default:
        return GpuTiming::Tag::OpaquePass;
    }
}

void RenderBackend::DrawGenericPass(const RenderPass *renderPass, const RHI::ConstantBuffer *viewConstantBuffer) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs == 0) {
        return;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTagForRenderPass(renderPass->type));

    const SurfaceInstanceBatch *hiZCullBatches = nullptr;
    uint32_t hiZCullBatchCount = 0;

    if (currentFrameCamera->hiZOcclusionCulling && IsHiZCullRenderPass(renderPass->type)) {
        const uint32_t passIndex = static_cast<uint32_t>(renderPass - currentFrameCamera->renderPasses);

        if (passIndex < static_cast<uint32_t>(hiZCullPassRanges.Count())) {
            const HiZCullPassRange &passRange = hiZCullPassRanges[passIndex];

            if (passRange.numBatches > 0) {
                hiZCullBatches = hiZCullDrawBatches.Ptr() + passRange.firstBatchIndex;
                hiZCullBatchCount = passRange.numBatches;
            }
        }
    }

    DrawGenericSurfaces(renderPass->drawSurfs, numDrawSurfs, viewConstantBuffer, hiZCullBatches, hiZCullBatchCount);
}

void RenderBackend::DrawSkyPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSkyPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs == 0) {
        return;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SkyPass);

    DrawSkySurfaces(renderPass->drawSurfs, numDrawSurfs);
}

void RenderBackend::DrawGpuOcean() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGpuOcean");

    const FrameOceanSurface *frameOceanSurface = frame->GetFrameOceanSurface();
    if (!frameOceanSurface || BE1::HasFlag(currentFrameCamera->flags, RenderCamera::Flag::StaticOnly) || !currentFrameCamera->oceanSurfacePSO) {
        return;
    }

    GpuOcean &gpuOcean = renderSystem->GetGpuOcean();
    GpuOceanSurface &gpuOceanSurface = renderSystem->GetGpuOceanSurface();
    if (!gpuOcean.AreCascadeOutputsReady() || !gpuOceanSurface.IsGridReady()) {
        return;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::OceanPass);
    GpuOceanSurface::View surfaceView;
    surfaceView.origin = currentFrameCamera->origin;
    surfaceView.renderWidth = currentFrameCamera->renderWidth;
    surfaceView.renderHeight = currentFrameCamera->renderHeight;
    surfaceView.infiniteZFar = currentFrameCamera->projectionInfiniteZFar;
    if (currentFrameCamera->projectionOrthogonal) {
        surfaceView.box = &currentFrameCamera->viewBox;
    } else {
        surfaceView.frustum = &currentFrameCamera->viewFrustum;
    }

    GpuOceanSurface::DrawStats drawStats;
    if (gpuOceanSurface.Draw(mainCommandList, frame->GetThreadData(0), currentFrameCamera->oceanSurfacePSO, frameConstantBuffer, cameraConstantBuffer, frameOceanSurface->surfaceSettings, surfaceView,
        currentFrameCamera->snapshotColorTexture, currentFrameCamera->snapshotDepthTexture, &drawStats)) {
        RecordDrawStats(drawStats.drawCount, drawStats.triangleCount, drawStats.instanceBufferAllocCount, drawStats.instanceBufferUploadBytes);
    }
}

void RenderBackend::DrawGuiPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGuiPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs == 0) {
        return;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::GuiPass);

    DrawGenericSurfacesWithoutTask(renderPass->drawSurfs, numDrawSurfs, cameraConstantBuffer);
}

bool RenderBackend::DispatchLightCulling() {
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::LightCulling);

    return frame->DispatchLightCulling(frame->GetThreadData(0), mainCommandList, frameLightCullingPSO);
}

static bool CanBatchSurfaceInstances(const RenderPassDrawSurf *baseDrawSurf, const RenderPassDrawSurf *drawSurf) {
    return drawSurf->pso &&
        drawSurf->pso == baseDrawSurf->pso &&
        drawSurf->frameSurf->subMesh == baseDrawSurf->frameSurf->subMesh &&
        drawSurf->frameSurf->lodLevel == baseDrawSurf->frameSurf->lodLevel &&
        drawSurf->inputLayoutId == baseDrawSurf->inputLayoutId;
}

// startIndex 부터 batch 렌더링할 수 있는 최대 endIndex 를 구한다 (MaxInstancesPerDraw 제한은 있음)
uint32_t RenderBackend::FindSurfaceInstanceBatchEnd(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t startIndex) const {
    assert(startIndex < numDrawSurfs);

    const RenderPassDrawSurf *baseDrawSurf = drawSurfs[startIndex];
    uint32_t endIndex = startIndex + 1;
    while (endIndex < numDrawSurfs && endIndex - startIndex < MaxInstancesPerDraw && CanBatchSurfaceInstances(baseDrawSurf, drawSurfs[endIndex])) {
        endIndex++;
    }
    return endIndex;
}

RHI::Buffer *RenderBackend::AllocInstanceToShaderSurfaceTable(RHI::FrameThreadData *frameThreadData, const RenderPassDrawSurf **drawSurfs, uint32_t instanceCount) const {
    RHI::Buffer *buffer = frameThreadData->AllocStructuredBuffer(sizeof(uint32_t), instanceCount, nullptr);
    if (!buffer) {
        return nullptr;
    }

    uint32_t *shaderSurfaceIndices = reinterpret_cast<uint32_t *>(buffer->mappedPtr);
    for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
        shaderSurfaceIndices[instanceIndex] = drawSurfs[instanceIndex]->frameSurf->index;
    }

    return buffer;
}

bool RenderBackend::PrepareSurfaceInstanceBatch(RHI::FrameThreadData *frameThreadData, const RenderPassDrawSurf **drawSurfs, uint32_t drawSurfIndex, uint32_t instanceCount, SurfaceInstanceBatch &batch, uint32_t &instanceTableAllocCount, uint32_t &instanceTableUploadBytes) const {
    assert(instanceCount > 0);

    RHI::Buffer *instanceToShaderSurfaceTable = AllocInstanceToShaderSurfaceTable(frameThreadData, drawSurfs, instanceCount);
    if (!instanceToShaderSurfaceTable) {
        batch = {};
        return false;
    }

    batch.drawSurfIndex = drawSurfIndex;
    batch.instanceCount = instanceCount;
    batch.instanceToShaderSurfaceTable = instanceToShaderSurfaceTable;
    batch.instanceBaseOffset = 0;
    batch.indirectArgsBuffer = nullptr;
    batch.indirectArgsOffset = 0;
    batch.valid = true;

    instanceTableAllocCount++;
    instanceTableUploadBytes += sizeof(uint32_t) * instanceCount;

    return true;
}

void RenderBackend::DrawSurfaceInstanceBatch(RHI::CommandList *commandList, const RenderPassDrawSurf **drawSurfs, const SurfaceInstanceBatch &batch, uint32_t &drawCount, uint64_t &drawTriangleCount) {
    assert(batch.valid);
    assert(batch.instanceCount > 0);
    assert(batch.instanceToShaderSurfaceTable);

    SurfaceDrawParams surfaceDrawParams = {};
    surfaceDrawParams.instanceBaseOffset = batch.instanceBaseOffset;

    const FrameSurf *firstFrameSurf = drawSurfs[0]->frameSurf;

    RHI::renderer->SetConstants(commandList, 2, &surfaceDrawParams, sizeof(surfaceDrawParams), 0);
    RHI::renderer->SetBuffer(commandList, 0, false, batch.instanceToShaderSurfaceTable);

    if (batch.indirectArgsBuffer) {
        RHI::renderer->DrawIndexedInstancedIndirect(commandList, batch.indirectArgsBuffer, batch.indirectArgsOffset);
    } else {
        RHI::renderer->DrawIndexedInstanced(commandList, firstFrameSurf->numIndexes, batch.instanceCount, 0, 0, 0);

        drawTriangleCount += firstFrameSurf->numIndexes / 3 * batch.instanceCount;
    }

    drawCount++;
}
