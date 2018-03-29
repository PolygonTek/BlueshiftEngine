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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Heap.h"
#include "Platform/PlatformTime.h"

BE_NAMESPACE_BEGIN

RenderContext::RenderContext() {
    deviceWidth = 0;
    deviceHeight = 0;
    renderingWidth = 0;
    renderingHeight = 0;

    frameCount = 0;

    screenColorTexture = nullptr;
    screenDepthTexture = nullptr;
    screenNormalTexture = nullptr;
    screenLitAccTexture = nullptr;
    screenSelectionTexture = nullptr;

    homTexture = nullptr;
    memset(ppTextures, 0, sizeof(ppTextures));

    screenRT = nullptr;
    screenLitAccRT = nullptr;
    screenSelectionRT = nullptr;
    screenSelectionScale = 0.25f;
    homRT = nullptr;
    memset(ppRTs, 0, sizeof(ppRTs));

    currentRenderTexture = nullptr;
    updateCurrentRenderTexture = false;

    prevLumTarget = 1;
    currLumTarget = 2;

    memset(hdrBloomTexture, 0, sizeof(hdrBloomTexture));
    memset(hdrLumAverageTexture, 0, sizeof(hdrLumAverageTexture));
    memset(hdrLuminanceTexture, 0, sizeof(hdrLuminanceTexture));
    
    memset(hdrBloomRT, 0, sizeof(hdrBloomRT));
    memset(hdrLumAverageRT, 0, sizeof(hdrLumAverageRT));
    memset(hdrLuminanceRT, 0, sizeof(hdrLuminanceRT));

    indirectionCubeMapTexture = nullptr;

    shadowRenderTexture = nullptr;
    vscmTexture = nullptr;

    shadowMapRT = nullptr;
    vscmRT = nullptr;

    vscmCleared[0] = false;
    vscmCleared[1] = false;
    vscmCleared[2] = false;
    vscmCleared[3] = false;
    vscmCleared[4] = false;
    vscmCleared[5] = false;
}

void RenderContext::Init(RHI::WindowHandle hwnd, int renderingWidth, int renderingHeight, RHI::DisplayContextFunc displayFunc, void *displayFuncDataPtr, int flags) {
    this->contextHandle = rhi.CreateContext(hwnd, (flags & Flag::UseSharedContext) ? true : false);
    this->flags = flags;
    
    rhi.GetContextSize(this->contextHandle, &this->windowWidth, &this->windowHeight, &this->deviceWidth, &this->deviceHeight);
    
    // actual rendering resolution that will be upscaled if it is smaller than device resolution
    this->renderingWidth = renderingWidth;
    this->renderingHeight = renderingHeight;
    
    this->guiMesh.Clear();
    this->guiMesh.SetCoordFrame(GuiMesh::CoordFrame2D);
    this->guiMesh.SetClipRect(Rect(0, 0, renderingWidth, renderingHeight));
    
    rhi.SetContextDisplayFunc(contextHandle, displayFunc, displayFuncDataPtr, (flags & Flag::OnDemandDrawing) ? true : false);

    InitScreenMapRT();

    InitHdrMapRT();

    InitShadowMapRT();
    
    this->random.SetSeed(123123);

    this->elapsedTime = PlatformTime::Milliseconds() * 0.001f;
}

void RenderContext::Shutdown() {
    FreeScreenMapRT();

    FreeHdrMapRT();

    FreeShadowMapRT();

    rhi.DestroyContext(contextHandle);
}

static Image::Format GetScreenImageFormat() {
    if (r_HDR.GetInteger() == 0) {
        return Image::RGBA_8_8_8_8;
    }
    
    if (r_HDR.GetInteger() == 1 && rhi.SupportsPackedFloat()) {
        return Image::RGB_11F_11F_10F;
    }
    
    if (r_HDR.GetInteger() == 3) {
        return Image::RGBA_32F_32F_32F_32F;
    }

    return Image::RGBA_16F_16F_16F_16F;
}

