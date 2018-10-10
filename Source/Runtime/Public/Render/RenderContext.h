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

#pragma once

#include "GuiMesh.h"

BE_NAMESPACE_BEGIN

enum ppRT_t {
    PP_RT_2X,
    PP_RT_4X,
    PP_RT_TEMP,
    PP_RT_TEMP_2X,
    PP_RT_TEMP_4X,
    PP_RT_BLUR,
    PP_RT_LINEAR_DEPTH,
    PP_RT_DEPTH_2X,
    PP_RT_DEPTH_4X,
    PP_RT_DEPTH_TEMP_4X,
    PP_RT_VEL,
    PP_RT_AO,
    PP_RT_AO_TEMP,
    MAX_PP_RTS
};

enum ppTexture_t {
    PP_TEXTURE_COLOR_2X,
    PP_TEXTURE_COLOR_4X,
    PP_TEXTURE_COLOR_TEMP,
    PP_TEXTURE_COLOR_TEMP_2X,
    PP_TEXTURE_COLOR_TEMP_4X,
    PP_TEXTURE_LINEAR_DEPTH,
    PP_TEXTURE_DEPTH_2X,
    PP_TEXTURE_DEPTH_4X,
    PP_TEXTURE_DEPTH_TEMP_4X,
    PP_TEXTURE_VEL,
    PP_TEXTURE_AO,
    PP_TEXTURE_AO_TEMP,
    MAX_PP_TEXTURES
};

struct RenderCounter {
    int                     frontEndMsec;
    int                     backEndMsec;
    int                     frameMsec;
    int                     homGenMsec;
    int                     homQueryMsec;
    int                     homCullMsec;

    unsigned int            drawCalls;
    unsigned int            drawVerts;
    unsigned int            drawIndexes;

    unsigned int            shadowDrawCalls;
    unsigned int            shadowDrawVerts;
    unsigned int            shadowDrawIndexes;

    unsigned int            numShadowMapDraw;
    unsigned int            numSkinningEntities;
};

class Image;
class Texture;
class RenderTarget;
class Material;
class RenderWorld;

class RenderContext {
public:
    friend class RenderSystem;

    /// Flags for initialization
    enum Flag {
        OnDemandDrawing     = BIT(0),
        UseSelectionBuffer  = BIT(1),
        InstantToneMapping  = BIT(2),
        UseSharedContext    = BIT(3),
    };

    /// Inclusion type for query selection
    enum Inclusion {
        Crossing,           ///< Select all objects within the region, plus any objects crossing the boundaries of the region
        Window              ///< Select only the objects within the selection
    };

    RenderContext();

    void                    Init(RHI::WindowHandle hwnd, int renderingWidth, int renderingHeight, RHI::DisplayContextFunc displayFunc, void *displayFuncDataPtr, int flags = 0);
    void                    Shutdown();

    RHI::Handle             GetContextHandle() const { return contextHandle; }
    
                            // logical screen resolution
    int                     GetScreenWidth() const { return windowWidth; }
    int                     GetScreenHeight() const { return windowHeight; }

                            // device resolution
    int                     GetDeviceWidth() const { return deviceWidth; }
    int                     GetDeviceHeight() const { return deviceHeight; }

                            // actual screen render target resolution
    int                     GetRenderingWidth() const { return renderingWidth; }
    int                     GetRenderingHeight() const { return renderingHeight; }

                            // upscale factor for render to device mapping
    float                   GetUpscaleFactorX() const { return (float)deviceWidth / renderingWidth; }
    float                   GetUpscaleFactorY() const { return (float)deviceHeight / renderingHeight; }

    Rect                    GetSafeAreaInsets() const { return safeAreaInsets; }

    void                    OnResize(int width, int height);

    void                    Display();

    void                    BeginFrame();
    void                    EndFrame();

    Color4                  GetColor() const;
    Color4                  GetClearColor() const;
    float                   GetClearDepth() const;

    void                    SetColor(const Color4 &color);
    void                    SetClearColor(const Color4 &clearColor);
    void                    SetClearDepth(float clearDepth);
                            
                            // set clipping rect for DrawPic/DrawStretchPic
    void                    SetClipRect(const Rect &clipRect);

    GuiMesh &               GetGuiMesh() { return guiMesh; }

    void                    AdjustFrom640x480(float *x, float *y, float *w, float *h) const;

