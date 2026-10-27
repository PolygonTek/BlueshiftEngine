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
#include "Render/ShadowAtlas.h"
#include "Render/ShadowResolutionHistory.h"
#include "Render/ScreenSpaceAOResources.h"
#include "Render/SSRHiZResources.h"
#include "Render/SSRSceneColorHistoryResources.h"
#include "Render/HiZResources.h"
#include "Render/BloomResources.h"

class RenderWorld;
class Texture;
struct SSRHistoryRenderState;

class RenderCamera {
    friend class RenderWorld;

public:
    enum class Flag : uint16_t {
        None                    = 0,
        WireframeMode           = BIT(0),               ///< Surface 를 와이어프레임으로 렌더링한다.
        SkipDebugDraw           = BIT(1),               ///< Debug primitive 렌더링을 생략한다.
        SkipPostProcess         = BIT(2),               ///< Post-process 렌더링을 생략한다.
        DisableTemporalHistory  = BIT(3),               ///< Temporal history 사용과 갱신을 비활성화한다.
        DisableHiZCulling       = BIT(4),               ///< Hi-Z 기반 occlusion culling 을 비활성화한다.
        ForceSyncPassPSOBuild   = BIT(5),               ///< Render pass PSO 를 강제로 동기적으로 빌드한다.
        FlipOutputTextureY      = BIT(6),               ///< output texture 에 최종 렌더링할 때 Y축을 뒤집는다.
        StaticOnly              = BIT(7),               ///< Static render object 만 렌더링한다.
        SkipSkyboxLighting      = BIT(8)                ///< 환경광 베이크를 위해 sky 표시용 색, 밝기와 회전을 제외한 원본 radiance 를 렌더링한다.
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

                                /// Returns the six outward-facing view volume planes for either projection type.
    const BE1::Plane *          GetViewPlanes() const { return viewPlanes; }

                                /// Calculates the requested minimum bounding sphere and/or planes of the camera view between the given distances.
    void                        CalcSlicedViewVolume(float nearDistance, float farDistance, BE1::Sphere *outBoundingSphere, BE1::Plane *outViewPlanes = nullptr) const;

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

    BloomResources &            GetBloomResources() { return bloomResources; }
    const BloomResources &      GetBloomResources() const { return bloomResources; }

    bool                        EnsureRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, bool needNormalRoughnessTexture, bool needVelocityTexture, const BE1::Color4 &optimizedClearColor);
    bool                        EnsureShadowAtlas(const RenderFrameSettings &renderFrameSettings, bool hasShadowCandidates);
    bool                        EnsureResolvedDepthTexture(uint32_t width, uint32_t height, uint32_t sampleCount, bool needed);
    bool                        EnsureSceneSnapshotResources(bool needed, bool halfResolution);
    bool                        EnsureSSRResources(uint32_t width, uint32_t height, uint32_t resolutionMode, bool needed);
    bool                        EnsureTransparentSSRResources(uint32_t width, uint32_t height, RHI::Format depthFormat, uint32_t resolutionMode, bool needed);
    void                        EnsurePostProcessResources(bool skipPostProcess, RHI::Format colorFormat, bool needColorHistory, bool needDepthHistory, const RenderFrameSettings &renderFrameSettings);

    void                        InvalidateTemporalHistory();

    uint32_t                    GetRenderWidth() const { return renderWidth; }
    uint32_t                    GetRenderHeight() const { return renderHeight; }

    RHI::Texture *              GetColorTexture() const { return colorTexture; }
    RHI::Texture *              GetDepthTexture() const { return depthHistoryAsRenderTarget ? depthHistoryTextures[currentHistoryTextureIndex] : depthTexture; }
    RHI::Texture *              GetNormalRoughnessTexture() const { return normalRoughnessTexture; }
    RHI::Texture *              GetVelocityTexture() const { return velocityTexture; }
    RHI::Texture *              GetTAAReactiveMaskTexture() const { return taaReactiveMaskTexture; }