void RenderContext::InitScreenMapRT() {
    FreeScreenMapRT();

    //--------------------------------------
    // screenRT 만들기
    //--------------------------------------

    int screenTextureFlags = Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality | Texture::NonPowerOfTwo | Texture::HighPriority;

    Image::Format screenImageFormat = GetScreenImageFormat();

    screenColorTexture = textureManager.AllocTexture(va("_%i_screenColor", (int)contextHandle));
    screenColorTexture->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    
    screenDepthTexture = textureManager.AllocTexture(va("_%i_screenDepthstencil", (int)contextHandle));
    screenDepthTexture->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::Depth_24, screenTextureFlags | Texture::Nearest);

    if (r_useDeferredLighting.GetBool()) {
        screenNormalTexture = textureManager.AllocTexture(va("_%i_screenNormal", (int)contextHandle));
        screenNormalTexture->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::LA_16F_16F, screenTextureFlags | Texture::Nearest);		

        Texture *colorTextures[2];
        colorTextures[0] = screenColorTexture;
        colorTextures[1] = screenNormalTexture;
        screenRT = RenderTarget::Create(2, (const Texture **)colorTextures, screenDepthTexture, 0);
        //screenRT->SetMRTMask(3);

        screenLitAccTexture = textureManager.AllocTexture(va("_%i_screenLitAcc", (int)contextHandle));
        screenLitAccTexture->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Nearest | Texture::SRGB);
        screenLitAccRT = RenderTarget::Create(screenLitAccTexture, nullptr, 0);
    } else {
        screenRT = RenderTarget::Create(screenColorTexture, screenDepthTexture, 0);
    }

    screenRT->Clear(Color4::black, 1.0f, 0);

    if (flags & UseSelectionBuffer) {
        screenSelectionTexture = textureManager.AllocTexture(va("_%i_screenSelection", (int)contextHandle));
        screenSelectionTexture->CreateEmpty(RHI::Texture2D, renderingWidth * screenSelectionScale, renderingHeight * screenSelectionScale, 1, 1, 1, 
            Image::RGBA_8_8_8_8, screenTextureFlags);
    
        screenSelectionRT = RenderTarget::Create(screenSelectionTexture, nullptr, RHI::HasDepthBuffer);
    }

    if (r_HOM.GetBool()) {
        int w = Math::FloorPowerOfTwo(renderingWidth >> 1);
        int h = Math::FloorPowerOfTwo(renderingHeight >> 1);

        homTexture = textureManager.AllocTexture(va("_%i_hom", (int)contextHandle));
        homTexture->CreateEmpty(RHI::Texture2D, w, h, 1, 1, 1, Image::Depth_32F, 
            Texture::Clamp | Texture::HighQuality | Texture::HighPriority | Texture::Nearest);
        homTexture->Bind();
        rhi.GenerateMipmap();
        homRT = RenderTarget::Create(nullptr, (const Texture *)homTexture, 0);
    }

    //--------------------------------------
    // Post processing 용 RT 만들기
    //--------------------------------------	

    ppTextures[PP_TEXTURE_COLOR_2X] = textureManager.AllocTexture(va("_%i_screenColorD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_2X]->CreateEmpty(RHI::Texture2D, renderingWidth >> 1, renderingHeight >> 1, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_2X], nullptr, 0);

    ppTextures[PP_TEXTURE_COLOR_4X] = textureManager.AllocTexture(va("_%i_screenColorD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_4X]->CreateEmpty(RHI::Texture2D, renderingWidth >> 2, renderingHeight >> 2, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_COLOR_TEMP] = textureManager.AllocTexture(va("_%i_screenColorTemp", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP]->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_TEMP] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP], nullptr, 0);

    ppTextures[PP_TEXTURE_COLOR_TEMP_2X] = textureManager.AllocTexture(va("_%i_screenColorTempD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP_2X]->CreateEmpty(RHI::Texture2D, renderingWidth >> 1, renderingHeight >> 1, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_TEMP_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_2X], nullptr, 0);

    ppTextures[PP_TEXTURE_COLOR_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenColorTempD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP_4X]->CreateEmpty(RHI::Texture2D, renderingWidth >> 2, renderingHeight >> 2, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_TEMP_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_4X], nullptr, 0);

    //ppTextures[PP_TEXTURE_COLOR_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenColorTempD4x", (int)contextHandle));
    //ppTextures[PP_TEXTURE_COLOR_TEMP_4X]->CreateEmpty(RHI::Texture2D, renderingWidth >> 2, renderingHeight >> 2, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    //ppRTs[PP_RT_BLUR] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_LINEAR_DEPTH] = textureManager.AllocTexture(va("_%i_screenLinearDepth", (int)contextHandle));
    ppTextures[PP_TEXTURE_LINEAR_DEPTH]->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::L_16F, screenTextureFlags);
    ppRTs[PP_RT_LINEAR_DEPTH] = RenderTarget::Create(ppTextures[PP_TEXTURE_LINEAR_DEPTH], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_2X] = textureManager.AllocTexture(va("_%i_screenDepthD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_2X]->CreateEmpty(RHI::Texture2D, renderingWidth >> 1, renderingHeight >> 1, 1, 1, 1, Image::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_2X], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_4X] = textureManager.AllocTexture(va("_%i_screenDepthD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_4X]->CreateEmpty(RHI::Texture2D, renderingWidth >> 2, renderingHeight >> 2, 1, 1, 1, Image::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenDepthTempD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_TEMP_4X]->CreateEmpty(RHI::Texture2D, renderingWidth >> 2, renderingHeight >> 2, 1, 1, 1, Image::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_TEMP_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_TEMP_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_VEL] = textureManager.AllocTexture(va("_%i_screenVelocity", (int)contextHandle));
    ppTextures[PP_TEXTURE_VEL]->CreateEmpty(RHI::Texture2D, renderingWidth >> 1, renderingHeight >> 1, 1, 1, 1, Image::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_VEL] = RenderTarget::Create(ppTextures[PP_TEXTURE_VEL], nullptr, 1);

    ppTextures[PP_TEXTURE_AO] = textureManager.AllocTexture(va("_%i_screenAo", (int)contextHandle));
    ppTextures[PP_TEXTURE_AO]->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_AO] = RenderTarget::Create(ppTextures[PP_TEXTURE_AO], nullptr, 1);

    ppTextures[PP_TEXTURE_AO_TEMP] = textureManager.AllocTexture(va("_%i_screenAoTemp", (int)contextHandle));
    ppTextures[PP_TEXTURE_AO_TEMP]->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_AO_TEMP] = RenderTarget::Create(ppTextures[PP_TEXTURE_AO_TEMP], nullptr, 1);

    //--------------------------------------
    // refraction 등을 구현하기 위한 screen 복사 texture 만들기
    //--------------------------------------

    if (!currentRenderTexture) {
        currentRenderTexture = textureManager.AllocTexture(va("_%i_currentRender", (int)contextHandle));
    }

    currentRenderTexture->Purge();
    currentRenderTexture->CreateEmpty(RHI::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, screenImageFormat,
        Texture::Clamp | Texture::NoMipmaps | Texture::NonPowerOfTwo | Texture::HighPriority | Texture::SRGB);
}

void RenderContext::FreeScreenMapRT() {
    if (screenRT) {
        if (screenColorTexture) {
            textureManager.ReleaseTexture(screenColorTexture, true);
            screenColorTexture = nullptr;
        }

        if (screenNormalTexture) {
            textureManager.ReleaseTexture(screenNormalTexture, true);
            screenNormalTexture = nullptr;
        }

        if (screenDepthTexture) {
            textureManager.ReleaseTexture(screenDepthTexture, true);
            screenDepthTexture = nullptr;
        }

        RenderTarget::Delete(screenRT);
        screenRT = nullptr;
    }	

    if (screenLitAccRT) {
        if (screenLitAccTexture) {
            textureManager.ReleaseTexture(screenLitAccTexture, true);
            screenLitAccTexture = nullptr;
        }
        RenderTarget::Delete(screenLitAccRT);
        screenLitAccRT = nullptr;
    }

    if (screenSelectionRT) {
        if (screenSelectionTexture) {
            textureManager.ReleaseTexture(screenSelectionTexture, true);
            screenSelectionTexture = nullptr;
        }
        RenderTarget::Delete(screenSelectionRT);
        screenSelectionRT = nullptr;
    }

    if (homRT) {
        if (homTexture) {
            textureManager.ReleaseTexture(homTexture, true);
            homTexture = nullptr;
        }
        RenderTarget::Delete(homRT);
        homRT = nullptr;
    }

    for (int i = 0; i < MAX_PP_TEXTURES; i++) {
        if (ppTextures[i]) {
            textureManager.ReleaseTexture(ppTextures[i], true);
            ppTextures[i] = nullptr;
        }
    }

    for (int i = 0; i < MAX_PP_RTS; i++) {
        if (ppRTs[i]) {
            RenderTarget::Delete(ppRTs[i]);
            ppRTs[i] = nullptr;
        }
    }

    if (currentRenderTexture) {
        textureManager.ReleaseTexture(currentRenderTexture, true);
        currentRenderTexture = nullptr;
    }
}

void RenderContext::InitHdrMapRT() {
    FreeHdrMapRT();

    if (r_HDR.GetInteger() == 0) {
        return;
    }

    //--------------------------------------
    // HDR RT 
    //--------------------------------------
   
    Image::Format screenImageFormat = GetScreenImageFormat();
    Image hdrBloomImage;
    hdrBloomImage.Create2D(renderingWidth >> 2, renderingHeight >> 2, 1, screenImageFormat, nullptr, Image::LinearSpaceFlag);

    for (int i = 0; i < COUNT_OF(hdrBloomRT); i++) {
        hdrBloomTexture[i] = textureManager.AllocTexture(va("_%i_hdrBloom%i", (int)contextHandle, i));
        hdrBloomTexture[i]->Create(RHI::Texture2D, hdrBloomImage, 
            Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality | Texture::NonPowerOfTwo | Texture::HighPriority);
        hdrBloomRT[i] = RenderTarget::Create(hdrBloomTexture[i], nullptr, 0);
    }

    Image::Format lumImageFormat = Image::L_16F;
    if (r_HDR.GetInteger() == 3) {
        lumImageFormat = Image::L_32F;
    }

    int size = renderingWidth >> 2;

    for (int i = 0; i < COUNT_OF(hdrLumAverageRT); i++) {
        size = size >> 2;

        hdrLumAverageTexture[i] = textureManager.AllocTexture(va("_%i_hdrLumAverage%i", (int)contextHandle, i));
        hdrLumAverageTexture[i]->CreateEmpty(RHI::Texture2D, size, size, 1, 1, 1, lumImageFormat, 
            Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality | Texture::NonPowerOfTwo | Texture::HighPriority);
        hdrLumAverageRT[i] = RenderTarget::Create(hdrLumAverageTexture[i], nullptr, 0);
            
        if (size <= 4) {
            break;
        }
    }

    for (int i = 0; i < COUNT_OF(hdrLuminanceTexture); i++) {
        hdrLuminanceTexture[i] = textureManager.AllocTexture(va("_%i_hdrLuminance%i", (int)contextHandle, i));
        hdrLuminanceTexture[i]->CreateEmpty(RHI::Texture2D, 1, 1, 1, 1, 1, lumImageFormat, 
            Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality | Texture::HighPriority);
        hdrLuminanceRT[i] = RenderTarget::Create(hdrLuminanceTexture[i], nullptr, 0);        
        //hdrLuminanceRT[i]->Clear(Vec4(0.5, 0.5, 0.5, 1.0), 0.0f, 0.0f);
    }
}

void RenderContext::FreeHdrMapRT() {
    for (int i = 0; i < COUNT_OF(hdrBloomRT); i++) {
        if (hdrBloomRT[i]) {
            if (hdrBloomTexture[i]) {
                textureManager.ReleaseTexture(hdrBloomTexture[i], true);
                hdrBloomTexture[i] = nullptr;
            }
            RenderTarget::Delete(hdrBloomRT[i]);
            hdrBloomRT[i] = nullptr;
        }
    }

    for (int i = 0; i < COUNT_OF(hdrLumAverageRT); i++) {
        if (hdrLumAverageRT[i]) {
            if (hdrLumAverageTexture[i]) {
                textureManager.ReleaseTexture(hdrLumAverageTexture[i], true);
                hdrLumAverageTexture[i] = nullptr;
            }
            RenderTarget::Delete(hdrLumAverageRT[i]);
            hdrLumAverageRT[i] = nullptr;
        }
    }

    for (int i = 0; i < 3; i++) {
        if (hdrLuminanceRT[i]) {
            if (hdrLuminanceTexture[i]) {
                textureManager.ReleaseTexture(hdrLuminanceTexture[i], true);
                hdrLuminanceTexture[i] = nullptr;
            }
            RenderTarget::Delete(hdrLuminanceRT[i]);
            hdrLuminanceRT[i] = nullptr;
        }
    }
}

void RenderContext::InitShadowMapRT() {
    FreeShadowMapRT();

    if (r_shadows.GetInteger() == 0) {
        return;
    }

    Image::Format shadowImageFormat = (r_shadowMapFloat.GetBool() && rhi.SupportsDepthBufferFloat()) ? Image::Depth_32F : Image::Depth_24;

    RHI::TextureType textureType = RHI::Texture2DArray;

    int csmCount = r_CSM_count.GetInteger();

    // Cascaded shadow map
    shadowRenderTexture = textureManager.AllocTexture(va("_%i_shadowRender", (int)contextHandle));
    shadowRenderTexture->CreateEmpty(textureType, r_shadowMapSize.GetInteger(), r_shadowMapSize.GetInteger(), 1, csmCount, 1, shadowImageFormat,
        Texture::Shadow | Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality | Texture::HighPriority);
    shadowMapRT = RenderTarget::Create(nullptr, shadowRenderTexture, 0);

    // Virtual shadow cube map
    vscmTexture = textureManager.AllocTexture(va("_%i_vscmRender", (int)contextHandle));
    vscmTexture->CreateEmpty(RHI::Texture2D, r_shadowCubeMapSize.GetInteger(), r_shadowCubeMapSize.GetInteger(), 1, 1, 1, shadowImageFormat,
        Texture::Shadow | Texture::Clamp | Texture::NoMipmaps | Texture::NonPowerOfTwo | Texture::HighQuality | Texture::HighPriority);
    vscmRT = RenderTarget::Create(nullptr, vscmTexture, 0);
    vscmRT->Clear(Color4(0, 0, 0, 0), 1.0f, 0);

    vscmCleared[0] = false;
    vscmCleared[1] = false;
    vscmCleared[2] = false;
    vscmCleared[3] = false;
    vscmCleared[4] = false;
    vscmCleared[5] = false;

    if (!indirectionCubeMapTexture) {
        indirectionCubeMapTexture = textureManager.AllocTexture(va("_%i_indirectionCubeMap", (int)contextHandle));
        indirectionCubeMapTexture->CreateIndirectionCubemap(256, vscmRT->GetWidth(), vscmRT->GetHeight());
    }
    vscmBiasedFov = DEG2RAD(90.0f + 0.8f);
    vscmBiasedScale = 1.0f / Math::Tan(vscmBiasedFov * 0.5f);
}

void RenderContext::FreeShadowMapRT() {
    if (shadowMapRT) {
        if (shadowRenderTexture) {
            textureManager.ReleaseTexture(shadowRenderTexture, true);
        }

        if (shadowMapRT) {
            RenderTarget::Delete(shadowMapRT);
            shadowMapRT = nullptr;
        }
    }

    if (vscmRT) {
        if (vscmTexture) {
            textureManager.ReleaseTexture(vscmTexture, true);
        }

        if (vscmRT) {
            RenderTarget::Delete(vscmRT);
            vscmRT = nullptr;
        }
    }

    if (indirectionCubeMapTexture) {
        textureManager.ReleaseTexture(indirectionCubeMapTexture, true);
        indirectionCubeMapTexture = nullptr;
    }
}

void RenderContext::OnResize(int width, int height) {
    float upscaleX = GetUpscaleFactorX();
    float upscaleY = GetUpscaleFactorY();

    rhi.GetContextSize(this->contextHandle, &this->windowWidth, &this->windowHeight, &this->deviceWidth, &this->deviceHeight);

    //this->windowWidth = width;
    //this->windowHeight = height;
    //this->deviceWidth = width;
    //this->deviceHeight = height;

    // deferred resizing render targets
    renderingWidth = this->deviceWidth / upscaleX;
    renderingHeight = this->deviceHeight / upscaleY;

    guiMesh.SetClipRect(Rect(0, 0, renderingWidth, renderingHeight));

    //rhi.ChangeDisplaySettings(deviceWidth, deviceHeight, rhi.IsFullScreen());
}

void RenderContext::Display() {
    rhi.DisplayContext(contextHandle);
}

void RenderContext::BeginFrame() {
    renderSystem.currentContext = this;

    startFrameMsec = PlatformTime::Milliseconds();

    frameTime = startFrameMsec * 0.001f - elapsedTime;

    elapsedTime = startFrameMsec * 0.001f;

    memset(&renderCounter, 0, sizeof(renderCounter));

    rhi.SetContext(GetContextHandle());

    // Window size have changed since last call of BeginFrame()
    if (renderingWidth != screenRT->GetWidth() || renderingHeight != screenRT->GetHeight()) {
        FreeScreenMapRT();
        FreeHdrMapRT();

        InitScreenMapRT();
        InitHdrMapRT();
    }
    
    BeginContextRenderCommand *cmd = (BeginContextRenderCommand *)renderSystem.GetCommandBuffer(sizeof(BeginContextRenderCommand));
    cmd->commandId = BeginContextCommand;
    cmd->renderContext = this;

    bufferCacheManager.BeginWrite();
}

void RenderContext::EndFrame() {
    frameCount++;

    // Adds GUI commands
    renderSystem.primaryWorld->EmitGuiFullScreen(guiMesh);

    // Adds swap buffer command
    SwapBuffersRenderCommand *cmd = (SwapBuffersRenderCommand *)renderSystem.GetCommandBuffer(sizeof(SwapBuffersRenderCommand));
    cmd->commandId = SwapBuffersCommand;

    bufferCacheManager.BeginBackEnd();
    
    renderSystem.IssueCommands();

    bufferCacheManager.EndDrawCommand();

    guiMesh.Clear();

    frameData.ToggleFrame();

    renderCounter.frameMsec = PlatformTime::Milliseconds() - startFrameMsec;

    if (r_showStats.GetInteger() > 0) {
        switch (r_showStats.GetInteger()) {
        case 1:
            BE_LOG(L"draw:%i verts:%i tris:%i sdraw:%i sverts:%i stris:%i\n", 
                renderCounter.drawCalls, renderCounter.drawVerts, renderCounter.drawIndexes / 3, 
                renderCounter.shadowDrawCalls, renderCounter.shadowDrawVerts, renderCounter.shadowDrawIndexes / 3);
            break;
        case 2:
            BE_LOG(L"frame:%i rb:%i homGen:%i homQuery:%i homCull:%i\n", 
                renderCounter.frameMsec, renderCounter.backEndMsec, renderCounter.homGenMsec, renderCounter.homQueryMsec, renderCounter.homCullMsec);
            break;
        case 3:
            BE_LOG(L"shadowmap:%i skinning:%i\n",
                renderCounter.numShadowMapDraw, renderCounter.numSkinningEntities);
            break;
        }
    }

    renderSystem.currentContext = nullptr;
}

void RenderContext::AdjustFrom640x480(float *x, float *y, float *w, float *h) const {
    float xScale = deviceWidth / 640.0f;
    float yScale = deviceHeight / 480.0f;

    if (x) {
        *x *= xScale;
    }
    if (y) {
        *y *= yScale;
    }
    if (w) {
        *w *= xScale;
    }
    if (h) {
        *h *= yScale;
    }
}

void RenderContext::SetClipRect(const Rect &clipRect) {
    guiMesh.SetClipRect(clipRect);
}

void RenderContext::DrawPic(float x, float y, float w, float h, const Material *material) {
    guiMesh.SetColor(color);
    guiMesh.DrawPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, material);
}

void RenderContext::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material) {
    guiMesh.SetColor(color);
    guiMesh.DrawPic(x, y, w, h, s1, t1, s2, t2, material);
}

