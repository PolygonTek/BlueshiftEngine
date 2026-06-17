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

#pragma once

#include "RHI.h"

class RenderWorld;

static constexpr int RenderBloomMaxLevelCount = 8;

enum class RenderCameraFlag : uint8_t {
    NoFlag                      = 0,
    WireframeMode               = BIT(0),
    SkipPostProcess             = BIT(1)
};

enum class RenderCameraClearMethod : uint8_t {
    NoClear,
    DepthOnly,
    Color,
    Skybox
};

enum class RenderCameraClearColorMode : uint8_t {
    RenderTargetDefault,
    Explicit
};

struct RenderCameraDesc {
    int                         time = 0;               ///< Time in milliseconds for shader effects and other time dependent rendering issues
    RenderCameraFlag            flags = RenderCameraFlag::NoFlag;

    BE1::Rect                   outputRect;             ///< Output rectangle in the swapchain.
    uint32_t                    renderWidth = 0;        ///< Local render target width. Falls back to outputRect.w if zero.
    uint32_t                    renderHeight = 0;       ///< Local render target height. Falls back to outputRect.h if zero.
    BE1::Vec3                   origin;                 ///< View origin
    BE1::Mat3                   axis;                   ///< View axis [FORWARD, LEFT, UP]

    RenderCameraClearMethod     clearMethod = RenderCameraClearMethod::Color; ///< Clearing method before rendering view
    RenderCameraClearColorMode  clearColorMode = RenderCameraClearColorMode::RenderTargetDefault; ///< Source of color clear value
    BE1::Color4                 clearColor = BE1::Color4::black; ///< Clearing color is needed if clearing method is set to 'ColorClear'

    float                       fovX;                   ///< Perspective projection FOV in horizontal axis
    float                       fovY;                   ///< Perspective projection FOV in vertical axis
    float                       sizeX;                  ///< Orthogonal projection size in horizontal axis
    float                       sizeY;                  ///< Orthogonal projection size in vertical axis
    float                       zNear;                  ///< Near distance in z axis
    float                       zFar;                   ///< Far distance in z axis
    bool                        orthogonal = false;     ///< True for orthogonal projection or false for perspective projection
};

class RenderCamera {
    friend class RenderWorld;

public:
    ~RenderCamera() {
        DestroyPostProcessResources();
        DestroyRenderTextures();
    }

    const RenderCameraDesc &    GetDesc() const { return desc; }

    const BE1::OBB              GetBox() const { return box; }

    const BE1::Frustum &        GetFrustum() const { return frustum; }
    const BE1::Plane &          GetFrustumPlane(int index) const { return frustumPlanes[index]; }

    const BE1::Mat4 &           GetViewMatrix() const { return viewMatrix; }
    const BE1::Mat4 &           GetProjMatrix() const { return projMatrix; }
    const BE1::Mat4 &           GetViewProjMatrix() const { return viewProjMatrix; }

                                /// Transforms world coordinates to normalized device coordinates.
    bool                        TransformWorldToNDC(const BE1::Vec3 &worldCoords, BE1::Vec3 &normalizedDeviceCoords) const;

                                /// Transform normalized device coordinates to pixel coordinates.
    void                        TransformNDCToPixel(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &pixelCoords) const;

                                /// Transforms world coordinates to pixel coordinates.
    bool                        TransformWorldToPixel(const BE1::Vec3 &worldCoords, BE1::Vec3 &pixelCoords) const;

                                /// Un-transform pixel coordinates to normalized device coordinates.
    void                        UntransformPixelToNDC(const BE1::Vec3 &pixelCoords, BE1::Vec3 &normalizedDeviceCoords) const;

                                /// Un-transform normalized device coordinates to world coordinates.
    bool                        UntransformNDCToWorld(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &worldCoords) const;

                                /// Un-transforms pixel coordinates to world coordinates.
    bool                        UntransformPixelToWorld(const BE1::Vec3 &pixelCoords, BE1::Vec3 &worldCoords) const;

                                /// Calculates clipping rectangle from bounding sphere (Different camera axis with Eric Lengyel's method)
    bool                        CalcClipRectFromSphere(const BE1::Sphere &sphere, BE1::Rect &clipRect) const;

                                /// Calculates clipping rectangle from axis-aligned bounding box.
    bool                        CalcClipRectFromAABB(const BE1::AABB &aabb, BE1::Rect &clipRect) const;
    
                                /// Calculates clipping rectangle from oriented bounding box.
    bool                        CalcClipRectFromOBB(const BE1::OBB &obb, BE1::Rect &clipRect) const;
    
