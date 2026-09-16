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

#include "RHI/RHI.h"
#include "Render/RenderFrameSettings.h"

class RenderWorld;
class Texture;

static constexpr int RenderBloomDownMaxLevelCount = 8;
static constexpr int RenderBloomUpMaxLevelCount = RenderBloomDownMaxLevelCount - 1;
static constexpr int RenderConvolutionBloomMaxMipLevelCount = 16;
static constexpr int RenderHiZMaxLevelCount = 16;
static constexpr int RenderScreenSpaceDepthPyramidMaxLevelCount = 16;
static constexpr int RenderHiZCullMaxInstanceCount = 2048;
static constexpr int RenderHiZCullMaxVisibleInstanceCount = 0x40000;
static constexpr int RenderHiZCullMaxBatchCount = RenderHiZCullMaxVisibleInstanceCount;

class RenderCamera {
    friend class RenderWorld;

public:
    enum class Flag : uint8_t {
        None                    = 0,
        WireframeMode           = BIT(0),               ///< Surface 를 와이어프레임으로 렌더링한다.
        SkipDebugDraw           = BIT(1),               ///< Debug primitive 렌더링을 생략한다.
        SkipPostProcess         = BIT(2),               ///< Post-process 렌더링을 생략한다.
        DisableTemporalHistory  = BIT(3),               ///< Temporal history 사용과 갱신을 비활성화한다.
        DisableHiZCulling       = BIT(4),               ///< Hi-Z 기반 occlusion culling 을 비활성화한다.
        ForceSyncPassPSOBuild   = BIT(5),               ///< Render pass PSO 를 강제로 동기적으로 빌드한다.
        FlipOutputTextureY      = BIT(6),               ///< output texture 에 최종 렌더링할 때 Y축을 뒤집는다.
        StaticOnly              = BIT(7)                ///< Static render object 만 렌더링한다.
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

    enum class ClearMethod : uint8_t {
        NoClear,
        DepthOnly,
        Color,
        Skybox
    };

    enum class ClearColorMode : uint8_t {
        RenderTargetDefault,
        Explicit
    };

    struct Desc {
        int                     time = 0;               ///< Time in milliseconds for shader effects and other time dependent rendering issues
        Flag                    flags = Flag::None;
        uint32_t                layerMask = ~0u;        ///< Bit mask of visible RenderObject and RenderLight layers

        BE1::Rect               outputRect;             ///< Output rectangle in outputTexture, or in the swap chain when outputTexture is null.
        RHI::Texture *          outputTexture = nullptr; ///< Optional final output render target. The caller must keep it alive until rendering completes.
        RHI::ViewID             outputViewID = RHI::ViewID::PrimaryID;
        RHI::GPUResourceState   outputBeforeState = RHI::GPUResourceState::ShaderResource;
        RHI::GPUResourceState   outputAfterState = RHI::GPUResourceState::ShaderResource;
        uint32_t                renderWidth = 0;        ///< Local render target width. Falls back to outputRect.w if zero.
        uint32_t                renderHeight = 0;       ///< Local render target height. Falls back to outputRect.h if zero.
        BE1::Vec3               origin;                 ///< View origin
        BE1::Mat3               axis;                   ///< View axis [FORWARD, LEFT, UP]

        ClearMethod             clearMethod = ClearMethod::Color; ///< Clearing method before rendering view
        ClearColorMode          clearColorMode = ClearColorMode::RenderTargetDefault; ///< Source of color clear value
        BE1::Color4             clearColor = BE1::Color4::black; ///< Clearing color is needed if clearing method is set to 'ColorClear'

        float                   fovX;                   ///< Perspective projection FOV in horizontal axis
        float                   fovY;                   ///< Perspective projection FOV in vertical axis
        float                   sizeX;                  ///< Orthogonal projection size in horizontal axis
        float                   sizeY;                  ///< Orthogonal projection size in vertical axis
        float                   zNear;                  ///< Near distance in z axis
        float                   zFar;                   ///< Finite visibility/culling far distance; ignored by infinite-far perspective projection
        RHI::ProjectionPolicy   projectionPolicy;       ///< Graphics API projection convention used for shader-visible matrices
        bool                    infiniteZFar = false;   ///< Use an infinite far plane for perspective projection
        bool                    orthogonal = false;     ///< True for orthogonal projection or false for perspective projection
    };