void RenderContext::DrawBar(float x, float y, float w, float h) {
    guiMesh.SetColor(color);
    guiMesh.DrawPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, color.a < 1.0f ? materialManager.blendMaterial : materialManager.whiteMaterial);
}

void RenderContext::DrawRect(float x, float y, float w, float h) {
    guiMesh.SetColor(color);

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

void RenderContext::UpdateCurrentRenderTexture() const {
    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SelectTextureUnit(0);
    
    //float starttime = PlatformTime::Seconds();

    currentRenderTexture->Bind();
    rhi.CopyTextureSubImage2D(0, 0, 0, 0, currentRenderTexture->GetWidth(), currentRenderTexture->GetHeight());
    //rhi.GenerateMipmap();
    
    //BE_LOG(L"%f\n", PlatformTime::Seconds() - starttime);
}

float RenderContext::QueryDepth(const Point &point) {
    Image::Format format = screenSelectionRT->ColorTexture()->GetFormat();
    byte *depthData = (byte *)_alloca(4);

    float scaleX = (float)screenSelectionRT->GetWidth() / screenRT->GetWidth();
    float scaleY = (float)screenSelectionRT->GetHeight() / screenRT->GetHeight();

    Vec2 scaledReadPoint;
    scaledReadPoint.x = point.x * scaleX;
    scaledReadPoint.y = (screenRT->GetHeight() - (point.y + 1)) * scaleY;

    screenSelectionRT->Begin();

    // FIXME: is depth format confirmed ?
    rhi.ReadPixels(scaledReadPoint.x, scaledReadPoint.y, 1, 1, Image::Depth_24, depthData); 
    screenSelectionRT->End();

    float depth = (float)MAKE_FOURCC(depthData[2], depthData[1], depthData[0], 0) / (float)(BIT(24) - 1);
    return depth;
}

int RenderContext::QuerySelection(const Point &point) {
    Image::Format format = screenSelectionRT->ColorTexture()->GetFormat();
    byte *data = (byte *)_alloca(Image::BytesPerPixel(format));

    float scaleX = (float)screenSelectionRT->GetWidth() / screenRT->GetWidth();
    float scaleY = (float)screenSelectionRT->GetHeight() / screenRT->GetHeight();

    Vec2 scaledReadPoint;
    scaledReadPoint.x = point.x * scaleX;
    scaledReadPoint.y = (screenRT->GetHeight() - (point.y + 1)) * scaleY;

    screenSelectionRT->Begin();
    rhi.ReadPixels(scaledReadPoint.x, scaledReadPoint.y, 1, 1, format, data);
    screenSelectionRT->End();

    int id = ((int)data[2] << 16) | ((int)data[1] << 8) | ((int)data[0]);

    return id;
}

bool RenderContext::QuerySelection(const Rect &rect, Inclusion inclusion, Array<int> &indexes) {
    if (rect.IsEmpty()) {
        return false;
    }

    float scaleX = (float)screenSelectionRT->GetWidth() / screenRT->GetWidth();
    float scaleY = (float)screenSelectionRT->GetHeight() / screenRT->GetHeight();

    Rect scaledReadRect;
    scaledReadRect.x = rect.x * scaleX;
    scaledReadRect.y = (screenRT->GetHeight() - (rect.y + rect.h)) * scaleY;
    scaledReadRect.w = Max(rect.w * scaleX, 1.0f);
    scaledReadRect.h = Max(rect.h * scaleY, 1.0f);

    Image::Format format = screenSelectionRT->ColorTexture()->GetFormat();
    int bpp = Image::BytesPerPixel(format);

    if (inclusion == Inclusion::Crossing) {
        int pixelCount = scaledReadRect.w * scaledReadRect.h;
        byte *data = (byte *)Mem_Alloc(bpp * pixelCount);
    
        screenSelectionRT->Begin();
        rhi.ReadPixels(scaledReadRect.x, scaledReadRect.y, scaledReadRect.w, scaledReadRect.h, format, data);
        screenSelectionRT->End();

        indexes.Clear();

        byte *data_ptr = data;

        for (int i = 0; i < pixelCount; i++) {
            int id = (((int)data_ptr[2]) << 16) | (((int)data_ptr[1]) << 8) | ((int)data_ptr[0]);
            indexes.AddUnique(id);

            data_ptr += bpp;
        }

        Mem_Free(data);
    } else if (inclusion == Inclusion::Window) {
        byte *data = (byte *)Mem_Alloc(bpp * screenSelectionRT->GetWidth() * screenSelectionRT->GetHeight());
    
        screenSelectionRT->Begin();
        rhi.ReadPixels(0, 0, screenSelectionRT->GetWidth(), screenSelectionRT->GetHeight(), format, data);
        screenSelectionRT->End();

        indexes.Clear();

        for (int y = scaledReadRect.y; y < scaledReadRect.Y2(); y++) {
            int pitch = y * screenSelectionRT->GetWidth() * bpp;

            for (int x = scaledReadRect.x; x < scaledReadRect.X2(); x++) {
                byte *data_ptr = &data[pitch + x * bpp];

                int id = (((int)data_ptr[2]) << 16) | (((int)data_ptr[1]) << 8) | ((int)data_ptr[0]);
                indexes.AddUnique(id);
            }
        }
        
        for (int y = 0; y < screenSelectionRT->GetHeight(); y++) {
            int pitch = y * screenSelectionRT->GetWidth() * bpp;

            for (int x = 0; x < screenSelectionRT->GetWidth(); x++) {
                if (!scaledReadRect.IsContainPoint(x, y)) {
                    byte *data_ptr = &data[pitch + x * bpp];

                    int id = (((int)data_ptr[2]) << 16) | (((int)data_ptr[1]) << 8) | ((int)data_ptr[0]);
                    indexes.Remove(id);
                }
            }
        }

        Mem_Free(data);
    }

    if (indexes.Count() > 0) {
        return true;
    }

    return false;
}

void RenderContext::TakeScreenShot(const char *filename, RenderWorld *renderWorld, const Vec3 &origin, const Mat3 &axis, float fov, int width, int height) {
    char path[256];

    SceneView view;
    SceneView::Parms viewParms;
    memset(&viewParms, 0, sizeof(viewParms));
    viewParms.flags = SceneView::Flag::TexturedMode | SceneView::Flag::NoSubViews | SceneView::Flag::SkipPostProcess | SceneView::Flag::SkipDebugDraw;
    viewParms.clearMethod = SceneView::SkyboxClear;
    viewParms.clearColor = Color4(0.29f, 0.33f, 0.35f, 0);
    viewParms.layerMask = BIT(0);
    viewParms.renderRect.Set(0, 0, width, height);
    viewParms.origin = origin;
    viewParms.axis = axis;

    Vec3 v;
    renderWorld->GetStaticAABB().GetFarthestVertexFromDir(axis[0], v);
    viewParms.zFar = Max(8192.0f, origin.Distance(v));
    viewParms.zNear = 4;

    SceneView::ComputeFov(fov, 1.25f, (float)width / height, &viewParms.fovX, &viewParms.fovY);

    view.Update(&viewParms);

    BeginFrame();
    renderWorld->RenderScene(&view);
    Str::snPrintf(path, sizeof(path), "%s.png", filename);
    renderSystem.CmdScreenshot(0, 0, width, height, path);
    EndFrame();

    BE_LOG(L"Screenshot saved to \"%hs\"\n", path);
}

void RenderContext::CaptureEnvCubeImage(RenderWorld *renderWorld, const Vec3 &origin, int size, Image &envCubeImage) {    
    SceneView view;
    SceneView::Parms viewParms;
    memset(&viewParms, 0, sizeof(viewParms));
    viewParms.flags = SceneView::Flag::TexturedMode | SceneView::NoSubViews | SceneView::SkipPostProcess | SceneView::Flag::SkipDebugDraw;
    viewParms.clearMethod = SceneView::SkyboxClear;
    viewParms.layerMask = BIT(0);
    viewParms.renderRect.Set(0, 0, size, size);
    viewParms.fovX = 90;
    viewParms.fovY = 90;
    viewParms.zNear = 4.0f;
    viewParms.zFar = 8192.0f;
    viewParms.origin = origin;

    Mat3 viewAxis[6];
    viewAxis[0] = Angles( 90,   0, 0).ToMat3();
    viewAxis[1] = Angles(-90,   0, 0).ToMat3();
    viewAxis[2] = Angles(  0, -90, 0).ToMat3();
    viewAxis[3] = Angles(  0,  90, 0).ToMat3();
    viewAxis[4] = Angles(  0,   0, 0).ToMat3();
    viewAxis[5] = Angles(180,   0, 0).ToMat3();

#if 1
    Image image;
    image.Create2D(size, size, 1, Image::RGB_32F_32F_32F, nullptr, Image::LinearSpaceFlag);
    Texture *targetTexture = new Texture;
    targetTexture->Create(RHI::Texture2D, image, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    RenderTarget *targetRT = RenderTarget::Create(targetTexture, nullptr, 0);

    Rect srcRect = Rect(0, 0, size, size);
    Rect dstRect = Rect(0, 0, size, size);

    srcRect.y = screenRT->GetHeight() - (srcRect.y + srcRect.h);

    Image faceImages[6];

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        viewParms.axis = viewAxis[faceIndex];
        view.Update(&viewParms);

        BeginFrame();

        renderWorld->RenderScene(&view);

        EndFrame();

        screenRT->Blit(srcRect, dstRect, targetRT, RHI::ColorBlitMask, RHI::NearestBlitFilter);

        faceImages[faceIndex].Create2D(size, size, 1, Image::RGB_32F_32F_32F, nullptr, 0);

        targetRT->ColorTexture()->Bind();
        targetRT->ColorTexture()->GetTexels2D(Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels());

        faceImages[faceIndex].FlipX(); // Flip for environment image to cubemap face image
        faceImages[faceIndex].FlipY(); // Flip upside down
    }

    envCubeImage.CreateCubeFrom6Faces(faceImages);

    SAFE_DELETE(targetTexture);

    RenderTarget::Delete(targetRT);
#else
    Texture *targetCubeTexture = new Texture;
    targetCubeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, 1, Image::RGB_32F_32F_32F, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    
    RenderTarget *targetCubeRT = RenderTarget::Create(targetCubeTexture, nullptr, 0);

    //Rect srcRect = Rect(0, 0, size, size);
    //Rect dstRect = Rect(0, 0, size, size);

    //srcRect.y = screenRT->GetHeight() - (srcRect.y + srcRect.h);

    Image faceImages[6];

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        faceImages[faceIndex].Create2D(size, size, 1, Image::RGB_32F_32F_32F, nullptr, Image::LinearSpaceFlag);

        targetCubeRT->Begin(0, 0);

        viewParms.axis = viewAxis[faceIndex];
        view.Update(&viewParms);

        BeginFrame();
        
        renderWorld->RenderScene(&view);
        
        EndFrame();

        //screenRT->Blit(srcRect, dstRect, targetRT, RHI::ColorBlitMask, RHI::NearestBlitFilter);

        rhi.ReadPixels(0, 0, size, size, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels());

        targetCubeRT->End();

        //targetRT->ColorTexture()->Bind();
        //targetRT->ColorTexture()->GetTexels2D(Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels());

        faceImages[faceIndex].FlipX(); // Flip for environment image to cubemap face image
        faceImages[faceIndex].FlipY(); // Flip upside down
    }

    envCubeImage.CreateCubeFrom6Faces(faceImages);

    SAFE_DELETE(targetCubeTexture);

    RenderTarget::Delete(targetCubeRT);
#endif
}

