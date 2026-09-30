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
#include "Math/Matrix4.h"
#include "Math/OBB.h"
#include "Math/Plane.h"
#include "Shaders/ShaderInterop.h"
#include "ShaderPipeline/VertexInputLayoutFactory.h"
#include "Material/Material.h"
#include "Font/Font.h"
#include "Mesh/Mesh.h"
#include "Mesh/MeshInstance.h"
#include "Frontend/RenderObject.h"
#include "Frontend/RenderCamera.h"
#include "Frontend/RenderLight.h"
#include "RenderFrameSettings.h"
#include "RenderUtils.h"
#include "ScreenSpaceAOResources.h"
#include "ScreenSpaceDepthPyramidResources.h"
#include "HiZResources.h"
#include "BloomResources.h"
#include "GpuOcean.h"
#include "GpuOceanSurface.h"
#include "DebugDrawPipeline.h"

class Texture;

enum class FrameSurfFlag : uint8_t {
    None                            = 0,
    CameraVisible                   = BIT(0),   ///< Surface is visible by camera
    ShowWires                       = BIT(1),
    SkipSelection                   = BIT(2),
    Skybox                          = BIT(3)
};

constexpr std::true_type EnableBitmaskEnum(FrameSurfFlag) noexcept { return {}; }

// Frame* 는 해당 프레임의 렌더링을 위해 구성한 CPU 데이터다.
// 카메라별 데이터는 같은 원본 객체라도 view 마다 별도로 생성한다.
// 참조하는 공유 GPU 리소스는 별도 소유자가 관리하며 여러 프레임에 걸쳐 재사용할 수 있다.
struct FrameLight;
struct FrameSkinnedMesh;

struct FrameSurf {
    using Flag = FrameSurfFlag;

    uint32_t                        index;
    const FrameObject *             space;
    const SubMesh *                 subMesh;
    const RHI::IndexBuffer *        indexBuffer;            ///< 선택된 LOD 의 index buffer.
    uint32_t                        numIndexes;
    int32_t                         lodLevel;
    float                           viewZ = 0.0f;           ///< 소팅용 카메라 뷰 공간 깊이 (카메라 전방은 음수)
    const Material *                material;
    ShaderFeatureSet                baseFeatureSet;
    uint32_t                        shaderMaterialIndex;
    Flag                            flags;
    uint8_t                         shadowCascadeMask;      ///< 이 surface 를 그릴 directional shadow cascade bit mask.
};

struct RenderPassDrawSurf {
    const FrameSurf *               frameSurf;
    uint64_t                        sortKey;

    VertexInputLayoutId             inputLayoutId;
    RHI::PipelineState *            pso;
};

enum class RenderPassType : uint8_t {
    DepthPrepass,
    Shadow,
    Opaque,
    Skybox,
    Transparent,
    Gui
};

struct RenderPass {
    RenderPassType                  type;
    GraphicsPipelineDesc            graphicsPipelineDesc;
    ShaderFeatureSet                featureSet;
    VertexInputLayoutId             baseInputLayoutId;

    uint32_t                        maxDrawSurfs;
    uint32_t                        numDrawSurfs;
    const RenderPassDrawSurf **     drawSurfs;
    RenderPassDrawSurf *            drawSurfStorage;
};

// 한 cascade 의 광원 투영과 depth pass 데이터.
struct CSMCascade {
    // XY 는 border 를 포함하여 texel grid 에 snap 되고, Z 범위는 receiver sphere 와 깊이 여유분으로 정한다.
    BE1::OBB                        shadowMapBounds;                ///< shadow-map 투영 영역의 world space OBB.
    float                           worldUnitsPerTexel = 0.0f;      ///< shadow-map texel 하나가 차지하는 world-space 길이.
    ALIGN_AS32 BE1::Mat4            viewProjMatrix;                 ///< shadow-map view-projection matrix.
    RHI::ViewID                     depthDSV = RHI::ViewID::InvalidID;
    RenderPass                      renderPass {};

    CSMCascade() { renderPass.type = RenderPassType::Shadow; }
};

