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
#include "RenderContext.h"
#include "RenderInternal.h"

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
    constants->shaderMaterialTableIndex = frameData->GetShaderMaterialTableBindlessIndex();
    constants->feedbackBufferIndex = currentContext->textureStreamingFeedback.buffer ? currentContext->textureStreamingFeedback.buffer->GetBindlessIndex(RHI::ViewType::UAV) : 0;
    constants->padding0 = 0;
    constants->padding1 = 0;

    return true;
}

bool RenderBackend::UploadCameraConstants() {
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
    constants->surfaceTableIndex = frameData->GetShaderSurfaceTableBindlessIndex();

    return true;
}

const void *RenderBackend::ExecuteBeginContext(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteBeginContext");

    const BeginContextRenderCommand *cmd = reinterpret_cast<const BeginContextRenderCommand *>(data);

    currentContext = cmd->renderContext;
    currentImagePSO = cmd->imagePSO;

    statDrawCount = 0;
    statDrawTriangleCount = 0;
    statInstanceTableAllocCount = 0;
    statInstanceTableUploadBytes = 0;

    mainCommandList = frameData->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

    BeginGpuFrameTiming(mainCommandList);

#ifdef USE_MIPMAP_STREAMING
    currentContext->textureStreamingFeedback.Clear(mainCommandList);
#endif

    // 이번 프레임의 카메라들이 참조할 ShaderSurface 테이블 용량을 미리 확보한다.
    if (!frameData->EnsureShaderSurfaceTableCapacity()) {
        BE_WARNLOG("RenderBackend::ExecuteBeginContext: failed to ensure shader surface table capacity\n");
        mainCommandList->CloseAndExecute();
        return (const void *)(cmd + 1);
    }
    assert(frameData->GetShaderSurfaceTableBindlessIndex() != 0);

    // 등록된 ShaderMaterial 테이블을 GPU 에 업로드한다.
    frameData->UploadShaderMaterialTable(frameData->GetThreadData(0), mainCommandList);
    assert(frameData->GetShaderMaterialTableBindlessIndex() != 0);

    // 이번 프레임에 사용할 상수 데이터들을 준비한다.
    UploadFrameConstants();

    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
}

const void *RenderBackend::ExecuteDrawCamera(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteDrawCamera");

    const DrawCameraRenderCommand *cmd = reinterpret_cast<const DrawCameraRenderCommand *>(data);

    currentVisCamera = cmd->visCamera;

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

    // 뷰포트 & ScissorRect 의 초기값 설정
    RHI::renderer->SetViewport(mainCommandList, currentVisCamera->renderRect);
    RHI::renderer->SetScissorRect(mainCommandList, currentVisCamera->renderRect);

    RHI::ClearFlag clearFlags = RHI::ClearFlag::None;
    BE1::Color4 clearColor = BE1::Color4::black;
    float clearDepth = 1.0f;

    if (currentVisCamera->clearMethod == RenderCameraClearMethod::Color) {
        clearFlags |= (RHI::ClearFlag::Color | RHI::ClearFlag::Depth);
        clearColor = currentVisCamera->clearColor;
    } else if (currentVisCamera->clearMethod == RenderCameraClearMethod::DepthOnly || currentVisCamera->clearMethod == RenderCameraClearMethod::Skybox) {
        clearFlags |= RHI::ClearFlag::Depth;
    }

#ifndef USE_OFFSCREEN_MAIN_RENDER_TARGET
    RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, currentContext->mainRTDepthTexture, clearColor, clearDepth, 0, clearFlags);
#else
    RHI::RenderPassImage::LoadAction colorLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Color) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;
    RHI::RenderPassImage::LoadAction depthLoadAction = BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth) ? RHI::RenderPassImage::LoadAction::Clear : RHI::RenderPassImage::LoadAction::Load;

    if (currentContext->mainRTSampleCount > 1) {
        RHI::RenderPassImage renderPassImages[] = {
            RHI::RenderPassImage::Color(currentContext->mainRTColorMSAATexture, RHI::ViewID::PrimaryID, colorLoadAction),
            RHI::RenderPassImage::DepthStencil(currentContext->mainRTDepthTexture, RHI::ViewID::PrimaryID, depthLoadAction),
            RHI::RenderPassImage::ResolveColor(currentContext->mainRTColorTexture, RHI::ViewID::PrimaryID, 0)
        };
        RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));
    } else {
        RHI::RenderPassImage renderPassImages[] = {
            RHI::RenderPassImage::Color(currentContext->mainRTColorTexture, RHI::ViewID::PrimaryID, colorLoadAction),
            RHI::RenderPassImage::DepthStencil(currentContext->mainRTDepthTexture, RHI::ViewID::PrimaryID, depthLoadAction)
        };
        RHI::renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));
    }
