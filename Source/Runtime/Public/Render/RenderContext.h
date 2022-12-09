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
    uint32_t                frontEndMsec;
    uint32_t                backEndMsec;
    uint32_t                frameMsec;
    uint32_t                homGenMsec;
    uint32_t                homQueryMsec;
    uint32_t                homCullMsec;

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
    struct Flag {
        enum Enum {
            OnDemandDrawing     = BIT(0),
            UseSelectionBuffer  = BIT(1),
            UseSharedContext    = BIT(4),
        };
    };

    /// Inclusion type for query selection
    struct Inclusion {
        enum Enum {
            Crossing,           ///< Select all objects within the region, plus any objects crossing the boundaries of the region
            Window              ///< Select only the objects within the selection
        };
    };

    RenderContext();

    void                    Init(RHI::WindowHandle hwnd, int renderingWidth, int renderingHeight, RHI::DisplayContextFunc displayFunc, void *displayFuncDataPtr, int flags = 0);
    void                    Shutdown();

    RHI::Handle             GetContextHandle() const { return contextHandle; }
    
                            // Returns logical screen resolution.
    int                     GetScreenWidth() const { return windowWidth; }
    int                     GetScreenHeight() const { return windowHeight; }

                            // Returns device resolution.
    int                     GetDeviceWidth() const { return deviceWidth; }
    int                     GetDeviceHeight() const { return deviceHeight; }

                            // Returns actual screen render target resolution.
    int                     GetRenderingWidth() const { return renderingWidth; }
    int                     GetRenderingHeight() const { return renderingHeight; }

                            // Returns upscale factor for render to device mapping.
    float                   GetUpscaleFactorX() const { return (float)deviceWidth / renderingWidth; }
    float                   GetUpscaleFactorY() const { return (float)deviceHeight / renderingHeight; }

    Rect                    GetSafeAreaInsets() const { return safeAreaInsets; }

    void                    OnResize(int width, int height);

    const RenderCounter &   GetPrevFrameRenderCounter() const { return renderCounters[(frameCount + 1) & 1]; }
    RenderCounter &         GetRenderCounter() { return renderCounters[frameCount & 1]; }

    void                    Display();

    void                    BeginFrame();
    void                    EndFrame();

    Color4                  GetColor() const;
    Color4                  GetClearColor() const;
    float                   GetClearDepth() const;

    void                    SetColor(const Color4 &color);
    void                    SetClearColor(const Color4 &clearColor);
    void                    SetClearDepth(float clearDepth);
                            
                            // Set clipping rect for DrawPic/DrawStretchPic
    void                    SetClipRect(const Rect &clipRect);

    GuiMesh &               GetGuiMesh() { return guiMesh; }

    void                    AdjustFrom640x480(float *x, float *y, float *w, float *h) const;

    void                    DrawPic(float x, float y, float w, float h, const Material *material);
    void                    DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material);
    void                    DrawBar(float x, float y, float w, float h);
    void                    DrawRect(float x, float y, float w, float h);

    void                    UpdateCurrentRenderTexture() const;

                            // Query in render coordinates
    float                   QueryDepth(const Point &pt);
    bool                    QuerySelection(const Point &pt, uint32_t &index);
    bool                    QuerySelection(const Rect &rect, Inclusion::Enum inclusion, Array<uint32_t> &indexes);

    void                    TakeScreenShot(const char *filename, RenderWorld *renderWorld, int layerMask, const Vec3 &origin, const Mat3 &axis, float fov, int width, int height);

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

    int                     frameCount;
    uint32_t                frameMsec;
    uint32_t                lastFrameMsec;

    Color4                  color;
    Color4                  clearColor;
    float                   clearDepth;

    GuiMesh                 guiMesh;

    int                     prevLumTarget;
    int                     currLumTarget;

    RenderCounter           renderCounters[2];
    uint32_t                startFrameMsec;

    Texture *               screenColorTexture = nullptr;
    Texture *               screenDepthTexture = nullptr;
    Texture *               screenNormalTexture = nullptr;
    Texture *               screenLitAccTexture = nullptr;
    Texture *               screenSelectionTexture = nullptr;

    Texture *               homTexture = nullptr;
    Texture *               ppTextures[MAX_PP_TEXTURES] = { nullptr, };

    Texture *               hdrBloomTexture[2] = { nullptr, };
    Texture *               hdrLumAverageTexture[6] = { nullptr, };
    Texture *               hdrLuminanceTexture[3] = { nullptr, };

    Texture *               currentRenderTexture = nullptr;
    bool                    updateCurrentRenderTexture = false;

    RenderTarget *          screenRT = nullptr;
    RenderTarget *          screenLitAccRT = nullptr;
    RenderTarget *          screenSelectionRT = nullptr;
    RenderTarget *          homRT = nullptr;
    RenderTarget *          ppRTs[MAX_PP_RTS] = { nullptr, };

    RenderTarget *          hdrBloomRT[2] = { nullptr, };
    RenderTarget *          hdrLumAverageRT[6] = { nullptr, };
    RenderTarget *          hdrLuminanceRT[3] = { nullptr, };

    Texture *               indirectionCubeMapTexture = nullptr;
    float                   vscmBiasedScale;
    float                   vscmBiasedFov;

    bool                    vscmCleared[6] = { false, };

    Texture *               shadowRenderTexture = nullptr;
    Texture *               vscmTexture = nullptr;

    RenderTarget *          shadowMapRT = nullptr;
    RenderTarget *          vscmRT = nullptr;           // VSCM (Virtual Shadow Cube Map)
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