    void                    DrawPic(float x, float y, float w, float h, const Material *material);
    void                    DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material);
    void                    DrawBar(float x, float y, float w, float h);
    void                    DrawRect(float x, float y, float w, float h);

    void                    UpdateCurrentRenderTexture() const;

                            // query in render coordinates
    float                   QueryDepth(const Point &pt);
    int                     QuerySelection(const Point &pt);
    bool                    QuerySelection(const Rect &rect, Inclusion inclusion, Array<int> &indexes);

    void                    TakeScreenShot(const char *filename, RenderWorld *renderWorld, int layerMask, const Vec3 &origin, const Mat3 &axis, float fov, int width, int height);
    void                    TakeEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, const Vec3 &origin, int size = 256);
    void                    TakeIrradianceEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, const Vec3 &origin);
    void                    TakePrefilteredEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, const Vec3 &origin);

    void                    WriteBRDFIntegrationLUT(const char *filename, int size) const;

    void                    CaptureEnvCubeImage(RenderWorld *renderWorld, int layerMask, const Vec3 &origin, int size, Image &envCubeImage);

                            // Generate irradiance environment cubemap using SH convolution method
    void                    GenerateIrradianceEnvCubeImageSHConvolv(const Image &envCubeImage, int size, Image &irradianceEnvCubeImage) const;

                            // Generate irradiance environment cubemap
    void                    GenerateIrradianceEnvCubeImage(const Image &envCubeImage, int size, Image &irradianceEnvCubeImage) const;

                            // Generate Phong specular prefiltered environment cubemap
    void                    GeneratePhongSpecularPrefilteredEnvCubeImage(const Image &envCubeImage, int size, int maxSpecularPower, Image &prefilteredCubeImage) const;

                            // Generate GGX specular prefiltered environment cubemap
    void                    GenerateGGXPrefilteredEnvCubeImage(const Image &envCubeImage, int size, Image &prefilteredCubeImage) const;

                            // Generate GGX BRDF integration 2D LUT
    void                    GenerateGGXIntegrationLUTImage(int size, Image &integrationImage) const;

//private:
    void                    InitScreenMapRT();
    void                    FreeScreenMapRT();
    void                    InitHdrMapRT();
    void                    FreeHdrMapRT();
    void                    InitShadowMapRT();
    void                    FreeShadowMapRT();

    RHI::Handle             contextHandle;

    int                     flags;
    int                     windowWidth;
    int                     windowHeight;
    int                     deviceWidth;
    int                     deviceHeight;
    int                     renderingWidth;
    int                     renderingHeight;
    float                   screenSelectionScale;
    Rect                    safeAreaInsets;         // left, top, right, bottom

    Random                  random;

    float                   elapsedTime;
    float                   frameTime;
    int                     frameCount;

    Color4                  color;
    Color4                  clearColor;
    float                   clearDepth;

    GuiMesh                 guiMesh;

    int                     prevLumTarget;
    int                     currLumTarget;

    RenderCounter           renderCounter;
    int                     startFrameMsec;

    Texture *               screenColorTexture;
    Texture *               screenDepthTexture;
    Texture *               screenNormalTexture;
    Texture *               screenLitAccTexture;
    Texture *               screenSelectionTexture;

    Texture *               homTexture;
    Texture *               ppTextures[MAX_PP_TEXTURES];

    Texture *               hdrBloomTexture[2];
    Texture *               hdrLumAverageTexture[5];
    Texture *               hdrLuminanceTexture[3];

    Texture *               currentRenderTexture;
    bool                    updateCurrentRenderTexture;

    RenderTarget *          screenRT;
    RenderTarget *          screenLitAccRT;
    RenderTarget *          screenSelectionRT;
    RenderTarget *          homRT;
    RenderTarget *          ppRTs[MAX_PP_RTS];

    RenderTarget *          hdrBloomRT[2];
    RenderTarget *          hdrLumAverageRT[5];
    RenderTarget *          hdrLuminanceRT[3];

    Texture *               indirectionCubeMapTexture;
    float                   vscmBiasedScale;
    float                   vscmBiasedFov;

    bool                    vscmCleared[6];

    Texture *               shadowRenderTexture;
    Texture *               vscmTexture;

    RenderTarget *          shadowMapRT;
    RenderTarget *          vscmRT;         // VSCM (Virtual Shadow Cube Map)
};

BE_INLINE Color4 RenderContext::GetColor() const {
    return color;
}

BE_INLINE Color4 RenderContext::GetClearColor() const {
    return clearColor;
}

BE_INLINE float RenderContext::GetClearDepth() const {
    return clearDepth;
}

BE_INLINE void RenderContext::SetColor(const Color4 &color) {
    this->color = color;
}

BE_INLINE void RenderContext::SetClearColor(const Color4 &clearColor) {
    this->clearColor = clearColor;
}

BE_INLINE void RenderContext::SetClearDepth(float clearDepth) {
    this->clearDepth = clearDepth;
}

BE_NAMESPACE_END
