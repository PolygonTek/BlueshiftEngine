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
#include "Shaders/ShaderInterop.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"
#include "Render/Material/Material.h"
#include "Render/Font/Font.h"
#include "Render/Mesh/Mesh.h"
#include "Render/Mesh/MeshInstance.h"
#include "Render/Frontend/RenderObject.h"
#include "Render/Frontend/RenderCamera.h"
#include "Render/Frontend/RenderLight.h"
#include "Render/RenderFrameSettings.h"
#include "Render/ScreenSpaceAOResources.h"
#include "Math/Matrix4.h"
#include "Math/OBB.h"
#include "Math/Plane.h"
#include "DebugDrawPipeline.h"

class Texture;
struct VisLight;
struct VisSkinnedMesh;

enum class VisSurfFlag : uint8_t {
    None                        = 0,
    CameraVisible               = BIT(0),           ///< Surface is visible by camera
    ShowWires                   = BIT(1),
    SkipSelection               = BIT(2),
    Skybox                      = BIT(3)
};

template<>
struct BE1::enable_bitmask_operators<VisSurfFlag> {
    static constexpr bool enable = true;
};

struct VisSurf {
    uint32_t                    index;
    const VisObject *           space;
    const SubMesh *             subMesh;
    const RHI::IndexBuffer *    indexBuffer;        ///< 선택한 LOD 의 index buffer.
    uint32_t                    numIndexes;
    int                         lodLevel;
    const Material *            material;
    ShaderFeatureSet            baseFeatureSet;
    uint32_t                    shaderMaterialIndex;
    VisSurfFlag                 flags;
    uint8_t                     shadowCascadeMask;  ///< 이 surface 를 그릴 directional shadow cascade bit mask.
};

struct RenderPassDrawSurf {
    const VisSurf *             visSurf;
    uint64_t                    sortKey;

    VertexInputLayoutId         inputLayoutId;
    RHI::PipelineState *        pso;
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
    RenderPassType              type;
    GraphicsPipelineDesc        graphicsPipelineDesc;
    ShaderFeatureSet            featureSet;
    VertexInputLayoutId         baseInputLayoutId;

    uint32_t                    maxDrawSurfs;
    uint32_t                    numDrawSurfs;
    const RenderPassDrawSurf ** drawSurfs;
    RenderPassDrawSurf *        drawSurfStorage;
};

// 한 cascade 의 광원 투영과 depth pass 데이터.
struct CSMCascade {
    // XY 는 border 를 포함하여 texel grid 에 snap 되고, Z 범위는 receiver sphere 와 깊이 여유분으로 정한다.
    BE1::OBB                    shadowMapBounds;            ///< shadow-map 투영 영역의 world space OBB.
    float                       worldUnitsPerTexel = 0.0f;  ///< shadow-map texel 하나가 차지하는 world-space 길이.
    ALIGN_AS32 BE1::Mat4        viewProjMatrix;             ///< shadow-map view-projection matrix.
    RHI::ViewID                 depthDSV = RHI::ViewID::InvalidID;
    RenderPass                  renderPass {};

    CSMCascade() { renderPass.type = RenderPassType::Shadow; }
};

// 한 프레임 동안 한 카메라의 cascaded shadow 렌더링에 사용하는 공통 데이터.
struct VisCascadedShadow {
    uint32_t                    cascadeCount = 0;
    float                       splitDistances[RenderFrameSettings::CSMCascadeCount + 1] = {};
    BE1::Plane                  viewVolumePlanes[6];        ///< 전체 CSM 카메라 view volume. 모든 cascade caster 컬링에 공유한다.
    CSMCascade                  cascades[RenderFrameSettings::CSMCascadeCount];

    // 메인 조명 패스에서 사용할 샘플링 데이터.
    ShaderCSMData               shaderData {};

    // RenderContext 소유 shadow texture 에 대한 비소유 참조.
    RHI::Texture *              depthTexture = nullptr;
};

