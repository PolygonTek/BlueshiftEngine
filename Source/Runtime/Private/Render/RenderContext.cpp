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
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

RenderContext::RenderContext() {
    deviceWidth = 0;
    deviceHeight = 0;
    renderingWidth = 0;
    renderingHeight = 0;

    frameCount = 0;

    screenSelectionScale = 0.25f;

    prevLumTarget = 1;
    currLumTarget = 2;
}

void RenderContext::Init(RHI::WindowHandle hwnd, int renderingWidth, int renderingHeight, RHI::DisplayContextFunc displayFunc, void *displayFuncDataPtr, int flags) {
    this->contextHandle = rhi.CreateContext(hwnd, (flags & Flag::UseSharedContext) ? true : false);
    this->flags = flags;

    RHI::DisplayMetrics displayMetrics;
    rhi.GetDisplayMetrics(this->contextHandle, &displayMetrics);

    // This is window resolution in logical units (points).
    this->windowWidth = displayMetrics.screenWidth;
    this->windowHeight = displayMetrics.screenHeight;

    // This is device resolution in pixels.
    this->deviceWidth = displayMetrics.backingWidth;
    this->deviceHeight = displayMetrics.backingHeight;
    
    // This is actual rendering resolution that will be upscaled if it is smaller than device resolution.
    this->renderingWidth = renderingWidth;
    this->renderingHeight = renderingHeight;
    
    this->guiMesh.Clear();
    this->guiMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame2D);
    this->guiMesh.SetClipRect(Rect(0, 0, renderingWidth, renderingHeight));
    
    rhi.SetContextDisplayFunc(contextHandle, displayFunc, displayFuncDataPtr, (flags & Flag::OnDemandDrawing) ? true : false);

    InitScreenMapRT();

    InitHdrMapRT();

    InitShadowMapRT();

    this->random.SetSeed(123123);

    this->lastFrameMsec = PlatformTime::Milliseconds();
}

void RenderContext::Shutdown() {
    FreeScreenMapRT();

    FreeHdrMapRT();

    FreeShadowMapRT();

    rhi.DestroyContext(contextHandle);
}

static Image::Format::Enum GetScreenImageFormat() {
    if (r_HDR.GetInteger() == 0) {
        return Image::Format::RGBA_8_8_8_8;
    }
    
    if (r_HDR.GetInteger() == 1 && rhi.SupportsPackedFloat()) {
        return Image::Format::RGB_11F_11F_10F;
    }
    
    if (r_HDR.GetInteger() == 3) {
        return Image::Format::RGBA_32F_32F_32F_32F;
    }

    return Image::Format::RGBA_16F_16F_16F_16F;
}

