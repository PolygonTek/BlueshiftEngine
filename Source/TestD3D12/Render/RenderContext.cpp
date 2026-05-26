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

void RenderContext::Init(void *windowHandle) {
    // 프레임 별로 사용할 프레임 데이터들을 초기화한다.
    // 프레임 데이터: 임시 메모리, 커맨드 리스트 풀, 디스크립터 힙, 다이나믹 버퍼와 그 디스크립터 풀
    for (RenderFrameData &frameData : frames) {
        frameData.Init();
    }

    currentFrontendFrameIndex = 0;

    // 윈도우 크기 얻기
    // TODO: 윈도우 크기를 얻는 부분은 플랫폼 별로 분리하자.
    RECT rc;
    HWND hwnd = reinterpret_cast<HWND>(windowHandle);
    GetClientRect(hwnd, &rc);
    uint32_t backBufferWidth = rc.right;
    uint32_t backBufferHeight = rc.bottom;

    // 스왑 체인 (백버퍼) 생성
    swapChain = RHI::renderer->CreateSwapChain(hwnd, backBufferWidth, backBufferHeight, RHI::Format::R8G8B8A8_UNORM);

    // 렌더 타겟 텍스쳐 & 뎁스 텍스쳐 생성
    CreateMainRenderTextures(backBufferWidth, backBufferHeight);

    InitFullScreenTrianglePSO();

    InitPSO();

    textureStreamingFeedback.Init();

    guiMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame2D);
    guiMesh.SetClipRect(BE1::Rect(0, 0, backBufferWidth, backBufferHeight));
}

void RenderContext::Shutdown() {
    // GPU 명령들이 완료될 때까지 기다린다.
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);
    RHI::renderer->Finish(RHI::CommandQueueType::Compute);

    textureStreamingFeedback.Shutdown();

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
    if (litPSO) {
        RHI::renderer->DestroyPSO(litPSO);
    }
    if (litAlphaBlendPSO) {
        RHI::renderer->DestroyPSO(litAlphaBlendPSO);
    }
    if (litInstancedPSO) {
        RHI::renderer->DestroyPSO(litInstancedPSO);
    }
    if (litInstancedAlphaBlendPSO) {
        RHI::renderer->DestroyPSO(litInstancedAlphaBlendPSO);
    }

    DestroyMainRenderTextures();

    for (RenderFrameData &frameData : frames) {
        frameData.Shutdown();
    }

    RHI::renderer->DestroySwapChain(swapChain);
}

void RenderContext::OnResize(int width, int height) {
    // 모든 프레임에 대해서 렌더 스레드가 렌더링을 완료할 때까지 기다린다.
    if (renderSystem->IsUsingRenderThread()) {
        for (RenderFrameData &frameData : frames) {
            renderSystem->WaitRenderCompleted(&frameData);
        }
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
    BE1::Image::Format colorImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(mainRTColorFormat, &colorImageFormat, nullptr);

    std::shared_ptr<BE1::Image> colorImage = std::make_shared<BE1::Image>();
    colorImage->InitFromMemory(width, height, 1, 1, 1, colorImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    mainRTColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess,
        colorImage, false, RHI::ClearValue::Color(0.0f, 0.0f, 0.5f, 0.0f), 1);
    RHI::renderer->SetName(mainRTColorTexture, "mainRTColorTexture");

    if (mainRTSampleCount > 1) {
        mainRTColorMSAATexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource,
            colorImage, false, RHI::ClearValue::Color(0.0f, 0.0f, 5.0f, 0.0f), mainRTSampleCount);
        RHI::renderer->SetName(mainRTColorMSAATexture, "mainRTColorMSAATexture");
    }

    BE1::Image::Format depthImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(mainRTDepthFormat, &depthImageFormat, nullptr);

    std::shared_ptr<BE1::Image> depthStencilImage = std::make_shared<BE1::Image>();
    depthStencilImage->InitFromMemory(width, height, 1, 1, 1, depthImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    mainRTDepthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::DepthStencil,
        depthStencilImage, false, RHI::ClearValue::DepthStencil(1.0f, 0), mainRTSampleCount, RHI::GPUResourceState::DepthWrite);
    RHI::renderer->SetName(mainRTColorTexture, "mainRTDepthTexture");
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
    renderDest.depthStencilFormat = GetMainRTDepthFormat();

#ifdef USE_OFFSCREEN_MAIN_RENDER_TARGET
    renderDest.renderTargetFormats[0] = GetMainRTColorFormat();
    renderDest.sampleCount = GetMainRTSampleCount();
#else
    renderDest.renderTargetFormats[0] = swapChain->GetRTVFormat();
    renderDest.sampleCount = 1;
#endif

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

    RHI::Shader *litVS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Lit.hlsl", "VSMain"));
    RHI::Shader *litPS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Lit.hlsl", "PSMain"));

    if (litVS && litPS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = litVS;
        psoDesc.ps = litPS;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        litPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);

        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::AlphaBlend);
        litAlphaBlendPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (litVS) {
        RHI::renderer->DestroyShader(litVS, true);
    }
    if (litPS) {
        RHI::renderer->DestroyShader(litPS, true);
    }

    RHI::Shader *litInstancingVS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/LitInstancing.hlsl", "VSMain"));
    RHI::Shader *litInstancingPS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/LitInstancing.hlsl", "PSMain"));

    if (litInstancingVS && litInstancingPS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = litInstancingVS;
        psoDesc.ps = litInstancingPS;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        litInstancedPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);

        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::AlphaBlend);
        litInstancedAlphaBlendPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (litInstancingVS) {
        RHI::renderer->DestroyShader(litInstancingVS, true);
    }
    if (litInstancingPS) {
        RHI::renderer->DestroyShader(litInstancingPS, true);
    }
}