void RenderContext::TakeEnvShot(const char *filename, RenderWorld *renderWorld, const Vec3 &origin, int size) {
    Image envCubeImage;
    CaptureEnvCubeImage(renderWorld, origin, size, envCubeImage);

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.dds", filename);
    envCubeImage.ConvertFormatSelf(Image::RGB_11F_11F_10F, false, Image::HighQuality);
    envCubeImage.WriteDDS(path);

    BE_LOG(L"Environment cubemap snapshot saved to \"%hs\"\n", path);
}

void RenderContext::TakeIrradianceEnvShot(const char *filename, RenderWorld *renderWorld, const Vec3 &origin) {
    Image envCubeImage;
    CaptureEnvCubeImage(renderWorld, origin, 256, envCubeImage);

    Image irradianceEnvCubeImage;
#if 1
    GenerateIrradianceEnvCubeImage(envCubeImage, 64, irradianceEnvCubeImage);
#else
    GenerateIrradianceEnvCubeImageSHConvolv(envCubeImage, 64, irradianceEnvCubeImage);
#endif

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.dds", filename);
    irradianceEnvCubeImage.ConvertFormatSelf(Image::RGB_11F_11F_10F, false, Image::HighQuality);
    irradianceEnvCubeImage.WriteDDS(path);

    BE_LOG(L"Generated diffuse irradiance cubemap to \"%hs\"\n", path);
}