                                /// Calculates clipping rectangle from bounding frustum.
    bool                        CalcClipRectFromFrustum(const BE1::Frustum &frustum, BE1::Rect &clipRect) const;

    bool                        CalcDepthBoundsFromPoints(int numPoints, const BE1::Vec3 *points, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                        CalcDepthBoundsFromSphere(const BE1::Sphere &sphere, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                        CalcDepthBoundsFromAABB(const BE1::AABB &bounds, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                        CalcDepthBoundsFromOBB(const BE1::OBB &box, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                        CalcDepthBoundsFromFrustum(const BE1::Frustum &frustum, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;

    void                        Update(const RenderCameraDesc &desc);

    void                        EnsureRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, const BE1::Color4 &optimizedClearColor);
    void                        EnsurePostProcessResources();

    uint32_t                    GetRenderWidth() const { return renderWidth; }
    uint32_t                    GetRenderHeight() const { return renderHeight; }
    RHI::Texture *              GetColorTexture() const { return colorTexture; }
    RHI::Texture *              GetColorMSAATexture() const { return colorMSAATexture; }
    RHI::Texture *              GetDepthTexture() const { return depthTexture; }
    RHI::Texture *              GetPostProcessColorTexture() const { return postProcessColorTexture; }
    RHI::Texture *              GetBloomDownTexture() const { return bloomDownTexture; }
    RHI::Texture *              GetBloomUpTexture() const { return bloomUpTexture; }
    const RHI::ViewID *         GetBloomDownMipSRVs() const { return bloomDownMipSRVs; }
    const RHI::ViewID *         GetBloomDownMipUAVs() const { return bloomDownMipUAVs; }
    const RHI::ViewID *         GetBloomUpMipSRVs() const { return bloomUpMipSRVs; }
    const RHI::ViewID *         GetBloomUpMipUAVs() const { return bloomUpMipUAVs; }
    const uint32_t *            GetBloomMipWidths() const { return bloomMipWidths; }
    const uint32_t *            GetBloomMipHeights() const { return bloomMipHeights; }
    uint32_t                    GetBloomMipCount() const { return bloomMipCount; }
    RHI::Buffer *               GetLuminanceBuffer() const { return luminanceBuffer; }

    static void                 CalculateFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY);

private:
    void                        DestroyPostProcessResources();
    bool                        CreateRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, const BE1::Color4 &optimizedClearColor);
    void                        DestroyRenderTextures();
    bool                        CreatePostProcessColorTexture(uint32_t width, uint32_t height);
    void                        DestroyPostProcessColorTexture();
    bool                        CreatePyramidBloomTextures(uint32_t width, uint32_t height, uint32_t requestedBloomMipMaxLevelCount);
    void                        DestroyPyramidBloomTextures();
    void                        CreateLuminanceBuffer();
    void                        DestroyLuminanceBuffer();

    RenderCameraDesc            desc;
    BE1::Frustum                frustum;
    BE1::Plane                  frustumPlanes[6];
    BE1::Vec3                   frustumPoints[8];
    BE1::OBB                    box;
    ALIGN_AS32 BE1::Mat4        viewMatrix;
    ALIGN_AS32 BE1::Mat4        projMatrix;
    ALIGN_AS32 BE1::Mat4        viewProjMatrix;

    uint32_t                    renderWidth = 0;
    uint32_t                    renderHeight = 0;
    RHI::Format                 renderColorFormat = RHI::Format::Unknown;
    RHI::Format                 renderDepthFormat = RHI::Format::Unknown;
    uint32_t                    renderSampleCount = 1;

    RHI::Texture *              colorTexture = nullptr;
    RHI::Texture *              colorMSAATexture = nullptr;
    RHI::Texture *              depthTexture = nullptr;
    RHI::Texture *              postProcessColorTexture = nullptr;

    RHI::Texture *              bloomDownTexture = nullptr;
    RHI::Texture *              bloomUpTexture = nullptr;
    RHI::ViewID                 bloomDownMipSRVs[RenderBloomMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipUAVs[RenderBloomMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipSRVs[RenderBloomMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipUAVs[RenderBloomMaxLevelCount] = {};
    uint32_t                    bloomMipWidths[RenderBloomMaxLevelCount] = {};
    uint32_t                    bloomMipHeights[RenderBloomMaxLevelCount] = {};
    uint32_t                    bloomMipCount = 0;
    uint32_t                    bloomMipMaxLevelCount = 0;

    RHI::Buffer *               luminanceBuffer = nullptr;
};

template<>
struct enable_bitmask_operators<RenderCameraFlag> {
    static const bool enable = true;
};