void RenderContext::InitFullScreenTrianglePSO() {
    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = swapChain->GetRTVFormat();
    renderDest.sampleCount = 1;

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
        frameData.WaitFrameFence();
    }
}

void RenderContext::RequestScreenshot(const char *filename) {
    if (!filename || !filename[0]) {
        return;
    }

    for (const PendingScreenshot &pendingScreenshot : pendingScreenshots) {
        if (!BE1::Str::Cmp(pendingScreenshot.filename, filename)) {
            return;
        }
    }

    PendingScreenshot &pendingScreenshot = pendingScreenshots.Alloc();
    BE1::Str::Copynz(pendingScreenshot.filename, filename, COUNT_OF(pendingScreenshot.filename));
}

void RenderContext::FlushPendingScreenshots(RenderFrameData *frameData) {
    for (const PendingScreenshot &pendingScreenshot : pendingScreenshots) {
        frameData->CmdScreenshot(0, 0, swapChain->GetWidth(), swapChain->GetHeight(), pendingScreenshot.filename);
    }

    pendingScreenshots.SetCount(0, false);
}

void RenderContext::EnsureTextureStreamingFeedbackCapacity(uint32_t requiredCapacity) {
    if (!textureStreamingFeedback.NeedsGrowCapacity(requiredCapacity)) {
        return;
    }

    if (renderSystem->IsUsingRenderThread()) {
        for (RenderFrameData &frameData : frames) {
            renderSystem->WaitRenderCompleted(&frameData);
        }
    }

    WaitAllFrameFences();

    textureStreamingFeedback.EnsureCapacity(requiredCapacity);
}

void RenderContext::BeginFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::BeginFrame");

    assert(BE1::Engine::IsInMainThread());

    if (RenderContext::activeContext) {
        BE_ERRLOG("RenderContext::BeginFrame: Make sure to call EndFrame() before calling BeginFrame again\n");
        return;
    }

    currentFrontendFrameIndex = frameCount % COUNT_OF(frames);
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    if (renderSystem->IsUsingRenderThread()) {
        // 현재 프론트엔드 프레임의 렌더 스레드 작업이 끝날 때까지 기다린다.
        renderSystem->WaitRenderCompleted(frameData);
    }

    frameData->BeginFrame();
    frameData->CmdBeginContext(this);

    RenderContext::activeContext = this;
}

void RenderContext::EndFrame() {
    PROFILER_CPU_SCOPED_EVENT("RenderContext::EndFrame");

    assert(BE1::Engine::IsInMainThread());

    RenderContext::activeContext = nullptr;

    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    FlushPendingScreenshots(frameData);

#ifdef USE_MIPMAP_STREAMING
    EnsureTextureStreamingFeedbackCapacity(textureManager.GetStreamingTextureCount());
#endif

    frameData->CmdSwapBuffers();
    frameData->EndFrame();

    frameCount++;

    renderSystem->SubmitFrame(this, frameData);
}

void RenderContext::DrawPic(float x, float y, float w, float h, const std::shared_ptr<Texture> &texture) {
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    frameData->RetainTexture(texture);
    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, currentColor.ToUInt32(), texture ? texture.get() : nullptr);
}

void RenderContext::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const std::shared_ptr<Texture> &texture) {
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    frameData->RetainTexture(texture);
    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, s1, t1, s2, t2, currentColor.ToUInt32(), texture ? texture.get() : nullptr);
}

void RenderContext::DrawBar(float x, float y, float w, float h) {
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    guiMesh.DrawPic(frameData->GetThreadData(0), x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, currentColor.ToUInt32(), textureManager.whiteTexture.get());
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

void RenderContext::SetFont(std::shared_ptr<Font> font) {
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
    RenderFrameData *frameData = GetCurrentFrontendFrameData();

    // 프레임 메모리에 텍스트 내용을 복사
    int size = BE1::Str::Length(text) + 1;
    char *frameTextPtr = (char *)frameData->MemAlloc(size);
    BE1::Str::Copynz(frameTextPtr, text, size);

    frameData->RetainFont(currentFont);

    guiMesh.DrawText2D(frameData->GetThreadData(0), rect, x, y, currentTextLineSpacing, currentTextScale, currentColor.ToUInt32(), currentTextShadowColor.ToUInt32(), currentTextShadowOffset, currentFont.get(), frameTextPtr, flags);
}

void RenderContext::DrawString(float x, float y, const BE1::Str &string, DrawTextFlag flags) {
    BE1::Rect textRect;
    textRect.x = 0;
    textRect.y = 0;
    textRect.w = GetWidth();
    textRect.h = GetHeight();

    DrawText(textRect, x, y, string, flags);
}