    RHI::Texture *              GetResolvedColorTexture() const { return resolvedColorTexture; }
    RHI::Texture *              GetResolvedDepthTexture() const { return resolvedDepthTexture; }
    RHI::Texture *              GetResolvedNormalRoughnessTexture() const { return resolvedNormalRoughnessTexture; }
    RHI::Texture *              GetResolvedVelocityTexture() const { return resolvedVelocityTexture; }
    RHI::Texture *              GetResolvedTAAReactiveMaskTexture() const { return resolvedTAAReactiveMaskTexture; }

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

    HiZResources &              GetHiZResources() { return hiZResources; }
    const HiZResources &        GetHiZResources() const { return hiZResources; }

    ShadowAtlas &               GetShadowAtlas() { return shadowAtlas; }
    const ShadowAtlas &         GetShadowAtlas() const { return shadowAtlas; }

    SSRHiZResources &           GetSSRHiZResources() { return ssrHiZResources; }
    const SSRHiZResources &     GetSSRHiZResources() const { return ssrHiZResources; }

    SSRSceneColorHistoryResources &GetSSRSceneColorHistoryResources() { return ssrSceneColorHistoryResources; }
    const SSRSceneColorHistoryResources &GetSSRSceneColorHistoryResources() const { return ssrSceneColorHistoryResources; }

    RHI::Texture *              GetSSRTraceTexture() const { return ssrTraceTexture; }
    RHI::Texture *              GetSSRReceiverSurfaceTexture() const { return ssrReceiverSurfaceTexture; }
    RHI::Texture *              GetSSRResolveReflectionTexture() const { return ssrResolveReflectionTexture; }
    RHI::Texture *              GetCurrentSSRTemporalReflectionTexture() const { return ssrTemporalReflectionTextures[ssrTemporalReflectionTextureIndex]; }
    RHI::Texture *              GetPreviousSSRTemporalReflectionTexture() const { return ssrTemporalReflectionTextures[ssrTemporalReflectionTextureIndex ^ 1u]; }
    RHI::Texture *              GetLastSSRTemporalReflectionTexture() const { return ssrTemporalHistoryValid ? ssrTemporalReflectionTextures[ssrTemporalReflectionTextureIndex ^ 1u] : nullptr; }
    bool                        IsSSRTemporalHistoryValid() const { return ssrTemporalHistoryValid && !temporalHistoryInvalidated; }
    void                        CommitSSRTemporalHistory(bool valid);
    RHI::Texture *              GetSSRUpsampledReflectionTexture() const { return ssrUpsampledReflectionTexture; }
    RHI::Texture *              GetCurrentTransparentSSRDepthTexture() const { return transparentSSRDepthTextures[transparentSSRHistoryTextureIndex]; }
    RHI::Texture *              GetPreviousTransparentSSRDepthTexture() const { return transparentSSRDepthTextures[transparentSSRHistoryTextureIndex ^ 1u]; }
    RHI::Texture *              GetTransparentSSRNormalRoughnessTexture() const { return transparentSSRNormalRoughnessTexture; }
    RHI::Texture *              GetTransparentSSRVelocityTexture() const { return transparentSSRVelocityTexture; }
    RHI::Texture *              GetCurrentTransparentSSRTemporalReflectionTexture() const { return transparentSSRTemporalReflectionTextures[transparentSSRHistoryTextureIndex]; }
    RHI::Texture *              GetPreviousTransparentSSRTemporalReflectionTexture() const { return transparentSSRTemporalReflectionTextures[transparentSSRHistoryTextureIndex ^ 1u]; }
    bool                        IsTransparentSSRTemporalHistoryValid() const { return transparentSSRTemporalHistoryValid && !temporalHistoryInvalidated; }
    void                        CommitTransparentSSRTemporalHistory(bool valid);
    RHI::Texture *              GetTransparentSSRUpsampledReflectionTexture() const { return transparentSSRUpsampledReflectionTexture; }
    RHI::Texture *              GetAOTexture() const { return screenSpaceAOResources.GetTextureSet().outputTexture; }

    RHI::Texture *              GetPostProcessColorTexture() const { return postProcessColorTexture; }
    RHI::Buffer *               GetLuminanceBuffer() const { return luminanceBuffer; }

    static void                 CalculateFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY);