void RenderContext::InitScreenMapRT() {
    FreeScreenMapRT();

    //--------------------------------------
    // Create screenRT
    //--------------------------------------

    int screenTextureFlags = Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo | Texture::Flag::HighPriority;

    Image::Format::Enum screenImageFormat = GetScreenImageFormat();

    screenColorTexture = textureManager.AllocTexture(va("_%i_screenColor", (int)contextHandle));
    screenColorTexture->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Flag::SRGBColorSpace);

    screenDepthTexture = textureManager.AllocTexture(va("_%i_screenDepthStencil", (int)contextHandle));
    screenDepthTexture->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::Format::Depth_24, screenTextureFlags | Texture::Flag::Nearest);

    if (r_useDeferredLighting.GetBool()) {
        screenNormalTexture = textureManager.AllocTexture(va("_%i_screenNormal", (int)contextHandle));
        screenNormalTexture->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::Format::RG_16F_16F, screenTextureFlags | Texture::Flag::Nearest);

        Texture *mrtTextures[2];
        mrtTextures[0] = screenColorTexture;
        mrtTextures[1] = screenNormalTexture;
        screenRT = RenderTarget::Create(2, (const Texture **)mrtTextures, screenDepthTexture, RHI::RenderTargetFlag::SRGBWrite);

        screenLitAccTexture = textureManager.AllocTexture(va("_%i_screenLitAcc", (int)contextHandle));
        screenLitAccTexture->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Flag::Nearest | Texture::Flag::SRGBColorSpace);
        screenLitAccRT = RenderTarget::Create(screenLitAccTexture, nullptr, RHI::RenderTargetFlag::SRGBWrite);
    } else if (r_usePostProcessing.GetBool() && r_SSAO.GetBool()) {
        screenNormalTexture = textureManager.AllocTexture(va("_%i_screenNormal", (int)contextHandle));
        screenNormalTexture->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::Format::RGBA_8_8_8_8, screenTextureFlags | Texture::Flag::Nearest);

        Texture *mrtTextures[2];
        mrtTextures[0] = screenColorTexture;
        mrtTextures[1] = screenNormalTexture;
        screenRT = RenderTarget::Create(2, (const Texture **)mrtTextures, screenDepthTexture, RHI::RenderTargetFlag::SRGBWrite);
    } else {
        screenRT = RenderTarget::Create(screenColorTexture, screenDepthTexture, RHI::RenderTargetFlag::SRGBWrite);
    }

    screenRT->Clear(Color4::black, 1.0f, 0);

    if (flags & Flag::UseSelectionBuffer) {
        screenSelectionTexture = textureManager.AllocTexture(va("_%i_screenSelection", (int)contextHandle));
        screenSelectionTexture->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth * screenSelectionScale, renderingHeight * screenSelectionScale, 1, 1, 1, Image::Format::RGBA_8_8_8_8, screenTextureFlags);
    
        screenSelectionRT = RenderTarget::Create(screenSelectionTexture, nullptr, RHI::RenderTargetFlag::HasDepthBuffer);
    }

    if (r_HOM.GetBool()) {
        int w = Math::RoundDownPowerOfTwo(renderingWidth >> 1);
        int h = Math::RoundDownPowerOfTwo(renderingHeight >> 1);

        homTexture = textureManager.AllocTexture(va("_%i_hom", (int)contextHandle));
        homTexture->CreateEmpty(RHI::TextureType::Texture2D, w, h, 1, 1, 1, Image::Format::Depth_32F, Texture::Flag::Clamp | Texture::Flag::HighQuality | Texture::Flag::HighPriority | Texture::Flag::Nearest);
        homTexture->Bind();
        homTexture->GenerateMipmap();
        homRT = RenderTarget::Create(nullptr, (const Texture *)homTexture, 0);
    }

    //--------------------------------------
    // Create RT for post processing
    //--------------------------------------

    int halfWidth = Math::Ceil(renderingWidth * 0.5f);
    int halfHeight = Math::Ceil(renderingHeight * 0.5f);
    int quarterWidth = Math::Ceil(renderingWidth * 0.25f);
    int quarterHeight = Math::Ceil(renderingHeight * 0.25f);

    ppTextures[PP_TEXTURE_COLOR_2X] = textureManager.AllocTexture(va("_%i_screenColorD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_2X]->CreateEmpty(RHI::TextureType::Texture2D, halfWidth, halfHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Flag::SRGBColorSpace);
    ppRTs[PP_RT_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_2X], nullptr, RHI::RenderTargetFlag::SRGBWrite);

    ppTextures[PP_TEXTURE_COLOR_4X] = textureManager.AllocTexture(va("_%i_screenColorD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_4X]->CreateEmpty(RHI::TextureType::Texture2D, quarterWidth, quarterHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Flag::SRGBColorSpace);
    ppRTs[PP_RT_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_4X], nullptr, RHI::RenderTargetFlag::SRGBWrite);

    ppTextures[PP_TEXTURE_COLOR_TEMP] = textureManager.AllocTexture(va("_%i_screenColorTemp", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP]->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Flag::SRGBColorSpace);
    ppRTs[PP_RT_TEMP] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP], nullptr, RHI::RenderTargetFlag::SRGBWrite);

    ppTextures[PP_TEXTURE_COLOR_TEMP_2X] = textureManager.AllocTexture(va("_%i_screenColorTempD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP_2X]->CreateEmpty(RHI::TextureType::Texture2D, halfWidth, halfHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Flag::SRGBColorSpace);
    ppRTs[PP_RT_TEMP_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_2X], nullptr, RHI::RenderTargetFlag::SRGBWrite);

    ppTextures[PP_TEXTURE_COLOR_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenColorTempD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_COLOR_TEMP_4X]->CreateEmpty(RHI::TextureType::Texture2D, quarterWidth, quarterHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Flag::SRGBColorSpace);
    ppRTs[PP_RT_TEMP_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_4X], nullptr, RHI::RenderTargetFlag::SRGBWrite);

    //ppTextures[PP_TEXTURE_COLOR_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenColorTempD4x", (int)contextHandle));
    //ppTextures[PP_TEXTURE_COLOR_TEMP_4X]->CreateEmpty(RHI::TextureType::Texture2D, quarterWidth, quarterHeight, 1, 1, 1, screenImageFormat, screenTextureFlags | Texture::Flag::SRGBColorSpace);
    //ppRTs[PP_RT_BLUR] = RenderTarget::Create(ppTextures[PP_TEXTURE_COLOR_TEMP_4X], nullptr, RHI::SRGBWrite);

    ppTextures[PP_TEXTURE_LINEAR_DEPTH] = textureManager.AllocTexture(va("_%i_screenLinearDepth", (int)contextHandle));
    ppTextures[PP_TEXTURE_LINEAR_DEPTH]->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::Format::L_16F, screenTextureFlags);
    ppRTs[PP_RT_LINEAR_DEPTH] = RenderTarget::Create(ppTextures[PP_TEXTURE_LINEAR_DEPTH], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_2X] = textureManager.AllocTexture(va("_%i_screenDepthD2x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_2X]->CreateEmpty(RHI::TextureType::Texture2D, halfWidth, halfHeight, 1, 1, 1, Image::Format::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_2X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_2X], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_4X] = textureManager.AllocTexture(va("_%i_screenDepthD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_4X]->CreateEmpty(RHI::TextureType::Texture2D, quarterWidth, quarterHeight, 1, 1, 1, Image::Format::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_DEPTH_TEMP_4X] = textureManager.AllocTexture(va("_%i_screenDepthTempD4x", (int)contextHandle));
    ppTextures[PP_TEXTURE_DEPTH_TEMP_4X]->CreateEmpty(RHI::TextureType::Texture2D, quarterWidth, quarterHeight, 1, 1, 1, Image::Format::L_16F, screenTextureFlags);
    ppRTs[PP_RT_DEPTH_TEMP_4X] = RenderTarget::Create(ppTextures[PP_TEXTURE_DEPTH_TEMP_4X], nullptr, 0);

    ppTextures[PP_TEXTURE_VEL] = textureManager.AllocTexture(va("_%i_screenVelocity", (int)contextHandle));
    ppTextures[PP_TEXTURE_VEL]->CreateEmpty(RHI::TextureType::Texture2D, halfWidth, halfHeight, 1, 1, 1, Image::Format::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_VEL] = RenderTarget::Create(ppTextures[PP_TEXTURE_VEL], nullptr, 0);

    ppTextures[PP_TEXTURE_AO] = textureManager.AllocTexture(va("_%i_screenAo", (int)contextHandle));
    ppTextures[PP_TEXTURE_AO]->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::Format::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_AO] = RenderTarget::Create(ppTextures[PP_TEXTURE_AO], nullptr, 0);

    ppTextures[PP_TEXTURE_AO_TEMP] = textureManager.AllocTexture(va("_%i_screenAoTemp", (int)contextHandle));
    ppTextures[PP_TEXTURE_AO_TEMP]->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, Image::Format::RGBA_8_8_8_8, screenTextureFlags);
    ppRTs[PP_RT_AO_TEMP] = RenderTarget::Create(ppTextures[PP_TEXTURE_AO_TEMP], nullptr, 0);

    //--------------------------------------
    // Create screen copy texture for refraction
    //--------------------------------------

    if (!currentRenderTexture) {
        currentRenderTexture = textureManager.AllocTexture(va("_%i_currentRender", (int)contextHandle));
    }

    currentRenderTexture->Purge();
    currentRenderTexture->CreateEmpty(RHI::TextureType::Texture2D, renderingWidth, renderingHeight, 1, 1, 1, screenImageFormat,
        Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::NonPowerOfTwo | Texture::Flag::HighPriority | Texture::Flag::SRGBColorSpace);
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

    int quarterWidth = Math::Ceil(renderingWidth * 0.25f);
    int quarterHeight = Math::Ceil(renderingHeight * 0.25f);

    //--------------------------------------
    // HDR RT 
    //--------------------------------------
   
    Image::Format::Enum screenImageFormat = GetScreenImageFormat();
    Image hdrBloomImage;
    hdrBloomImage.Create2D(quarterWidth, quarterHeight, 1, screenImageFormat, Image::GammaSpace::Linear, nullptr, 0);

    for (int i = 0; i < COUNT_OF(hdrBloomRT); i++) {
        hdrBloomTexture[i] = textureManager.AllocTexture(va("_%i_hdrBloom%i", (int)contextHandle, i));
        hdrBloomTexture[i]->Create(RHI::TextureType::Texture2D, hdrBloomImage,
            Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo | Texture::Flag::HighPriority);
        hdrBloomRT[i] = RenderTarget::Create(hdrBloomTexture[i], nullptr, 0);
    }

    Image::Format::Enum lumImageFormat = Image::Format::L_16F;
    if (r_HDR.GetInteger() == 3) {
        lumImageFormat = Image::Format::L_32F;
    }

    int size = Min(Math::RoundUpPowerOfTwo(Max(quarterWidth, quarterHeight)) >> 2, 1024);

    for (int i = 0; i < COUNT_OF(hdrLumAverageRT); i++) {
        hdrLumAverageTexture[i] = textureManager.AllocTexture(va("_%i_hdrLumAverage%i", (int)contextHandle, i));
        hdrLumAverageTexture[i]->CreateEmpty(RHI::TextureType::Texture2D, size, size, 1, 1, 1, lumImageFormat,
            Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo | Texture::Flag::HighPriority);
        hdrLumAverageRT[i] = RenderTarget::Create(hdrLumAverageTexture[i], nullptr, 0);
            
        if (size == 1) {
            break;
        }

        size = Max(size >> 2, 1);
    }

    for (int i = 0; i < COUNT_OF(hdrLuminanceTexture); i++) {
        hdrLuminanceTexture[i] = textureManager.AllocTexture(va("_%i_hdrLuminance%i", (int)contextHandle, i));
        hdrLuminanceTexture[i]->CreateEmpty(RHI::TextureType::Texture2D, 1, 1, 1, 1, 1, lumImageFormat,
            Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::HighPriority);
        hdrLuminanceRT[i] = RenderTarget::Create(hdrLuminanceTexture[i], nullptr, 0);
        //hdrLuminanceRT[i]->Clear(Color4(0.5, 0.5, 0.5, 1.0), 0.0f, 0.0f);
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

    Image::Format::Enum shadowImageFormat = Image::Format::Depth_24;
    Image::Format::Enum shadowCubeImageFormat = (r_shadowCubeMapFloat.GetBool() && rhi.SupportsDepthBufferFloat()) ? Image::Format::Depth_32F : Image::Format::Depth_24;

    RHI::TextureType::Enum textureType = RHI::TextureType::Texture2DArray;

    int csmCount = r_CSM_count.GetInteger();

    // Create cascaded shadow map.
    shadowRenderTexture = textureManager.AllocTexture(va("_%i_shadowRender", (int)contextHandle));
    shadowRenderTexture->CreateEmpty(textureType, r_shadowMapSize.GetInteger(), r_shadowMapSize.GetInteger(), 1, csmCount, 1, shadowImageFormat,
        Texture::Flag::Shadow | Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::HighPriority);
    shadowMapRT = RenderTarget::Create(nullptr, shadowRenderTexture, 0);

    // Create virtual shadow cube map.
    vscmTexture = textureManager.AllocTexture(va("_%i_vscmRender", (int)contextHandle));
    vscmTexture->CreateEmpty(RHI::TextureType::Texture2D, r_shadowCubeMapSize.GetInteger(), r_shadowCubeMapSize.GetInteger(), 1, 1, 1, shadowCubeImageFormat,
        Texture::Flag::Shadow | Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::HighPriority);
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

    RHI::DisplayMetrics displayMetrics;

    rhi.GetDisplayMetrics(this->contextHandle, &displayMetrics);

    this->windowWidth = displayMetrics.screenWidth;
    this->windowHeight = displayMetrics.screenHeight;
    this->deviceWidth = displayMetrics.backingWidth;
    this->deviceHeight = displayMetrics.backingHeight;
    this->safeAreaInsets = displayMetrics.safeAreaInsets;

    // deferred resizing render targets
    renderingWidth = this->deviceWidth / upscaleX;
    renderingHeight = this->deviceHeight / upscaleY;

    guiMesh.SetClipRect(Rect(0, 0, renderingWidth, renderingHeight));

    //rhi.ChangeDisplaySettings(deviceWidth, deviceHeight, rhi.IsFullScreen());
}