void RenderContext::TakePrefilteredEnvShot(const char *filename, RenderWorld *renderWorld, const Vec3 &origin) {
    Image envCubeImage;
    CaptureEnvCubeImage(renderWorld, origin, 256, envCubeImage);

    Image prefilteredCubeImage;
#if 1
    GenerateGGXPrefilteredEnvCubeImage(envCubeImage, 256, prefilteredCubeImage);
#else
    GeneratePhongSpecularPrefilteredEnvCubeImage(envCubeImage, 128, 2048, prefilteredCubeImage);
#endif

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.dds", filename);
    prefilteredCubeImage.ConvertFormatSelf(Image::RGB_11F_11F_10F, false, Image::HighQuality);
    prefilteredCubeImage.WriteDDS(path);

    BE_LOG(L"Generated specular prefiltered cubemap to \"%hs\"\n", path);
}

void RenderContext::WriteBRDFIntegrationLUT(const char *filename, int size) const {
    Image integrationImage;
    GenerateGGXIntegrationLUTImage(size, integrationImage);

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.dds", filename);
    integrationImage.WriteDDS(path);

    BE_LOG(L"Generated GGX integration LUT to \"%hs\"\n", path);
}

void RenderContext::GenerateIrradianceEnvCubeImageSHConvolv(const Image &envCubeImage, int size, Image &irradianceEnvCubeImage) const {
    Texture *radianceCubeTexture = new Texture;
    radianceCubeTexture->Create(RHI::TextureCubeMap, envCubeImage, Texture::CubeMap | Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);

    //-------------------------------------------------------------------------------
    // Create 4-by-4 envmap sized block weight map for each faces
    //------------------------------------------------------------------------------- 
    int envMapSize = envCubeImage.GetWidth();
    Texture *weightTextures[6];

    float *weightData = (float *)Mem_Alloc(envMapSize * 4 * envMapSize * 4 * sizeof(float));
    float invSize = 1.0f / (envMapSize - 1);

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        for (int y = 0; y < envMapSize; y++) {
            for (int x = 0; x < envMapSize; x++) {
                float s = (x + 0.5f) * invSize;
                float t = (y + 0.5f) * invSize;

                // Gets sample direction for each faces 
                Vec3 dir = Image::FaceToCubeMapCoords((Image::CubeMapFace)faceIndex, s, t);
                dir.Normalize();

                // 9 terms are required for order 3 SH basis functions
                float basisEval[16] = { 0, };
                // Evaluates the 9 SH basis functions Ylm with the given direction
                SphericalHarmonics::EvalBasis(3, dir, basisEval);

                // Solid angle of the cubemap texel
                float dw = Image::CubeMapTexelSolidAngle(x, y, envMapSize);

                // Precalculates 9 terms (basisEval * dw) for each envmap pixel in the 4-by-4 envmap sized block texture for each faces  
                for (int j = 0; j < 4; j++) {
                    for (int i = 0; i < 4; i++) {
                        int offset = (((j * envMapSize + y) * envMapSize) << 2) + i * envMapSize + x;

                        weightData[offset] = basisEval[(j << 2) + i] * dw;
                    }
                }
            }
        }

        weightTextures[faceIndex] = new Texture;
        weightTextures[faceIndex]->Create(RHI::Texture2D, Image(envMapSize * 4, envMapSize * 4, 1, 1, 1, Image::L_32F, (byte *)weightData, Image::LinearSpaceFlag),
            Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    }

    Mem_Free(weightData);

    //-------------------------------------------------------------------------------
    // SH projection of (Li * dw) and create 9 coefficents in a single 4x4 texture
    //-------------------------------------------------------------------------------
    Shader *weightedSHProjShader = shaderManager.GetShader("Shaders/WeightedSHProj.shader");
    Shader *shader = weightedSHProjShader->InstantiateShader(Array<Shader::Define>());

    Image image;
    image.Create2D(4, 4, 1, Image::RGB_32F_32F_32F, nullptr, Image::LinearSpaceFlag);
    Texture *incidentCoeffTexture = new Texture;
    incidentCoeffTexture->Create(RHI::Texture2D, image, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    
    RenderTarget *incidentCoeffRT = RenderTarget::Create(incidentCoeffTexture, nullptr, 0);

    incidentCoeffRT->Begin();

    Rect prevViewportRect = rhi.GetViewport();
    rhi.SetViewport(Rect(0, 0, 4, 4));
    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    shader->Bind();
    shader->SetTextureArray("weightMap", 6, (const Texture **)weightTextures);
    shader->SetTexture("radianceCubeMap", radianceCubeTexture);
    shader->SetConstant1i("radianceCubeMapSize", radianceCubeTexture->GetWidth());
    shader->SetConstant1f("radianceScale", 1.0f);

    RB_DrawClipRect(0, 0, 1.0f, 1.0f);

    //rhi.ReadPixels(0, 0, 4, 4, Image::RGB_32F_32F_32F, image.GetPixels());

    rhi.SetViewport(prevViewportRect);
    incidentCoeffRT->End();

    shaderManager.ReleaseShader(shader);
    shaderManager.ReleaseShader(weightedSHProjShader);

    SAFE_DELETE(radianceCubeTexture);

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        SAFE_DELETE(weightTextures[faceIndex]);
    }

    //-------------------------------------------------------------------------------
    // SH convolution
    //-------------------------------------------------------------------------------
    Texture *irradianceEnvCubeTexture = new Texture;
    irradianceEnvCubeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, 1, Image::RGB_32F_32F_32F, Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);
    
    RenderTarget *irradianceEnvCubeRT = RenderTarget::Create(irradianceEnvCubeTexture, nullptr, 0);

    Shader *genDiffuseCubeMapSHConvolv = shaderManager.GetShader("Shaders/GenIrradianceEnvCubeMapSHConvolv.shader");
    shader = genDiffuseCubeMapSHConvolv->InstantiateShader(Array<Shader::Define>());

    // Precompute ZH coefficients * sqrt(4PI/(2l + 1)) of Lambert diffuse spherical function cos(theta) / PI
    // which function is rotationally symmetric so only 3 terms are needed
    float al[3];
    al[0] = SphericalHarmonics::Lambert_Al_Evaluator(0); // 1
    al[1] = SphericalHarmonics::Lambert_Al_Evaluator(1); // 2/3
    al[2] = SphericalHarmonics::Lambert_Al_Evaluator(2); // 1/4

    Image faceImages[6];

    for (int faceIndex = RHI::PositiveX; faceIndex <= RHI::NegativeZ; faceIndex++) {
        faceImages[faceIndex].Create2D(size, size, 1, Image::RGB_32F_32F_32F, nullptr, Image::LinearSpaceFlag);

        irradianceEnvCubeRT->Begin(0, faceIndex);
        Rect prevViewportRect = rhi.GetViewport();
        rhi.SetViewport(Rect(0, 0, size, size));
        rhi.SetStateBits(RHI::ColorWrite);
        rhi.SetCullFace(RHI::NoCull);

        shader->Bind();
        shader->SetTexture("incidentCoeffMap", incidentCoeffRT->ColorTexture());
        shader->SetConstant1i("targetCubeMapSize", size);
        shader->SetConstant1i("targetCubeMapFace", faceIndex);
        shader->SetConstantArray1f("lambertCoeff", COUNT_OF(al), al);

        RB_DrawClipRect(0, 0, 1.0f, 1.0f);

        rhi.ReadPixels(0, 0, size, size, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels());

        rhi.SetViewport(prevViewportRect);

        irradianceEnvCubeRT->End();

        //irradianceEnvCubeRT->ColorTexture()->Bind();
        //irradianceEnvCubeRT->ColorTexture()->GetTexelsCubemap((RHI::CubeMapFace)faceIndex, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels());
    }

    irradianceEnvCubeImage.CreateCubeFrom6Faces(faceImages);

    SAFE_DELETE(irradianceEnvCubeTexture);

    RenderTarget::Delete(irradianceEnvCubeRT);

    SAFE_DELETE(incidentCoeffTexture);
    
    RenderTarget::Delete(incidentCoeffRT);

    shaderManager.ReleaseShader(shader);
    shaderManager.ReleaseShader(genDiffuseCubeMapSHConvolv);
}