// 한 프레임 동안 한 카메라의 cascaded shadow 렌더링에 사용하는 공통 데이터.
struct FrameCascadedShadow {
    uint32_t                        cascadeCount = 0;
    float                           splitDistances[RenderFrameSettings::CSMCascadeCount + 1] = {};
    BE1::Plane                      viewPlanes[6];                  ///< 전체 CSM 카메라 view volume. 모든 cascade caster 컬링에 공유한다.
    CSMCascade                      cascades[RenderFrameSettings::CSMCascadeCount];

    ShaderCSMData                   shaderData {};                  ///< 메인 조명 패스에서 사용할 샘플링 데이터.
    RHI::Texture *                  depthTexture = nullptr;         ///< RenderContext 소유 shadow texture 에 대한 비소유 참조.

                                    // 입력 mask 에 포함된 cascade 만 검사하고 남은 mask 를 반환한다.
    uint8_t                         CullCascades(const BE1::AABB &worldBounds, uint8_t cascadeMask) const;
};

static_assert(ShaderCSMCascadeCount == RenderFrameSettings::CSMCascadeCount, "CPU and shader cascade counts must match");

struct FrameLocalShadow {
    const FrameLight *              light = nullptr;
    float                           squaredRadius = 0.0f;           // 포인트 라이트의 반경 제곱
    float                           viewPlaneToOriginDistances[6];  // 각 view volume 평면에서 광원까지 거리.
    uint32_t                        firstShadowView = 0;            // FrameShadowAtlas::shadowViews 의 첫번째 index
    uint32_t                        numShadowViews = 0;             // shadow view 개수 (Spot: 1, Point: 6)
    uint32_t                        requestedResolution = 0;        // 요청한 해상도
    uint32_t                        resolution = 0;                 // 할당된 해상도. (border 포함)
};

struct FrameLocalShadowView {
    ALIGN_AS32 BE1::Mat4            viewProjMatrix;
    BE1::Frustum                    frustum;                        // shadow view frustum
    BE1::Plane                      frustumPlanes[6];               // shadow view frustum planes
    BE1::Rect                       tileRect;                       // border 를 포함한 atlas 내 물리적 영역.
    uint32_t                        borderTexels = 0;               // 한쪽 border texel 수. shadow map 의 valid area 한 변은 tileRect.w - 2 * borderTexels.
    uint32_t                        shadowIndex = ~0u;              // FrameShadowAtlas::shadows 의 index
    uint32_t                        faceIndex = 0;                  // Point: 0 = +X, 1 = -X, 2 = +Y, 3 = -Y, 4 = +Z, 5 = -Z. Spot: 0.
    uint32_t                        numShadowSurfs = 0;
    const FrameSurf **              shadowSurfs = nullptr;
    RenderPass                      renderPass {};

    FrameLocalShadowView() { renderPass.type = RenderPassType::Shadow; }
};

struct FrameShadowAtlas {
    RHI::Texture *                  shadowTexture = nullptr;        // RenderCamera 소유 리소스에 대한 비소유 참조.
    RHI::ViewID                     shadowDSV = RHI::ViewID::InvalidID;
    RHI::ViewID                     shadowSRV = RHI::ViewID::InvalidID;
    uint32_t                        size = 0;
    uint32_t                        numShadows = 0;
    FrameLocalShadow *              shadows = nullptr;
    uint32_t                        maxShadowViews = 0;
    uint32_t                        numShadowViews = 0;
    FrameLocalShadowView *          shadowViews = nullptr;

    // 할당 결과는 광원 단위로 집계한다. Point 의 6면도 한 요청이며, 축소 수는 할당 성공 중 일부이다.
    uint32_t                        numCandidates = 0;              // shadow atlas 할당 후보 광원 수.
    uint32_t                        numResolutionDowngrades = 0;    // 해상도를 낮춰 할당에 성공한 광원 수.
    uint32_t                        numAllocationFailures = 0;      // atlas 타일 할당에 실패한 광원 수.
    uint32_t                        numViewBudgetSkipped = 0;       // shadow view 수 제한으로 제외된 광원 수.
    uint32_t                        numFrameAllocationSkipped = 0;  // 프레임 메모리 할당 실패로 제외된 광원 수.

    bool                            depthRendered = false;          // 초기화 후 백엔드에서만 접근한다. 해당 카메라의 depth 기록이 성공한 뒤 true 로 바꾼다.
};

