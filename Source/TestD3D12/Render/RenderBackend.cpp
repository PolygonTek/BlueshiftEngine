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
#include "RenderCvars.h"
#include "RenderContext.h"
#include "RenderSystem.h"
#include "RenderInternal.h"
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
        case RenderCommandId::BeginContext:
            data = ExecuteBeginContext(data);
            continue;
        case RenderCommandId::DrawCamera:
            data = ExecuteDrawCamera(data);
            continue;
        case RenderCommandId::ScreenShot:
            data = ExecuteScreenshot(data);
            continue;
        case RenderCommandId::GenerateConvolutionBloomKernel:
            data = ExecuteGenerateConvolutionBloomKernel(data);
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

        const uint32_t threadGroupCountX = (visCamera.lightCulling.tileCountX + ShaderLightCullingThreadGroupSizeX - 1) / ShaderLightCullingThreadGroupSizeX;
        const uint32_t threadGroupCountY = (visCamera.lightCulling.tileCountY + ShaderLightCullingThreadGroupSizeY - 1) / ShaderLightCullingThreadGroupSizeY;

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
    constants->feedbackBufferIndex = currentContext->textureStreamingFeedback.GetBindlessIndex();

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

    return true;
}

void RenderBackend::UpdateFramePSOs() {
    frameGenHiZMipFirstBatchPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipFirstBatchPipeline);
    frameGenHiZMipFirstBatchMSAAPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipFirstBatchMSAAPipeline);
    frameGenHiZMipBatchPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipBatchPipeline);
    frameGenHiZMipOddDimPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipOddDimPipeline);
    frameHiZCullInstancesPSO = ShaderPipeline::GetReadyPSO(renderSystem->hiZCullInstancesPipeline);
    frameLightCullingPSO = ShaderPipeline::GetReadyPSO(renderSystem->lightCullingPipeline);
    frameHiZDebugPreviewPSO = ShaderPipeline::GetReadyPSO(renderSystem->hiZDebugPreviewPipeline);
    frameDepthHistoryResolvePSO = ShaderPipeline::GetReadyPSO(renderSystem->depthHistoryResolvePipeline);
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
    frameConvolutionBloomPrefilterPSO = ShaderPipeline::GetReadyPSO(renderSystem->convolutionBloomPrefilterPipeline);
    frameConvolutionBloomKernelPreparePSO = ShaderPipeline::GetReadyPSO(renderSystem->convolutionBloomKernelPreparePipeline);
    frameConvolutionBloomResolvePSO = ShaderPipeline::GetReadyPSO(renderSystem->convolutionBloomResolvePipeline);
}

const void *RenderBackend::ExecuteBeginContext(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteBeginContext");

    const BeginContextRenderCommand *cmd = reinterpret_cast<const BeginContextRenderCommand *>(data);

    currentContext = cmd->renderContext;
    frameImagePSO = cmd->imagePSO;

    statDrawCount = 0;
    statDrawTriangleCount = 0;
    statInstanceTableAllocCount = 0;
    statInstanceTableUploadBytes = 0;

    mainCommandList = frameData->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

    gpuTiming.BeginFrame(mainCommandList, frameData, currentContext);

#ifdef USE_MIPMAP_STREAMING
    currentContext->textureStreamingFeedback.ClearBuffer(mainCommandList);
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
    float clearDepth = currentVisCamera->projectionPolicy.GetDepthClearValue();
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
        ExecuteDrawCamera2D(clearFlags, clearColor, clearDepth);
    } else {
        ExecuteDrawCamera3D(clearFlags, clearColor, useExplicitClearColor);
    }

    // command list 에 기록을 마치고 실행
    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
}

void RenderBackend::ExecuteDrawCamera2D(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor, float clearDepth) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteDrawCamera2D");

    const RHI::ClearFlag swapChainClearFlags = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::ClearFlag::Color : RHI::ClearFlag::None;
    RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, nullptr, clearColor, clearDepth, 0, swapChainClearFlags);

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

    // main pass
    const RHI::RenderPassImage::LoadAction mainDepthLoadAction = hasDepthPrepass ? RHI::RenderPassImage::LoadAction::Load : depthLoadAction;
    ExecuteDrawCamera3DMainPass(clearFlags, clearColor, useExplicitClearColor, mainDepthLoadAction, hasDepthPrepass);

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

    PostProcessAndBlitToSwapChain(surfaceOutputsReady);
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