void RenderContext::Display() {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderContext::Display");

    rhi.DisplayContext(contextHandle);
}

void RenderContext::BeginFrame() {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderContext::BeginFrame");

    startFrameMsec = PlatformTime::Milliseconds();

    frameMsec = startFrameMsec - lastFrameMsec;

    lastFrameMsec = startFrameMsec;

    memset(&renderCounters[frameCount & 1], 0, sizeof(renderCounters[0]));

    renderSystem.UpdateEnvProbes();

    renderSystem.BeginCommands(this);

    // Window size have changed since last call of BeginFrame()
    if (renderingWidth != screenRT->GetWidth() || renderingHeight != screenRT->GetHeight()) {
        FreeScreenMapRT();
        FreeHdrMapRT();

        InitScreenMapRT();
        InitHdrMapRT();
    }
}

void RenderContext::EndFrame() {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderContext::EndFrame");

    // Adds system GUI commands
    renderSystem.primaryWorld->DrawGUICamera(guiMesh);

    // Adds swap buffer command
    SwapBuffersRenderCommand *cmd = (SwapBuffersRenderCommand *)renderSystem.GetCommandBuffer(sizeof(SwapBuffersRenderCommand));
    cmd->commandId = RenderCommand::SwapBuffers;

    renderSystem.EndCommands();

    guiMesh.Clear();

    RenderCounter *currentRenderCounter = &renderCounters[frameCount & 1];

    currentRenderCounter->frameMsec = PlatformTime::Milliseconds() - startFrameMsec;

    frameCount++;
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
    guiMesh.DrawPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, color.a < 1.0f ? materialManager.blendMaterial : materialManager.unlitMaterial);
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
    
    //double starttime = PlatformTime::Seconds();

    currentRenderTexture->Bind();
    rhi.CopyTextureSubImage2D(0, 0, 0, 0, currentRenderTexture->GetWidth(), currentRenderTexture->GetHeight());
    //currentRenderTexture->GenerateMipmap();
    
    //BE_LOG("%lf\n", PlatformTime::Seconds() - starttime);
}