// 같은 debug draw pipeline 을 사용하는 프리미티브들을 하나의 배치로 묶는다.
struct DebugDrawBatch {
    DebugDrawPipelineId             pipelineId = DebugDrawPipelineId::DepthTestedOneSidedFill;
    uint32_t                        firstVertex = 0;
    uint32_t                        numVerts = 0;
};

struct DebugDrawData {
    // 최대 6 종의 배치가 가능
    // 1. TriangleList, depthTest, one-sided
    // 2. TriangleList, depthTest, two-sided
    // 3. LineList, depthTest
    // 4. TriangleList, no depthTest, one-sided
    // 5. TriangleList, no depthTest, two-sided
    // 6. LineList, no depthTest
    static constexpr uint32_t       MaxBatches = static_cast<uint32_t>(DebugDrawPipelineId::Count);

    RHI::VertexBuffer *             vertexBuffer = nullptr;
    uint32_t                        numVerts = 0;
    uint32_t                        numBatches = 0;
    DebugDrawBatch                  batches[MaxBatches];
};

struct LightCulling {
    uint32_t                        tileCountX = 0;                 // X 축 타일 개수
    uint32_t                        tileCountY = 0;                 // Y 축 타일 개수
    uint32_t                        wordsPerTile = 0;               // 타일 당 마스크 워드 개수
    uint32_t *                      zBins = nullptr;                // packed min/max visible light IDs for each linear-depth Z-bin
    float                           zBinScale = 0.0f;               // zBinIndex = viewZ * zBinScale + zBinBias
    float                           zBinBias = 0.0f;                // zBinIndex = viewZ * zBinScale + zBinBias
    uint32_t                        lightRefTableOffset = ~0u;      // 현재 카메라가 셰이더에서 사용할 라이트 참조 테이블 오프셋
    uint32_t                        zBinTableOffset = ~0u;          // 현재 카메라가 셰이더에서 사용할 라이트 z-bin 테이블 오프셋
    uint32_t                        tileWordTableOffset = ~0u;      // 현재 카메라가 셰이더에서 사용할 라이트 bitmask word 테이블 오프셋
};

struct FrameOceanSurface {
    GpuOcean::SimulationSettings    simulationSettings;
    GpuOceanSurface::Settings       surfaceSettings;
    const OceanSurfaceQueryInput *  surfaceQueries = nullptr;
    uint32_t                        surfaceQueryCount = 0;
    uint64_t                        settingsRevision = 0;
};

struct FrameHiZ {
    RHI::Texture *                  texture = nullptr;
    RHI::ViewID                     mipSRVs[RenderHiZMaxLevelCount] = {};
    RHI::ViewID                     mipUAVs[RenderHiZMaxLevelCount] = {};
    uint32_t                        mipWidths[RenderHiZMaxLevelCount] = {};
    uint32_t                        mipHeights[RenderHiZMaxLevelCount] = {};
    uint32_t                        mipCount = 0;
    RHI::Texture *                  debugPreviewTexture = nullptr;
    RHI::Buffer *                   visibleInstanceBuffer = nullptr;
    RHI::Buffer *                   indirectArgsBuffer = nullptr;
    float                           debugLinearDepthScale = 1.0f;
};