static_assert(ShaderCSMCount == RenderFrameSettings::CSMCascadeCount, "CPU and shader cascade counts must match");

// 같은 debug draw pipeline 을 사용하는 프리미티브들을 하나의 배치로 묶는다.
struct DebugDrawBatch {
    DebugDrawPipelineId         pipelineId = DebugDrawPipelineId::DepthTestedOneSidedFill;
    uint32_t                    firstVertex = 0;
    uint32_t                    numVerts = 0;
};

struct DebugDrawData {
    // 최대 6 종의 배치가 가능
    // 1. TriangleList, depthTest, one-sided
    // 2. TriangleList, depthTest, two-sided
    // 3. LineList, depthTest
    // 4. TriangleList, no depthTest, one-sided
    // 5. TriangleList, no depthTest, two-sided
    // 6. LineList, no depthTest
    static constexpr uint32_t   MaxBatches = static_cast<uint32_t>(DebugDrawPipelineId::Count);

    RHI::VertexBuffer *         vertexBuffer = nullptr;
    uint32_t                    numVerts = 0;
    uint32_t                    numBatches = 0;
    DebugDrawBatch              batches[MaxBatches];
};

struct LightCulling {
    uint32_t                    tileCountX = 0;             // X 축 타일 개수
    uint32_t                    tileCountY = 0;             // Y 축 타일 개수
    uint32_t                    wordsPerTile = 0;           // 타일 당 마스크 워드 개수
    uint32_t *                  zBins = nullptr;            // packed min/max visible light IDs for each linear-depth Z-bin
    float                       zBinScale = 0.0f;           // zBinIndex = viewZ * zBinScale + zBinBias
    float                       zBinBias = 0.0f;            // zBinIndex = viewZ * zBinScale + zBinBias
    uint32_t                    lightRefTableOffset = ~0u;  // 현재 카메라가 셰이더에서 사용할 라이트 참조 테이블 오프셋
    uint32_t                    zBinTableOffset = ~0u;      // 현재 카메라가 셰이더에서 사용할 라이트 z-bin 테이블 오프셋
    uint32_t                    tileWordTableOffset = ~0u;  // 현재 카메라가 셰이더에서 사용할 라이트 bitmask word 테이블 오프셋
};

struct ScreenSpaceAOFrameData {
    RenderAOMode                mode = RenderAOMode::Off;
    RHI::Texture *              depthTexture = nullptr;
    RHI::Texture *              normalTexture = nullptr;
    ScreenSpaceAOTextures       textures;
    RHI::ProjectionPolicy       projectionPolicy;
    bool                        temporalNoise = false;
};

// 한 프레임 렌더링에서만 유효한 렌더링 카메라
// 매 프레임 생성되고, 생성될 때 어떠한 힙 할당도 일어나지 않는다.
struct VisCamera {
    static constexpr int        MaxVisSurfs = 0x20000;
    static constexpr int        MaxRenderPasses = 8;

    ALIGN_AS32 BE1::Mat4        viewMatrix;
    ALIGN_AS32 BE1::Mat4        viewProjMatrix;             // jittered
    ALIGN_AS32 BE1::Mat4        invViewProjMatrix;          // inverse jittered
    ALIGN_AS32 BE1::Mat4        prevViewProjMatrix;         // jittered previous
    ALIGN_AS32 BE1::Mat4        prevInvViewProjMatrix;      // inverse jittered previous
    ALIGN_AS32 BE1::Mat4        hiZCullViewProjMatrix;
    ALIGN_AS32 BE1::AABB        worldAABB;

    BE1::Vec3                   origin = BE1::Vec3::origin; // View origin in world space
    BE1::Mat3                   axis = BE1::Mat3::identity; // View axis [FORWARD, LEFT, UP] in world space
    BE1::Frustum                viewFrustum;                // Perspective view culling volume
    BE1::OBB                    viewBox;                    // Orthographic view culling volume