void RenderContext::GenerateIrradianceEnvCubeImage(const Image &envCubeImage, int size, Image &irradianceEnvCubeImage) const {
    Shader *genDiffuseCubeMapShader = shaderManager.GetShader("Shaders/GenIrradianceEnvCubeMap.shader");
    Shader *shader = genDiffuseCubeMapShader->InstantiateShader(Array<Shader::Define>());

    Texture *radianceCubeTexture = new Texture;
    radianceCubeTexture->Create(RHI::TextureCubeMap, envCubeImage, Texture::CubeMap | Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);

    Texture *irradianceEnvCubeTexture = new Texture;
    irradianceEnvCubeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, 1, Image::RGB_32F_32F_32F, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    
    RenderTarget *irradianceEnvCubeRT = RenderTarget::Create(irradianceEnvCubeTexture, nullptr, 0);

    Image faceImages[6];

    for (int faceIndex = RHI::PositiveX; faceIndex <= RHI::NegativeZ; faceIndex++) {
        faceImages[faceIndex].Create2D(size, size, 1, Image::RGB_32F_32F_32F, nullptr, Image::LinearSpaceFlag);

        irradianceEnvCubeRT->Begin(0, faceIndex);

        Rect prevViewportRect = rhi.GetViewport();
        rhi.SetViewport(Rect(0, 0, size, size));
        rhi.SetStateBits(RHI::ColorWrite);
        rhi.SetCullFace(RHI::NoCull);

        shader->Bind();
        shader->SetTexture("radianceCubeMap", radianceCubeTexture);
        shader->SetConstant1i("radianceCubeMapSize", radianceCubeTexture->GetWidth());
        shader->SetConstant1i("targetCubeMapSize", size);
        shader->SetConstant1i("targetCubeMapFace", faceIndex);

        RB_DrawClipRect(0, 0, 1.0f, 1.0f);

        rhi.ReadPixels(0, 0, size, size, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels());

        rhi.SetViewport(prevViewportRect);

        irradianceEnvCubeRT->End();

        //irradianceEnvCubeRT->ColorTexture()->Bind();
        //irradianceEnvCubeRT->ColorTexture()->GetTexelsCubemap((RHI::CubeMapFace)faceIndex, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels());
    }

    irradianceEnvCubeImage.CreateCubeFrom6Faces(faceImages);

    SAFE_DELETE(irradianceEnvCubeTexture);
    SAFE_DELETE(radianceCubeTexture);

    RenderTarget::Delete(irradianceEnvCubeRT);

    shaderManager.ReleaseShader(shader);
    shaderManager.ReleaseShader(genDiffuseCubeMapShader);
}

