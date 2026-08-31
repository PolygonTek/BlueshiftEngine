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
#include "Render/RenderCvars.h"
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"
#include "Render/RenderInternal.h"
#include "Render/Texture/Texture.h"
#include "Shaders/ShaderInterop.h"

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

void RenderBackend::Execute(RenderFrameData *frameData) {
    uint64_t t0 = BE1::PlatformTime::Microseconds();

    // 백엔드가 처리할 프레임 데이터
    this->frameData = frameData;

    UpdateFramePSOs();

    frameConstantBuffer = nullptr;
    cameraConstantBuffer = nullptr;

    const void *data = frameData->GetCommands()->buffer;
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
            BE_ERRLOG("RenderBackend::Execute: invalid render command ID (%i)\n", cmdId);
            finished = true;
            continue;
        }
    }

    uint64_t t1 = BE1::PlatformTime::Microseconds();
    UpdateRenderFrameTime(t1 - t0);
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
    if (!frameData->IsStatEnabled()) {
        return;
    }

    statDrawCount += drawCount;
    statDrawTriangleCount += drawTriangleCount;
    statInstanceTableAllocCount += instanceTableAllocCount;
    statInstanceTableUploadBytes += instanceTableUploadBytes;
}

void RenderBackend::UpdateFrameStats() {
    RenderContext::Stats stats;
    stats.visibleCameraCount = frameData->NumVisCameras();
    stats.visibleObjectCount = frameData->NumVisObjects();
    stats.visibleLightCount = frameData->NumVisLights();
    stats.visiblePointLightCount = frameData->NumVisPointLights();
    stats.visibleSpotLightCount = frameData->NumVisSpotLights();
    stats.visibleBoxDirLightCount = frameData->NumVisBoxDirLights();
    stats.drawCount = statDrawCount;
    stats.drawTriangleCount = statDrawTriangleCount;
    stats.instanceTableAllocCount = statInstanceTableAllocCount;
    stats.instanceTableUploadBytes = statInstanceTableUploadBytes;
    stats.lightCullingZBinCount = frameData->NumLightCullingZBins();
    stats.lightCullingWordCount = frameData->NumLightCullingWords();

    const VisCamera *visCameras = frameData->GetVisCameras();
    for (uint32_t cameraIndex = 0; cameraIndex < frameData->NumVisCameras(); ++cameraIndex) {
        const VisCamera &visCamera = visCameras[cameraIndex];
        if (visCamera.lightCulling.wordsPerTile == 0) {
            continue;
        }

        const uint32_t threadGroupCountX = BE1::DivCeil(visCamera.lightCulling.tileCountX, ShaderLightCullingThreadGroupSizeX);
        const uint32_t threadGroupCountY = BE1::DivCeil(visCamera.lightCulling.tileCountY, ShaderLightCullingThreadGroupSizeY);

        stats.lightCullingThreadGroupCount += static_cast<uint64_t>(threadGroupCountX) * threadGroupCountY * visCamera.lightCulling.wordsPerTile;
    }

    currentContext->SetStats(stats);
}