    BE1::Mat3                   envLightAxis = BE1::Mat3::identity;
    BE1::Color3                 envLightColor = BE1::Color3::white;
    float                       envLightIntensity = 0.0f;
    RHI::Texture *              envLightDiffuseIrradianceCubeMap = nullptr;
    RHI::Texture *              envLightSpecularCubeMap = nullptr;
    uint32_t                    envLightSpecularMipCount = 0;

    BE1::Vec2                   jitterNDC = BE1::Vec2::zero;
    BE1::Vec2                   prevJitterNDC = BE1::Vec2::zero;

    uint32_t                    index = ~0;

    uint32_t                    firstVisObjectIndex = 0;   // 현재 카메라를 위해 등록된 VisObject 범위의 시작
    uint32_t                    firstVisSkinnedMeshIndex = 0; // 현재 카메라를 위해 등록된 VisSkinnedMesh 범위의 시작
    uint32_t                    firstVisTextIndex = 0;     // 현재 카메라를 위해 등록된 VisText 범위의 시작

    uint32_t                    maxVisSurfs = 0;
    uint32_t                    numVisSurfs = 0;
    const VisSurf **            visSurfs = nullptr;         // 카메라에 보이는 모든 surfaces 와 shadow surfaces

    RHI::Buffer *               skinningJointBuffer = nullptr;

    uint32_t                    maxVisLocalLights = 0;
    uint32_t                    numVisLocalLights = 0;
    const VisLight **           visLocalLights = nullptr;   // 카메라에 보이는 local light 포인터 배열

    VisCascadedShadow *         cascadedShadow = nullptr;

    const DebugDrawData *       debugDrawData = nullptr;
    RHI::PipelineState *        debugDrawPSOs[DebugDrawData::MaxBatches] = {};
    bool                        showDebugLights = false;
    RHI::PipelineState *        oceanSurfacePSO = nullptr;
    RHI::PipelineState *        sceneSnapshotDownsamplePSO = nullptr;
    RHI::Texture *              snapshotColorTexture = nullptr;
    RHI::Texture *              snapshotDepthTexture = nullptr;

    LightCulling                lightCulling;

    uint32_t                    numRenderPasses = 0;
    RenderPass *                renderPasses = nullptr;
    uint32_t                    maxRenderPassDrawSurfs = 0;
    uint32_t                    numRenderPassDrawSurfs = 0;
    const RenderPassDrawSurf ** renderPassDrawSurfs = nullptr; // RenderPass 들이 나눠 쓰는 draw surf pointer pool
    RenderPassDrawSurf *        renderPassDrawSurfStorage = nullptr; // RenderPass 들이 나눠 쓰는 draw surf pool

    BE1::Rect                   outputRect;
    RHI::Texture *              outputTexture = nullptr;
    RHI::ViewID                 outputViewID = RHI::ViewID::PrimaryID;
    RHI::GPUResourceState       outputBeforeState = RHI::GPUResourceState::ShaderResource;
    RHI::GPUResourceState       outputAfterState = RHI::GPUResourceState::ShaderResource;
    RHI::PipelineState *        outputImagePSO = nullptr;
    uint32_t                    renderWidth = 0;
    uint32_t                    renderHeight = 0;
    RenderCamera::ClearMethod   clearMethod;
    RenderCamera::ClearColorMode clearColorMode;
    BE1::Color4                 clearColor;
    RHI::ProjectionPolicy       projectionPolicy;
    BE1::Vec4                   projectionXY = BE1::Vec4::zero; // XY scale and NDC offset before jitter
    BE1::Vec2                   projectionZ = BE1::Vec2::zero;
    bool                        projectionOrthogonal = false;
    bool                        projectionInfiniteZFar = false;
    float                       ndcDepthScale = 1.0f;
    float                       ndcDepthBias = 0.0f;
    RenderCamera::Flag          flags = RenderCamera::Flag::None;
    bool                        directionalLightVisible = false;
    bool                        needNormalOutput = false;
    bool                        needDepthNormalOutput = false; // Smooth view normals for AO, before opaque overwrites the normal buffer.
    bool                        needVelocityOutput = false;

