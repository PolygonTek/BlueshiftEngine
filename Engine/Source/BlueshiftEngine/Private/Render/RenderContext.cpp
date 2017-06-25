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
    renderWidth = 0;
    renderHeight = 0;

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
    screenSelectionScale = 0.5f;
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

void RenderContext::Init(RHI::WindowHandle hwnd, int renderWidth, int renderHeight, RHI::DisplayContextFunc displayFunc, void *displayFuncDataPtr, int flags) {
    this->contextHandle = rhi.CreateContext(hwnd, (flags & Flag::UseSharedContext) ? true : false);
    this->flags = flags;
    
    rhi.GetContextSize(this->contextHandle, &this->windowWidth, &this->windowHeight, &this->deviceWidth, &this->deviceHeight);
    
    // actual rendering resolution to be upscaled
    this->renderWidth = renderWidth;
    this->renderHeight = renderHeight;
    
    this->guiMesh.Clear();
    this->guiMesh.SetCoordFrame(GuiMesh::CoordFrame2D);
    this->guiMesh.SetClipRect(Rect(0, 0, renderWidth, renderHeight));
    
    rhi.SetContextDisplayFunc(contextHandle, displayFunc, displayFuncDataPtr, (flags & OnDemandDrawing) ? true : false);

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
    screenColorTexture->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    
    screenDepthTexture = textureManager.AllocTexture(va("_%i_screenDepthstencil", (int)contextHandle));
    screenDepthTexture->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, Image::Depth_24, screenTextureFlags | Texture::Nearest);

    if (r_useDeferredLighting.GetBool()) {
        screenNormalTexture = textureManager.AllocTexture(va("_%i_screenNormal", (int)contextHandle));
        screenNormalTexture->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, Image::LA_16F_16F, screenTextureFlags | Texture::Nearest);		

        Texture *colorTextures[2];
        colorTextures[0] = screenColorTexture;
        colorTextures[1] = screenNormalTexture;
        screenRT = RenderTarget::Create(2, (const Texture **)colorTextures, screenDepthTexture, 0);
        //screenRT->SetMRTMask(3);

        screenLitAccTexture = textureManager.AllocTexture(va("_%i_screenLitAcc", (int)contextHandle));
        screenLitAccTexture->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, screenImageFormat, screenTextureFlags | Texture::Nearest | Texture::SRGB);
        screenLitAccRT = RenderTarget::Create(screenLitAccTexture, nullptr, 0);
    } else {
        screenRT = RenderTarget::Create(screenColorTexture, screenDepthTexture, 0);
    }

    screenRT->Clear(Color4::black, 1.0f, 0);

    if (flags & UseSelectionBuffer) {
        screenSelectionTexture = textureManager.AllocTexture(va("_%i_screenSelection", (int)contextHandle));
        screenSelectionTexture->CreateEmpty(RHI::Texture2D, renderWidth * screenSelectionScale, renderHeight * screenSelectionScale, 1, 1, Image::RGBA_8_8_8_8, screenTextureFlags);
    
        screenSelectionRT = RenderTarget::Create(screenSelectionTexture, nullptr, RHI::HasDepthBuffer);
    }

    if (r_HOM.GetBool()) {
        int w = Math::FloorPowerOfTwo(renderWidth >> 1);
        int h = Math::FloorPowerOfTwo(renderHeight >> 1);

        homTexture = textureManager.AllocTexture(va("_%i_hom", (int)contextHandle));
        homTexture->CreateEmpty(RHI::Texture2D, w, h, 1, 1, Image::Depth_32F, 
            Texture::Clamp | Texture::HighQuality | Texture::HighPriority | Texture::Nearest);			
        homTexture->Bind();
        rhi.GenerateMipmap();
        homRT = RenderTarget::Create(nullptr, (const Texture *)homTexture, 0);
    }

    //--------------------------------------
    // Post processing 용 RT 만들기
    //--------------------------------------	

    ppTextures[PP_TEXTURE_COLOR_2X] = textureManager.AllocTexture(va("_%i_screenColorD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_2X]->CreateEmpty(RHI::Texture2D, renderWidth >> 1, renderHeight >> 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_2X], nullptr, 0);	

    ppTextures[PP_TEXTURE_COLOR_4X] = textureManager.AllocTexture(va("_%i_screenColorD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_4X]->CreateEmpty(RHI::Texture2D, renderWidth >> 2, renderHeight >> 2, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_COLOR_TEMP] = textureManager.AllocTexture(va("_%i_screenColorTemp", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP]->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_TEMP] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP], nullptr, 0);

    ppTextures[PP_TEXTURE_COLOR_TEMP_2X] = textureManager.AllocTexture(va("_%i_screenColorTempD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP_2X]->CreateEmpty(RHI::Texture2D, renderWidth >> 1, renderHeight >> 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_TEMP_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_2X], nullptr, 0);

    ppTextures[PP_TEXTURE_COLOR_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenColorTempD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP_4X]->CreateEmpty(RHI::Texture2D, renderWidth >> 2, renderHeight >> 2, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    ppRTs[PP_RT_TEMP_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_4X], nullptr, 0);	

    //ppTextures[PP_TEXTURE_COLOR_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenColorTempD4x", (int)contextHandle));
    //ppTextures[PP_TEXTURE_COLOR_TEMP_4X]->CreateEmpty(RHI::Texture2D, renderWidth >> 2, renderHeight >> 2, 1, 1, screenImageFormat, screenTextureFlags | Texture::SRGB);
    //ppRTs[PP_RT_BLUR] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_LINEAR_DEPTH] = textureManager.AllocTexture(va("_%i_screenLinearDepth", (int)contextHandle));
    ppTextures[PP_TEXTURE_LINEAR_DEPTH]->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, Image::L_16F, screenTextureFlags);
    ppRTs[PP_RT_LINEAR_DEPTH] = RenderTarget::Create(ppTextures[PP_TEXTURE_LINEAR_DEPTH], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_2X] = textureManager.AllocTexture(va("_%i_screenDepthD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_2X]->CreateEmpty(RHI::Texture2D, renderWidth >> 1, renderHeight >> 1, 1, 1, Image::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_2X], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_4X] = textureManager.AllocTexture(va("_%i_screenDepthD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_4X]->CreateEmpty(RHI::Texture2D, renderWidth >> 2, renderHeight >> 2, 1, 1, Image::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenDepthTempD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_TEMP_4X]->CreateEmpty(RHI::Texture2D, renderWidth >> 2, renderHeight >> 2, 1, 1, Image::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_TEMP_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_TEMP_4X], nullptr, 0);		

    ppTextures[PP_TEXTURE_VEL] = textureManager.AllocTexture(va("_%i_screenVelocity", (int)contextHandle));
    ppTextures[PP_TEXTURE_VEL]->CreateEmpty(RHI::Texture2D, renderWidth >> 1, renderHeight >> 1, 1, 1, Image::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_VEL] = RenderTarget::Create(ppTextures[PP_TEXTURE_VEL], nullptr, 1);	

    ppTextures[PP_TEXTURE_AO] = textureManager.AllocTexture(va("_%i_screenAo", (int)contextHandle));
    ppTextures[PP_TEXTURE_AO]->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, Image::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_AO] = RenderTarget::Create(ppTextures[PP_TEXTURE_AO], nullptr, 1);

    ppTextures[PP_TEXTURE_AO_TEMP] = textureManager.AllocTexture(va("_%i_screenAoTemp", (int)contextHandle));
    ppTextures[PP_TEXTURE_AO_TEMP]->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, Image::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_AO_TEMP] = RenderTarget::Create(ppTextures[PP_TEXTURE_AO_TEMP], nullptr, 1);	

    //--------------------------------------
    // refraction 등을 구현하기 위한 screen 복사 texture 만들기
    //--------------------------------------

    if (!currentRenderTexture) {
        currentRenderTexture = textureManager.AllocTexture(va("_%i_currentRender", (int)contextHandle));
    }

    currentRenderTexture->Purge();
    currentRenderTexture->CreateEmpty(RHI::Texture2D, renderWidth, renderHeight, 1, 1, screenImageFormat,
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
    hdrBloomImage.Create2D(renderWidth >> 2, renderHeight >> 2, 1, screenImageFormat, nullptr, Image::LinearFlag);

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

    int size = renderWidth >> 2;

    for (int i = 0; i < COUNT_OF(hdrLumAverageRT); i++) {
        size = size >> 2;

        hdrLumAverageTexture[i] = textureManager.AllocTexture(va("_%i_hdrLumAverage%i", (int)contextHandle, i));
        hdrLumAverageTexture[i]->CreateEmpty(RHI::Texture2D, size, size, 1, 1, lumImageFormat, 
            Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality | Texture::NonPowerOfTwo | Texture::HighPriority);
        hdrLumAverageRT[i] = RenderTarget::Create(hdrLumAverageTexture[i], nullptr, 0);
            
        if (size <= 4) {
            break;
        }
    }

    for (int i = 0; i < COUNT_OF(hdrLuminanceTexture); i++) {
        hdrLuminanceTexture[i] = textureManager.AllocTexture(va("_%i_hdrLuminance%i", (int)contextHandle, i));
        hdrLuminanceTexture[i]->CreateEmpty(RHI::Texture2D, 1, 1, 1, 1, lumImageFormat, 
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
    shadowRenderTexture->CreateEmpty(textureType, r_shadowMapSize.GetInteger(), r_shadowMapSize.GetInteger(), 1, csmCount, shadowImageFormat,
        Texture::Shadow | Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality | Texture::HighPriority);
    shadowMapRT = RenderTarget::Create(nullptr, shadowRenderTexture, 0);

    // Virtual shadow cube map
    vscmTexture = textureManager.AllocTexture(va("_%i_vscmRender", (int)contextHandle));
    vscmTexture->CreateEmpty(RHI::Texture2D, r_shadowCubeMapSize.GetInteger(), r_shadowCubeMapSize.GetInteger(), 1, 1, shadowImageFormat,
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
    renderWidth = this->deviceWidth / upscaleX;
    renderHeight = this->deviceHeight / upscaleY;

    guiMesh.SetClipRect(Rect(0, 0, renderWidth, renderHeight));

    //rhi.ChangeDisplaySettings(deviceWidth, deviceHeight, rhi.IsFullScreen());
}

void RenderContext::Display() {
    BE1::rhi.DisplayContext(contextHandle);
}

void RenderContext::BeginFrame() {
    renderSystem.currentContext = this;

    startFrameMsec = PlatformTime::Milliseconds();

    frameTime = startFrameMsec * 0.001f - elapsedTime;

    elapsedTime = startFrameMsec * 0.001f;

    memset(&renderCounter, 0, sizeof(renderCounter));

    rhi.SetContext(GetContextHandle());

    if (renderWidth != screenRT->GetWidth() || renderHeight != screenRT->GetHeight()) {
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
    viewParms.flags = SceneView::Flag::NoSubViews | SceneView::Flag::SkipPostProcess;// | SceneView::Flag::NoUpscale;
    viewParms.clearMethod = SceneView::ColorClear;
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

    BE_LOG(L"screenshot saved to \"%hs\"\n", path);
}

// environment cubemap shot
void RenderContext::TakeEnvShot(const char *filename, const Vec3 &origin, int size) {
    static const char cubemap_postfix[6][3] = { "px", "nx", "py", "ny", "pz", "nz" };
    char path[256];

    SceneView view;
    SceneView::Parms viewParms;
    memset(&viewParms, 0, sizeof(viewParms));
    viewParms.flags = SceneView::NoSubViews;// | NoUpscale | SkipPostProcess;
    viewParms.layerMask = BIT(0);
    viewParms.renderRect.Set(0, 0, size, size);
    viewParms.fovX = 90;
    viewParms.fovY = 90;
    viewParms.zNear = 4.0f;
    viewParms.zFar = 8192.0f;
    viewParms.origin = origin;

    Mat3 axis[6];
    axis[0]	= Angles(  0,   0, -90).ToMat3();
    axis[1] = Angles(180,   0,  90).ToMat3();
    axis[2] = Angles( 90,   0, 180).ToMat3();
    axis[3] = Angles(-90,   0,   0).ToMat3();
    axis[4] = Angles(-90, -90,   0).ToMat3();
    axis[5] = Angles( 90,  90,   0).ToMat3();

    for (int i = 0; i < 6; i++) {
        viewParms.axis = axis[i];

        view.Update(&viewParms);

        BeginFrame();
        renderSystem.primaryWorld->RenderScene(&view);
        Str::snPrintf(path, sizeof(path), "%s_%s.png", filename, cubemap_postfix[i]);
        renderSystem.CmdScreenshot(0, 0, size, size, path);
        EndFrame();

        BE_LOG(L"envshot saved to \"%hs\"\n", path);
    }
}

// irradiance cubemap shot
void RenderContext::TakeIrradianceShot(const char *filename, const Vec3 &origin, int size) {
    static const char cubemap_postfix[6][3] = { "px", "nx", "py", "ny", "pz", "nz" };
    char path[256];
    int envmapSize = 256;

    SceneView view;
    SceneView::Parms viewParms;
    memset(&viewParms, 0, sizeof(viewParms));
    viewParms.flags = SceneView::NoSubViews;// | NoUpscale | SceneView::SkipPostProcess;
    viewParms.layerMask = BIT(0);
    viewParms.renderRect.Set(0, 0, envmapSize, envmapSize);
    viewParms.fovX = 90;
    viewParms.fovY = 90;
    viewParms.zNear = 4.0f;
    viewParms.zFar = 8192.0f;
    viewParms.origin = origin;

    Mat3 axis[6];
    axis[0]	= Angles(  0,   0, -90).ToMat3();
    axis[1] = Angles(180,   0,  90).ToMat3();
    axis[2] = Angles( 90,   0, 180).ToMat3();
    axis[3] = Angles(-90,   0,   0).ToMat3();
    axis[4] = Angles(-90, -90,   0).ToMat3();
    axis[5] = Angles( 90,  90,   0).ToMat3();

    for (int i = 0; i < 6; i++) {
        viewParms.axis = axis[i];
        view.Update(&viewParms);

        BeginFrame();
        renderSystem.primaryWorld->RenderScene(&view);
        Str::snPrintf(path, sizeof(path), "%s_%s.png", filename, cubemap_postfix[i]);
        renderSystem.CmdScreenshot(0, 0, envmapSize, envmapSize, path);
        EndFrame();
    }
    
    Texture *radianceCubeMap = new Texture;
    radianceCubeMap->Load(filename, Texture::CubeMap | Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);

    //-------------------------------------------------------------------------------
    // create 4x4 tiled weight cubemap
    //-------------------------------------------------------------------------------	
    
    Texture *weightTextures[6];

    // NOTE: 단순하게 4pi / 6 * envmapSize * envmapSize 로 했다. (문제가 있을지도 모르지만 GPU gems2 의 소스는 이상하다)
    // 정확하게 cubemap 텍셀당 solid angle 을 구하는 방법을 생각해봐야함
    float d_omega = 4.0 * Math::Pi / (6.0 * envmapSize * envmapSize);

    float tile[16];
    memset(tile, 0, sizeof(tile));

    float *weightData = (float *)Mem_Alloc(envmapSize * 4 * envmapSize * 4 * sizeof(float));
    
    for (int face = 0; face < 6; face++) {
        for (int y = 0; y < envmapSize; y++) {
            for (int x = 0; x < envmapSize; x++) {
                Vec3 dir = R_CubeCoord(envmapSize, face, x, y);
                dir.Normalize();

                float basisProj[16];
                // compute the N^2 spherical harmonic basis functions
                R_SH_EvalDirection(2, dir, basisProj);
                
                for (int i = 0; i < 9; i++) {
                    tile[i] = basisProj[i] * d_omega;
                }

                for (int j = 0; j < 4; j++) {
                    for (int i = 0; i < 4; i++) {
                        int offset = (((j * envmapSize + y) * envmapSize) << 2) + i * envmapSize + x;
                        weightData[offset] = tile[(j << 2) + i];
                    }
                }
            }
        }

        weightTextures[face] = new Texture;
        weightTextures[face]->Create(RHI::Texture2D, Image(envmapSize * 4, envmapSize * 4, 1, 1, 1, Image::L_32F, (byte *)weightData, Image::LinearFlag),
            Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    }

    Mem_Free(weightData);
    
    //-------------------------------------------------------------------------------
    // SH projection
    //-------------------------------------------------------------------------------
    Image image;
    image.Create2D(4, 4, 1, Image::RGB_32F_32F_32F, nullptr, Image::LinearFlag);
    Texture *shCoefficientTexture = new Texture;
    shCoefficientTexture->Create(RHI::Texture2D, image, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    RenderTarget *shCoefficientRT = RenderTarget::Create(shCoefficientTexture, nullptr, 0);
    
    shCoefficientRT->Begin();
    Rect prevViewportRect = rhi.GetViewport();
    rhi.SetViewport(Rect(0, 0, shCoefficientRT->GetWidth(), shCoefficientRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::shProjectionShader;

    shader->Bind();
    shader->SetTextureArray("weightMap", 6, (const Texture **)weightTextures);
    shader->SetTexture("radianceCubeMap", radianceCubeMap);
    shader->SetConstant1i("radianceCubeMapSize", radianceCubeMap->GetWidth());	
    shader->SetConstant1f("radianceScale", 0.5f);	
    RB_DrawClipRect(0, 0, 1.0f, 1.0f);
    
    rhi.SetViewport(prevViewportRect);
    shCoefficientRT->End();
    
    delete radianceCubeMap;

    for (int i = 0; i < 6; i++) {
        delete weightTextures[i];
    }
    
    //-------------------------------------------------------------------------------
    // SH evaluation
    //-------------------------------------------------------------------------------
    Texture *irradianceTexture;
    irradianceTexture = textureManager.AllocTexture("_irradiance");
    irradianceTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, Image::RGB_8_8_8, Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);
    RenderTarget *irradianceCubeRT = RenderTarget::Create(irradianceTexture, nullptr, 0);

    float al[3];
    al[0] = R_Lambert_Al_Evaluator(0);
    al[1] = R_Lambert_Al_Evaluator(1);
    al[2] = R_Lambert_Al_Evaluator(2);	
    
    for (int i = RHI::PositiveX; i <= RHI::NegativeZ; i++) {
        irradianceCubeRT->Begin(0, i);
        Rect prevViewportRect = rhi.GetViewport();
        rhi.SetViewport(Rect(0, 0, irradianceCubeRT->GetWidth(), irradianceCubeRT->GetHeight()));

        rhi.SetStateBits(RHI::ColorWrite);
        rhi.SetCullFace(RHI::NoCull);

        const Shader *shader = ShaderManager::shEvalIrradianceCubeMapShader;

        shader->Bind();
        shader->SetTexture("shCoefficientMap", shCoefficientRT->ColorTexture());
        shader->SetConstant1i("cubeMapSize", irradianceCubeRT->GetWidth());
        shader->SetConstant1i("cubeMapFace", i);
        shader->SetConstantArray1f("reflectanceTable", COUNT_OF(al), al);
        RB_DrawClipRect(0, 0, 1.0f, 1.0f);

        rhi.SetViewport(prevViewportRect);
        irradianceCubeRT->End();

        Image faceImage;
        faceImage.Create2D(irradianceCubeRT->GetWidth(), irradianceCubeRT->GetHeight(), 1, Image::RGB_8_8_8, nullptr, Image::LinearFlag);
        
        irradianceCubeRT->ColorTexture()->Bind();
        irradianceCubeRT->ColorTexture()->GetTexelsCubemap((RHI::CubeMapFace)i, Image::RGB_8_8_8, faceImage.GetPixels());

        faceImage.FlipY();
        
        Str::snPrintf(path, sizeof(path), "%s_irr_%s.png", filename, cubemap_postfix[i]);
        faceImage.Write(path);

        BE_LOG(L"irrshot saved to \"%hs\"\n", path);
    }
    
    textureManager.ReleaseTexture(irradianceTexture, true);
    RenderTarget::Delete(irradianceCubeRT);
    
    SAFE_DELETE(shCoefficientTexture);
    RenderTarget::Delete(shCoefficientRT);
}

BE_NAMESPACE_END