void RenderBackend::ExecuteDrawCamera3DMainPass(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor, bool useExplicitClearColor,
    RHI::RenderPassImage::LoadAction depthLoadAction, bool skipDepthPrepasses) {
    RHI::ClearValue clearColorValue = RHI::ClearValue::Color(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    RHI::RenderPassImage::LoadAction colorLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
    if (currentVisCamera->clearMethod == RenderCamera::ClearMethod::Skybox) {
        colorLoadAction = RHI::RenderPassImage::LoadAction::DontCare;
    }

    const bool useMSAA = currentVisCamera->rtColorMSAATexture != nullptr;

    // 나머지 모든 render pass 를 렌더링한다. 활성화된 surface output 만 빈 슬롯 없이 MRT 에 추가한다.
    RHI::RenderPassImage renderPassImages[5];
    uint32_t renderPassImageCount = 0;

    const RHI::Texture *colorTexture = useMSAA ? currentVisCamera->rtColorMSAATexture : currentVisCamera->rtColorTexture;
    RHI::RenderPassImage colorImage = RHI::RenderPassImage::Color(colorTexture, RHI::ViewID::PrimaryID, colorLoadAction);
    if (useExplicitClearColor) {
        colorImage = colorImage.WithClearValue(clearColorValue);
    }
    renderPassImages[renderPassImageCount++] = colorImage;

    const RHI::ClearValue zeroClearValue = RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f);
    if (currentVisCamera->needNormalOutput) {
        const RHI::Texture *normalTexture = useMSAA ? currentVisCamera->rtNormalMSAATexture : currentVisCamera->rtNormalTexture;
        renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::Color(normalTexture, RHI::ViewID::PrimaryID, RHI::RenderPassImage::LoadAction::Clear).WithClearValue(zeroClearValue);
    }
    if (currentVisCamera->needVelocityOutput) {
        const RHI::Texture *velocityTexture = useMSAA ? currentVisCamera->rtVelocityMSAATexture : currentVisCamera->rtVelocityTexture;
        renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::Color(velocityTexture, RHI::ViewID::PrimaryID, RHI::RenderPassImage::LoadAction::Clear).WithClearValue(zeroClearValue);
    }

    renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::DepthStencil(currentVisCamera->depthTexture, RHI::ViewID::PrimaryID, depthLoadAction, RHI::RenderPassImage::StoreAction::Store,
        RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::ShaderResource);

    if (useMSAA) {
        renderPassImages[renderPassImageCount++] = RHI::RenderPassImage::ResolveColor(currentVisCamera->rtColorTexture, RHI::ViewID::PrimaryID, 0);
    }
    RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, renderPassImageCount);

    DrawCamera3D(skipDepthPrepasses);

    RHI::renderer->EndRenderPass(mainCommandList);
}

void RenderBackend::PostProcessAndBlitToSwapChain(bool surfaceOutputsReady) {
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

    BlitTextureToSwapChain(finalColorTexture, currentVisCamera->outputRect);
    currentPostProcessInputTexture = nullptr;
}

void RenderBackend::BlitTextureToSwapChain(const RHI::Texture *texture, const BE1::Rect &destRect) {
    if (!texture) {
        return;
    }

    if (!frameImagePSO) {
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

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::BlitToSwapChain);

    RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, nullptr);
    RHI::renderer->SetViewport(mainCommandList, outputRect);
    RHI::renderer->SetScissorRect(mainCommandList, outputRect);
    RHI::renderer->SetPSO(mainCommandList, frameImagePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, texture);
    RHI::renderer->SetConstants(mainCommandList, 0, &imageParams, sizeof(imageParams), 0);
    RHI::renderer->Draw(mainCommandList, 3, 0);
    RecordDrawStats(1, 1, 0, 0);
    RHI::renderer->EndRenderPass(mainCommandList);
}

