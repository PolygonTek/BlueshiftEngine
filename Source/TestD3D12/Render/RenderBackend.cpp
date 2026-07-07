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

    // GPU 타임 스탬프가 초당 몇번 증가하는지를 얻어온다.
    gpuTimestampFrequency = RHI::renderer->GetTimestampFrequency(RHI::CommandQueueType::Graphics);

    // timestamp heap: Begin[0], End[0], Begin[1], End[1], Begin[2], End[2], ...
    RHI::QueryHeapDesc queryHeapDesc;
    queryHeapDesc.type = RHI::QueryType::Timestamp;
    queryHeapDesc.queryCount = GpuTimingSlotCount << 1; // 슬롯 별로 Timestamp 두개 (시작/끝) 가 필요하므로 2 를 곱한다.
    gpuTimestampQueryHeap = RHI::renderer->CreateQueryHeap(&queryHeapDesc);

    gpuTimestampReadbackBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Readback, RHI::BufferFlag::None, sizeof(uint64_t) * queryHeapDesc.queryCount, nullptr);
}

void RenderBackend::Shutdown() {
    if (gpuTimestampReadbackBuffer) {
        RHI::renderer->DestroyBuffer(gpuTimestampReadbackBuffer);
        gpuTimestampReadbackBuffer = nullptr;
    }

    if (gpuTimestampQueryHeap) {
        RHI::renderer->DestroyQueryHeap(gpuTimestampQueryHeap);
        gpuTimestampQueryHeap = nullptr;
    }
}

void RenderBackend::UpdateRenderFrameTime(uint64_t frameUsec) {
    static constexpr int StatUpdateFrameCount = 60;

    statRenderElapsedUsec += frameUsec;
    statRenderFrameCount++;

    if (statRenderFrameCount >= StatUpdateFrameCount) {
        statRenderAverageUsec.store((float)statRenderElapsedUsec / statRenderFrameCount);

        statRenderElapsedUsec = 0;
        statRenderFrameCount = 0;
    }
}

void RenderBackend::BeginGpuFrameTiming(RHI::CommandList *commandList) {
    if (!frameData->IsStatEnabled()) {
        return;
    }

    if (gpuTimingCurrentSlot >= 0) {
        // 이미 BeginGpuFrameTiming 이 호출되었다면, SetGpuFrameTimingFence 가 호출될 때까지 무시한다.
        return;
    }

    // GpuTimingSampleInterval 에 따라 한번씩만 처리한다.
    if ((currentContext->gpuTimingSampleFrameCount++ & (GpuTimingSampleInterval - 1)) != 0) {
        return;
    }

    // 완료된 슬롯들의 타임스탬프를 읽어서 처리한다.
    ReadCompletedGpuTimings();

    for (int i = 0; i < GpuTimingSlotCount; ++i) {
        int slotIndex = (gpuTimingNextSlot + i) % GpuTimingSlotCount;
        GpuTimingSlot &slot = gpuTimingSlots[slotIndex];

        if (!slot.pending) {
            gpuTimingCurrentSlot = slotIndex;
            gpuTimingNextSlot = (slotIndex + 1) % GpuTimingSlotCount;

            slot.renderContext = currentContext;

            // Begin Timestamp 찍기
            uint32_t stampIndex = slotIndex << 1;
            RHI::renderer->EndQuery(commandList, gpuTimestampQueryHeap, stampIndex);
            return;
        }
    }
}

void RenderBackend::EndGpuFrameTiming(RHI::CommandList *commandList) {
    if (gpuTimingCurrentSlot < 0) {
        return;
    }

    uint32_t stampIndex = gpuTimingCurrentSlot << 1;

    // End Timestamp 찍기
    RHI::renderer->EndQuery(commandList, gpuTimestampQueryHeap, stampIndex + 1);

    // 현재 슬롯의 Timestamp 2개를 Readback 버퍼로 읽어오기
    RHI::renderer->ResolveQuery(commandList, gpuTimestampQueryHeap, stampIndex, 2, gpuTimestampReadbackBuffer, sizeof(uint64_t) * stampIndex);
}