bool RenderBackend::UploadFrameConstants() {
    frameConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(FrameConstants));
    if (!frameConstantBuffer) {
        return false;
    }

    FrameConstants *constants = reinterpret_cast<FrameConstants *>(frameConstantBuffer->mappedPtr);
    constants->frameTime = frameData->GetFrameTime();
    constants->elapsedTime = frameData->GetElapsedTime();
    constants->elapsedPrevTime = frameData->GetElapsedPrevTime();
    constants->frameCount = frameData->GetFrameCount();
    constants->shaderMaterialTableIndex = frameData->GetShaderMaterialTableBindlessIndex();
    constants->shaderPointLightTableIndex = frameData->GetShaderPointLightTableBindlessIndex();
    constants->shaderSpotLightTableIndex = frameData->GetShaderSpotLightTableBindlessIndex();
    constants->shaderBoxDirLightTableIndex = frameData->GetShaderBoxDirLightTableBindlessIndex();
    constants->shaderLightRefTableIndex = frameData->GetShaderLightRefTableBindlessIndex();
    constants->shaderLightZBinTableIndex = frameData->GetShaderLightZBinTableBindlessIndex();
    constants->shaderLightTileWordTableIndex = frameData->GetShaderLightTileWordTableBindlessIndex();
    constants->feedbackBufferIndex = textureStreamingFeedbackEnabled ? currentContext->textureStreamingFeedback.GetBindlessIndex() : 0;

    const RHI::Texture *dfgSumGGXTexture = renderSystem->dfgSumGGXTexture ? renderSystem->dfgSumGGXTexture->GetReadyRHITexture() : nullptr;
    constants->dfgSumGGXTextureIndex = dfgSumGGXTexture ? dfgSumGGXTexture->GetBindlessIndex(RHI::ViewType::SRV) : 0;
    constants->padding0 = 0;
    constants->padding1 = 0;
    constants->padding2 = 0;

    ShaderDirectionalLight &shaderDirectionalLight = constants->directionalLight;
    const VisLight *primaryDirectionalLight = frameData->GetPrimaryDirectionalLight();
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

    if (!currentVisCamera) {
        return false;
    }

    cameraConstantBuffer = frameData->GetThreadData(0)->AllocConstant(sizeof(CameraConstants));
    if (!cameraConstantBuffer) {
        return false;
    }

    CameraConstants *constants = reinterpret_cast<CameraConstants *>(cameraConstantBuffer->mappedPtr);

    uint32_t flags = 0;
    if (currentVisCamera->projectionOrthogonal) {
        flags |= ShaderCameraFlagProjectionOrthogonal;
    }
    if (r_lightCullingZBin.GetBool()) {
        flags |= ShaderCameraFlagUseLightZBin;
    }
    if (currentVisCamera->directionalLightVisible) {
        flags |= ShaderCameraFlagUseDirectionalLight;
    }
    if (r_specularAA.GetBool()) {
        flags |= ShaderCameraFlagUseSpecularAA;
    }

    ALIGN_AS32 BE1::Mat4 unjitteredViewProjMatrix = currentVisCamera->viewProjMatrix;
    ALIGN_AS32 BE1::Mat4 prevUnjitteredViewProjMatrix = currentVisCamera->prevViewProjMatrix;

    // jittered view projection matrix 에서 jittering 제거
    RemoveProjectionJitter(unjitteredViewProjMatrix, currentVisCamera->jitterNDC);
    RemoveProjectionJitter(prevUnjitteredViewProjMatrix, currentVisCamera->prevJitterNDC);

    BE1::simdProcessor->Memcpy(&constants->viewProjMatrix, &currentVisCamera->viewProjMatrix, sizeof(constants->viewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->invViewProjMatrix, &currentVisCamera->invViewProjMatrix, sizeof(constants->invViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->unjitteredViewProjMatrix, &unjitteredViewProjMatrix, sizeof(constants->unjitteredViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->prevViewProjMatrix, &currentVisCamera->prevViewProjMatrix, sizeof(constants->prevViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->prevInvViewProjMatrix, &currentVisCamera->prevInvViewProjMatrix, sizeof(constants->prevInvViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->prevUnjitteredViewProjMatrix, &prevUnjitteredViewProjMatrix, sizeof(constants->prevUnjitteredViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->hiZCullViewProjMatrix, &currentVisCamera->hiZCullViewProjMatrix, sizeof(constants->hiZCullViewProjMatrix));

    constants->surfaceTableIndex = frameData->GetShaderSurfaceTableBindlessIndex();
    constants->ndcFarZ = currentVisCamera->projectionPolicy.GetNDCFarZ();
    constants->projectionDepthA = currentVisCamera->projectionDepthA;
    constants->projectionDepthB = currentVisCamera->projectionDepthB;
    constants->flags = flags;
    constants->numLocalLights = currentVisCamera->numVisLocalLights;
    constants->lightCullingLightRefTableOffset = currentVisCamera->lightCulling.lightRefTableOffset;
    constants->lightCullingZBinTableOffset = currentVisCamera->lightCulling.zBinTableOffset;
    constants->lightCullingTileWordTableOffset = currentVisCamera->lightCulling.tileWordTableOffset;
    constants->lightCullingZBinScale = currentVisCamera->lightCulling.zBinScale;
    constants->lightCullingZBinBias = currentVisCamera->lightCulling.zBinBias;
    constants->lightCullingTileCountX = currentVisCamera->lightCulling.tileCountX;
    constants->lightCullingTileCountY = currentVisCamera->lightCulling.tileCountY;
    constants->lightCullingWordsPerTile = currentVisCamera->lightCulling.wordsPerTile;
    constants->lightCullingDebugMode = static_cast<uint32_t>(r_lightCullingDebug.GetInteger());
    constants->padding0 = 0;

    constants->cameraOrigin = float3(currentVisCamera->origin.x, currentVisCamera->origin.y, currentVisCamera->origin.z);
    constants->padding1 = 0.0f;

    const BE1::Mat3 &environmentLightAxis = currentVisCamera->environmentLightAxis;
    constants->environmentLightAxis = float3x4(
        environmentLightAxis[0].x, environmentLightAxis[0].y, environmentLightAxis[0].z, 0.0f,
        environmentLightAxis[1].x, environmentLightAxis[1].y, environmentLightAxis[1].z, 0.0f,
        environmentLightAxis[2].x, environmentLightAxis[2].y, environmentLightAxis[2].z, 0.0f);
    constants->environmentLightColor = float3(currentVisCamera->environmentLightColor.r, currentVisCamera->environmentLightColor.g, currentVisCamera->environmentLightColor.b);
    constants->environmentLightIntensity = currentVisCamera->environmentLightIntensity;

    constants->environmentLightDiffuseIrradianceCubeMapIndex = currentVisCamera->environmentLightDiffuseIrradianceCubeMap ? currentVisCamera->environmentLightDiffuseIrradianceCubeMap->GetBindlessIndex(RHI::ViewType::SRV) : 0;
    constants->environmentLightSpecularCubeMapIndex = currentVisCamera->environmentLightSpecularCubeMap ? currentVisCamera->environmentLightSpecularCubeMap->GetBindlessIndex(RHI::ViewType::SRV) : 0;
    constants->environmentLightSpecularMipCount = currentVisCamera->environmentLightSpecularMipCount;
    constants->padding2 = 0;

    return true;
}

void RenderBackend::UpdateFramePSOs() {
    frameGenHiZMipFirstBatchPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipFirstBatchPipeline);
    frameGenHiZMipFirstBatchMSAAPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipFirstBatchMSAAPipeline);
    frameGenHiZMipBatchPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipBatchPipeline);
    frameGenHiZMipOddDimPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipOddDimPipeline);
    frameHiZCullInstancesPSO = ShaderPipeline::GetReadyPSO(renderSystem->hiZCullInstancesPipeline);
    frameHiZDebugPreviewPSO = ShaderPipeline::GetReadyPSO(renderSystem->hiZDebugPreviewPipeline);
    frameLightCullingPSO = ShaderPipeline::GetReadyPSO(renderSystem->lightCullingPipeline);
    frameDepthMSAAResolvePSO = ShaderPipeline::GetReadyPSO(renderSystem->depthMSAAResolvePipeline);
    frameSurfaceOutputMSAAResolveNormalPSO = ShaderPipeline::GetReadyPSO(renderSystem->surfaceOutputMSAAResolveNormalPipeline);
    frameSurfaceOutputMSAAResolveVelocityPSO = ShaderPipeline::GetReadyPSO(renderSystem->surfaceOutputMSAAResolveVelocityPipeline);
    frameSurfaceOutputMSAAResolveNormalVelocityPSO = ShaderPipeline::GetReadyPSO(renderSystem->surfaceOutputMSAAResolveNormalVelocityPipeline);
    frameFinalPostProcessPSO = ShaderPipeline::GetReadyPSO(renderSystem->finalPostProcessPipeline);
    frameTAAResolvePSO = ShaderPipeline::GetReadyPSO(renderSystem->taaResolvePipeline);
    frameLuminancePass1PSO = ShaderPipeline::GetReadyPSO(renderSystem->luminancePass1Pipeline);
    frameLuminancePass2PSO = ShaderPipeline::GetReadyPSO(renderSystem->luminancePass2Pipeline);
    frameBloomPrefilterPSO = ShaderPipeline::GetReadyPSO(renderSystem->bloomPrefilterPipeline);
    frameBloomDownsamplePSO = ShaderPipeline::GetReadyPSO(renderSystem->bloomDownsamplePipeline);
    frameBloomUpsamplePSO = ShaderPipeline::GetReadyPSO(renderSystem->bloomUpsamplePipeline);
    frameConvolutionBloomScreenPreparePSO = ShaderPipeline::GetReadyPSO(renderSystem->convolutionBloomScreenPreparePipeline);
    frameConvolutionBloomKernelPreparePSO = ShaderPipeline::GetReadyPSO(renderSystem->convolutionBloomKernelPreparePipeline);
    frameConvolutionBloomResolvePSO = ShaderPipeline::GetReadyPSO(renderSystem->convolutionBloomResolvePipeline);
}

const void *RenderBackend::ExecuteRenderWork(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteRenderWork");

    const RenderWorkRenderCommand *cmd = reinterpret_cast<const RenderWorkRenderCommand *>(data);
    RenderWork *renderWork = cmd->renderWork;
    if (!renderWork) {
        return (const void *)(cmd + 1);
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);
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

    statDrawCount = 0;
    statDrawTriangleCount = 0;
    statInstanceTableAllocCount = 0;
    statInstanceTableUploadBytes = 0;

    mainCommandList = frameData->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

    if (gpuTimingEnabled) {
        gpuTiming.BeginFrame(mainCommandList, frameData, currentContext);
    }

    // 메인 swap-chain 프레임마다 ocean simulation 을 한 프레임 진행한다.
    const VisOceanSurface *visOceanSurface = frameData->GetVisOceanSurface();
    if (visOceanSurface && swapChainOutputEnabled && currentContext == renderSystem->mainRenderContext) {
        renderSystem->gpuOcean.ApplySimulationSettings(visOceanSurface->simulationSettings, visOceanSurface->settingsRevision);
        renderSystem->gpuOcean.Update(mainCommandList, frameData->GetThreadData(0), frameData->GetFrameTime());
    }

    if (swapChainOutputEnabled) {
        if (currentContext->frameCompositionTextureNeedsClear) {
            // 새로 생성되었거나 다시 활성화된 frameCompositionTexture 는 실제로 처음 사용할 때 한 번만 초기화한다.
            const BE1::Color4 &clearColor = currentContext->GetBaseClearColor();
            RHI::RenderPassImage outputImage = RHI::RenderPassImage::Color(currentContext->frameCompositionTexture, RHI::ViewID::PrimaryID,
                RHI::RenderPassImage::LoadAction::Clear, RHI::RenderPassImage::StoreAction::Store, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource);
            outputImage = outputImage.WithClearValue(RHI::ClearValue::Color(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
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
    if (!frameData->EnsureShaderSurfaceTableCapacity()) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to ensure shader surface table capacity\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 등록된 ShaderMaterial 테이블을 GPU 에 업로드한다.
    if (!frameData->UploadShaderMaterialTable(frameData->GetThreadData(0), mainCommandList)) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to upload shader material table\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 각 카메라 별로 참조할 local ShaderLight 테이블을 GPU 에 업로드한다.
    if (!frameData->UploadShaderLightTable(frameData->GetThreadData(0), mainCommandList)) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to upload shader light table\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 각 카메라 별로 visible light 참조 테이블과 z-bin 테이블을 GPU 에 업로드한다.
    if (!frameData->UploadShaderLightCullingTable(frameData->GetThreadData(0), mainCommandList)) {
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

static const RenderPass *FindRenderPass(const VisCamera *visCamera, RenderPassType passType) {
    for (uint32_t passIndex = 0; passIndex < visCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &visCamera->renderPasses[passIndex];
        if (renderPass->type == passType) {
            return renderPass;
        }
    }

    return nullptr;
}

struct RenderPassImagesDesc {
    RHI::RenderPassImage::LoadAction colorLoadAction;
    RHI::RenderPassImage::LoadAction surfaceOutputLoadAction;
    RHI::RenderPassImage::LoadAction depthLoadAction;
    RHI::GPUResourceState            renderTargetBeforeState;
    RHI::GPUResourceState            renderTargetAfterState;
    RHI::GPUResourceState            depthBeforeState;
    RHI::GPUResourceState            depthAfterState;
    const RHI::Texture *             colorResolveTexture;
    const RHI::Texture *             depthResolveTexture;
    RHI::RenderPassImage::DepthResolveMode depthResolveMode;
};

// Camera3D render pass 에서 사용할 color, surface output, depth attachment 를 구성한다.
// 각 attachment 의 종료 상태를 다음 pass 의 시작 상태와 연결해 불필요한 resource transition 을 피하고,
// 지정된 resolve texture 로 MSAA color/depth 를 resolve 한다.
static uint32_t BuildCamera3DRenderPassImages(const VisCamera *visCamera, const RenderPassImagesDesc &desc, const RHI::ClearValue *clearColorValue, RHI::RenderPassImage renderPassImages[6]) {
    uint32_t renderPassImageCount = 0;

    const bool useMSAA = visCamera->rtColorMSAATexture != nullptr;

    const RHI::Texture *colorTexture = useMSAA ? visCamera->rtColorMSAATexture : visCamera->rtColorTexture;
    RHI::RenderPassImage colorImage = RHI::RenderPassImage::Color(colorTexture, RHI::ViewID::PrimaryID, desc.colorLoadAction, RHI::RenderPassImage::StoreAction::Store, desc.renderTargetBeforeState, desc.renderTargetAfterState);
    if (clearColorValue && desc.colorLoadAction == RHI::RenderPassImage::LoadAction::Clear) {
        colorImage = colorImage.WithClearValue(*clearColorValue);
    }
    renderPassImages[renderPassImageCount++] = colorImage;

    const RHI::ClearValue zeroClearValue = RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f);
    if (visCamera->needNormalOutput) {
        const RHI::Texture *normalTexture = useMSAA ? visCamera->rtNormalMSAATexture : visCamera->rtNormalTexture;
        RHI::RenderPassImage normalImage = RHI::RenderPassImage::Color(normalTexture, RHI::ViewID::PrimaryID, desc.surfaceOutputLoadAction, RHI::RenderPassImage::StoreAction::Store, desc.renderTargetBeforeState, desc.renderTargetAfterState);
        if (desc.surfaceOutputLoadAction == RHI::RenderPassImage::LoadAction::Clear) {
            normalImage = normalImage.WithClearValue(zeroClearValue);
        }
        renderPassImages[renderPassImageCount++] = normalImage;
    }

    if (visCamera->needVelocityOutput) {
        const RHI::Texture *velocityTexture = useMSAA ? visCamera->rtVelocityMSAATexture : visCamera->rtVelocityTexture;
        RHI::RenderPassImage velocityImage = RHI::RenderPassImage::Color(velocityTexture, RHI::ViewID::PrimaryID, desc.surfaceOutputLoadAction, RHI::RenderPassImage::StoreAction::Store, desc.renderTargetBeforeState, desc.renderTargetAfterState);
        if (desc.surfaceOutputLoadAction == RHI::RenderPassImage::LoadAction::Clear) {
            velocityImage = velocityImage.WithClearValue(zeroClearValue);
        }
        renderPassImages[renderPassImageCount++] = velocityImage;
    }

    renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::DepthStencil(visCamera->depthTexture, RHI::ViewID::PrimaryID, desc.depthLoadAction, RHI::RenderPassImage::StoreAction::Store, desc.depthBeforeState, RHI::GPUResourceState::DepthWrite, desc.depthAfterState);

    if (useMSAA && desc.colorResolveTexture) {
        renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::ResolveColor(desc.colorResolveTexture, RHI::ViewID::PrimaryID, 0);
    }
    if (useMSAA && desc.depthResolveTexture) {
        renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::ResolveDepth(desc.depthResolveTexture, RHI::ViewID::PrimaryID, desc.depthResolveMode);
    }

    return renderPassImageCount;
}

const void *RenderBackend::ExecuteDrawCamera(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteDrawCamera");

    const DrawCameraRenderCommand *cmd = reinterpret_cast<const DrawCameraRenderCommand *>(data);

    currentVisCamera = cmd->visCamera;

    if ((currentVisCamera->is2D && r_skipDrawCamera2D.GetBool()) || (!currentVisCamera->is2D && r_skipDrawCamera3D.GetBool())) {
        return (const void *)(cmd + 1);
    }

    // 커맨드 리스트 풀에서 새로운 커맨드 리스트를 얻어온다.
    mainCommandList = frameData->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

    // 현재 카메라에서 참조할 ShaderSurface 테이블을 GPU 에 업로드한다.
    if (!frameData->UploadShaderSurfaceTable(frameData->GetThreadData(0), mainCommandList, currentVisCamera)) {
        BE_WARNLOG("RenderBackend::ExecuteDrawCamera: failed to upload shader surface table\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    // 현재 카메라의 상수 데이터들을 준비한다.
    if (!UploadCameraConstants()) {
        BE_WARNLOG("RenderBackend::ExecuteDrawCamera: failed to upload camera constants\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }

    BE1::Rect viewportRect = currentVisCamera->outputRect;
    if (!currentVisCamera->is2D) {
        viewportRect.Set(0, 0, currentVisCamera->renderWidth, currentVisCamera->renderHeight);
    }

    // 뷰포트 & ScissorRect 의 초기값 설정
    RHI::renderer->SetViewport(mainCommandList, viewportRect);
    RHI::renderer->SetScissorRect(mainCommandList, viewportRect);

    RHI::ClearFlag clearFlags = RHI::ClearFlag::None;
    BE1::Color4 clearColor = currentContext->baseClearColor;
    bool useExplicitClearColor = false;

    if (currentVisCamera->clearMethod == RenderCamera::ClearMethod::Color) {
        clearFlags |= (RHI::ClearFlag::Color | RHI::ClearFlag::Depth);
        if (currentVisCamera->clearColorMode == RenderCamera::ClearColorMode::Explicit) {
            clearColor = currentVisCamera->clearColor;
            useExplicitClearColor = true;
        }
    } else if (currentVisCamera->clearMethod == RenderCamera::ClearMethod::DepthOnly || currentVisCamera->clearMethod == RenderCamera::ClearMethod::Skybox) {
        clearFlags |= RHI::ClearFlag::Depth;
    }

    if (currentVisCamera->is2D) {
        ExecuteDrawCamera2D(clearFlags, clearColor);
    } else {
        ExecuteDrawCamera3D(clearFlags, clearColor, useExplicitClearColor);
    }

    // command list 에 기록을 마치고 실행
    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
}

void RenderBackend::ExecuteDrawCamera2D(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteDrawCamera2D");

    if (!swapChainOutputEnabled) {
        BE_WARNLOG("RenderBackend::ExecuteDrawCamera2D: swap chain output is disabled\n");
        return;
    }

    // 백버퍼에 바로 그리지 않고, frameCompositionTexture 에 그린다.
    RHI::Texture *frameCompositionTexture = currentContext->GetFrameCompositionTexture();
    const RHI::RenderPassImage::LoadAction loadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
    RHI::RenderPassImage outputImage = RHI::RenderPassImage::Color(frameCompositionTexture, RHI::ViewID::PrimaryID, loadAction, RHI::RenderPassImage::StoreAction::Store, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource);
    outputImage = outputImage.WithClearValue(RHI::ClearValue::Color(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
    RHI::renderer->BeginRenderPass(mainCommandList, &outputImage, 1);

    DrawCamera2D();

    RHI::renderer->EndRenderPass(mainCommandList);
}

void RenderBackend::ExecuteDrawCamera3D(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor, bool useExplicitClearColor) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteDrawCamera3D");

    bool hiZCullingPrepared = false;
    if (currentVisCamera->hiZOcclusionCulling) {
        hiZCullingPrepared = PrepareHiZCulling();
    }

    const RHI::RenderPassImage::LoadAction depthLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
    const bool hasDepthPrepass = FindRenderPass(currentVisCamera, RenderPassType::DepthPrepass) != nullptr;

    // depth prepass
    if (hasDepthPrepass) {
        ExecuteDrawCamera3DDepthPrepass(depthLoadAction);
    }

    // Opaque 와 Skybox 를 렌더링한다.
    const RHI::RenderPassImage::LoadAction sceneDepthLoadAction = hasDepthPrepass ? RHI::RenderPassImage::LoadAction::Load : depthLoadAction;
    ExecuteDrawCamera3DScenePass(clearFlags, clearColor, useExplicitClearColor, sceneDepthLoadAction);

    // Ocean 이 현재 렌더 타겟을 읽는 feedback loop 없이 굴절할 수 있도록 Opaque scene color/depth 를 보존한다.
    CaptureSceneSnapshot();

    // Ocean, Transparent, Debug primitive 를 렌더링한다.
    ExecuteDrawCamera3DPostOpaquePass();

    // MSAA resolve for normal/velocity texture
    bool surfaceOutputsReady = true;
    if (currentVisCamera->rtColorMSAATexture && (currentVisCamera->needNormalOutput || currentVisCamera->needVelocityOutput)) {
        surfaceOutputsReady = DispatchSurfaceOutputMSAAResolve();
    }

    if (hiZCullingPrepared) {
        FinishHiZCulling();
    }

    if (currentVisCamera->generateHiZ && !r_hiZOcclusionCullingFreeze.GetBool()) {
        DispatchGenHiZ();
    }

    if (currentVisCamera->hiZDebugPreview) {
        DispatchHiZDebugPreview();
    }

    PostProcessAndBlitToOutput(surfaceOutputsReady);
}

void RenderBackend::ExecuteDrawCamera3DDepthPrepass(RHI::RenderPassImage::LoadAction depthLoadAction) {
    RHI::RenderPassImage depthImage = RHI::RenderPassImage::DepthStencil(currentVisCamera->depthTexture, RHI::ViewID::PrimaryID, depthLoadAction, RHI::RenderPassImage::StoreAction::Store,
        RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::ShaderResource);

    RHI::RenderPassImage renderPassImages[] = {
        depthImage
    };
    RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));

    DrawCamera3DDepthPrepass();

    RHI::renderer->EndRenderPass(mainCommandList);
}

void RenderBackend::ExecuteDrawCamera3DScenePass(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor, bool useExplicitClearColor, RHI::RenderPassImage::LoadAction depthLoadAction) {
    RHI::RenderPassImage::LoadAction colorLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
    if (currentVisCamera->clearMethod == RenderCamera::ClearMethod::Skybox) {
        colorLoadAction = RHI::RenderPassImage::LoadAction::DontCare;
    }

    const bool captureSceneSnapshot = currentVisCamera->oceanSurfacePSO && currentVisCamera->snapshotColorTexture && currentVisCamera->snapshotDepthTexture;
    const RHI::ClearValue clearColorValue = RHI::ClearValue::Color(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    const RenderPassImagesDesc passDesc = {
        colorLoadAction,
        RHI::RenderPassImage::LoadAction::Clear,
        depthLoadAction,
        RHI::GPUResourceState::ShaderResource,
        RHI::GPUResourceState::RenderTarget,
        RHI::GPUResourceState::ShaderResource,
        RHI::GPUResourceState::DepthWrite,
        captureSceneSnapshot ? currentVisCamera->snapshotColorTexture : nullptr,
        captureSceneSnapshot ? currentVisCamera->snapshotDepthTexture : nullptr,
        currentVisCamera->projectionPolicy.reverseZ ? RHI::RenderPassImage::DepthResolveMode::Max : RHI::RenderPassImage::DepthResolveMode::Min
    };
    RHI::RenderPassImage renderPassImages[6];
    const uint32_t renderPassImageCount = BuildCamera3DRenderPassImages(currentVisCamera, passDesc, useExplicitClearColor ? &clearColorValue : nullptr, renderPassImages);

    RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, renderPassImageCount);

    DrawCamera3DScenePass();

    RHI::renderer->EndRenderPass(mainCommandList);
}

void RenderBackend::CaptureSceneSnapshot() {
    RHI::Texture *snapshotColorTexture = currentVisCamera->snapshotColorTexture;
    RHI::Texture *snapshotDepthTexture = currentVisCamera->snapshotDepthTexture;
    if (!currentVisCamera->oceanSurfacePSO || !snapshotColorTexture || !snapshotDepthTexture || currentVisCamera->rtColorMSAATexture) {
        return;
    }

    RHI::Texture *sourceColorTexture = currentVisCamera->rtColorTexture;
    RHI::Texture *sourceDepthTexture = currentVisCamera->depthTexture;
    assert(sourceColorTexture->GetSampleCount() == 1 && sourceDepthTexture->GetSampleCount() == 1);
    assert(snapshotColorTexture->GetWidth() == sourceColorTexture->GetWidth() && snapshotColorTexture->GetHeight() == sourceColorTexture->GetHeight());
    assert(snapshotDepthTexture->GetWidth() == sourceDepthTexture->GetWidth() && snapshotDepthTexture->GetHeight() == sourceDepthTexture->GetHeight());

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SceneSnapshot);

    RHI::GPUBarrier copyBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(sourceColorTexture, RHI::GPUResourceState::RenderTarget, RHI::GPUResourceState::CopySrc),
        RHI::Renderer::MakeTextureBarrier(sourceDepthTexture, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::CopySrc),
        RHI::Renderer::MakeTextureBarrier(snapshotColorTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::CopyDst),
        RHI::Renderer::MakeTextureBarrier(snapshotDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::CopyDst)
    };
    RHI::renderer->Barrier(mainCommandList, copyBarriers, COUNT_OF(copyBarriers));

    RHI::renderer->CopyTexture(mainCommandList, snapshotColorTexture, 0, 0, 0, 0, 0, sourceColorTexture, 0, 0, 0, 0, 0, sourceColorTexture->GetWidth(), sourceColorTexture->GetHeight(), 1);
    RHI::renderer->CopyTexture(mainCommandList, snapshotDepthTexture, 0, 0, 0, 0, 0, sourceDepthTexture, 0, 0, 0, 0, 0, sourceDepthTexture->GetWidth(), sourceDepthTexture->GetHeight(), 1);

    RHI::GPUBarrier finishBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(sourceColorTexture, RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::RenderTarget),
        RHI::Renderer::MakeTextureBarrier(sourceDepthTexture, RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::DepthWrite),
        RHI::Renderer::MakeTextureBarrier(snapshotColorTexture, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(snapshotDepthTexture, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(mainCommandList, finishBarriers, COUNT_OF(finishBarriers));
}

void RenderBackend::ExecuteDrawCamera3DPostOpaquePass() {
    const RenderPassImagesDesc passDesc = {
        RHI::RenderPassImage::LoadAction::Load,
        RHI::RenderPassImage::LoadAction::Load,
        RHI::RenderPassImage::LoadAction::Load,
        RHI::GPUResourceState::RenderTarget,
        RHI::GPUResourceState::ShaderResource,
        RHI::GPUResourceState::DepthWrite,
        RHI::GPUResourceState::ShaderResource,
        currentVisCamera->rtColorTexture,
        nullptr,
        RHI::RenderPassImage::DepthResolveMode::None
    };
    RHI::RenderPassImage renderPassImages[6];
    const uint32_t renderPassImageCount = BuildCamera3DRenderPassImages(currentVisCamera, passDesc, nullptr, renderPassImages);

    RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, renderPassImageCount);

    DrawCamera3DPostOpaquePass();

    RHI::renderer->EndRenderPass(mainCommandList);
}

void RenderBackend::PostProcessAndBlitToOutput(bool surfaceOutputsReady) {
    const RHI::Texture *finalColorTexture = currentVisCamera->rtColorTexture;

    currentPostProcessInputTexture = currentVisCamera->rtColorTexture;

    if (!r_skipPostProcess.GetBool() && !BE1::HasFlag(currentVisCamera->flags, RenderCamera::Flag::SkipPostProcess)) {
        bool depthHistoryReady = true;
        if (currentVisCamera->taaResolve && currentVisCamera->depthTexture->GetSampleCount() > 1) {
            depthHistoryReady = DispatchCurrentDepthMSAAResolve();
        }

        if (surfaceOutputsReady && depthHistoryReady && DispatchTAAResolve()) {
            currentPostProcessInputTexture = currentVisCamera->colorHistoryTextures[currentVisCamera->currentHistoryTextureIndex];
            finalColorTexture = currentPostProcessInputTexture;
        }

        const bool useToneMapping = DispatchLuminance();
        const bool useBloom = DispatchBloom();

        if (DispatchFinalPostProcess(useToneMapping, useBloom)) {
            finalColorTexture = currentVisCamera->postProcessColorTexture;
        }
    }

    if (currentVisCamera->outputTexture) {
        if (finalColorTexture != currentVisCamera->outputTexture) {
            BlitTextureToRenderTarget(finalColorTexture, currentVisCamera->outputTexture, currentVisCamera->outputViewID, currentVisCamera->outputBeforeState, currentVisCamera->outputAfterState, currentVisCamera->outputRect, currentVisCamera->outputImagePSO);
        }
    } else if (swapChainOutputEnabled) {
        // 백버퍼에 바로 blit 하지 않고, frameCompositionTexture 에 먼저 blit 한다.
        RHI::Texture *frameCompositionTexture = currentContext->GetFrameCompositionTexture();
        BlitTextureToRenderTarget(finalColorTexture, frameCompositionTexture, RHI::ViewID::PrimaryID, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource, currentVisCamera->outputRect, frameImagePSO);
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

    if (BE1::HasFlag(currentVisCamera->flags, RenderCamera::Flag::FlipOutputTextureY)) {
        imageParams.texCoordScale.y = -imageParams.texCoordScale.y;
        imageParams.texCoordOffset.y = 1.0f - imageParams.texCoordOffset.y;
    }

    BE1::Rect outputRect(outputX0, outputY0, outputW, outputH);

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::BlitToOutput);

    // RTV 전체를 덮어쓰는 경우 이전 내용을 보존하지 않아 초기화되지 않은 렌더 타겟도 안전하게 사용할 수 있다.
    const bool overwriteWholeTarget = outputX0 == 0 && outputY0 == 0 && outputW == w && outputH == h;
    const RHI::RenderPassImage::LoadAction loadAction = overwriteWholeTarget ? RHI::RenderPassImage::LoadAction::DontCare : RHI::RenderPassImage::LoadAction::Load;
    RHI::RenderPassImage outputImage = RHI::RenderPassImage::Color(renderTargetTexture, renderTargetViewID, loadAction, RHI::RenderPassImage::StoreAction::Store, renderTargetBeforeState, renderTargetAfterState);
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
    screenImage.Create2D(captureRect.w, captureRect.h, 1, BE1::Image::Format::B8G8R8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);

    RHI::Texture *frameCompositionTexture = currentContext->GetFrameCompositionTexture();
    const int frameWidth = static_cast<int>(frameCompositionTexture->GetWidth());
    const int frameHeight = static_cast<int>(frameCompositionTexture->GetHeight());
    const bool hdrOutput = RHI::SwapChain::IsHDRColorSpace(currentContext->GetSwapChain()->GetColorSpace());
    if (hdrOutput) {
        // HDR 합성 텍스처는 paper-white-relative 선형 Rec.709 이다.
        // 디스플레이 피크를 white point 로 사용하는 extended Reinhard 커브로 SDR 범위에 압축한 뒤 sRGB 로 인코딩한다.
        BE1::Image compositionImage;
        compositionImage.Create2D(frameWidth, frameHeight, 1, BE1::Image::Format::R16G16B16A16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
        RHI::renderer->GetTextureImage2D(frameCompositionTexture, 0, compositionImage.GetFormat(), compositionImage.GetPixels());

        ConvertHDRScreenshotRegionToSDR(compositionImage, captureRect, currentContext->GetDisplayOutputSettings().maxDisplayLinearValue, screenImage);
    } else {
        // SDR 합성 텍스처에는 최종 sRGB 값이 저장되어 있으므로 색 변환 없이 캡쳐 영역만 복사한다.
        BE1::Image compositionImage;
        compositionImage.Create2D(frameWidth, frameHeight, 1, BE1::Image::Format::B8G8R8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
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
    const bool renderImGuiOverlay = cmd->renderImGuiOverlay && frameData->HasPendingImGuiOverlay();
    const bool finishGpuTiming = gpuTimingEnabled && gpuTiming.IsFrameActive();
    RHI::Texture *frameCompositionTexture = swapChainOutputEnabled ? currentContext->GetFrameCompositionTexture() : nullptr;

    bool needsCommandList = renderImGuiOverlay || finishGpuTiming || frameCompositionTexture;
#ifdef USE_MIPMAP_STREAMING
    needsCommandList |= textureStreamingFeedbackEnabled;
    TextureStreamingFeedback::ReadbackFrame *readbackFrame = nullptr;
#endif

    if (needsCommandList) {
        mainCommandList = frameData->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

#ifdef USE_MIPMAP_STREAMING
        if (textureStreamingFeedbackEnabled) {
            readbackFrame = currentContext->textureStreamingFeedback.Resolve(mainCommandList);
        }
#endif

        if (renderImGuiOverlay) {
            GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ImGuiOverlay);

            BE1::Rect viewportRect(0, 0, currentContext->GetWidth(), currentContext->GetHeight());

            // 뷰포트 & ScissorRect 의 초기값 설정
            RHI::renderer->SetViewport(mainCommandList, viewportRect);
            RHI::renderer->SetScissorRect(mainCommandList, viewportRect);

            RHI::RenderPassImage outputImage = RHI::RenderPassImage::Color(frameCompositionTexture, RHI::ViewID::PrimaryID, RHI::RenderPassImage::LoadAction::Load,
                RHI::RenderPassImage::StoreAction::Store, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResource);
            RHI::renderer->BeginRenderPass(mainCommandList, &outputImage, 1);

            frameData->RenderPendingImGuiOverlay(mainCommandList);

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
    frameData->SetFrameFenceValue(fenceValue);

    if (gpuTimingEnabled) {
        gpuTiming.SetFrameFence(fenceValue);
    }

    if (frameData->IsStatEnabled()) {
        UpdateFrameStats();
    }

#ifdef USE_MIPMAP_STREAMING
    if (readbackFrame) {
        readbackFrame->SetReadbackFenceValue(fenceValue);
    }
#endif

    return (const void *)(cmd + 1);
}

const void *RenderBackend::ExecuteSwapBuffers(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteSwapBuffers");

    const SwapBuffersRenderCommand *cmd = reinterpret_cast<const SwapBuffersRenderCommand *>(data);

    // 백버퍼를 전면버퍼와 교환한다.
    currentContext->GetSwapChain()->SwapBuffers(r_vsync.GetBool());

    PROFILER_FRAME_MARK();

    frameCount++;

    return (const void *)(cmd + 1);
}

void RenderBackend::DrawCamera3DDepthPrepass() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera3DDepthPrepass");

    for (uint32_t passIndex = 0; passIndex < currentVisCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentVisCamera->renderPasses[passIndex];
        if (renderPass->type == RenderPassType::DepthPrepass) {
            DrawGenericPass(renderPass);
        }
    }
}

void RenderBackend::DrawCamera3DScenePass() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera3DScenePass");

    if (const RenderPass *opaquePass = FindRenderPass(currentVisCamera, RenderPassType::Opaque)) {
        DrawGenericPass(opaquePass);
    }

    if (const RenderPass *skyboxPass = FindRenderPass(currentVisCamera, RenderPassType::Skybox)) {
        DrawSkyPass(skyboxPass);
    }
}

void RenderBackend::DrawCamera3DPostOpaquePass() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera3DPostOpaquePass");

    // GpuOcean surface 는 Transparent surface 보다 먼저 렌더링한다.
    DrawGpuOcean();

    if (const RenderPass *transparentPass = FindRenderPass(currentVisCamera, RenderPassType::Transparent)) {
        DrawGenericPass(transparentPass);
    }

    DrawDebugPrimitives();
}

void RenderBackend::DrawCamera2D() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera2D");

    for (uint32_t passIndex = 0; passIndex < currentVisCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentVisCamera->renderPasses[passIndex];
        if (renderPass->type == RenderPassType::Gui) {
            DrawGuiPass(renderPass);
        }
    }
}

void RenderBackend::UploadStaticDataToGPU(const RenderPass *renderPass) const {
    for (uint32_t drawSurfIndex = 0; drawSurfIndex < renderPass->numDrawSurfs; ++drawSurfIndex) {
        const VisSurf *visSurf = renderPass->drawSurfs[drawSurfIndex]->visSurf;

        if (!visSurf->subMesh->vertexBuffer) {
            visSurf->subMesh->UploadStaticDataToGPU();
        }
    }
}

void RenderBackend::SetPassVertexBuffers(RHI::CommandList *commandList, const RenderPassDrawSurf *drawSurf) const {
    const SubMesh *subMesh = drawSurf->visSurf->subMesh;

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

void RenderBackend::DrawGenericPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs == 0) {
        return;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTagForRenderPass(renderPass->type));

    UploadStaticDataToGPU(renderPass);

    const SurfaceInstanceBatch *hiZCullBatches = nullptr;
    uint32_t hiZCullBatchCount = 0;

    if (currentVisCamera->hiZOcclusionCulling && IsHiZCullRenderPass(renderPass->type)) {
        const uint32_t passIndex = static_cast<uint32_t>(renderPass - currentVisCamera->renderPasses);

        if (passIndex < static_cast<uint32_t>(hiZCullPassRanges.Count())) {
            const HiZCullPassRange &passRange = hiZCullPassRanges[passIndex];

            if (passRange.numBatches > 0) {
                hiZCullBatches = hiZCullDrawBatches.Ptr() + passRange.firstBatchIndex;
                hiZCullBatchCount = passRange.numBatches;
            }
        }
    }

    DrawGenericSurfaces(renderPass->drawSurfs, numDrawSurfs, hiZCullBatches, hiZCullBatchCount);
}

void RenderBackend::DrawSkyPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSkyPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs == 0) {
        return;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SkyPass);

    UploadStaticDataToGPU(renderPass);

    DrawSkySurfaces(renderPass->drawSurfs, numDrawSurfs);
}

void RenderBackend::DrawGpuOcean() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGpuOcean");

    const VisOceanSurface *visOceanSurface = frameData->GetVisOceanSurface();
    if (!visOceanSurface || BE1::HasFlag(currentVisCamera->flags, RenderCamera::Flag::StaticOnly) || !currentVisCamera->oceanSurfacePSO) {
        return;
    }

    GpuOcean &gpuOcean = renderSystem->GetGpuOcean();
    if (!gpuOcean.AreCascadeOutputsReady() || !gpuOcean.IsSurfaceGridReady()) {
        return;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::OceanPass);
    GpuOcean::SurfaceView surfaceView;
    surfaceView.origin = currentVisCamera->origin;
    surfaceView.infiniteZFar = currentVisCamera->projectionInfiniteZFar;
    if (currentVisCamera->projectionOrthogonal) {
        surfaceView.box = &currentVisCamera->viewBox;
    } else {
        surfaceView.frustum = &currentVisCamera->viewFrustum;
    }

    GpuOcean::SurfaceDrawStats drawStats;
    if (gpuOcean.DrawSurface(mainCommandList, frameData->GetThreadData(0), currentVisCamera->oceanSurfacePSO, frameConstantBuffer, cameraConstantBuffer, visOceanSurface->surfaceSettings, surfaceView,
        currentVisCamera->snapshotColorTexture, currentVisCamera->snapshotDepthTexture, &drawStats)) {
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

    DrawGenericSurfacesWithoutTask(renderPass->drawSurfs, numDrawSurfs);
}

bool RenderBackend::DispatchLightCulling() {
    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::LightCulling);

    return frameData->DispatchLightCulling(frameData->GetThreadData(0), mainCommandList, frameLightCullingPSO);
}

static bool CanBatchSurfaceInstances(const RenderPassDrawSurf *baseDrawSurf, const RenderPassDrawSurf *drawSurf) {
    return drawSurf->pso &&
        drawSurf->pso == baseDrawSurf->pso &&
        drawSurf->visSurf->subMesh == baseDrawSurf->visSurf->subMesh &&
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
        shaderSurfaceIndices[instanceIndex] = drawSurfs[instanceIndex]->visSurf->index;
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

    const VisSurf *firstVisSurf = drawSurfs[0]->visSurf;

    RHI::renderer->SetConstants(commandList, 2, &surfaceDrawParams, sizeof(surfaceDrawParams), 0);
    RHI::renderer->SetBuffer(commandList, 0, false, batch.instanceToShaderSurfaceTable);

    if (batch.indirectArgsBuffer) {
        RHI::renderer->DrawIndexedInstancedIndirect(commandList, batch.indirectArgsBuffer, batch.indirectArgsOffset);
    } else {
        RHI::renderer->DrawIndexedInstanced(commandList, firstVisSurf->subMesh->numIndexes, batch.instanceCount, 0, 0, 0);

        drawTriangleCount += firstVisSurf->subMesh->numIndexes / 3 * batch.instanceCount;
    }

    drawCount++;
}
