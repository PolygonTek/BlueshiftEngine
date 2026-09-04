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
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"
#include "Render/Material/Material.h"
#include "Render/Font/Font.h"
#include "Render/Mesh/Mesh.h"
#include "Render/Mesh/MeshInstance.h"
#include "Render/Frontend/RenderObject.h"
#include "Render/Frontend/RenderCamera.h"
#include "Render/Frontend/RenderLight.h"
#include "DebugDrawPipeline.h"

class Texture;
struct VisLight;

enum class VisSurfFlag : uint8_t {
    None                        = 0,
    CameraVisible               = BIT(0),           ///< Surface is visible by camera
    ShadowVisible               = BIT(1),           ///< Shadow of surface is visible by camera
    ShowWires                   = BIT(2),
    SkipSelection               = BIT(3),
    Skybox                      = BIT(4)
};

template<>
struct BE1::enable_bitmask_operators<VisSurfFlag> {
    static constexpr bool enable = true;
};

struct VisSurf {
    uint32_t                    index;
    const VisObject *           space;
    const SubMesh *             subMesh;
    const Material *            material;
    ShaderFeatureSet            baseFeatureSet;
    uint32_t                    shaderMaterialIndex;
    VisSurfFlag                 flags;
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

    BE1::Mat3                   environmentLightAxis = BE1::Mat3::identity;
    BE1::Color3                 environmentLightColor = BE1::Color3::white;
    float                       environmentLightIntensity = 0.0f;
    RHI::Texture *              environmentLightDiffuseIrradianceCubeMap = nullptr;
    RHI::Texture *              environmentLightSpecularCubeMap = nullptr;
    uint32_t                    environmentLightSpecularMipCount = 0;

    BE1::Vec2                   jitterNDC = BE1::Vec2::zero;
    BE1::Vec2                   prevJitterNDC = BE1::Vec2::zero;

    uint32_t                    index = ~0;

    uint32_t                    maxVisSurfs = 0;
    uint32_t                    numVisSurfs = 0;
    const VisSurf **            visSurfs = nullptr;         // 카메라에 보이는 모든 surfaces 와 shadow surfaces

    uint32_t                    maxVisLocalLights = 0;
    uint32_t                    numVisLocalLights = 0;
    const VisLight **           visLocalLights = nullptr;   // 카메라에 보이는 local light 포인터 배열

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
    float                       projectionDepthA = 0.0f;
    float                       projectionDepthB = 0.0f;
    bool                        projectionOrthogonal = false;
    bool                        projectionInfiniteZFar = false;
    float                       ndcDepthScale = 1.0f;
    float                       ndcDepthBias = 0.0f;
    RenderCamera::Flag          flags = RenderCamera::Flag::None;
    bool                        directionalLightVisible = false;
    bool                        needNormalOutput = false;
    bool                        needVelocityOutput = false;

    RHI::Texture *              rtColorTexture = nullptr;
    RHI::Texture *              rtColorMSAATexture = nullptr;
    RHI::Texture *              depthTexture = nullptr;
    RHI::Texture *              rtNormalTexture = nullptr;
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
    ALIGN_AS32 BE1::Mat3x4      worldMatrix;
    const Material *            baseMaterials[2] = {};
    ALIGN_AS32 BE1::Mat3x4      prevWorldMatrix;

    MeshInstance *              meshInstance = nullptr;     ///< Mesh instance
    const Material **           materials = nullptr;
    const Skeleton *            skeleton = nullptr;         ///< Skeleton information for skeletal mesh
    VisObject *                 nextSkinnedObject = nullptr;

    uint32_t                    index;                      ///< VisObject index
    uint32_t                    numMaterials = 0;

    bool                        cameraVisible = false;
    bool                        shadowVisible = false;
};

static_assert(sizeof(VisObject) == 160, "VisObject layout changed");

// 한 프레임 렌더링에서만 유효한 카메라별 텍스트 렌더링 데이터.
// 텍스트가 있는 VisObject 에 대해서만 생성하여 일반 VisObject 의 크기와 텍스트 패스의 순회 비용을 줄인다.
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
    ALIGN_AS32 BE1::Mat4        viewProjScaleBiasMatrix;

    const VisSpotLightData *    spotLightData = nullptr;

    uint32_t                    index;                      ///< VisLight index
    uint32_t                    typeIndex;                  ///< VisLight index for same light type
    uint32_t                    packedTileMin;              ///< Packed minimum light-culling tile coordinate
    uint32_t                    packedTileMax;              ///< Packed maximum light-culling tile coordinate

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