void RenderBackend::SetGpuFrameTimingFence(uint64_t fenceValue) {
    if (gpuTimingCurrentSlot < 0) {
        return;
    }

    GpuTimingSlot &slot = gpuTimingSlots[gpuTimingCurrentSlot];
    slot.fenceValue = fenceValue;
    slot.pending = true;
    gpuTimingCurrentSlot = -1;
}

void RenderBackend::ReadCompletedGpuTimings() {
    const uint64_t *timestamps = static_cast<const uint64_t *>(gpuTimestampReadbackBuffer->mappedPtr);

    for (int slotIndex = 0; slotIndex < GpuTimingSlotCount; ++slotIndex) {
        GpuTimingSlot &slot = gpuTimingSlots[slotIndex];
        if (!slot.pending) {
            continue;
        }
        if (!RHI::renderer->IsFenceComplete(RHI::CommandQueueType::Graphics, slot.fenceValue)) {
            continue;
        }

        uint64_t beginStamp = timestamps[(slotIndex << 1) + 0];
        uint64_t endStamp = timestamps[(slotIndex << 1) + 1];

        if (endStamp >= beginStamp && slot.renderContext) {
            float frameMsec = 1000.0f * (float)(endStamp - beginStamp) / gpuTimestampFrequency;
            slot.renderContext->UpdateGpuFrameTime(frameMsec);
        }

        slot.renderContext = nullptr;
        slot.fenceValue = 0;
        slot.pending = false;
    }
}