const void *RenderBackend::ExecuteScreenshot(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteScreenshot");

    const ScreenShotRenderCommand *cmd = reinterpret_cast<const ScreenShotRenderCommand *>(data);

    // 캡쳐 영역 Rect
    BE1::Rect captureRect(cmd->x, cmd->y, cmd->width, cmd->height);

    // SwapChain 백버퍼를 캡쳐해서 저장할 빈 이미지 (메모리) 를 생성한다.
    BE1::Image screenImage;
    screenImage.Create2D(captureRect.w, captureRect.h, 1, BE1::Image::Format::B8G8R8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);

    RHI::CommandList *commandList = frameData->GetThreadData(0)->AllocGraphicsCommandList();
    commandList->Reset(true);

    // 백버퍼의 내용을 (필요하다면 지정된 포맷으로 컨버팅하여) screenImage 에 저장한다.
    RHI::renderer->ReadPixels(commandList, currentContext->GetSwapChain(), captureRect.x, captureRect.y, captureRect.w, captureRect.h, BE1::Image::Format::B8G8R8, screenImage.GetPixels());

    // 이제 이미지 파일로 저장한다.
    BE1::Str filename = cmd->filename;
    filename.DefaultFileExtension(".png");
    screenImage.Write(filename);

    return (const void *)(cmd + 1);
}

const void *RenderBackend::ExecuteGenerateConvolutionBloomKernel(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteGenerateConvolutionBloomKernel");

    const GenerateConvolutionBloomKernelRenderCommand *cmd = reinterpret_cast<const GenerateConvolutionBloomKernelRenderCommand *>(data);

    ConvolutionBloomKernelGenerator &kernelGenerator = renderSystem->GetConvolutionBloomKernelGenerator();
    kernelGenerator.SetApertureScale(cmd->apertureScale);
    kernelGenerator.SetDiffractionPSFScale(cmd->diffractionPSFScale);
    kernelGenerator.SetDispersionAmount(cmd->dispersionAmount);

    if (!kernelGenerator.IsKernelDirty() && kernelGenerator.HasResources()) {
        return (const void *)(cmd + 1);
    }

    mainCommandList = frameData->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

    if (!kernelGenerator.GenerateKernel(mainCommandList, frameData)) {
        BE_WARNLOG("RenderBackend::ExecuteGenerateConvolutionBloomKernel: failed to generate convolution bloom kernel\n");
    }

    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
}

const void *RenderBackend::ExecuteSwapBuffers(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteSwapBuffers");

    const SwapBuffersRenderCommand *cmd = reinterpret_cast<const SwapBuffersRenderCommand *>(data);

    mainCommandList = frameData->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

#ifdef USE_MIPMAP_STREAMING
    TextureStreamingFeedback::ReadbackFrame *readbackFrame = currentContext->textureStreamingFeedback.Resolve(mainCommandList);
#endif

    const bool hasPendingImGuiOverlay = frameData->HasPendingImGuiOverlay();
    if (hasPendingImGuiOverlay) {
        GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::ImGuiOverlay);

        BE1::Rect viewportRect(0, 0, currentContext->GetWidth(), currentContext->GetHeight());

        // 뷰포트 & ScissorRect 의 초기값 설정
        RHI::renderer->SetViewport(mainCommandList, viewportRect);
        RHI::renderer->SetScissorRect(mainCommandList, viewportRect);

        RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, nullptr);

        frameData->RenderPendingImGuiOverlay(mainCommandList);

        RHI::renderer->EndRenderPass(mainCommandList);
    }

    gpuTiming.EndFrame(mainCommandList);

    mainCommandList->CloseAndExecute();

    // 이번 백엔드 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    uint64_t fenceValue = RHI::renderer->SignalFence(RHI::CommandQueueType::Graphics);
    frameData->SetFrameFenceValue(fenceValue);

    gpuTiming.SetFrameFence(fenceValue);

    if (frameData->IsStatEnabled()) {
        UpdateFrameStats();
    }

#ifdef USE_MIPMAP_STREAMING
    if (readbackFrame) {
        readbackFrame->SetReadbackFenceValue(fenceValue);
    }
#endif

    // 백버퍼를 전면버퍼와 교환한다.
    currentContext->GetSwapChain()->SwapBuffers(false);

    PROFILER_FRAME_MARK();

    frameCount++;

    return (const void *)(cmd + 1);
}

void RenderBackend::DrawCamera3D(bool skipDepthPrepasses) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera3D");

    for (uint32_t passIndex = 0; passIndex < currentVisCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentVisCamera->renderPasses[passIndex];
        if (skipDepthPrepasses) {
            if (renderPass->type == RenderPassType::DepthPrepass) {
                continue;
            }
        }

        switch (renderPass->type) {
        case RenderPassType::Skybox:
            DrawSkyPass(renderPass);
            break;
        case RenderPassType::DepthPrepass:
        case RenderPassType::Opaque:
        case RenderPassType::Transparent:
            DrawGenericPass(renderPass);
            break;
        default:
            break;
        }
    }

    DrawDebugPrimitives();
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