private:
    bool                        CreateRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, bool needNormalRoughnessTexture, bool needVelocityTexture, const BE1::Color4 &optimizedClearColor);
    void                        DestroyRenderTextures();
    bool                        CreateColorTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, uint32_t sampleCount, const BE1::Color4 &optimizedClearColor);
    bool                        CreateDepthTexture(uint32_t width, uint32_t height, RHI::Format depthFormat, uint32_t sampleCount);
    bool                        CreateNormalRoughnessTextures(uint32_t width, uint32_t height, uint32_t sampleCount);
    bool                        CreateVelocityTextures(uint32_t width, uint32_t height, uint32_t sampleCount, bool needTAAReactiveMask);
    bool                        CreateSceneSnapshotResources(bool halfResolution);
    void                        DestroySceneSnapshotResources();

    bool                        CreateResolvedDepthTexture(uint32_t width, uint32_t height);
    void                        DestroyResolvedDepthTexture();

    bool                        CreatePostProcessColorTexture(uint32_t width, uint32_t height, RHI::Format colorFormat);
    void                        DestroyPostProcessColorTexture();
    void                        DestroyPostProcessResources();

    bool                        CreateSSRTraceTexture(uint32_t width, uint32_t height);
    bool                        CreateSSRReceiverSurfaceTexture(uint32_t width, uint32_t height);
    bool                        CreateSSRResolveReflectionTexture(uint32_t width, uint32_t height);
    bool                        CreateSSRTemporalReflectionTextures(uint32_t width, uint32_t height);
    bool                        CreateSSRUpsampledReflectionTexture(uint32_t width, uint32_t height);
    void                        DestroySSRResources();
    bool                        CreateTransparentSSRResources(uint32_t width, uint32_t height, RHI::Format depthFormat, uint32_t resolutionMode);
    void                        DestroyTransparentSSRResources();

    bool                        EnsureColorHistoryTextures(uint32_t width, uint32_t height, RHI::Format colorFormat);
    bool                        CreateColorHistoryTextures(uint32_t width, uint32_t height, RHI::Format colorFormat);
    void                        DestroyColorHistoryTextures();
    bool                        EnsureDepthHistoryResolveTargets(uint32_t width, uint32_t height);
    bool                        CreateDepthHistoryResolveTargets(uint32_t width, uint32_t height);
    bool                        CreateDepthHistoryRenderTargets(uint32_t width, uint32_t height, RHI::Format depthFormat);
    void                        DestroyDepthHistoryTextures();

    void                        CreateLuminanceBuffer();
    void                        DestroyLuminanceBuffer();
    void                        UpdateTemporalJitter(bool enableJitter, uint32_t sampleCount);

    Desc                        desc;
    float                       tanHalfFovX;
    float                       tanHalfFovY;
    BE1::Frustum                frustum;
    BE1::Plane                  viewPlanes[6];
    BE1::OBB                    box; // for orthogoanl projection

    ALIGN_AS32 BE1::Mat4        viewMatrix;
    ALIGN_AS32 BE1::Mat4        projMatrix;

    ALIGN_AS32 BE1::Mat4        viewProjMatrix;
    ALIGN_AS32 BE1::Mat4        invViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        jitteredViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        jitteredInvViewProjMatrix;

    ALIGN_AS32 BE1::Mat4        prevViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        prevInvViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        prevJitteredViewProjMatrix;
    ALIGN_AS32 BE1::Mat4        prevJitteredInvViewProjMatrix;

    ALIGN_AS32 BE1::Mat4        hiZCullViewProjMatrix;

    BE1::Vec2                   currJitter = BE1::Vec2::zero;
    BE1::Vec2                   prevJitter = BE1::Vec2::zero;
    uint32_t                    temporalFrameIndex = 0;
    uint32_t                    temporalJitterSampleCount = 0;
    bool                        temporalJitterActive = false;
    bool                        prevMatrixValid = false;
    bool                        temporalHistoryInvalidated = false;

    uint32_t                    renderWidth = 0;
    uint32_t                    renderHeight = 0;
    RHI::Format                 renderColorFormat = RHI::Format::Unknown;
    RHI::Format                 renderDepthFormat = RHI::Format::Unknown;
    uint32_t                    renderSampleCount = 1;

    RHI::Texture *              colorTexture = nullptr;
    RHI::Texture *              depthTexture = nullptr; // MSAA off, TAA on 상태에서는 depthTexture 대신 depthHistoryTextures 를 사용한다.
    RHI::Texture *              normalRoughnessTexture = nullptr; // AO/SSR 에서 필요할 때 생성. RG: UNORM octahedral view normal, B: perceptual roughness.
    RHI::Texture *              velocityTexture = nullptr; // TAA 또는 SSR reprojection 에서 필요할 때 생성한다.
    RHI::Texture *              taaReactiveMaskTexture = nullptr; // TAA 사용 시 굴절 등 표면 velocity 로 추적할 수 없는 색의 기여도.

    RHI::Texture *              resolvedColorTexture = nullptr; // MSAA on 상태에서 생성, off 상태에서는 nullptr
    RHI::Texture *              resolvedDepthTexture = nullptr; // MSAA on 상태에서 depth prepass 이후 GTAO/SSR 에서 필요
    RHI::Texture *              resolvedNormalRoughnessTexture = nullptr; // MSAA on 이고 normalRoughnessTexture 가 필요할 때 생성, 그 외에는 nullptr
    RHI::Texture *              resolvedVelocityTexture = nullptr; // MSAA on 이고 velocityTexture 가 필요할 때 생성, 그 외에는 nullptr
    RHI::Texture *              resolvedTAAReactiveMaskTexture = nullptr; // MSAA on 이고 TAA 사용 시 생성한다.

    RHI::Texture *              snapshotColorTexture = nullptr;
    RHI::Texture *              snapshotDepthTexture = nullptr;
    RHI::Texture *              postProcessColorTexture = nullptr;

    RHI::Texture *              ssrTraceTexture = nullptr;
    RHI::Texture *              ssrReceiverSurfaceTexture = nullptr;
    RHI::Texture *              ssrResolveReflectionTexture = nullptr;
    RHI::Texture *              ssrTemporalReflectionTextures[2] = {};
    uint32_t                    ssrResolutionMode = 1;
    uint32_t                    ssrTemporalReflectionTextureIndex = 0;
    bool                        ssrTemporalHistoryValid = false;
    SharedPtr<SSRHistoryRenderState> ssrHistoryRenderState;
    uint64_t                    ssrHistorySerial = 0;
    RHI::Texture *              ssrUpsampledReflectionTexture = nullptr;

    RHI::Texture *              transparentSSRDepthTextures[2] = {};
    RHI::Texture *              transparentSSRNormalRoughnessTexture = nullptr; // RG: UNORM octahedral view-space normal, B: perceptual roughness.
    RHI::Texture *              transparentSSRVelocityTexture = nullptr; // Unjittered receiver velocity for SSR reprojection.
    RHI::Texture *              transparentSSRTemporalReflectionTextures[2] = {};
    RHI::Texture *              transparentSSRUpsampledReflectionTexture = nullptr;
    uint32_t                    transparentSSRResolutionMode = 1;
    uint32_t                    transparentSSRHistoryTextureIndex = 0;
    bool                        transparentSSRTemporalHistoryValid = false;
    RHI::Format                 transparentSSRDepthFormat = RHI::Format::Unknown;

    RHI::Texture *              colorHistoryTextures[2] = {};
    RHI::Texture *              depthHistoryTextures[2] = {};
    uint32_t                    currentHistoryTextureIndex = 0;
    bool                        depthHistoryAsRenderTarget = false; // MSAA off, TAA on 상태에서는 depthHistoryTextures 를 렌더 타겟으로 삼는다.
    bool                        historyValid = false;

    ShadowAtlas                 shadowAtlas;
    ShadowResolutionHistory     localShadowResolutionHistory;

    SSRHiZResources             ssrHiZResources;
    SSRSceneColorHistoryResources ssrSceneColorHistoryResources;
    ScreenSpaceAOResources      screenSpaceAOResources;
    HiZResources                hiZResources;
    BloomResources              bloomResources;
    RHI::Buffer *               luminanceBuffer = nullptr;
};