float RenderContext::QueryDepth(const Point &point) {
    Image::Format::Enum format = screenSelectionRT->ColorTexture()->GetFormat();
    byte *depthData = (byte *)_alloca(4);

    float scaleX = (float)screenSelectionRT->GetWidth() / screenRT->GetWidth();
    float scaleY = (float)screenSelectionRT->GetHeight() / screenRT->GetHeight();

    Vec2 scaledReadPoint;
    scaledReadPoint.x = point.x * scaleX;
    scaledReadPoint.y = (screenRT->GetHeight() - (point.y + 1)) * scaleY;

    screenSelectionRT->Begin();

    // FIXME: is depth format confirmed ?
    rhi.ReadPixels(scaledReadPoint.x, scaledReadPoint.y, 1, 1, Image::Format::Depth_24, depthData);
    screenSelectionRT->End();

    float depth = (float)MAKE_FOURCC(depthData[2], depthData[1], depthData[0], 0) / (float)(BIT(24) - 1);
    return depth;
}

bool RenderContext::QuerySelection(const Point &point, uint32_t &index) {
    Image::Format::Enum format = screenSelectionRT->ColorTexture()->GetFormat();
    byte *data = (byte *)_alloca(Image::BytesPerPixel(format));

    float scaleX = (float)screenSelectionRT->GetWidth() / screenRT->GetWidth();
    float scaleY = (float)screenSelectionRT->GetHeight() / screenRT->GetHeight();

    Vec2 scaledReadPoint;
    scaledReadPoint.x = point.x * scaleX;
    scaledReadPoint.y = (screenRT->GetHeight() - (point.y + 1)) * scaleY;

    screenSelectionRT->Begin();
    rhi.ReadPixels(scaledReadPoint.x, scaledReadPoint.y, 1, 1, format, data);
    screenSelectionRT->End();

    uint32_t id = ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | ((uint32_t)data[0]);
    if (id != 0x00FFFFFF) {
        index = id;
        return true;
    }

    return false;
}

