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
#include "RenderInternal.h"

RenderContext *     RenderContext::activeContext = nullptr;

void RenderContext::Init(void *windowHandle, bool useRenderThread) {
    // 렌더링 프레임 별로 사용할 프레임 데이터들을 초기화한다.
    // 프레임 데이터 : 임시 메모리, 커맨드 리스트 풀, 루트 디스크립터 힙, 다이나믹 버퍼와 그 디스크립터 풀
    for (RenderFrameData &frameData : frames) {
        frameData.Init();
    }

    // 렌더 스레드에서 이전 프레임의 프레임 데이터 사용이 완료되었는지 체크하기 위해 펜스를 친다.
    currentFrameIndex = 0;
    frames[currentFrameIndex].SetFenceValue(RHI::renderer->SignalFence(RHI::CommandQueueType::Graphics));

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
    swapChain = RHI::renderer->CreateSwapChain(hwnd, backBufferWidth, backBufferHeight, BE1::Image::Format::R8G8B8A8);

    // 렌더 타겟 텍스쳐 & 뎁스 텍스쳐 생성
    CreateMainRenderTextures(backBufferWidth, backBufferHeight);

    InitFullScreenTrianglePSO();

    InitPSO();

    guiMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame2D);
    guiMesh.SetClipRect(BE1::Rect(0, 0, backBufferWidth, backBufferHeight));
}

void RenderContext::Shutdown() {
    // 렌더 스레드 종료
    if (renderThread) {
        ShutdownRenderThread();
    }

    // GPU 명령들이 완료될 때까지 기다린다.
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);
    RHI::renderer->Finish(RHI::CommandQueueType::Compute);

    if (imagePSO) {
        RHI::renderer->DestroyPSO(imagePSO);
    }
    if (unlitPSO) {
        RHI::renderer->DestroyPSO(unlitPSO);
    }
    if (unlitAlphaBlendPSO) {
        RHI::renderer->DestroyPSO(unlitAlphaBlendPSO);
    }
    if (unlitInstancedPSO) {
        RHI::renderer->DestroyPSO(unlitInstancedPSO);
    }
    if (unlitInstancedAlphaBlendPSO) {
        RHI::renderer->DestroyPSO(unlitInstancedAlphaBlendPSO);
    }

    DestroyMainRenderTextures();

    for (RenderFrameData &frameData : frames) {
        frameData.Shutdown();
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

    guiMesh.SetClipRect(BE1::Rect(0, 0, width, height));

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

void RenderContext::InitPSO() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, OFFSET_OF(BE1::VertexGenericLit, xyz), 0, RHI::InputLayoutElement::Format::Float3 },
        { "TEXCOORD", 0, OFFSET_OF(BE1::VertexGenericLit, st), 0, RHI::InputLayoutElement::Format::Half2 },
        { "COLOR", 0, OFFSET_OF(BE1::VertexGenericLit, color), 0, RHI::InputLayoutElement::Format::UByte4N },
        { "NORMAL", 0, OFFSET_OF(BE1::VertexGenericLit, normal), 0, RHI::InputLayoutElement::Format::UByte4N },
        { "TEXCOORD", 1, OFFSET_OF(BE1::VertexGenericLit, tangent), 0, RHI::InputLayoutElement::Format::UByte4N },
    };

    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = GetMainRTColorFormat();
    renderDest.depthStencilFormat = GetMainRTDepthFormat();
    renderDest.sampleCount = GetMainRTSampleCount();

    RHI::Shader *unlitVS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Unlit.hlsl", "VSMain"));
    RHI::Shader *unlitPS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Unlit.hlsl", "PSMain"));

    if (unlitVS && unlitPS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = unlitVS;
        psoDesc.ps = unlitPS;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        unlitPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);

        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::AlphaBlend);
        unlitAlphaBlendPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (unlitVS) {
        RHI::renderer->DestroyShader(unlitVS, true);
    }
    if (unlitPS) {
        RHI::renderer->DestroyShader(unlitPS, true);
    }

    RHI::Shader *unlitInstancingVS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/UnlitInstancing.hlsl", "VSMain"));
    RHI::Shader *unlitInstancingPS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/UnlitInstancing.hlsl", "PSMain"));

    if (unlitInstancingVS && unlitInstancingPS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = unlitInstancingVS;
        psoDesc.ps = unlitInstancingPS;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        unlitInstancedPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);

        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::AlphaBlend);
        unlitInstancedAlphaBlendPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (unlitInstancingVS) {
        RHI::renderer->DestroyShader(unlitInstancingVS, true);
    }
    if (unlitInstancingPS) {
        RHI::renderer->DestroyShader(unlitInstancingPS, true);
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
    for (const RenderFrameData &frameData : frames) {
        RHI::renderer->WaitFence(frameData.GetFenceValue());
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
        renderSystem->GetBackend()->Execute(context->GetCurrentFrameData()->GetCommands()->buffer);

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
    frameData->BeginFrame();
    frameData->CmdBeginContext(this);

    RenderContext::activeContext = this;
}

void RenderContext::EndFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::EndFrame", 10);

    assert(BE1::Engine::IsInMainThread());

    RenderContext::activeContext = nullptr;

    RenderFrameData *frameData = GetCurrentFrameData();
    frameData->CmdSwapBuffers();
    frameData->EndFrame();

    if (IsUsingRenderThread()) {
        // 렌더 스레드를 깨운다.
        MarkUpdateCompleted();
    } else {
        // 렌더 스레드를 사용하지 않을 경우 직접 백엔드를 실행
        renderSystem->GetBackend()->Execute(frameData->GetCommands());
    }
}