struct FrameScreenSpaceDepthPyramid {
    RHI::Texture *                  texture = nullptr;
    RHI::ViewID                     mipSRVs[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    RHI::ViewID                     mipUAVs[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                        mipWidths[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                        mipHeights[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                        mipCount = 0;
    uint32_t                        validWidth = 0;
    uint32_t                        validHeight = 0;
};

struct FrameBloom {
    uint32_t                        renderWidth = 0;
    uint32_t                        renderHeight = 0;
    RHI::Texture *                  outputTexture = nullptr;
    RHI::ViewID                     outputViewID = RHI::ViewID::PrimaryID;

    RHI::Texture *                  bloomDirtMaskTexture = nullptr;

    RHI::Texture *                  bloomDownTexture = nullptr;
    RHI::Texture *                  bloomUpTexture = nullptr;
    uint32_t                        bloomMipWidths[RenderBloomDownMaxLevelCount] = {};
    uint32_t                        bloomMipHeights[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                     bloomDownMipSRVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                     bloomDownMipUAVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                     bloomUpMipSRVs[RenderBloomUpMaxLevelCount] = {};
    RHI::ViewID                     bloomUpMipUAVs[RenderBloomUpMaxLevelCount] = {};
    uint32_t                        bloomMipCount = 0;

    uint32_t                        fftBloomSize = 0;
    RHI::Buffer *                   fftBloomScreenSpatialBuffer = nullptr;
    RHI::Buffer *                   fftBloomScreenSpectrumBuffer = nullptr;
    RHI::Texture *                  fftBloomKernelSourceTexture = nullptr;
    RHI::Buffer *                   fftBloomKernelSpatialBuffer = nullptr;
    RHI::Buffer *                   fftBloomKernelSpectrumBuffer = nullptr;
    RHI::Buffer *                   fftBloomWorkBuffer = nullptr;
    RHI::Texture *                  fftBloomConvolutionOutputTexture = nullptr;
    uint32_t                        fftBloomConvolutionOutputWidth = 0;
    uint32_t                        fftBloomConvolutionOutputHeight = 0;
    RHI::ViewID                     fftBloomConvolutionOutputMipSRVs[RenderConvolutionBloomMaxMipLevelCount] = {};
    RHI::ViewID                     fftBloomConvolutionOutputMipUAVs[RenderConvolutionBloomMaxMipLevelCount] = {};
    uint32_t                        fftBloomConvolutionOutputMipCount = 0;
    float                           fftBloomKernelRotation = 0.0f;
    float                           fftBloomKernelScale = 1.0f;
    float                           fftBloomPaddingTrim = 0.0f;
    BE1::Vec2                       fftBloomKernelNormalizedExtents = BE1::Vec2(1.0f, 1.0f);
    BE1::RectF                      fftBloomMappedContentRect = BE1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
    BE1::Vec2                       fftBloomMappedKernelScale = BE1::Vec2(1.0f, 1.0f);
    float                           fftBloomKernelInverseAverageLuminance = 1.0f;
    bool                            fftBloomKernelDirty = true;
};

struct FrameScreenSpaceAO {
    RenderAOMode                    mode = RenderAOMode::Off;
    RHI::Texture *                  depthTexture = nullptr;
    RHI::Texture *                  normalTexture = nullptr;
    ScreenSpaceAOTextureSet         textureSet;
    RHI::ProjectionPolicy           projectionPolicy;
    bool                            temporalNoise = false;
};

// 한 프레임 렌더링에서만 유효한 렌더링 카메라
// 매 프레임 생성되고, 생성될 때 어떠한 힙 할당도 일어나지 않는다.
struct FrameCamera {
    static constexpr int            MaxFrameSurfs = 0x20000;
    static constexpr int            MaxRenderPasses = 8;

    ALIGN_AS32 BE1::Mat4            viewMatrix;
    ALIGN_AS32 BE1::Mat4            viewProjMatrix;                 // jittered
    ALIGN_AS32 BE1::Mat4            invViewProjMatrix;              // inverse jittered
    ALIGN_AS32 BE1::Mat4            prevViewProjMatrix;             // jittered previous
    ALIGN_AS32 BE1::Mat4            prevInvViewProjMatrix;          // inverse jittered previous
    ALIGN_AS32 BE1::Mat4            hiZCullViewProjMatrix;
    ALIGN_AS32 BE1::AABB            worldAABB;

    BE1::Vec3                       origin = BE1::Vec3::origin;     // View origin in world space
    BE1::Mat3                       axis = BE1::Mat3::identity;     // View axis [FORWARD, LEFT, UP] in world space
    BE1::Frustum                    viewFrustum;                    // Perspective view culling volume
    BE1::OBB                        viewBox;                        // Orthographic view culling volume

    BE1::Vec2                       jitterNDC = BE1::Vec2::zero;
    BE1::Vec2                       prevJitterNDC = BE1::Vec2::zero;

    uint32_t                        index = ~0;

    uint32_t                        firstFrameObjectIndex = 0;      // 현재 카메라를 위해 등록된 FrameObject 범위의 시작
    uint32_t                        firstFrameSkinnedMeshIndex = 0; // 현재 카메라를 위해 등록된 FrameSkinnedMesh 범위의 시작
    uint32_t                        firstFrameTextIndex = 0;        // 현재 카메라를 위해 등록된 FrameText 범위의 시작

    uint32_t                        maxFrameSurfs = 0;
    uint32_t                        numFrameSurfs = 0;
    const FrameSurf **              frameSurfs = nullptr;           // 카메라에 보이는 모든 surfaces 와 shadow surfaces

    RHI::Buffer *                   skinningJointBuffer = nullptr;

    BE1::Mat3                       envLightAxis = BE1::Mat3::identity;
    BE1::Color3                     envLightColor = BE1::Color3::white;
    float                           envLightIntensity = 0.0f;
    RHI::Texture *                  envLightDiffuseIrradianceCubeMap = nullptr;
    RHI::Texture *                  envLightSpecularCubeMap = nullptr;
    uint32_t                        envLightSpecularMipCount = 0;

    uint32_t                        maxFrameLocalLights = 0;
    uint32_t                        numFrameLocalLights = 0;
    const FrameLight **             frameLocalLights = nullptr;     // 카메라에 보이는 local light 포인터 배열

    FrameCascadedShadow *           frameCascadedShadow = nullptr;
    FrameShadowAtlas *              frameShadowAtlas = nullptr;

    const DebugDrawData *           debugDrawData = nullptr;
    RHI::PipelineState *            debugDrawPSOs[DebugDrawData::MaxBatches] = {};
    bool                            showDebugLights = false;
    RHI::PipelineState *            oceanSurfacePSO = nullptr;
    RHI::PipelineState *            sceneSnapshotDownsamplePSO = nullptr;
    RHI::Texture *                  snapshotColorTexture = nullptr;
    RHI::Texture *                  snapshotDepthTexture = nullptr;

    LightCulling                    lightCulling;

    uint32_t                        numRenderPasses = 0;
    RenderPass *                    renderPasses = nullptr;
    uint32_t                        maxRenderPassDrawSurfs = 0;
    uint32_t                        numRenderPassDrawSurfs = 0;
    const RenderPassDrawSurf **     renderPassDrawSurfs = nullptr;  // RenderPass 들이 나눠 쓰는 draw surf pointer pool
    RenderPassDrawSurf *            renderPassDrawSurfStorage = nullptr; // RenderPass 들이 나눠 쓰는 draw surf pool

    BE1::Rect                       outputRect;
    RHI::Texture *                  outputTexture = nullptr;
    RHI::ViewID                     outputViewID = RHI::ViewID::PrimaryID;
    RHI::GPUResourceState           outputBeforeState = RHI::GPUResourceState::ShaderResource;
    RHI::GPUResourceState           outputAfterState = RHI::GPUResourceState::ShaderResource;
    RHI::PipelineState *            outputBlitPSO = nullptr;
    uint32_t                        renderWidth = 0;
    uint32_t                        renderHeight = 0;
    RenderCamera::ClearMethod       clearMethod;
    RenderCamera::ClearColorMode    clearColorMode;
    BE1::Color4                     clearColor;
    RHI::ProjectionPolicy           projectionPolicy;
    BE1::Vec4                       projectionXY = BE1::Vec4::zero; // XY scale and NDC offset before jitter
    BE1::Vec2                       projectionZ = BE1::Vec2::zero;
    bool                            projectionOrthogonal = false;
    bool                            projectionInfiniteZFar = false;
    float                           ndcDepthScale = 1.0f;
    float                           ndcDepthBias = 0.0f;
    RenderCamera::Flag              flags = RenderCamera::Flag::None;
    bool                            directionalLightVisible = false;
    bool                            needNormalOutput = false;
    bool                            needDepthNormalOutput = false;  // Smooth view normals for AO, before opaque overwrites the normal buffer.
    bool                            needVelocityOutput = false;

    RHI::Texture *                  colorTexture = nullptr;
    RHI::Texture *                  depthTexture = nullptr;
    RHI::Texture *                  normalTexture = nullptr;        // Octahedral view normals: prepass smooth normals for AO, then shading normals for SSR.
    RHI::Texture *                  velocityTexture = nullptr;

    RHI::Texture *                  resolvedColorTexture = nullptr;
    RHI::Texture *                  resolvedDepthTexture = nullptr; // Prepass depth for AO, then opaque depth for SSR.
    RHI::Texture *                  resolvedNormalTexture = nullptr;
    RHI::Texture *                  resolvedVelocityTexture = nullptr;

    RHI::Texture *                  postProcessColorTexture = nullptr;
    RHI::Texture *                  colorHistoryTextures[2] = {};
    RHI::Texture *                  depthHistoryTextures[2] = {};
    uint32_t                        currentHistoryTextureIndex = 0;
    bool                            historyValid = false;

    bool                            generateHiZ = false;
    bool                            hiZOcclusionCulling = false;
    bool                            hiZDebugPreview = false;
    FrameHiZ                        hiZ;

    bool                            applyAO = false;
    FrameScreenSpaceAO              ao;

    bool                            generateScreenSpaceDepthPyramid = false;
    FrameScreenSpaceDepthPyramid    screenSpaceDepthPyramid;

    bool                            traceSSR = false;
    RHI::Texture *                  ssrTraceTexture = nullptr;

    bool                            taaResolve = false;
    RHI::Buffer *                   luminanceBuffer = nullptr;
    RHI::Texture *                  colorGradingLUTTexture = nullptr;
    FrameBloom                      bloom;

    bool                            is2D;
};

// 한 프레임 렌더링에서만 유효한 카메라별 렌더링 객체.
// 하나의 RenderObject 가 여러 카메라에 보이면 각 카메라 (view) 마다 별도의 FrameObject 로 등록된다.
// RenderObject 의 렌더링 데이터를 프레임 메모리에 복사하여 생성하므로 별도의 힙 할당은 발생하지 않는다.
// 참조하는 공유 자원은 렌더링 중 파괴되지 않도록 프레임이 끝날 때까지 유지한 후 해제한다.
struct FrameObject {
    enum class Flag : uint8_t {
        None                        = 0,
        CameraVisible               = BIT(0),
        LocalShadowCaster           = BIT(1)
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

    ALIGN_AS32 BE1::Mat3x4          worldMatrix;                    ///< World transform matrix
    MeshInstance *                  meshInstance = nullptr;         ///< Mesh instance
    FrameSkinnedMesh *              skinnedMesh = nullptr;          ///< Per-view skinning data, if present.
    ALIGN_AS32 BE1::Mat3x4          prevWorldMatrix;                ///< Previous world transform matrix

    union {
        const Material *            baseMaterials[2] = {};          ///< Base materials for the object, used when numMaterials <= 2.
        const Material **           materials;                      ///< External material array, used when numMaterials > 2.
    };

    uint16_t                        numMaterials = 0;               ///< Material 개수
    int8_t                          meshLOD = -1;                   ///< 요청 LOD. -1 은 자동 선택, 0~127 은 지정 LOD.
    Flag                            flags = Flag::None;
    uint8_t                         shadowCascadeMask = 0;          ///< 이 object 가 투영되는 directional shadow cascade bit mask.

    float                           squaredLODErrorLimit = 0.0f;    ///< 현재 view 에서 공유 LOD 를 선택하는 로컬 공간 허용 오차 제곱.

    uint32_t                        firstLocalShadowCasterRef = 0;  ///< 현재 카메라의 surface 를 구성하는 동안만 사용하는 RenderWorld::localShadowCasterRefs 범위.
    uint32_t                        numLocalShadowCasterRefs = 0;   ///< 백엔드는 이 임시 범위 대신 FrameLocalShadowView::shadowSurfs 를 사용한다.

    bool                            IsRenderable() const { return BE1::HasAnyFlag(flags, Flag::CameraVisible | Flag::LocalShadowCaster) || shadowCascadeMask != 0; }
    const Material *const *         GetMaterials() const { return numMaterials <= COUNT_OF(baseMaterials) ? baseMaterials : materials; }
};

static_assert(SubMesh::MaxLODLevel <= INT8_MAX, "FrameObject meshLOD must represent every LOD level");
static_assert(RenderObject::MaxMaterials <= UINT16_MAX, "FrameObject numMaterials must represent every material slot");

// 한 프레임 렌더링에서만 유효한 카메라별 skinned mesh 데이터.
// skinned mesh 가 있는 FrameObject 대해서만 생성하여 일반 FrameObject 의 크기와 skinned mesh 의 순회 비용을 줄인다.
struct FrameSkinnedMesh {
    FrameObject *                   space = nullptr;                ///< Object visibility and transform data.
    MeshInstance *                  meshInstance = nullptr;
    const Skeleton *                skeleton = nullptr;             ///< Skeleton information for skeletal mesh

    uint32_t                        skinningJointOffset = ShaderSkinningJointOffsetInvalid; ///< Joint element offset in the camera joint buffer.
    uint32_t                        prevSkinningJointOffset = ShaderSkinningJointOffsetInvalid; ///< Previous joint element offset in the camera joint buffer.
};

// 한 프레임 렌더링에서만 유효한 카메라별 텍스트 렌더링 데이터.
// 텍스트가 있는 FrameObject 에 대해서만 생성하여 일반 FrameObject 의 크기와 텍스트의 순회 비용을 줄인다.
struct FrameText {
    FrameObject *                   space = nullptr;
    Font *                          font = nullptr;
    const char *                    text = nullptr;
    BE1::RectF                      textRect;
    BE1::Vec2                       textShadowOffset;
    uint32_t                        textShadowColor;
    RenderObject::TextParams        textParams;
    float                           textScale;
    float                           textLineSpacing;
};

// Spot 광원에서만 사용하는 한 프레임 렌더링 데이터.
struct FrameSpotLightData {
    ALIGN_AS32 BE1::Mat3x4          fallOffMatrix;
};

// 한 프레임 렌더링에서만 유효한 카메라별 렌더링 광원.
// 하나의 RenderLight 가 여러 카메라에 보이면 각 카메라 (view) 마다 별도의 FrameLight 로 등록된다.
// RenderLight 의 렌더링 데이터를 프레임 메모리에 복사하여 생성하므로 별도의 힙 할당은 발생하지 않는다.
struct FrameLight {
    static constexpr uint32_t       TileCoordBitCount = 8;
    static constexpr uint32_t       TileCoordMask = (1u << TileCoordBitCount) - 1;
    static constexpr uint32_t       MaxRenderDimension = (TileCoordMask + 1) * ShaderLightTileSize;

    ALIGN_AS32 BE1::Mat4            viewProjScaleBiasMatrix;

    const FrameSpotLightData *      spotLightData = nullptr;

    uint16_t                        index;                          ///< FrameLight index
    uint16_t                        typeIndex;                      ///< FrameLight index for same light type
    uint16_t                        packedTileMin;                  ///< Packed minimum light-culling tile coordinate: low 8 bits X, high 8 bits Y
    uint16_t                        packedTileMax;                  ///< Packed maximum light-culling tile coordinate: low 8 bits X, high 8 bits Y

    BE1::Mat3                       axis;
    BE1::Vec3                       origin;
    BE1::Vec3                       size;                           ///< Point: x is radius. Spot/BoxDir: shape extents
    float                           zNear;

    BE1::Color3                     lightColor;                     ///< Linear RGB light color multiplied by intensity

    float                           viewZMin;                       ///< Minimum positive-forward linear view Z
    float                           viewZMax;                       ///< Maximum positive-forward linear view Z

    float                           fallOffExponent;
    float                           shadowOffsetUnits;
    float                           shadowOffsetFactor;

    RenderLight::Type               type;
    RenderLight::Flag               flags;
    uint32_t                        localShadowIndex = ~0u;         ///< 카메라의 FrameShadowAtlas::shadows 인덱스. ~0u 이면 shadow 없음.
    uint32_t                        shadowRequestedResolution = 0;  ///< 화면 크기로 정한 border 포함 요청 해상도. Point 는 한 면 기준이며 비대상은 0.

    int                             GetShadowViewCount() const { return type == RenderLight::Type::Point ? 6 : type == RenderLight::Type::Spot ? 1 : 0; }
    void                            CalcSpotShadowProjection(FrameLocalShadowView &shadowView) const;
    BE1::Vec2                       GetPointShadowDepthRange() const; // X = near, Y = far. Depth 렌더링과 샘플링 계수에 같은 범위를 사용한다.
    void                            CalcPointShadowProjections(FrameLocalShadowView shadowViews[6]) const;
};
