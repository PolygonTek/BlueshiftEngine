// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Platform/PlatformSystem.h"
#include "RenderContext.h"
#include "RenderFrameData.h"
#include "VisObject.h"
#include "D3D12Renderer/D3D12Renderer.h"

void RenderContext::Init(HWND hwnd) {
#ifdef USE_RENDER_TASK
    // 렌더 태스크 스레드 개수는 물리코어 개수를 넘지 않는다.
    int numCores = BE1::PlatformSystem::NumCPUCores();
    int numTaskThreads = BE1::Min(numCores, MaxRenderTaskThreads);

    renderTaskManager.Start(numTaskThreads);

    BE_LOG("Rendering task threads (%i) started\n", numTaskThreads);
#else
    int numTaskThreads = 1;
#endif

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Init(numTaskThreads);
    }

    currentFrameIndex = 0;
    frameData[currentFrameIndex].SetFenceValue(renderer->SignalFence(RHI::CommandQueueType::Graphics));

#ifdef USE_RENDER_THREAD
    InitRenderThread();
#endif

    // 윈도우 크기 얻기
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT backBufferWidth = rc.right;
    UINT backBufferHeight = rc.bottom;

    // 스왑 체인 (백버퍼) 생성
    swapChain = renderer->CreateSwapChain(hwnd, backBufferWidth, backBufferHeight, BE1::Image::Format::RGBA_8_8_8_8);

    // Viewport 설정을 백버퍼 크기에 맞게 설정
    viewportRect.x = 0.0f;
    viewportRect.y = 0.0f;
    viewportRect.w = (float)backBufferWidth;
    viewportRect.h = (float)backBufferHeight;

    // ScissorRect 설정을 백버퍼 크기에 맞게 설정
    scissorRect.x = 0;
    scissorRect.y = 0;
    scissorRect.w = backBufferWidth;
    scissorRect.h = backBufferHeight;

    // 렌더 타겟 텍스쳐 & 뎁스 텍스쳐 생성
    CreateMainRenderTextures(backBufferWidth, backBufferHeight);

    InitFullScreenTrianglePSO();
}

void RenderContext::Shutdown() {
#ifdef USE_RENDER_THREAD
    ShutdownRenderThread();
#endif

#ifdef USE_RENDER_TASK
    renderTaskManager.Stop();
#endif

    renderer->Finish(RHI::CommandQueueType::Graphics);
    renderer->Finish(RHI::CommandQueueType::Compute);

    renderer->DestroyPSO(imagePSO);

    DestroyMainRenderTextures();

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Shutdown();
    }

    renderer->DestroySwapChain(swapChain);
}

void RenderContext::OnResize(int width, int height) {
#ifdef USE_RENDER_THREAD
    WaitRenderCompleted();
#endif

    renderer->Finish(RHI::CommandQueueType::Graphics);

    swapChain->Resize(width, height);

    viewportRect.w = static_cast<float>(width);
    viewportRect.h = static_cast<float>(height);

    scissorRect.w = width;
    scissorRect.h = height;

    DestroyMainRenderTextures();
    CreateMainRenderTextures(width, height);
}

void RenderContext::CreateMainRenderTextures(uint32_t width, uint32_t height) {
    BE1::Image colorImage;
    colorImage.InitFromMemory(width, height, 1, 1, 1, mainRTColorFormat, BE1::Image::GammaSpace::Linear, nullptr, 0);
    mainRTColorTexture = renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::RenderTarget | RHI::ResourceFlag::ShaderResource | RHI::ResourceFlag::UnorderedAccess,
        &colorImage, RHI::ClearValue::Color(0.0f, 0.0f, 1.0f, 0.0f), 1);

    if (mainRTSampleCount > 1) {
        mainRTColorMSAATexture = renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::RenderTarget | RHI::ResourceFlag::ShaderResource,
            &colorImage, RHI::ClearValue::Color(0.0f, 0.0f, 1.0f, 0.0f), mainRTSampleCount);
    }

    BE1::Image depthStencilImage;
    depthStencilImage.InitFromMemory(width, height, 1, 1, 1, mainRTDepthFormat, BE1::Image::GammaSpace::Linear, nullptr, 0);
    mainRTDepthTexture = renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::DepthStencil,
        &depthStencilImage, RHI::ClearValue::DepthStencil(1.0f, 0), mainRTSampleCount, RHI::GPUResourceState::DepthWrite);
}