    RenderCamera() = default;
    ~RenderCamera();

    const Desc &                GetDesc() const { return desc; }

                                /// Returns orthogonal projection oriented box bounds.
    const BE1::OBB              GetBox() const { return box; }

                                /// Returns perspective projection frustum bounds.
    const BE1::Frustum &        GetFrustum() const { return frustum; }

                                /// Returns perspective projection frustum planes.
    const BE1::Plane &          GetFrustumPlane(int index) const { return frustumPlanes[index]; }

                                /// Calculates the requested bounding sphere and/or planes of the camera view between the given distances.
    void                        CalcSlicedViewVolume(float nearDistance, float farDistance, BE1::Sphere *boundingSphere, BE1::Plane *planes = nullptr) const;

    const BE1::Mat4 &           GetViewMatrix() const { return viewMatrix; }
    const BE1::Mat4 &           GetProjMatrix() const { return projMatrix; }
    const BE1::Mat4 &           GetViewProjMatrix() const { return viewProjMatrix; }

                                /// Returns true if the bounds are outside the camera view volume.
    bool                        Cull(const BE1::AABB &bounds) const;
    bool                        Cull(const BE1::OBB &bounds) const;
    bool                        Cull(const BE1::Sphere &bounds) const;
    bool                        Cull(const BE1::Frustum &bounds) const;

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

                                /// Calculates a screen rectangle clipped to the camera output rectangle from bounding sphere (Different camera axis with Eric Lengyel's method)
    bool                        CalcClipRectFromSphere(const BE1::Sphere &sphere, BE1::Rect &clipRect) const;

                                /// Calculates a screen rectangle clipped to the camera output rectangle from axis-aligned bounding box.
    bool                        CalcClipRectFromAABB(const BE1::AABB &aabb, BE1::Rect &clipRect) const;
    
                                /// Calculates a screen rectangle clipped to the camera output rectangle from oriented bounding box.
    bool                        CalcClipRectFromOBB(const BE1::OBB &obb, BE1::Rect &clipRect) const;
    
                                /// Calculates a screen rectangle clipped to the camera output rectangle from bounding frustum.
    bool                        CalcClipRectFromFrustum(const BE1::Frustum &frustum, BE1::Rect &clipRect) const;