    RHI::Texture *              rtColorTexture = nullptr;
    RHI::Texture *              rtColorMSAATexture = nullptr;
    RHI::Texture *              depthTexture = nullptr;
    RHI::Texture *              rtNormalTexture = nullptr; // Octahedral view normals: prepass smooth normals for AO, then shading normals for SSR.
    RHI::Texture *              rtNormalMSAATexture = nullptr;
    RHI::Texture *              rtVelocityTexture = nullptr;
    RHI::Texture *              rtVelocityMSAATexture = nullptr;
    RHI::Texture *              postProcessColorTexture = nullptr;

    RHI::Texture *              colorHistoryTextures[2] = {};
    RHI::Texture *              depthHistoryTextures[2] = {};
    uint32_t                    currentHistoryTextureIndex = 0;
    bool                        historyValid = false;

    bool                        generateHiZ = false;
    RHI::Texture *              hiZTexture = nullptr;
    RHI::ViewID                 hiZMipSRVs[RenderHiZMaxLevelCount] = {};
    RHI::ViewID                 hiZMipUAVs[RenderHiZMaxLevelCount] = {};
    uint32_t                    hiZMipWidths[RenderHiZMaxLevelCount] = {};
    uint32_t                    hiZMipHeights[RenderHiZMaxLevelCount] = {};
    uint32_t                    hiZMipCount = 0;
    bool                        hiZOcclusionCulling = false;
    RHI::Buffer *               hiZCullVisibleInstanceBuffer = nullptr;
    RHI::Buffer *               hiZCullIndirectArgsBuffer = nullptr;

    bool                        hiZDebugPreview = false;
    RHI::Texture *              hiZDebugPreviewTexture = nullptr;
    float                       hiZDebugLinearDepthScale = 1.0f;

    bool                        applyAO = false;
    ScreenSpaceAOFrameData      ao;