void RenderContext::DestroyMainRenderTextures() {
    if (mainRTColorTexture) {
        renderer->DestroyTexture(mainRTColorTexture, true);
        mainRTColorTexture = nullptr;
    }
    if (mainRTColorMSAATexture) {
        renderer->DestroyTexture(mainRTColorMSAATexture, true);
        mainRTColorMSAATexture = nullptr;
    }
    if (mainRTDepthTexture) {
        renderer->DestroyTexture(mainRTDepthTexture, true);
        mainRTDepthTexture = nullptr;
    }
}

void RenderContext::InitFullScreenTrianglePSO() {
    BE1::Image::Format::Enum imageFormat = BE1::Image::Format::Unknown;
    bool isSRGB = false;
    swapChain->GetFormat(&imageFormat, &isSRGB);

    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = imageFormat;
    renderDest.renderTargetForematSRGBs[0] = isSRGB;

    RHI::Shader *vs = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "VSMain"));
    RHI::Shader *ps = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "PSMain"));

    if (vs && ps) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = vs;
        psoDesc.ps = ps;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHI::DepthStencilStateType::Never);
        psoDesc.blendState = renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        imagePSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (vs) {
        renderer->DestroyShader(vs, true);
    }
    if (ps) {
        renderer->DestroyShader(ps, true);
    }
}

void RenderContext::WaitAllFrameFences() {
    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        renderer->WaitFence(frameData[frameIndex].GetFenceValue());
    }
}

void RenderContext::InitRenderThread() {
    smpLock = BE1::PlatformSRWLock::Create();
    renderCompletedCondition = BE1::PlatformCondition::Create();
    updateCompletedCondition = BE1::PlatformCondition::Create();

    renderThread = BE1::PlatformThread::Start(RenderContext::RenderThreadProc, this);
}

void RenderContext::ShutdownRenderThread() {
    {
        BE1::ScopedWriteLock lock(smpLock);
        isStoppingRenderThread = true;
        BE1::PlatformCondition::Signal(updateCompletedCondition);
    }
    BE1::PlatformThread::Join(renderThread);
    renderThread = nullptr;

    BE1::PlatformCondition::Destroy(renderCompletedCondition);
    BE1::PlatformCondition::Destroy(updateCompletedCondition);
    BE1::PlatformSRWLock::Destroy(smpLock);
}

void RenderContext::WaitRenderCompleted() {
    assert(BE1::Engine::IsInMainThread());

    if (!renderThread) {
        return;
    }

    BE1::ScopedReadLock lock(smpLock);

    // 렌더 스레드가 렌더링이 완료되어 (다음) 업데이트를 기다리는 상태가 될 때까지 기다린다.
    BE1::PlatformCondition::Wait(renderCompletedCondition, smpLock, false, [this] {
        return frameSyncState == FrameSyncState::WaitingForUpdateCompleted;
    });
}

void RenderContext::MarkUpdateCompleted() {
    assert(BE1::Engine::IsInMainThread());

    BE1::ScopedWriteLock lock(smpLock);

    // (렌더 스레드의) 다음 렌더링이 끝나기를 기다리는 상태로 변경
    frameSyncState = FrameSyncState::WaitingForRenderCompleted;

    // 업데이트가 완료되었다고 신호를 보내고, 이후 다음 프레임의 업데이트를 진행한다.
    BE1::PlatformCondition::Signal(updateCompletedCondition);
}

unsigned int RenderContext::RenderThreadProc(void *param) {
    RenderContext *context = reinterpret_cast<RenderContext *>(param);

    BE1::PlatformThread::SetCurrentThreadName("RenderContext::RenderThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    while (1) {
        PIX_CPU_SCOPED_EVENT(7, "RenderThreadProcLoop");
        {
            BE1::ScopedReadLock lock(context->smpLock);

            // 메인 스레드가 업데이트가 완료되어 (다음) 렌더링을 기다리는 상태가 될 때까지 기다린다.
            BE1::PlatformCondition::Wait(context->updateCompletedCondition, context->smpLock, false, [context] {
                return context->frameSyncState == FrameSyncState::WaitingForRenderCompleted || context->isStoppingRenderThread;
            });

            if (context->isStoppingRenderThread) {
                break;
            }
        }

        context->BeginFrame();
        context->RenderFrame();
        context->EndFrame();

        {
            BE1::ScopedWriteLock lock(context->smpLock);

            context->renderFrameIndex ^= context->renderFrameIndex;

            // (메인 스레드의) 다음 업데이트가 끝나기를 기다리는 상태로 변경
            context->frameSyncState = FrameSyncState::WaitingForUpdateCompleted;

            BE1::PlatformCondition::Signal(context->renderCompletedCondition);
        }
    }
    return 0;
}