    bool                        CalcDepthBoundsFromPoints(int numPoints, const BE1::Vec3 *points, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                        CalcDepthBoundsFromSphere(const BE1::Sphere &sphere, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                        CalcDepthBoundsFromAABB(const BE1::AABB &bounds, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                        CalcDepthBoundsFromOBB(const BE1::OBB &box, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                        CalcDepthBoundsFromFrustum(const BE1::Frustum &frustum, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;

                                /// 픽셀 단위 LOD 허용 오차 (pixelErrorLimit) 를 모델 로컬 공간의 허용 오차 제곱으로 변환한다.
                                /// Update() 와 EnsureRenderTextures() 이후 내부 렌더링 해상도를 기준으로 평가한다.
                                /// pixelErrorLimit 가 0 이하이거나 유효한 투영 평가가 불가능하면 0 을 반환한다.
    float                       CalcLocalLODErrorLimitSqr(const BE1::Mat3x4 &worldMatrix, const BE1::AABB &shadowMapBounds, float pixelErrorLimit) const;

    void                        Update(const Desc &desc);

                                /// Calculates the effective normalized extents and inverse average luminance of a convolution bloom kernel image.
    static void                 CalcConvolutionBloomKernelParams(const BE1::Image &kernelImage, BE1::Vec2 &outNormalizedKernelExtents, float &inverseAverageLuminance);

    bool                        SetConvolutionBloomKernelFromImage(const char *textureName, const BE1::Image &kernelImage);
    bool                        SetConvolutionBloomKernelFromFile(const char *filename);
    void                        SetConvolutionBloomKernelTexture(const SharedPtr<Texture> &texture, const BE1::Vec2 &normalizedKernelExtents, float inverseAverageLuminance);
    void                        SetBloomDirtMaskTexture(const SharedPtr<Texture> &texture) { bloomDirtMaskTexture = texture; }
    void                        SetConvolutionBloomKernelRotation(float rotation);
    void                        SetConvolutionBloomKernelScale(float scale);
    void                        SetConvolutionBloomPaddingTrim(float paddingTrim);

    bool                        EnsureRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, const BE1::Color4 &optimizedClearColor);
    bool                        EnsureSceneSnapshotResources(bool needed, bool halfResolution);
    void                        EnsureScreenSpaceDepthPyramidResources(uint32_t width, uint32_t height, bool needed);
    void                        EnsureSSRTraceResources(uint32_t width, uint32_t height, uint32_t sampleCount, bool needed);
    void                        EnsureSSAOResources(uint32_t width, uint32_t height, bool needResolvedDepth, const RenderFrameSettings &settings);
    void                        EnsurePostProcessResources(bool skipPostProcess, RHI::Format colorFormat, bool needColorHistory, bool needDepthHistory, const RenderFrameSettings &renderFrameSettings);

    void                        InvalidateTemporalHistory();

    uint32_t                    GetRenderWidth() const { return renderWidth; }
    uint32_t                    GetRenderHeight() const { return renderHeight; }
    RHI::Texture *              GetRTColorTexture() const { return rtColorTexture; }
    RHI::Texture *              GetRTColorMSAATexture() const { return rtColorMSAATexture; }
    RHI::Texture *              GetDepthTexture() const { return depthHistoryAsRenderTarget ? depthHistoryTextures[currentHistoryTextureIndex] : depthTexture; }
    RHI::Texture *              GetRTNormalTexture() const { return rtNormalTexture; }
    RHI::Texture *              GetRTNormalMSAATexture() const { return rtNormalMSAATexture; }
    RHI::Texture *              GetRTVelocityTexture() const { return rtVelocityTexture; }
    RHI::Texture *              GetRTVelocityMSAATexture() const { return rtVelocityMSAATexture; }
    RHI::Texture *              GetSnapshotColorTexture() const { return snapshotColorTexture; }
    RHI::Texture *              GetSnapshotDepthTexture() const { return snapshotDepthTexture; }
    RHI::Texture *              GetColorHistoryTexture(uint32_t index) const { return colorHistoryTextures[index % 2]; }
    RHI::Texture *              GetCurrentColorHistoryTexture() const { return colorHistoryTextures[currentHistoryTextureIndex]; }
    RHI::Texture *              GetPreviousColorHistoryTexture() const { return colorHistoryTextures[currentHistoryTextureIndex ^ 1]; }
    RHI::Texture *              GetDepthHistoryTexture(uint32_t index) const { return depthHistoryTextures[index % 2]; }
    RHI::Texture *              GetCurrentDepthHistoryTexture() const { return depthHistoryTextures[currentHistoryTextureIndex]; }
    RHI::Texture *              GetPreviousDepthHistoryTexture() const { return depthHistoryTextures[currentHistoryTextureIndex ^ 1]; }
    uint32_t                    GetCurrentHistoryTextureIndex() const { return currentHistoryTextureIndex; }
    bool                        IsHistoryValid() const { return historyValid && !temporalHistoryInvalidated; }

    RHI::Texture *              GetHiZTexture() const { return hiZTexture; }
    const RHI::ViewID *         GetHiZMipSRVs() const { return hiZMipSRVs; }
    const RHI::ViewID *         GetHiZMipUAVs() const { return hiZMipUAVs; }
    const uint32_t *            GetHiZMipWidths() const { return hiZMipWidths; }
    const uint32_t *            GetHiZMipHeights() const { return hiZMipHeights; }
    uint32_t                    GetHiZMipCount() const { return hiZMipCount; }
    RHI::Texture *              GetHiZDebugPreviewTexture() const { return hiZDebugPreviewTexture; }
    bool                        IsPreviousHiZValid() const { return previousHiZValid; }
    RHI::Buffer *               GetHiZCullVisibleInstanceBuffer() const { return hiZCullVisibleInstanceBuffer; }
    RHI::Buffer *               GetHiZCullIndirectArgsBuffer() const { return hiZCullIndirectArgsBuffer; }

    RHI::Texture *              GetScreenSpaceDepthPyramidTexture() const { return screenSpaceDepthPyramidTexture; }
    const RHI::ViewID *         GetScreenSpaceDepthPyramidMipSRVs() const { return screenSpaceDepthPyramidMipSRVs; }
    const RHI::ViewID *         GetScreenSpaceDepthPyramidMipUAVs() const { return screenSpaceDepthPyramidMipUAVs; }
    const uint32_t *            GetScreenSpaceDepthPyramidMipWidths() const { return screenSpaceDepthPyramidMipWidths; }
    const uint32_t *            GetScreenSpaceDepthPyramidMipHeights() const { return screenSpaceDepthPyramidMipHeights; }
    uint32_t                    GetScreenSpaceDepthPyramidMipCount() const { return screenSpaceDepthPyramidMipCount; }
    uint32_t                    GetScreenSpaceDepthPyramidValidWidth() const { return screenSpaceDepthPyramidValidWidth; }
    uint32_t                    GetScreenSpaceDepthPyramidValidHeight() const { return screenSpaceDepthPyramidValidHeight; }
    RHI::Texture *              GetSSRResolvedDepthTexture() const { return ssrResolvedDepthTexture; }
    RHI::Texture *              GetSSRTraceTexture() const { return ssrTraceTexture; }
    RHI::Texture *              GetSSAOTexture() const { return ssaoTexture; }
    RHI::Texture *              GetSSAORawTexture() const { return ssaoRawTexture; }
    RHI::Texture *              GetSSAOFilterTexture() const { return ssaoFilterTexture; }
    RHI::Texture *              GetSSAOHalfDepthTexture() const { return ssaoHalfDepthTexture; }
    RHI::Texture *              GetSSAOResolvedDepthTexture() const { return ssaoResolvedDepthTexture; }

    RHI::Texture *              GetPostProcessColorTexture() const { return postProcessColorTexture; }
    RHI::Buffer *               GetLuminanceBuffer() const { return luminanceBuffer; }

    RHI::Texture *              GetBloomDownTexture() const { return bloomDownTexture; }
    RHI::Texture *              GetBloomUpTexture() const { return bloomUpTexture; }
    const RHI::ViewID *         GetBloomDownMipSRVs() const { return bloomDownMipSRVs; }
    const RHI::ViewID *         GetBloomDownMipUAVs() const { return bloomDownMipUAVs; }
    const RHI::ViewID *         GetBloomUpMipSRVs() const { return bloomUpMipSRVs; }
    const RHI::ViewID *         GetBloomUpMipUAVs() const { return bloomUpMipUAVs; }
    const uint32_t *            GetBloomMipWidths() const { return bloomMipWidths; }
    const uint32_t *            GetBloomMipHeights() const { return bloomMipHeights; }
    uint32_t                    GetBloomMipCount() const { return bloomMipCount; }
    const SharedPtr<Texture> &  GetBloomDirtMaskTexture() const { return bloomDirtMaskTexture; }

    uint32_t                    GetFFTBloomSize() const { return fftBloomSize; }
    RHI::Buffer *               GetFFTBloomScreenSpatialBuffer() const { return fftBloomScreenSpatialBuffer; }
    RHI::Buffer *               GetFFTBloomScreenSpectrumBuffer() const { return fftBloomScreenSpectrumBuffer; }
    const SharedPtr<Texture> &  GetFFTBloomKernelTexture() const { return fftBloomKernelSourceTexture; }
    float                       GetFFTBloomKernelInverseAverageLuminance() const { return fftBloomKernelInverseAverageLuminance; }
    const BE1::Vec2 &           GetFFTBloomKernelNormalizedExtents() const { return fftBloomKernelNormalizedExtents; }
    RHI::Buffer *               GetFFTBloomKernelSpatialBuffer() const { return fftBloomKernelSpatialBuffer; }
    RHI::Buffer *               GetFFTBloomKernelSpectrumBuffer() const { return fftBloomKernelSpectrumBuffer; }
    RHI::Buffer *               GetFFTBloomWorkBuffer() const { return fftBloomWorkBuffer; }
    RHI::Texture *              GetFFTBloomConvolutionOutputTexture() const { return fftBloomConvolutionOutputTexture; }
    uint32_t                    GetFFTBloomConvolutionOutputWidth() const { return fftBloomConvolutionOutputWidth; }
    uint32_t                    GetFFTBloomConvolutionOutputHeight() const { return fftBloomConvolutionOutputHeight; }
    const RHI::ViewID *         GetFFTBloomConvolutionOutputMipSRVs() const { return fftBloomConvolutionOutputMipSRVs; }
    const RHI::ViewID *         GetFFTBloomConvolutionOutputMipUAVs() const { return fftBloomConvolutionOutputMipUAVs; }
    uint32_t                    GetFFTBloomConvolutionOutputMipCount() const { return fftBloomConvolutionOutputMipCount; }
    bool                        IsFFTBloomKernelDirty() const { return fftBloomKernelDirty; }
    float                       GetFFTBloomKernelRotation() const { return fftBloomKernelRotation; }
    float                       GetFFTBloomKernelScale() const { return fftBloomKernelScale; }
    float                       GetFFTBloomPaddingTrim() const { return fftBloomPaddingTrim; }
    const BE1::RectF &          GetFFTBloomMappedContentRect() const { return fftBloomMappedContentRect; }
    const BE1::Vec2 &           GetFFTBloomMappedKernelScale() const { return fftBloomMappedKernelScale; }

    static void                 CalculateFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY);

private:
    void                        DestroyPostProcessResources();
    bool                        CreateRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, const BE1::Color4 &optimizedClearColor);
    void                        DestroyRenderTextures();
    bool                        CreateRTColorTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, uint32_t sampleCount, const BE1::Color4 &optimizedClearColor);
    bool                        CreateDepthTexture(uint32_t width, uint32_t height, RHI::Format depthFormat, uint32_t sampleCount);
    bool                        CreateSceneSnapshotResources(bool halfResolution);
    void                        DestroySceneSnapshotResources();
    bool                        CreateScreenSpaceDepthPyramidTexture(uint32_t width, uint32_t height);
    void                        DestroyScreenSpaceDepthPyramidTexture();
    bool                        CreateSSRTraceTexture(uint32_t width, uint32_t height);
    void                        DestroySSRTraceTexture();
    bool                        CreateSSRResolvedDepthTexture(uint32_t width, uint32_t height);
    void                        DestroySSRResolvedDepthTexture();
    void                        DestroySSAOResources();
    void                        DestroySSAOFilterResources();
    void                        EnsureHiZResources(uint32_t width, uint32_t height, bool needHiZTexture, bool needHiZCullResources, bool needHiZDebugPreview);
    bool                        CreateHiZTexture(uint32_t width, uint32_t height);
    void                        DestroyHiZTexture();
    void                        DestroyHiZResources();
    bool                        EnsureHiZDebugPreviewTexture(uint32_t width, uint32_t height);
    bool                        CreateHiZDebugPreviewTexture(uint32_t width, uint32_t height);
    void                        DestroyHiZDebugPreviewTexture();
    bool                        CreateHiZCullResources();
    void                        DestroyHiZCullResources();
    bool                        CreateRTNormalTextures(uint32_t width, uint32_t height, uint32_t sampleCount);
    bool                        CreateRTVelocityTextures(uint32_t width, uint32_t height, uint32_t sampleCount);
    bool                        CreatePostProcessColorTexture(uint32_t width, uint32_t height, RHI::Format colorFormat);
    void                        DestroyPostProcessColorTexture();
    bool                        EnsureColorHistoryTextures(uint32_t width, uint32_t height, RHI::Format colorFormat);
    bool                        CreateColorHistoryTextures(uint32_t width, uint32_t height, RHI::Format colorFormat);
    void                        DestroyColorHistoryTextures();
    bool                        EnsureDepthHistoryTextures(uint32_t width, uint32_t height);
    bool                        CreateDepthHistoryTextures(uint32_t width, uint32_t height);
    bool                        CreateDepthHistoryRenderTargets(uint32_t width, uint32_t height, RHI::Format depthFormat);
    void                        DestroyDepthHistoryTextures();
    bool                        CreatePyramidBloomTextures(uint32_t width, uint32_t height, uint32_t requestedBloomMipMaxLevelCount);
    void                        DestroyPyramidBloomTextures();
    bool                        CreateConvolutionBloomResources(uint32_t size);
    void                        DestroyConvolutionBloomResources();
    bool                        CreateConvolutionBloomOutputTexture(uint32_t width, uint32_t height);
    void                        DestroyConvolutionBloomOutputTexture();
    BE1::Vec2                   CalcNormalizedKernelExtents() const;
    void                        UpdateConvolutionBloomMapping();
    void                        MarkConvolutionBloomKernelClean() { fftBloomKernelDirty = false; }
    void                        CreateLuminanceBuffer();
    void                        DestroyLuminanceBuffer();
    void                        UpdateTemporalJitter(bool enableJitter, uint32_t sampleCount);

    Desc                        desc;
    float                       tanHalfFovX;
    float                       tanHalfFovY;
    BE1::Frustum                frustum;
    BE1::Plane                  frustumPlanes[6];
    BE1::Vec3                   frustumPoints[8];
    BE1::OBB                    box; // for orthogoanl projection

    ALIGN_AS32 BE1::Mat4        viewMatrix;
    ALIGN_AS32 BE1::Mat4        projMatrix;

    ALIGN_AS32 BE1::Mat4        viewProjMatrix;
    ALIGN_AS32 BE1::Mat4        invViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        jitteredViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        jitteredInvViewProjMatrix;

    ALIGN_AS32 BE1::Mat4        previousViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        previousInvViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        previousJitteredViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        previousJitteredInvViewProjMatrix;

    ALIGN_AS32 BE1::Mat4        hiZCullViewProjMatrix;

    BE1::Vec2                   currentJitter = BE1::Vec2::zero;
    BE1::Vec2                   previousJitter = BE1::Vec2::zero;
    uint32_t                    temporalFrameIndex = 0;
    uint32_t                    temporalJitterSampleCount = 0;
    bool                        temporalJitterActive = false;
    bool                        previousMatrixValid = false;
    bool                        temporalHistoryInvalidated = false;

    uint32_t                    renderWidth = 0;
    uint32_t                    renderHeight = 0;
    RHI::Format                 renderColorFormat = RHI::Format::Unknown;
    RHI::Format                 renderDepthFormat = RHI::Format::Unknown;
    uint32_t                    renderSampleCount = 1;

    RHI::Texture *              rtColorTexture = nullptr;
    RHI::Texture *              rtColorMSAATexture = nullptr;
    RHI::Texture *              depthTexture = nullptr;
    RHI::Texture *              rtNormalTexture = nullptr;
    RHI::Texture *              rtNormalMSAATexture = nullptr;
    RHI::Texture *              rtVelocityTexture = nullptr;
    RHI::Texture *              rtVelocityMSAATexture = nullptr;
    RHI::Texture *              snapshotColorTexture = nullptr;
    RHI::Texture *              snapshotDepthTexture = nullptr;
    RHI::Texture *              postProcessColorTexture = nullptr;

    RHI::Texture *              screenSpaceDepthPyramidTexture = nullptr;
    RHI::ViewID                 screenSpaceDepthPyramidMipSRVs[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    RHI::ViewID                 screenSpaceDepthPyramidMipUAVs[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                    screenSpaceDepthPyramidMipWidths[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                    screenSpaceDepthPyramidMipHeights[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                    screenSpaceDepthPyramidMipCount = 0;
    uint32_t                    screenSpaceDepthPyramidValidWidth = 0;
    uint32_t                    screenSpaceDepthPyramidValidHeight = 0;
    RHI::Texture *              ssrResolvedDepthTexture = nullptr;
    RHI::Texture *              ssrTraceTexture = nullptr;
    RHI::Texture *              ssaoTexture = nullptr;
    RHI::Texture *              ssaoRawTexture = nullptr;
    RHI::Texture *              ssaoFilterTexture = nullptr;
    RHI::Texture *              ssaoHalfDepthTexture = nullptr;
    RHI::Texture *              ssaoResolvedDepthTexture = nullptr;

    RHI::Texture *              colorHistoryTextures[2] = {};
    RHI::Texture *              depthHistoryTextures[2] = {};
    bool                        depthHistoryAsRenderTarget = false;
    uint32_t                    currentHistoryTextureIndex = 0;
    bool                        historyValid = false;

    RHI::Texture *              hiZTexture = nullptr;
    RHI::ViewID                 hiZMipSRVs[RenderHiZMaxLevelCount] = {};
    RHI::ViewID                 hiZMipUAVs[RenderHiZMaxLevelCount] = {};
    uint32_t                    hiZMipWidths[RenderHiZMaxLevelCount] = {};
    uint32_t                    hiZMipHeights[RenderHiZMaxLevelCount] = {};
    uint32_t                    hiZMipCount = 0;
    RHI::Texture *              hiZDebugPreviewTexture = nullptr;
    RHI::Buffer *               hiZCullVisibleInstanceBuffer = nullptr;
    RHI::Buffer *               hiZCullIndirectArgsBuffer = nullptr;
    bool                        previousHiZValid = false;

    RHI::Texture *              bloomDownTexture = nullptr;
    RHI::Texture *              bloomUpTexture = nullptr;
    uint32_t                    bloomMipWidths[RenderBloomDownMaxLevelCount] = {};
    uint32_t                    bloomMipHeights[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipSRVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipUAVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipSRVs[RenderBloomUpMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipUAVs[RenderBloomUpMaxLevelCount] = {};
    uint32_t                    bloomMipCount = 0;
    uint32_t                    bloomMipMaxLevelCount = 0;
    SharedPtr<Texture>          bloomDirtMaskTexture;

    uint32_t                    fftBloomSize = 0;
    SharedPtr<Texture>          fftBloomKernelSourceTexture;
    float                       fftBloomKernelInverseAverageLuminance = 1.0f;
    BE1::Vec2                   fftBloomKernelNormalizedExtents = BE1::Vec2(1.0f, 1.0f);
    RHI::Buffer *               fftBloomKernelSpatialBuffer = nullptr;
    RHI::Buffer *               fftBloomKernelSpectrumBuffer = nullptr;
    BE1::RectF                  fftBloomMappedContentRect = BE1::RectF(0.0f, 0.0f, 0.0f, 0.0f); // fftBloomSize x fftBloomSize 공간에서 convolution padding 을 확보한 뒤 content 가 배치되는 픽셀 단위 영역
    BE1::Vec2                   fftBloomMappedKernelScale = BE1::Vec2(1.0f, 1.0f); // content padding 에 따른 축소율과 fftBloomKernelScale 을 반영해 FFT 공간의 kernel 에 적용할 최종 X/Y scale
    float                       fftBloomKernelRotation = 0.0f;
    float                       fftBloomKernelScale = 1.0f;
    float                       fftBloomPaddingTrim = 0.0f;
    bool                        fftBloomKernelDirty = true;
    RHI::Buffer *               fftBloomScreenSpatialBuffer = nullptr;
    RHI::Buffer *               fftBloomScreenSpectrumBuffer = nullptr;
    RHI::Buffer *               fftBloomWorkBuffer = nullptr;
    RHI::Texture *              fftBloomConvolutionOutputTexture = nullptr;
    uint32_t                    fftBloomConvolutionOutputWidth = 0;
    uint32_t                    fftBloomConvolutionOutputHeight = 0;
    RHI::ViewID                 fftBloomConvolutionOutputMipSRVs[RenderConvolutionBloomMaxMipLevelCount] = {};
    RHI::ViewID                 fftBloomConvolutionOutputMipUAVs[RenderConvolutionBloomMaxMipLevelCount] = {};
    uint32_t                    fftBloomConvolutionOutputMipCount = 0;

    RHI::Buffer *               luminanceBuffer = nullptr;
};