void RenderContext::GeneratePhongSpecularPrefilteredEnvCubeImage(const Image &envCubeImage, int size, int maxSpecularPower, Image &prefilteredCubeImage) const {
    Shader *genSpecularCubeMapShader = shaderManager.GetShader("Shaders/GenPhongSpecularPrefilteredEnvCubeMap.shader");
    Shader *shader = genSpecularCubeMapShader->InstantiateShader(Array<Shader::Define>());

    Texture *radianceCubeTexture = new Texture;
    radianceCubeTexture->Create(RHI::TextureCubeMap, envCubeImage, Texture::CubeMap | Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);

    int numMipLevels = Math::Log(2, size) + 1;

    // power drop range [maxSpecularPower, 2]
    float powerDropOnMip = Math::Pow(maxSpecularPower, -1.0f / numMipLevels);

    Texture *prefilteredCubeTexture = new Texture;
    prefilteredCubeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, numMipLevels, Image::RGB_32F_32F_32F, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    RenderTarget *prefilteredCubeRT = RenderTarget::Create(prefilteredCubeTexture, nullptr, 0);

    Image faceImages[6];

    for (int faceIndex = RHI::PositiveX; faceIndex <= RHI::NegativeZ; faceIndex++) {
        faceImages[faceIndex].Create2D(size, size, numMipLevels, Image::RGB_32F_32F_32F, nullptr, Image::LinearSpaceFlag);

        float specularPower = maxSpecularPower;

        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            int mipSize = size >> mipLevel;

            prefilteredCubeRT->Begin(mipLevel, faceIndex);

            Rect prevViewportRect = rhi.GetViewport();
            rhi.SetViewport(Rect(0, 0, mipSize, mipSize));
            rhi.SetStateBits(RHI::ColorWrite);
            rhi.SetCullFace(RHI::NoCull);

            shader->Bind();
            shader->SetTexture("radianceCubeMap", radianceCubeTexture);
            shader->SetConstant1i("radianceCubeMapSize", radianceCubeTexture->GetWidth());
            shader->SetConstant1i("targetCubeMapSize", mipSize);
            shader->SetConstant1i("targetCubeMapFace", faceIndex);
            shader->SetConstant1f("specularPower", specularPower);

            RB_DrawClipRect(0, 0, 1.0f, 1.0f);

            rhi.ReadPixels(0, 0, mipSize, mipSize, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels(mipLevel));

            rhi.SetViewport(prevViewportRect);

            prefilteredCubeRT->End();

            //prefilteredCubeRT->ColorTexture()->Bind();
            //prefilteredCubeRT->ColorTexture()->GetTexelsCubemap((RHI::CubeMapFace)faceIndex, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels(mipLevel));

            specularPower *= powerDropOnMip;
        }
    }

    prefilteredCubeImage.CreateCubeFrom6Faces(faceImages);

    SAFE_DELETE(radianceCubeTexture);
    SAFE_DELETE(prefilteredCubeTexture);

    RenderTarget::Delete(prefilteredCubeRT);

    shaderManager.ReleaseShader(shader);
    shaderManager.ReleaseShader(genSpecularCubeMapShader);
}