bool RenderContext::QuerySelection(const Rect &rect, Inclusion::Enum inclusion, Array<uint32_t> &indexes) {
    if (rect.IsEmpty()) {
        return false;
    }

    if (rect.w == 1 && rect.h == 1) {
        uint32_t index;

        if (QuerySelection(rect.GetPoint(0), index)) {
            indexes.Append(index);
            return true;
        }
        return false;
    }

    float scaleX = (float)screenSelectionRT->GetWidth() / screenRT->GetWidth();
    float scaleY = (float)screenSelectionRT->GetHeight() / screenRT->GetHeight();

    Rect scaledReadRect;
    scaledReadRect.x = rect.x * scaleX;
    scaledReadRect.y = (screenRT->GetHeight() - (rect.y + rect.h)) * scaleY;
    scaledReadRect.w = Max(rect.w * scaleX, 1.0f);
    scaledReadRect.h = Max(rect.h * scaleY, 1.0f);

    Image::Format::Enum format = screenSelectionRT->ColorTexture()->GetFormat();
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
            uint32_t id = (((uint32_t)data_ptr[2]) << 16) | (((uint32_t)data_ptr[1]) << 8) | ((uint32_t)data_ptr[0]);
            if (id != 0x00FFFFFF) {
                indexes.AddUnique(id);
            }

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

                uint32_t id = (((uint32_t)data_ptr[2]) << 16) | (((uint32_t)data_ptr[1]) << 8) | ((uint32_t)data_ptr[0]);
                if (id != 0x00FFFFFF) {
                    indexes.AddUnique(id);
                }
            }
        }
        
        for (int y = 0; y < screenSelectionRT->GetHeight(); y++) {
            int pitch = y * screenSelectionRT->GetWidth() * bpp;

            for (int x = 0; x < screenSelectionRT->GetWidth(); x++) {
                if (!scaledReadRect.IsContainPoint(x, y)) {
                    byte *data_ptr = &data[pitch + x * bpp];

                    uint32_t id = (((uint32_t)data_ptr[2]) << 16) | (((uint32_t)data_ptr[1]) << 8) | ((uint32_t)data_ptr[0]);
                    if (id != 0x00FFFFFF) {
                        indexes.RemoveFast(id);
                    }
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

void RenderContext::TakeScreenShot(const char *filename, RenderWorld *renderWorld, int layerMask, const Vec3 &origin, const Mat3 &axis, float fov, int width, int height) {
    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.png", filename);

    RenderCamera renderCamera;
    RenderCamera::State cameraDef;
    cameraDef.flags = RenderCamera::Flag::TexturedMode | RenderCamera::Flag::NoSubViews | RenderCamera::Flag::SkipDebugDraw | RenderCamera::Flag::ConstantToneMapping; // FIXME
    cameraDef.clearMethod = RenderCamera::ClearMethod::Skybox;
    cameraDef.clearColor = Color4(0.29f, 0.33f, 0.35f, 0);
    cameraDef.layerMask = layerMask;
    cameraDef.renderRect.Set(0, 0, width, height);
    cameraDef.origin = origin;
    cameraDef.axis = axis;
    cameraDef.orthogonal = false;

    AABB worldAABB = renderWorld->GetStaticAABB();
    if (!worldAABB.IsCleared()) {
        Vec3 v;
        worldAABB.GetFarthestVertexFromDir(axis[0], v);
        cameraDef.zFar = Max(MeterToUnit(100.0f), origin.Distance(v));
    } else {
        cameraDef.zFar = MeterToUnit(100.0f);
    }
    cameraDef.zNear = CentiToUnit(5.0f);

    RenderCamera::ComputeFov(fov, 1.25f, (float)width / height, &cameraDef.fovX, &cameraDef.fovY);

    renderCamera.Update(&cameraDef);

    BeginFrame();

    renderWorld->RenderScene(&renderCamera);

    renderSystem.CmdScreenshot(0, 0, width, height, path);

    EndFrame();

    BE_DLOG("Screenshot saved to \"%s\"\n", path);
}

BE_NAMESPACE_END
