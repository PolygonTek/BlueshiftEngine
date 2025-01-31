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
#include "RenderSystem.h"
#include "RenderContext.h"
#include "RenderBackEnd.h"
#include "RenderInternal.h"

void RenderContext::Init(void *windowHandle, bool useRenderThread) {
    // 렌더링 프레임 별로 사용할 프레임 데이터들을 초기화한다.
    // 프레임 데이터 : 임시 메모리, 커맨드 리스트 풀, 루트 디스크립터 힙, 다이나믹 버퍼와 그 디스크립터 풀
    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Init();
    }

    // 렌더 스레드에서 이전 프레임의 프레임 데이터 사용이 완료되었는지 체크하기 위해 펜스를 친다.
    currentFrameIndex = 0;
    frameData[currentFrameIndex].SetFenceValue(RHI::renderer->SignalFence(RHI::CommandQueueType::Graphics));

    // 렌더 스레드 초기화
    if (useRenderThread) {
        InitRenderThread();
    }

    // 윈도우 크기 얻기
    RECT rc;
    HWND hwnd = reinterpret_cast<HWND>(windowHandle);
    GetClientRect(hwnd, &rc);
    uint32_t backBufferWidth = rc.right;
    uint32_t backBufferHeight = rc.bottom;

    // 스왑 체인 (백버퍼) 생성
    swapChain = RHI::renderer->CreateSwapChain(hwnd, backBufferWidth, backBufferHeight, BE1::Image::Format::RGBA_8_8_8_8);

    // 렌더 타겟 텍스쳐 & 뎁스 텍스쳐 생성
    CreateMainRenderTextures(backBufferWidth, backBufferHeight);

    InitFullScreenTrianglePSO();
}

void RenderContext::Shutdown() {
    // 렌더 스레드 종료
    if (renderThread) {
        ShutdownRenderThread();
    }

    // GPU 명령들이 완료될 때까지 기다린다.
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);
    RHI::renderer->Finish(RHI::CommandQueueType::Compute);

    RHI::renderer->DestroyPSO(imagePSO);

    DestroyMainRenderTextures();

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Shutdown();
    }

    RHI::renderer->DestroySwapChain(swapChain);
}

void RenderContext::OnResize(int width, int height) {
    // 렌더 스레드가 렌더링을 완료할 때까지 기다린다.
    if (IsUsingRenderThread()) {
        WaitRenderCompleted();
    }

    // GPU 명령들이 완료될 때까지 기다린다.
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);
    RHI::renderer->Finish(RHI::CommandQueueType::Compute);

    // 스왑 체인의 크기 조정
    swapChain->Resize(width, height);

    // 렌더 타겟 텍스쳐 & 뎁스 텍스쳐 재생성
    DestroyMainRenderTextures();
    CreateMainRenderTextures(width, height);
}

void RenderContext::CreateMainRenderTextures(uint32_t width, uint32_t height) {
    BE1::Image colorImage;
    colorImage.InitFromMemory(width, height, 1, 1, 1, mainRTColorFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    mainRTColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::RenderTarget | RHI::ResourceFlag::ShaderResource | RHI::ResourceFlag::UnorderedAccess,
        &colorImage, false, RHI::ClearValue::Color(0.0f, 0.0f, 1.0f, 0.0f), 1);

    if (mainRTSampleCount > 1) {
        mainRTColorMSAATexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::RenderTarget | RHI::ResourceFlag::ShaderResource,
            &colorImage, false, RHI::ClearValue::Color(0.0f, 0.0f, 1.0f, 0.0f), mainRTSampleCount);
    }

    BE1::Image depthStencilImage;
    depthStencilImage.InitFromMemory(width, height, 1, 1, 1, mainRTDepthFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    mainRTDepthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::DepthStencil,
        &depthStencilImage, false, RHI::ClearValue::DepthStencil(1.0f, 0), mainRTSampleCount, RHI::GPUResourceState::DepthWrite);
}