void RenderContext::GenerateGGXPrefilteredEnvCubeImage(const Image &envCubeImage, int size, Image &prefilteredCubeImage) const {
    Shader *genSpecularCubeMapShader = shaderManager.GetShader("Shaders/GenGGXSpecularPrefilteredEnvCubeMap.shader");
    Shader *shader = genSpecularCubeMapShader->InstantiateShader(Array<Shader::Define>());

    Texture *radianceCubeTexture = new Texture;
    radianceCubeTexture->Create(RHI::TextureCubeMap, envCubeImage, Texture::CubeMap | Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);

    int numMipLevels = Math::Log(2, size) + 1;

    Texture *prefilteredCubeTexture = new Texture;
    prefilteredCubeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, numMipLevels, Image::RGB_32F_32F_32F, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    
    RenderTarget *prefilteredCubeRT = RenderTarget::Create(prefilteredCubeTexture, nullptr, 0);

    Image faceImages[6];

    for (int faceIndex = RHI::PositiveX; faceIndex <= RHI::NegativeZ; faceIndex++) {
        faceImages[faceIndex].Create2D(size, size, numMipLevels, Image::RGB_32F_32F_32F, nullptr, Image::LinearSpaceFlag);

        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            int mipSize = size >> mipLevel;

            float roughness = (float)mipLevel / (numMipLevels - 1);
            
            prefilteredCubeRT->Begin(mipLevel, faceIndex);

            Rect prevViewportRect = rhi.GetViewport();
            rhi.SetViewport(Rect(0, 0, mipSize, mipSize));
            rhi.SetStateBits(RHI::ColorWrite);
            rhi.SetCullFace(RHI::NoCull);

            shader->Bind();
            shader->SetTexture("radianceCubeMap", radianceCubeTexture);
            shader->SetConstant1i("radianceCubeMapSize", radianceCubeTexture->GetWidth());
            shader->SetConstant1i("targetCubeMapSize", mipSize);
            shader->SetConstant1i("targetCubeMapFace", faceIndex);
            shader->SetConstant1f("roughness", roughness);

            RB_DrawClipRect(0, 0, 1.0f, 1.0f);

            rhi.ReadPixels(0, 0, mipSize, mipSize, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels(mipLevel));

            rhi.SetViewport(prevViewportRect);

            prefilteredCubeRT->End();

            //prefilteredCubeRT->ColorTexture()->Bind();
            //prefilteredCubeRT->ColorTexture()->GetTexelsCubemap((RHI::CubeMapFace)faceIndex, Image::RGB_32F_32F_32F, faceImages[faceIndex].GetPixels(mipLevel));
        }
    }

    prefilteredCubeImage.CreateCubeFrom6Faces(faceImages);

    SAFE_DELETE(radianceCubeTexture);
    SAFE_DELETE(prefilteredCubeTexture);

    RenderTarget::Delete(prefilteredCubeRT);

    shaderManager.ReleaseShader(shader);
    shaderManager.ReleaseShader(genSpecularCubeMapShader);
}

void RenderContext::GenerateGGXIntegrationLUTImage(int size, Image &integrationImage) const {
    Shader *genBrdfIntegrationLutShader = shaderManager.GetShader("Shaders/GenBRDFIntegrationLUT.shader");
    Shader *shader = genBrdfIntegrationLutShader->InstantiateShader(Array<Shader::Define>());

    Texture *integrationLutTexture = new Texture;
    integrationLutTexture->CreateEmpty(RHI::Texture2D, size, size, 1, 1, 1, Image::RG_16F_16F, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    
    RenderTarget *integrationLutRT = RenderTarget::Create(integrationLutTexture, nullptr, 0);

    integrationLutRT->Begin(0, 0);

    Rect prevViewportRect = rhi.GetViewport();
    rhi.SetViewport(Rect(0, 0, size, size));
    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    shader->Bind();

    RB_DrawClipRect(0, 0, 1.0f, 1.0f);

    integrationImage.Create2D(size, size, 1, Image::RG_16F_16F, nullptr, Image::LinearSpaceFlag);

    rhi.ReadPixels(0, 0, size, size, Image::RG_16F_16F, integrationImage.GetPixels());

    rhi.SetViewport(prevViewportRect);

    integrationLutRT->End();

    SAFE_DELETE(integrationLutTexture);

    RenderTarget::Delete(integrationLutRT);

    shaderManager.ReleaseShader(shader);
    shaderManager.ReleaseShader(genBrdfIntegrationLutShader);
}

BE_NAMESPACE_END