void RenderBackend::DiscardGpuFrameTimings(const RenderContext *renderContext) {
    for (int slotIndex = 0; slotIndex < GpuTimingSlotCount; ++slotIndex) {
        GpuTimingSlot &slot = gpuTimingSlots[slotIndex];

        if (slot.renderContext != renderContext) {
            continue;
        }

        if (gpuTimingCurrentSlot == slotIndex) {
            gpuTimingCurrentSlot = -1;
        }

        slot.renderContext = nullptr;
        slot.fenceValue = 0;
        slot.pending = false;
    }
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

void RenderBackend::RecordDrawStats(uint32_t drawCount, uint64_t drawTriangleCount, uint32_t instanceTableAllocCount, uint32_t instanceTableUploadBytes) {
    if (!frameData->IsStatEnabled()) {
        return;
    }

    statDrawCount += drawCount;
    statDrawTriangleCount += drawTriangleCount;
    statInstanceTableAllocCount += instanceTableAllocCount;
    statInstanceTableUploadBytes += instanceTableUploadBytes;
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
    constants->feedbackBufferIndex = currentContext->textureStreamingFeedback.GetBindlessIndex();
    constants->padding0 = 0;
    constants->padding1 = 0;

    return true;
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
    static_assert(sizeof(constants->viewProjMatrix) == sizeof(currentVisCamera->viewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->viewProjMatrix, &currentVisCamera->viewProjMatrix, sizeof(constants->viewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->invViewProjMatrix, &currentVisCamera->invViewProjMatrix, sizeof(constants->invViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->prevViewProjMatrix, &currentVisCamera->prevViewProjMatrix, sizeof(constants->prevViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->prevInvViewProjMatrix, &currentVisCamera->prevInvViewProjMatrix, sizeof(constants->prevInvViewProjMatrix));
    BE1::simdProcessor->Memcpy(&constants->hiZCullViewProjMatrix, &currentVisCamera->hiZCullViewProjMatrix, sizeof(constants->hiZCullViewProjMatrix));
    constants->surfaceTableIndex = frameData->GetShaderSurfaceTableBindlessIndex();
    constants->ndcFarZ = currentVisCamera->projectionPolicy.GetNDCFarZ();
    constants->padding1 = 0;
    constants->padding2 = 0;

    return true;
}

void RenderBackend::UpdateFramePSOs() {
    frameGenHiZMipFirstBatchPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipFirstBatchPipeline);
    frameGenHiZMipBatchPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipBatchPipeline);
    frameGenHiZMipOddDimPSO = ShaderPipeline::GetReadyPSO(renderSystem->genHiZMipOddDimPipeline);
    frameHiZCullInstancesPSO = ShaderPipeline::GetReadyPSO(renderSystem->hiZCullInstancesPipeline);
    frameHiZDebugPreviewPSO = ShaderPipeline::GetReadyPSO(renderSystem->hiZDebugPreviewPipeline);
    frameToneMappingPSO = ShaderPipeline::GetReadyPSO(renderSystem->toneMappingPipeline);
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

    BeginGpuFrameTiming(mainCommandList);

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

    // 이번 프레임에 사용할 상수 데이터들을 준비한다.
    UploadFrameConstants();

    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
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

    if (!currentVisCamera->is2D &&
        (currentVisCamera->renderWidth == 0 ||
        currentVisCamera->renderHeight == 0 ||
        !currentVisCamera->rtColorTexture ||
        !currentVisCamera->depthTexture)) {
        BE_WARNLOG("RenderBackend::ExecuteDrawCamera: invalid camera render target\n");
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

    if (currentVisCamera->clearMethod == RenderCameraClearMethod::Color) {
        clearFlags |= (RHI::ClearFlag::Color | RHI::ClearFlag::Depth);
        if (currentVisCamera->clearColorMode == RenderCameraClearColorMode::Explicit) {
            clearColor = currentVisCamera->clearColor;
            useExplicitClearColor = true;
        }
    } else if (currentVisCamera->clearMethod == RenderCameraClearMethod::DepthOnly || currentVisCamera->clearMethod == RenderCameraClearMethod::Skybox) {
        clearFlags |= RHI::ClearFlag::Depth;
    }

    if (currentVisCamera->is2D) {
        const RHI::ClearFlag swapChainClearFlags = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::ClearFlag::Color : RHI::ClearFlag::None;
        RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, nullptr, clearColor, clearDepth, 0, swapChainClearFlags);

        DrawCamera2D();

        RHI::renderer->EndRenderPass(mainCommandList);
    } else {
        const bool hiZCullingPrepared = PrepareHiZCulling();

        RHI::RenderPassImage::LoadAction colorLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
        if (currentVisCamera->clearMethod == RenderCameraClearMethod::Skybox) {
            colorLoadAction = RHI::RenderPassImage::LoadAction::DontCare;
        }
        RHI::RenderPassImage::LoadAction depthLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
        RHI::ClearValue clearColorValue = RHI::ClearValue::Color(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        RHI::RenderPassImage depthImage = RHI::RenderPassImage::DepthStencil(currentVisCamera->depthTexture, RHI::ViewID::PrimaryID, depthLoadAction, RHI::RenderPassImage::StoreAction::Store,
            RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::ShaderResource);

        if (currentVisCamera->rtColorMSAATexture) {
            RHI::RenderPassImage colorImage = RHI::RenderPassImage::Color(currentVisCamera->rtColorMSAATexture, RHI::ViewID::PrimaryID, colorLoadAction);
            if (useExplicitClearColor) {
                colorImage = colorImage.WithClearValue(clearColorValue);
            }

            RHI::RenderPassImage renderPassImages[] = {
                colorImage,
                depthImage,
                RHI::RenderPassImage::ResolveColor(currentVisCamera->rtColorTexture, RHI::ViewID::PrimaryID, 0)
            };
            RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));
        } else {
            RHI::RenderPassImage colorImage = RHI::RenderPassImage::Color(currentVisCamera->rtColorTexture, RHI::ViewID::PrimaryID, colorLoadAction);
            if (useExplicitClearColor) {
                colorImage = colorImage.WithClearValue(clearColorValue);
            }

            RHI::RenderPassImage renderPassImages[] = {
                colorImage,
                depthImage
            };
            RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));
        }

        DrawCamera3D();

        RHI::renderer->EndRenderPass(mainCommandList);

        if (hiZCullingPrepared) {
            FinishHiZCulling();
        }

        if (currentVisCamera->generateHiZ && !r_hiZOcclusionCullingFreeze.GetBool()) {
            DispatchGenHiZ();
        }

        if (currentVisCamera->hiZDebugPreview) {
            DispatchHiZDebugPreview();
        }

        PostProcessAndBlitToSwapChain();
    }

    // command list 에 기록을 마치고 실행
    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
}