void RenderContext::DrawPic(float x, float y, float w, float h, const Texture *texture) {
    RenderFrameData *frameData = GetCurrentFrameData();

    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, currentColor.ToUInt32(), texture);
}

void RenderContext::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Texture *texture) {
    RenderFrameData *frameData = GetCurrentFrameData();

    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, s1, t1, s2, t2, currentColor.ToUInt32(), texture);
}

void RenderContext::DrawBar(float x, float y, float w, float h) {
    RenderFrameData *frameData = GetCurrentFrameData();

    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, currentColor.ToUInt32(), textureManager.whiteTexture);
}

void RenderContext::DrawRect(float x, float y, float w, float h) {
    if (w > 1) {
        DrawBar(x, y, w, 1);
        if (h > 1) {
            DrawBar(x, y + h - 1, w, 1);
        }
    }

    if (h > 2) {
        DrawBar(x, y + 1, 1, h - 2);
        if (w > 2) {
            DrawBar(x + w - 1, y + 1, 1, h - 2);
        }
    }
}

void RenderContext::SetColor(const BE1::Color4 &color) {
    currentColor = color;
}

void RenderContext::SetFont(Font *font) {
    if (!font) {
        currentFont = fontManager.defaultFont;
        return;
    }
    currentFont = font;
}

void RenderContext::SetTextScale(float scale) {
    currentTextScale = scale;
}

void RenderContext::SetTextLineSpacing(float lineSpacing) {
    currentTextLineSpacing = lineSpacing;
}

void RenderContext::SetTextShadowColor(const BE1::Color4 &shadowColor) {
    currentTextShadowColor = shadowColor;
}

void RenderContext::SetTextShadowOffset(float shadowOffsetX, float shadowOffsetY) {
    currentTextShadowOffset.x = shadowOffsetX;
    currentTextShadowOffset.y = shadowOffsetY;
}

void RenderContext::DrawText(const BE1::Rect &rect, float x, float y, const char *text, DrawTextFlag flags) {
    RenderFrameData *frameData = GetCurrentFrameData();

    // 프레임 메모리에 텍스트 내용을 복사
    int size = BE1::Str::Length(text) + 1;
    char *frameTextPtr = (char *)frameData->MemAlloc(size);
    BE1::Str::Copynz(frameTextPtr, text, size);

    guiMesh.DrawText2D(frameData->GetThreadData(0), rect, x, y, currentTextLineSpacing, currentTextScale, currentColor.ToUInt32(), currentTextShadowColor.ToUInt32(), currentTextShadowOffset, currentFont, frameTextPtr, flags);
}

void RenderContext::DrawString(float x, float y, const BE1::Str &string, DrawTextFlag flags) {
    BE1::Rect textRect;
    textRect.x = 0;
    textRect.y = 0;
    textRect.w = GetWidth();
    textRect.h = GetHeight();

    DrawText(textRect, x, y, string, flags);
}