void RenderContext::DestroyMainRenderTextures() {
    if (mainRTColorTexture) {
        RHI::renderer->DestroyTexture(mainRTColorTexture, true);
        mainRTColorTexture = nullptr;
    }
    if (mainRTColorMSAATexture) {
        RHI::renderer->DestroyTexture(mainRTColorMSAATexture, true);
        mainRTColorMSAATexture = nullptr;
    }
    if (mainRTDepthTexture) {
        RHI::renderer->DestroyTexture(mainRTDepthTexture, true);
        mainRTDepthTexture = nullptr;
    }
}

void RenderContext::InitFullScreenTrianglePSO() {
    BE1::Image::Format imageFormat = BE1::Image::Format::Unknown;
    bool isSRGB = false;
    swapChain->GetFormat(&imageFormat, &isSRGB);

    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = imageFormat;
    renderDest.renderTargetForematSRGBs[0] = isSRGB;

    RHI::Shader *vs = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "VSMain"));
    RHI::Shader *ps = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "PSMain"));

    if (vs && ps) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = vs;
        psoDesc.ps = ps;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Never);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        imagePSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (vs) {
        RHI::renderer->DestroyShader(vs, true);
    }
    if (ps) {
        RHI::renderer->DestroyShader(ps, true);
    }
}

void RenderContext::WaitAllFrameFences() {
    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        RHI::renderer->WaitFence(frameData[frameIndex].GetFenceValue());
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

    // 렌더 스레드에게 업데이트가 완료되었다고 신호를 보낸다.
    BE1::PlatformCondition::Signal(updateCompletedCondition);
}

unsigned int RenderContext::RenderThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("RenderContext::RenderThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    RenderContext *context = reinterpret_cast<RenderContext *>(param);

    while (1) {
        PROFILER_CPU_SCOPED_EVENT("RenderThreadProcLoop", 0);
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

        // 렌더링 백엔드를 실행한다.
        // 업데이트 (프론트 엔드) 단에서 현재 프레임에 대한 커맨드들이 준비되어 있어야 한다.
        renderSystem->GetBackEnd()->Execute(context->GetCurrentFrameData()->GetCommands()->buffer);

        {
            BE1::ScopedWriteLock lock(context->smpLock);

            // (메인 스레드의) 다음 업데이트가 끝나기를 기다리는 상태로 변경
            context->frameSyncState = FrameSyncState::WaitingForUpdateCompleted;

            BE1::PlatformCondition::Signal(context->renderCompletedCondition);
        }
    }
    return 0;
}

void RenderContext::BeginFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::BeginFrame", 10);

    assert(BE1::Engine::IsInMainThread());

    if (IsUsingRenderThread()) {
        // 렌더 스레드 작업이 끝날 때까지 기다린다.
        WaitRenderCompleted();
    }

    RenderFrameData *frameData = GetCurrentFrameData();

    // 이번 프레임에서 사용할 임시 메모리를 미리 할당한다.
    frameData->BeginFrameMemAllocs();

    frameData->CmdBeginContext(this);
}

void RenderContext::EndFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::EndFrame", 10);

    assert(BE1::Engine::IsInMainThread());

    RenderFrameData *frameData = GetCurrentFrameData();
    frameData->CmdSwapBuffers();

    RenderCommandBuffer *cmds = frameData->GetCommands();
    // 커맨드의 끝을 기록
    *(uint32_t *)(cmds->buffer + cmds->used) = static_cast<uint32_t>(RenderCommandId::End);

    // 커맨드 버퍼 비우기
    cmds->used = 0;

    if (IsUsingRenderThread()) {
        // 렌더 스레드를 깨운다.
        MarkUpdateCompleted();
    } else {
        // 렌더 스레드를 사용하지 않을 경우 직접 백엔드를 실행
        renderSystem->GetBackEnd()->Execute(cmds);
    }
}