void RenderContext::BeginFrame() {
    PIX_SCOPED_EVENT(renderer->commandQueues[to_int(RHI::CommandQueueType::Graphics)], 0, "RenderContext::BeginFrame");

    // 프레임 데이터를 초기화하고, 이전 프레임에 대한 펜스를 기다린다.
    RenderFrameData *currentFrameData = GetCurrentFrameData();
    currentFrameData->BeginFrame();

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    RHI::FrameThreadData *frameThreadData = currentFrameData->GetThreadData(0);
    mainCommandList = frameThreadData->AllocGraphicsCommandList();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    mainCommandList->Reset();

    // 뷰포트 & ScissorRect 설정
    renderer->SetViewport(mainCommandList, viewportRect);
    renderer->SetScissorRect(mainCommandList, scissorRect);

#if 1
    renderer->BeginRenderPass(mainCommandList, swapChain, mainRTDepthTexture, BE1::Color4::blue, 1.0f, 0, RHI::ClearFlag::Color | RHI::ClearFlag::Depth);
#else
    if (mainRTSampleCount > 1) {
        RHI::RenderPassImage renderPassImages[] = {
            RHI::RenderPassImage::Color(mainRTColorMSAATexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::DepthStencil(mainRTDepthTexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::ResolveColor(mainRTColorTexture, 0, 0)
        };
        renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));
    } else {
        RHI::RenderPassImage renderPassImages[] = {
            RHI::RenderPassImage::Color(mainRTColorTexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::DepthStencil(mainRTDepthTexture, 0, RHI::RenderPassImage::LoadAction::Clear)
        };
        renderer->BeginRenderPass(mainCommandList, renderPassImages, COUNT_OF(renderPassImages));
    }
#endif
}

void RenderContext::EndFrame() {
    PIX_SCOPED_EVENT(renderer->commandQueues[to_int(RHI::CommandQueueType::Graphics)], 1, "RenderContext::EndFrame");

    // TODO: 렌더큐에 종료 마킹을 하고, 렌더큐를 실행한다.

#if 1
    renderer->EndRenderPass(mainCommandList);
#else
    renderer->BeginRenderPass(mainCommandList, swapChain, nullptr);
    renderer->SetPSO(mainCommandList, imagePSO);
    renderer->SetTexture(mainCommandList, 0, false, mainRTColorTexture);
    renderer->Draw(mainCommandList, 3, 0);
    renderer->EndRenderPass(mainCommandList);
#endif

    // CommandList 기록을 마치고 CommandQueue 로 실행
    mainCommandList->CloseAndExecute(RHI::CommandQueueType::Graphics);

    // 이번 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    GetCurrentFrameData()->EndFrame();

    // 백버퍼를 전면버퍼와 교환한다.
    SwapBuffers(false);

    frameCount++;

    currentFrameIndex = frameCount % NumFrameResources;

    // 메인 스레드에서 사용할 수 있도록 이전 프레임에 할당했던 메모리를 초기화한다.
    GetCurrentFrameData()->ClearMemAllocs();
}

void RenderContext::RenderFrame() {
    PIX_SCOPED_EVENT(renderer->commandQueues[to_int(RHI::CommandQueueType::Graphics)], 4, "RenderContext::RenderFrame");

    RenderFrameData *currentFrameData = GetCurrentFrameData();
    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
#ifdef USE_RENDEROBJECT_INSTANCING
    int numDrawCalls = (int)BE1::Math::Ceil((float)numVisObjects / 1024);
#else
    int numDrawCalls = numVisObjects;
#endif

    int numTasks = BE1::Min(renderTaskManager.NumThreads(), (int)BE1::Math::Ceil((float)numDrawCalls / MaxDrawCallsPerTask));
    if (numTasks > 1) {
        DrawVisObjectsWithTask(numTasks);
    } else {
        DrawVisObjectsWithoutTask();
    }
#else
    DrawVisObjectsWithoutTask();
#endif
}

// 특정 인덱스 범위의 visObjects 를 그린다.
void RenderContext::DrawVisObjects(int threadIndex, RHI::CommandList *commandList, int startIndex, int endIndex) {
    //PIX_SCOPED_EVENT(commandList->GetGraphicsCommandList(), 6, "RenderContext::DrawVisObjects");

    RenderFrameData *currentFrameData = GetCurrentFrameData();
    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

    VisObject *visObjects = currentFrameData->GetVisObjects();
    int index = startIndex;

    while (index <= endIndex) {
        VisObject *currentVisObjectPtr = &visObjects[index];

#ifdef USE_RENDEROBJECT_INSTANCING
        int instanceCount = BE1::Min(1024, endIndex - index + 1);
        if (instanceCount > 1) {
            VisObject::DrawInstanced(this, commandList, currentVisObjectPtr, instanceCount);
            index += instanceCount;
        } else {
            VisObject::Draw(this, commandList, &currentVisObjectPtr[0]);
            ++index;
        }
#else
        VisObject::Draw(this, commandList, &currentVisObjectPtr[0]);
        ++index;
#endif
    }
}

// 전체 visObjects 를 task 없이 한번에 그린다.
void RenderContext::DrawVisObjectsWithoutTask() {
    PIX_CPU_SCOPED_EVENT(4, "RenderContext::DrawVisObjectsWithoutTask");

    RenderFrameData *currentFrameData = GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(0);

    // Secondary CommandList 를 시작한다.
    RHI::CommandList *commandList = currentFrameThreadData->BeginSecondaryCommandList(mainCommandList);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(0, commandList, 0, currentFrameData->NumVisObjects() - 1);

    // Secondary CommandList 를 닫고 메인 CommandList 에 등록한다.
    commandList->CloseAndExecuteSecondary(mainCommandList, currentFrameThreadData);
}

#ifdef USE_RENDER_TASK
// taskDesc 에 담겨있는 정보를 기반으로 visObjects 를 그린다.
void RenderContext::DrawVisObjectsByTask(RenderContext::DrawObjectTaskDesc *taskDesc) {
    PIX_CPU_SCOPED_EVENT(5, "RenderContext::DrawVisObjectsByTask");

    RenderFrameData *currentFrameData = GetCurrentFrameData();
    RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(taskDesc->threadIndex);

    // Secondary CommandList 를 시작한다.
    RHI::CommandList *commandList = currentFrameThreadData->BeginSecondaryCommandList(mainCommandList);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(taskDesc->threadIndex, commandList, taskDesc->visObjectStartIndex, taskDesc->visObjectEndIndex);

    // CommandList 기록을 마친다.
    commandList->Close();

    // 사용 중인 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->activeCommandList = commandList;
}

void RenderContext::DrawVisObjectsByTaskFunction(void *data) {
    RenderContext::DrawObjectTaskDesc *taskDesc = reinterpret_cast<RenderContext::DrawObjectTaskDesc *>(data);
    taskDesc->renderContext->DrawVisObjectsByTask(taskDesc);
}

// 전체 visObjects 를 task 로 나눠서 그린다.
void RenderContext::DrawVisObjectsWithTask(int numTasks) {
    PIX_CPU_SCOPED_EVENT(6, "RenderContext::DrawVisObjectsWithTask");

    RenderFrameData *currentFrameData = GetCurrentFrameData();
    int numVisObjects = currentFrameData->NumVisObjects();
    int numVisObjectsPerTasks = (int)BE1::Math::Ceil((float)numVisObjects / numTasks);
    int threadIndex = 0;
    int lastEndIndex = -1;

    // 태스크 정보 초기화
    objectDrawingTaskDescs.Reserve(renderTaskManager.NumThreads());
    objectDrawingTaskDescs.SetCount(0, false);

    // 최대 쓰레드 개수만큼 task 를 실행한다.
    while (lastEndIndex < numVisObjects - 1) {
        DrawObjectTaskDesc &currentThreadDesc = objectDrawingTaskDescs.Alloc();

        currentThreadDesc.renderContext = this;
        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.visObjectStartIndex = lastEndIndex + 1;
        currentThreadDesc.visObjectEndIndex = BE1::Min(currentThreadDesc.visObjectStartIndex + numVisObjectsPerTasks, numVisObjects) - 1;
        renderTaskManager.AddTask(RenderContext::DrawVisObjectsByTaskFunction, &currentThreadDesc, false);

        lastEndIndex = currentThreadDesc.visObjectEndIndex;
    }

    renderTaskManager.WaitFinish(true);

    // Main CommandList 에 모든 태스크의 Secondary CommandList 들을 기록한다.
    int renderTaskCount = objectDrawingTaskDescs.Count();
    for (int threadIndex = 0; threadIndex < renderTaskCount; ++threadIndex) {
        const RHI::FrameThreadData *currentFrameThreadData = currentFrameData->GetThreadData(threadIndex);

        objectDrawingTaskDescs[threadIndex].activeCommandList->ExecuteSecondary(mainCommandList, currentFrameThreadData);
    }
}
#endif // USE_RENDER_TASK

void RenderContext::SwapBuffers(bool vsync) {
    PIX_SCOPED_EVENT(renderer->commandQueues[to_int(RHI::CommandQueueType::Graphics)], 2, "RenderContext::SwapBuffers");

    swapChain->SwapBuffers(vsync);
}