void RenderBackend::PostProcessAndBlitToSwapChain() {
    const RHI::Texture *finalColorTexture = currentVisCamera->rtColorTexture;

    if (!r_skipPostProcess.GetBool() && !BE1::HasFlag(currentVisCamera->flags, RenderCameraFlag::SkipPostProcess)) {
        const bool hasLuminance = DispatchLuminance();
        const bool hasBloom = hasLuminance && DispatchBloom();

        if (hasLuminance && DispatchToneMapping(hasBloom)) {
            finalColorTexture = currentVisCamera->postProcessColorTexture;
        }
    }

    BlitRectToSwapChain(finalColorTexture, currentVisCamera->outputRect);
}

void RenderBackend::BlitRectToSwapChain(const RHI::Texture *texture, const BE1::Rect &rect) {
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

    if (rect.w <= 0 || rect.h <= 0) {
        return;
    }

    const int outputX0 = BE1::Max(rect.x, 0);
    const int outputY0 = BE1::Max(rect.y, 0);
    const int outputX1 = BE1::Min(rect.x + rect.w, static_cast<int>(w));
    const int outputY1 = BE1::Min(rect.y + rect.h, static_cast<int>(h));
    if (outputX1 <= outputX0 || outputY1 <= outputY0) {
        return;
    }

    const uint32_t outputW = static_cast<uint32_t>(outputX1 - outputX0);
    const uint32_t outputH = static_cast<uint32_t>(outputY1 - outputY0);

    FullscreenImageParams imageParams = {};
    imageParams.texCoordScale = float2(static_cast<float>(outputW) / rect.w, static_cast<float>(outputH) / rect.h);
    imageParams.texCoordOffset = float2(static_cast<float>(outputX0 - rect.x) / rect.w, static_cast<float>(outputY0 - rect.y) / rect.h);

    BE1::Rect outputRect(outputX0, outputY0, outputW, outputH);

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
        BE1::Rect viewportRect(0, 0, currentContext->GetWidth(), currentContext->GetHeight());

        // 뷰포트 & ScissorRect 의 초기값 설정
        RHI::renderer->SetViewport(mainCommandList, viewportRect);
        RHI::renderer->SetScissorRect(mainCommandList, viewportRect);

        RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, nullptr);

        frameData->RenderPendingImGuiOverlay(mainCommandList);

        RHI::renderer->EndRenderPass(mainCommandList);
    }

    EndGpuFrameTiming(mainCommandList);

    mainCommandList->CloseAndExecute();

    // 이번 백엔드 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    uint64_t fenceValue = RHI::renderer->SignalFence(RHI::CommandQueueType::Graphics);
    frameData->SetFrameFenceValue(fenceValue);

    SetGpuFrameTimingFence(fenceValue);

    if (frameData->IsStatEnabled()) {
        currentContext->SetRenderStats(frameData->GetStatRenderObjectCount(), frameData->GetStatVisibleObjectCount(), statDrawCount, statDrawTriangleCount, statInstanceTableAllocCount, statInstanceTableUploadBytes);
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

void RenderBackend::DrawCamera3D() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawCamera3D");

    for (uint32_t passIndex = 0; passIndex < currentVisCamera->numRenderPasses; ++passIndex) {
        const RenderPass *renderPass = &currentVisCamera->renderPasses[passIndex];
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

void RenderBackend::DrawGenericPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGenericPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs > 0) {
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
}

void RenderBackend::DrawSkyPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSkyPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs > 0) {
        UploadStaticDataToGPU(renderPass);

        DrawSkySurfaces(renderPass->drawSurfs, numDrawSurfs);
    }
}

void RenderBackend::DrawGuiPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGuiPass");

    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;
    if (numDrawSurfs > 0) {
        DrawGenericSurfacesWithoutTask(renderPass->drawSurfs, numDrawSurfs);
    }
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