#endif

    if (currentVisCamera->is2D) {
        DrawCamera2D();
    } else {
        DrawCamera3D();
    }

    RHI::renderer->EndRenderPass(mainCommandList);

    // command list 에 기록을 마치고 실행
    mainCommandList->CloseAndExecute();

    return (const void *)(cmd + 1);
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

const void *RenderBackend::ExecuteSwapBuffers(const void *data) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::ExecuteSwapBuffers");

    const SwapBuffersRenderCommand *cmd = reinterpret_cast<const SwapBuffersRenderCommand *>(data);

    mainCommandList = frameData->GetThreadData(0)->BeginCommandList(RHI::CommandQueueType::Graphics);

#ifdef USE_MIPMAP_STREAMING
    TextureStreamingFeedback::ReadbackFrame *readbackFrame = currentContext->textureStreamingFeedback.Resolve(mainCommandList);
#endif

    const bool hasPendingImGuiOverlay = frameData->HasPendingImGuiOverlay();

#ifdef USE_OFFSCREEN_MAIN_RENDER_TARGET
    const bool needsSwapChainRenderPass = true;
#else
    const bool needsSwapChainRenderPass = hasPendingImGuiOverlay;
#endif

    if (needsSwapChainRenderPass) {
        BE1::Rect renderRect(0, 0, currentContext->GetWidth(), currentContext->GetHeight());
        if (currentVisCamera) {
            renderRect = currentVisCamera->renderRect;
        }

        // 뷰포트 & ScissorRect 의 초기값 설정
        RHI::renderer->SetViewport(mainCommandList, renderRect);
        RHI::renderer->SetScissorRect(mainCommandList, renderRect);

        RHI::renderer->BeginRenderPass(mainCommandList, currentContext->swapChain, nullptr);
#ifdef USE_OFFSCREEN_MAIN_RENDER_TARGET
        if (currentImagePSO) {
            RHI::renderer->SetPSO(mainCommandList, currentImagePSO);
            RHI::renderer->SetTexture(mainCommandList, 0, false, currentContext->mainRTColorTexture);
            RHI::renderer->Draw(mainCommandList, 3, 0);
            //RecordDrawStats(1, 1, 0, 0);
        }
#endif

        if (hasPendingImGuiOverlay) {
            frameData->RenderPendingImGuiOverlay(mainCommandList);
        }

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
        currentContext->textureStreamingFeedback.SetReadbackFenceValue(readbackFrame, fenceValue);
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
        case RenderPassType::MainOpaque:
        case RenderPassType::MainBlend:
            DrawMainPass(renderPass);
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

void RenderBackend::DrawMainPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawMainPass");

    const PassDrawSurf **drawSurfs = renderPass->drawSurfs;
    const uint32_t numDrawSurfs = renderPass->numDrawSurfs;

    for (uint32_t drawSurfIndex = 0; drawSurfIndex < numDrawSurfs; ++drawSurfIndex) {
        const VisSurf *visSurf = drawSurfs[drawSurfIndex]->visSurf;

        if (!visSurf->subMesh->vertexBuffer) {
            visSurf->subMesh->UploadStaticDataToGPU();
        }
    }

    if (numDrawSurfs > 0) {
        DrawLitSurfaces(drawSurfs, numDrawSurfs);
    }
}

void RenderBackend::DrawGuiPass(const RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawGuiPass");

    RHI::PipelineState *currentPSO = nullptr;
    const SubMesh *currentSubMesh = nullptr;
    VertexInputLayoutId currentInputLayoutId = VertexInputLayoutId::Count;

    uint32_t drawSurfIndex = 0;
    while (drawSurfIndex < renderPass->numDrawSurfs) {
        const PassDrawSurf *drawSurf = renderPass->drawSurfs[drawSurfIndex];
        if (!drawSurf->pso) {
            drawSurfIndex++;
            continue;
        }

        if (currentPSO != drawSurf->pso) {
            currentPSO = drawSurf->pso;
            RHI::renderer->SetPSO(mainCommandList, currentPSO);
            RHI::renderer->SetConstantBuffer(mainCommandList, 0, frameConstantBuffer);
            RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
        }

        if (currentSubMesh != drawSurf->visSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->visSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;
            SetPassVertexBuffers(mainCommandList, drawSurf);
            RHI::renderer->SetIndexBuffer(mainCommandList, currentSubMesh->indexBuffer);
        }

        const uint32_t instanceStartIndex = drawSurfIndex;
        drawSurfIndex = FindSurfaceInstanceBatchEnd(renderPass->drawSurfs, renderPass->numDrawSurfs, instanceStartIndex);

        DrawSurfaceInstances(frameData->GetThreadData(0), mainCommandList, &renderPass->drawSurfs[instanceStartIndex], drawSurfIndex - instanceStartIndex, statDrawCount, statDrawTriangleCount, statInstanceTableAllocCount, statInstanceTableUploadBytes);
    }
}