    bool                        generateScreenSpaceDepthPyramid = false;
    RHI::Texture *              screenSpaceDepthPyramidTexture = nullptr;
    RHI::ViewID                 screenSpaceDepthPyramidMipSRVs[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    RHI::ViewID                 screenSpaceDepthPyramidMipUAVs[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                    screenSpaceDepthPyramidMipWidths[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                    screenSpaceDepthPyramidMipHeights[RenderScreenSpaceDepthPyramidMaxLevelCount] = {};
    uint32_t                    screenSpaceDepthPyramidMipCount = 0;
    uint32_t                    screenSpaceDepthPyramidValidWidth = 0;
    uint32_t                    screenSpaceDepthPyramidValidHeight = 0;
    bool                        traceSSR = false;
    RHI::Texture *              ssrResolvedDepthTexture = nullptr;
    RHI::Texture *              ssrTraceTexture = nullptr;

    bool                        taaResolve = false;
    RHI::Buffer *               luminanceBuffer = nullptr;
    RHI::Texture *              colorGradingLUTTexture = nullptr;
    RHI::Texture *              bloomDirtMaskTexture = nullptr;

    RHI::Texture *              bloomDownTexture = nullptr;
    RHI::Texture *              bloomUpTexture = nullptr;
    uint32_t                    bloomMipWidths[RenderBloomDownMaxLevelCount] = {};
    uint32_t                    bloomMipHeights[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipSRVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipUAVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipSRVs[RenderBloomUpMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipUAVs[RenderBloomUpMaxLevelCount] = {};
    uint32_t                    bloomMipCount = 0;

    uint32_t                    fftBloomSize = 0;
    RHI::Buffer *               fftBloomScreenSpatialBuffer = nullptr;
    RHI::Buffer *               fftBloomScreenSpectrumBuffer = nullptr;
    RHI::Texture *              fftBloomKernelSourceTexture = nullptr;
    RHI::Buffer *               fftBloomKernelSpatialBuffer = nullptr;
    RHI::Buffer *               fftBloomKernelSpectrumBuffer = nullptr;
    RHI::Buffer *               fftBloomWorkBuffer = nullptr;
    RHI::Texture *              fftBloomConvolutionOutputTexture = nullptr;
    uint32_t                    fftBloomConvolutionOutputWidth = 0;
    uint32_t                    fftBloomConvolutionOutputHeight = 0;
    RHI::ViewID                 fftBloomConvolutionOutputMipSRVs[RenderConvolutionBloomMaxMipLevelCount] = {};
    RHI::ViewID                 fftBloomConvolutionOutputMipUAVs[RenderConvolutionBloomMaxMipLevelCount] = {};
    uint32_t                    fftBloomConvolutionOutputMipCount = 0;
    float                       fftBloomKernelRotation = 0.0f;
    float                       fftBloomKernelScale = 1.0f;
    float                       fftBloomPaddingTrim = 0.0f;
    BE1::Vec2                   fftBloomKernelNormalizedExtents = BE1::Vec2(1.0f, 1.0f);
    BE1::RectF                  fftBloomMappedContentRect = BE1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
    BE1::Vec2                   fftBloomMappedKernelScale = BE1::Vec2(1.0f, 1.0f);
    float                       fftBloomKernelInverseAverageLuminance = 1.0f;
    bool                        fftBloomKernelDirty = true;

    bool                        is2D;
};

// 한 프레임 렌더링에서만 유효한 카메라별 렌더링 객체.
// 하나의 RenderObject 가 여러 카메라에 보이면 각 카메라 (view) 마다 별도의 VisObject 로 등록된다.
// RenderObject 의 렌더링 데이터를 프레임 메모리에 복사하여 생성하므로 별도의 힙 할당은 발생하지 않는다.
// 참조하는 공유 자원은 렌더링 중 파괴되지 않도록 프레임이 끝날 때까지 유지한 후 해제한다.
struct VisObject {
    enum class Flag : uint8_t {
        None                    = 0,
        CameraVisible           = BIT(0)
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

    ALIGN_AS32 BE1::Mat3x4      worldMatrix;                ///< World transform matrix
    MeshInstance *              meshInstance = nullptr;     ///< Mesh instance
    VisSkinnedMesh *            skinnedMesh = nullptr;      ///< Per-view skinning data, if present.
    ALIGN_AS32 BE1::Mat3x4      prevWorldMatrix;            ///< Previous world transform matrix

    union {
        const Material *        baseMaterials[2] = {};      ///< Base materials for the object, used when numMaterials <= 2.
        const Material **       materials;                  ///< External material array, used when numMaterials > 2.
    };

    uint16_t                    numMaterials = 0;           ///< Material 개수
    int8_t                      meshLOD = -1;               ///< 요청 LOD. -1 은 자동 선택, 0~127 은 지정 LOD.
    Flag                        flags = Flag::None;
    uint8_t                     shadowCascadeMask = 0;      ///< 이 object 가 투영되는 directional shadow cascade bit mask.

    float                       squaredLODErrorLimit = 0.0f;///< 현재 view 에서 공유 LOD 를 선택하는 로컬 공간 허용 오차 제곱.

    bool                        IsRenderable() const { return BE1::HasFlag(flags, Flag::CameraVisible) || shadowCascadeMask != 0; }
    const Material *const *     GetMaterials() const { return numMaterials <= COUNT_OF(baseMaterials) ? baseMaterials : materials; }
};

static_assert(sizeof(VisObject) == 160, "VisObject layout changed");
static_assert(SubMesh::MaxLODLevel <= INT8_MAX, "VisObject meshLOD must represent every LOD level");
static_assert(RenderObject::MaxMaterials <= UINT16_MAX, "VisObject numMaterials must represent every material slot");

// 한 프레임 렌더링에서만 유효한 카메라별 skinned mesh 데이터.
// skinned mesh 가 있는 VisObject 대해서만 생성하여 일반 VisObject 의 크기와 skinned mesh 의 순회 비용을 줄인다.
struct VisSkinnedMesh {
    VisObject *                 space = nullptr;            ///< Object visibility and transform data.
    MeshInstance *              meshInstance = nullptr;
    const Skeleton *            skeleton = nullptr;         ///< Skeleton information for skeletal mesh

    uint32_t                    skinningJointOffset = ShaderSkinningJointOffsetInvalid; ///< Joint element offset in the camera joint buffer.
    uint32_t                    prevSkinningJointOffset = ShaderSkinningJointOffsetInvalid; ///< Previous joint element offset in the camera joint buffer.
};

static_assert(sizeof(VisSkinnedMesh) == 32, "VisSkinnedMesh layout changed");

// 한 프레임 렌더링에서만 유효한 카메라별 텍스트 렌더링 데이터.
// 텍스트가 있는 VisObject 에 대해서만 생성하여 일반 VisObject 의 크기와 텍스트의 순회 비용을 줄인다.
struct VisText {
    VisObject *                 space = nullptr;
    Font *                      font = nullptr;
    const char *                text = nullptr;
    BE1::RectF                  textRect;
    BE1::Vec2                   textShadowOffset;
    uint32_t                    textShadowColor;
    RenderObject::TextParams    textParams;
    float                       textScale;
    float                       textLineSpacing;
};

static_assert(sizeof(VisText) == 64, "VisText layout changed");

// Spot 광원에서만 사용하는 한 프레임 렌더링 데이터.
struct VisSpotLightData {
    ALIGN_AS32 BE1::Mat3x4      fallOffMatrix;
};

static_assert(sizeof(VisSpotLightData) == 64, "VisSpotLightData layout changed");

// 한 프레임 렌더링에서만 유효한 카메라별 렌더링 광원.
// 하나의 RenderLight 가 여러 카메라에 보이면 각 카메라 (view) 마다 별도의 VisLight 로 등록된다.
// RenderLight 의 렌더링 데이터를 프레임 메모리에 복사하여 생성하므로 별도의 힙 할당은 발생하지 않는다.
struct VisLight {
    static constexpr uint32_t   TileCoordBitCount = 8;
    static constexpr uint32_t   TileCoordMask = (1u << TileCoordBitCount) - 1;
    static constexpr uint32_t   MaxRenderDimension = (TileCoordMask + 1) * ShaderLightTileSize;

    ALIGN_AS32 BE1::Mat4        viewProjScaleBiasMatrix;

    const VisSpotLightData *    spotLightData = nullptr;

    uint16_t                    index;                      ///< VisLight index
    uint16_t                    typeIndex;                  ///< VisLight index for same light type
    uint16_t                    packedTileMin;              ///< Packed minimum light-culling tile coordinate: low 8 bits X, high 8 bits Y
    uint16_t                    packedTileMax;              ///< Packed maximum light-culling tile coordinate: low 8 bits X, high 8 bits Y

    BE1::Mat3                   axis;
    BE1::Vec3                   origin;
    BE1::Vec3                   size;                       ///< Point: x is radius. Spot/BoxDir: shape extents
    float                       zNear;

    BE1::Color3                 lightColor;                 ///< Linear RGB light color multiplied by intensity

    float                       viewZMin;                   ///< Minimum positive-forward linear view Z
    float                       viewZMax;                   ///< Maximum positive-forward linear view Z

    float                       fallOffExponent;
    float                       shadowOffsetFactor;
    float                       shadowOffsetUnits;

    RenderLight::Type           type;
    RenderLight::Flag           flags;
};

static_assert(sizeof(VisLight) == 192, "VisLight layout changed");
