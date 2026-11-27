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

#ifndef __SHADERINTEROP_H__
#define __SHADERINTEROP_H__

#ifdef __cplusplus

#include <cstddef>
#include "Shaders/ShaderTypes.h"

#else

#include "Common.hlsli"
#include "ShaderPacking.hlsli"

#endif // __cplusplus

#include "Fog/FogInterop.h"

static const uint IBLIntegrationBlockSizeX = 8;
static const uint IBLIntegrationBlockSizeY = 8;

static const uint ShaderSurfaceFlagNonUniformScale = 1u << 0;
// Pack object fading into unused flag bits to preserve the 128-byte surface stride.
static const uint ShaderSurfaceDitherPatternShift = 1;
static const uint ShaderSurfaceDitherPatternMask = 63u;
static const uint ShaderSurfaceFadeOpacityShift = 16;
static const uint ShaderSkinningJointOffsetInvalid = 0xFFFFFFFF;

// Independent material properties shared by CPU and GPU code.
static const uint ShaderMaterialFlagTransparent = 1u << 0;
static const uint ShaderMaterialFlagTexCoordTransform = 1u << 1;

// Only transparent materials carry a fog blend mode; opaque fog is composited separately.
static const uint ShaderMaterialVFogBlendShift = 2;
static const uint ShaderMaterialVFogBlendMask = 3u << ShaderMaterialVFogBlendShift;
static const uint ShaderMaterialVFogBlendAlpha = 1u << ShaderMaterialVFogBlendShift;
static const uint ShaderMaterialVFogBlendPremultiplied = 2u << ShaderMaterialVFogBlendShift;
static const uint ShaderMaterialVFogBlendAdditive = 3u << ShaderMaterialVFogBlendShift;
// Upper six bits of ShaderMaterial::flags hold a stable 8x8 dither pattern offset.
static const uint ShaderMaterialDitherPatternShift = 26;

// Match RenderLight::Flag; CastVolumetricShadows requires CastShadows.
static const uint ShaderLightFlagCastShadows = 1u << 1;
static const uint ShaderLightFlagCastVolumetricShadows = 1u << 3;

static const uint ShaderLightTypeBitCount = 2;
static const uint ShaderLightTypeMask = (1u << ShaderLightTypeBitCount) - 1u;
static const uint ShaderLightTypePoint = 0;
static const uint ShaderLightTypeSpot = 1;
static const uint ShaderLightTileSize = 32;
static const uint ShaderLightTileCoordBitCount = 8;
static const uint ShaderLightTileCoordMask = (1u << ShaderLightTileCoordBitCount) - 1u;
static const uint ShaderLightWordBits = 32;
static const uint ShaderLightWordShift = 5;
static const uint ShaderLightCullingThreadGroupSizeX = 8;
static const uint ShaderLightCullingThreadGroupSizeY = 8;
static const uint ShaderLightZBinCount = 4096;
static const uint ShaderLightZBinMaxLightCount = 1u << 16;
static const uint ShaderLightZBinIndexMask = ShaderLightZBinMaxLightCount - 1u;
static const uint ShaderLightZBinEmpty = ShaderLightZBinIndexMask; // min ID = 0xFFFF, max ID = 0
static const uint ShaderLightInvalidOffset = 0xFFFFFFFFu;
static const uint ShaderLightCullingDebugModeNone = 0;
static const uint ShaderLightCullingDebugModeTileMask = 1;
static const uint ShaderLightCullingDebugModeTileMaskAndZBin = 2;

static const uint ShaderShadowDebugModeNone = 0;
static const uint ShaderShadowDebugModeVisibility = 1;
static const uint ShaderShadowDebugModeCascadeCoverage = 2;
static const uint ShaderCSMCascadeMaxCount = 4;

// Point/Spot pack a 24-bit shadow index and the full 8-bit RenderLight::Flag without changing their strides.
static const uint ShaderLocalLightFlagsShift = 24;
static const uint ShaderLocalShadowIndexMask = (1u << ShaderLocalLightFlagsShift) - 1u;
static const uint ShaderLocalShadowIndexInvalid = ShaderLocalShadowIndexMask; // Test the index bits only; flags remain valid without a shadow.

// Point shadow 는 광원 회전과 무관한 월드 축 6면을 사용한다. View 는 firstShadowView + faceIndex 로 찾는다.
static const uint ShaderPointShadowFacePositiveX = 0;
static const uint ShaderPointShadowFaceNegativeX = 1;
static const uint ShaderPointShadowFacePositiveY = 2;
static const uint ShaderPointShadowFaceNegativeY = 3;
static const uint ShaderPointShadowFacePositiveZ = 4;
static const uint ShaderPointShadowFaceNegativeZ = 5;
static const uint ShaderPointShadowFaceInvalid = 0xFFFFFFFFu;

static const uint ShaderCameraFlagProjectionOrthogonal = 1u << 0;
static const uint ShaderCameraFlagUseLightZBin = 1u << 1;
static const uint ShaderCameraFlagUseDirectionalLight = 1u << 2;
static const uint ShaderCameraFlagUseSpecularAA = 1u << 3;
static const uint ShaderCameraFlagUseScreenSpaceAO = 1u << 4;
static const uint ShaderCameraFlagUseTAA = 1u << 5;
static const uint ShaderCameraFlagUseSSR = 1u << 6;
static const uint ShaderCameraFlagUseTransparentSSR = 1u << 7;
static const uint ShaderCameraFlagUseVFog = 1u << 8; // This camera's fog volume is ready for surface shading.
static const uint ShaderCameraFlagUseHeightFog = 1u << 9; // This camera's analytic height fog is enabled for surface shading.

struct GenLDSumGGXParams {
    uint radianceCubeMapSize;
    float roughness;
    float minRadianceMipLevel;
};

#ifdef __cplusplus
static_assert(sizeof(GenLDSumGGXParams) == 12, "GenLDSumGGXParams must match the HLSL root constants");
#endif

struct GenLDSumPhongSpecularParams {
    float specularPower;
};

#ifdef __cplusplus
static_assert(sizeof(GenLDSumPhongSpecularParams) == 4, "GenLDSumPhongSpecularParams must match the HLSL root constants");
#endif

struct ReconstructCubemapSHParams {
    float3 bandFactors;
};

#ifdef __cplusplus
static_assert(sizeof(ReconstructCubemapSHParams) == 12, "ReconstructCubemapSHParams must match the HLSL root constants");
#endif

struct ShaderLightCullingParams {
    uint numLocalLights;
    uint lightRefTableOffset;
    uint tileWordTableOffset;
    uint tileCountX;
    uint tileCountY;
    uint wordsPerTile;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderLightCullingParams) == 24, "ShaderLightCullingParams must match the HLSL root constants");
static_assert(offsetof(ShaderLightCullingParams, wordsPerTile) == 20, "Light culling parameters must occupy six consecutive root constants");
#endif

struct ShaderDirectionalLight {
    float3 direction;
    uint flags;
    float3 intensityRGB; // Luminance-normalized RGB times normal illuminance (lux).
    float padding0;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderDirectionalLight) == 32, "ShaderDirectionalLight must match the HLSL constant-buffer layout");
#endif

struct ShaderPointLight {
    float4 worldToProjectionU;
    float4 worldToProjectionV;
    float3 origin;
    float invRadiusSquared;
    float3 intensityRGB; // Luminance-normalized RGB times luminous intensity (cd).
    uint shadowIndexAndFlags; // 하위 24비트: 카메라의 local shadow 테이블 인덱스, 상위 8비트: RenderLight::Flag. 하위 인덱스가 ShaderLocalShadowIndexInvalid 이면 shadow 없음.
};

#ifdef __cplusplus
static_assert(sizeof(ShaderPointLight) == 64, "ShaderPointLight must remain a tightly packed StructuredBuffer element");
#endif

struct ShaderSpotLight {
    float3 origin;
    float invRadiusSquared;
    float3 direction; // 광원에서 나가는 단위 방향.
    float cosOuterCone;
    float3 intensityRGB; // Luminance-normalized RGB times luminous intensity inside the inner cone (cd).
    float invCosConeDifference;
    uint shadowIndexAndFlags; // 하위 24비트: 카메라의 local shadow 테이블 인덱스, 상위 8비트: RenderLight::Flag. 하위 인덱스가 ShaderLocalShadowIndexInvalid 이면 shadow 없음.
};

#ifdef __cplusplus
static_assert(sizeof(ShaderSpotLight) == 52, "ShaderSpotLight must remain a tightly packed StructuredBuffer element");
#endif

struct FrameConstants {
    float frameTime;
    float elapsedTime;
    float elapsedPrevTime;
    uint frameCount;
    uint shaderMaterialTableIndex;
    uint shaderSurfaceTableIndex;
    uint shaderPointLightTableIndex;
    uint shaderSpotLightTableIndex;
    uint shaderLightRefTableIndex;
    uint shaderLightZBinTableIndex;
    uint shaderLightTileWordTableIndex;
    uint shaderLocalFogVolumeTableIndex;
    uint feedbackBufferIndex;
    uint dfgSumGGXTextureIndex;
    uint stbnUnitVec2TextureIndex;
    uint stbnVec2TextureIndex;
    float specularAAVariance;
    float specularAALimit; // Maximum additive squared perceptual roughness.
    uint shaderLocalShadowTableIndex;
    uint shaderLocalShadowViewTableIndex;
    ShaderDirectionalLight directionalLight;
};

#ifdef __cplusplus
static_assert(sizeof(FrameConstants) == 112, "FrameConstants must match the HLSL constant-buffer layout");
static_assert(offsetof(FrameConstants, shaderLocalShadowTableIndex) == 72, "Shadow table indices must occupy the former frame padding");
static_assert(offsetof(FrameConstants, shaderLocalShadowViewTableIndex) == 76, "Shadow view table index must match the HLSL offset");
static_assert(offsetof(FrameConstants, directionalLight) == 80, "Directional light constants must remain register-aligned");
#endif

// 광원 하나의 샘플링 정보. 광원 타입은 ShaderPointLight/ShaderSpotLight 에서 이미 결정된다.
// 인덱스는 프레임 공용 테이블 내 현재 카메라 구간 기준이며 CPU allocator handle 이나 영속 광원 ID 가 아니다.
struct ShaderLocalShadow {
    uint firstShadowView;       // 카메라의 view 구간에서 이 광원의 첫 요소. Point 6면도 연속된 요소를 사용한다.
    uint numShadowViews;        // Spot: 1, Point: 6. 0 이면 유효한 view 가 없다.
    float filterRadius;         // Atlas texel 단위의 PCF 반경. 업로드 시 border 안에 들어가도록 제한한다.
    float normalizedBorder;     // borderTexels / 할당된 타일 해상도. Point 는 모든 면이 같은 해상도/border 를 사용한다.
    float3 pointOrigin;         // Point depth 기록 당시의 광원 위치. 조명 계산의 현재 위치와 별개다.
    float2 pointDepthScaleBias; // Point 면의 전방 거리 d 로 depth = X + Y / d 를 계산한다.
};

// Spot 한 개 또는 Point 한 면의 샘플링 정보. 렌더링용 ShadowViewConstants 와 별도다.
struct ShaderLocalShadowView {
    // world position -> homogeneous tile coordinate. XYZ 를 W 로 나눈 뒤 XY 는 타일 전체의 UV (Y down), Z 는 비교 depth [0, 1].
    // border 를 포함한 reverse-Z 렌더링 투영을 사용하며 near = 1, far = 0 이다. perspective divide 전 W <= 0 은 샘플링하지 않는다.
    // 포인트 라이트는 보존한 위치와 depth 계수로 좌표를 직접 계산하므로 이 행렬을 사용하지 않는다.
    row_major float4x4 worldToShadowCoordMatrix;
    float4 atlasScaleBias;      // XY = tileSize / atlasSize, ZW = tileOrigin / atlasSize. atlasUV = tileUV * XY + ZW.
};

// 현재 카메라의 atlas 와 프레임 공용 shadow/view 테이블 내 구간을 참조하는 샘플링 데이터.
struct ShaderShadowAtlasData {
    uint textureIndex;          // Texture2D<float> 의 bindless SRV index.
    uint shadowTableOffset;     // 프레임 공용 ShaderLocalShadow 테이블에서 이 카메라의 첫 요소.
    uint shadowViewTableOffset; // 프레임 공용 ShaderLocalShadowView 테이블에서 이 카메라의 첫 요소.
    uint numShadows;            // 0 이면 atlas/테이블에 접근하지 않는다. 업로드와 depth 기록을 마친 뒤에만 활성화한다.
    float2 texelSize;           // 1 / atlas 크기. 모든 타일의 PCF offset 에 공유한다.
    uint numShadowViews;
    uint padding;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderLocalShadow) == 36, "ShaderLocalShadow must match the HLSL structured-buffer stride");
static_assert(sizeof(ShaderLocalShadowView) == 80, "ShaderLocalShadowView must match the HLSL structured-buffer stride");
static_assert(sizeof(ShaderShadowAtlasData) == 32, "ShaderShadowAtlasData must match the HLSL constant-buffer layout");
static_assert(offsetof(ShaderShadowAtlasData, shadowTableOffset) == 4, "Shadow table offset must match the HLSL layout");
static_assert(offsetof(ShaderShadowAtlasData, shadowViewTableOffset) == 8, "Shadow view table offset must match the HLSL layout");
#endif

// CSM 을 샘플링하는 데 필요한 데이터.
struct ShaderCSMData {
    // world position -> shadow texture coordinate.
    // XY 는 UV, Z 는 비교할 depth [0, 1].
    // orthogonal projection 이므로 perspective divide 없이 float3x4 로 표현할 수 있다.
    row_major float3x4 worldToShadowCoordMatrices[ShaderCSMCascadeMaxCount];

    float4 cascadeFilterRadii;  // 현재 해상도와 cascade 크기에 맞춘 texel 반경.

    uint textureIndex;          // Texture2DArray<float> 의 bindless SRV index.
    uint cascadeCount;          // 0 이면 shadow texture 에 접근하지 않는다. Depth 기록 후에만 활성화한다.
    float2 texelSize;

    float normalizedBorder;     // normalized border width in [0, 1] range.
    float invBlendWidth;        // normalized inverse blend width.
    float farFadeStart;
    float invFarFadeRange;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderCSMData) == 240, "CSM data must match the HLSL constant-buffer layout");
static_assert(offsetof(ShaderCSMData, cascadeFilterRadii) == 192, "CSM filter radii must remain register-aligned");
static_assert(offsetof(ShaderCSMData, textureIndex) == 208, "CSM texture index must match the HLSL offset");
static_assert(offsetof(ShaderCSMData, texelSize) == 216, "CSM texel size must share a register with the texture index and cascade count");
static_assert(offsetof(ShaderCSMData, normalizedBorder) == 224, "CSM blend and fade parameters must occupy one register");
#endif

struct CameraConstants {
    row_major float4x4 viewProjMatrix;
    row_major float4x4 invViewProjMatrix;
    row_major float4x4 unjitteredViewProjMatrix;

    row_major float4x4 prevViewProjMatrix;
    row_major float4x4 prevInvViewProjMatrix;
    row_major float4x4 prevUnjitteredViewProjMatrix;

    row_major float4x4 hiZCullViewProjMatrix;
    row_major float3x4 viewRotation; // World-to-view directions; fourth column is zero (16-byte row alignment).

    float3 cameraOrigin;
    float depthClearValue;

    float3 cameraForward;
    uint flags;

    float3 prevCameraOrigin;
    uint cameraUnchanged; // Valid previous camera with unchanged unjittered projection, position and orientation.

    float3 prevCameraForward;
    float fogAnisotropy; // Shared Henyey-Greenstein g for height fog and local fog volumes.

    float4 projectionScaleBiasXY; // clip.xy = view.xy * projectionScaleBiasXY.xy + clip.w * projectionScaleBiasXY.zw (including jitter).
    float4 uvToViewScaleBiasXY;   // view.xy = (uv * uvToViewScaleBiasXY.xy + uvToViewScaleBiasXY.zw) * clip.w (top-left UV in [0, 1], independent of pass resolution).

    float2 renderSize;           // Size of this camera's main render target in pixels.
    float2 renderSizeRcp;        // Reciprocal size of this camera's main render target.

    float2 uvJitter;             // UV-space jitter offset; rasterUV - uvJitter gives unjittered UV.

    // Use the remaining two scalars in this register for the camera's range in the shared local fog table.
    uint localFogVolumeTableOffset;
    uint numLocalFogVolumes;

    // Pack depth coefficients into one 16-byte constant-buffer register.
    float2 projectionScaleBiasZ;  // clip.z = view.z * projectionScaleBiasZ.x + projectionScaleBiasZ.y
    float lightCullingZBinScale;
    float lightCullingZBinBias;

    uint numLocalLights;
    uint lightCullingLightRefTableOffset;
    uint lightCullingZBinTableOffset;
    uint lightCullingTileWordTableOffset;
    uint lightCullingTileCountX;
    uint lightCullingTileCountY;
    uint lightCullingWordsPerTile;
    uint lightCullingDebugMode;
    uint lightCullingPadding;

    uint ssrUpsampledReflectionTextureIndex;
    uint transparentSSRUpsampledReflectionTextureIndex;
    uint transparentSSRDepthTextureIndex;

    row_major float3x4 envLightAxis;
    float3 envLightColor;
    float envLightIntensity;

    uint envLightDiffuseIrradianceCubeMapIndex;
    uint envLightSpecularCubeMapIndex;
    uint envLightSpecularMipCount;
    uint aoTextureIndex;

    ShaderCSMData csm;
    ShaderShadowAtlasData shadowAtlas;
    float4 skyboxRadianceScale;

    HeightFogParams heightFogParams;
    VFogParams vfogParams;
    uint vfogSourceTextureIndex;     // Zero until this camera's fog dispatch succeeds.
    uint vfogIntegratedTextureIndex;
    float fogNearDepth;            // Shared fog start depth (-view Z), independent of volume availability.
    uint envLightRadianceSHBufferIndex; // Bindless Buffer<float4>, 9 RGB SH coefficients. Zero when environment lighting or SH is unavailable.

#ifndef __cplusplus
    inline bool IsOrthogonal() {
        return (flags & ShaderCameraFlagProjectionOrthogonal) != 0u;
    }

    inline float GetLinearDepth(float depth) {
        return ToLinearDepth(depth, projectionScaleBiasZ, IsOrthogonal());
    }

    inline float3 GetViewPosition(float2 uv, float linearDepth) {
        return ReconstructViewPosition(uv, linearDepth, uvToViewScaleBiasXY, IsOrthogonal());
    }

    inline float4 GetClipPosition(float3 viewPosition) {
        return ProjectViewPosition(viewPosition, projectionScaleBiasXY, projectionScaleBiasZ, IsOrthogonal());
    }

    inline float3 GetScreenPosition(float4 clipPosition) {
        return ClipPositionToScreenPosition(clipPosition);
    }
#endif
};

#ifdef __cplusplus
static_assert(sizeof(CameraConstants) == 1184, "CameraConstants must match the HLSL constant-buffer layout");
static_assert(offsetof(CameraConstants, fogAnisotropy) == 556, "Shared fog anisotropy must match HLSL");
#endif

struct SurfaceDrawParams {
    uint instanceBaseOffset;
};

struct ShaderSkinningJoint {
    row_major float3x4 matrix;
};

struct ShaderSurface {
    row_major float3x4 worldMatrix;
    row_major float3x4 prevWorldMatrix;

    float4 localBoundingSphere;
    uint shaderMaterialIndex;
    uint flags; // ShaderSurfaceFlag* bits, object dither pattern and UNorm16 fade opacity.
    uint skinningJointOffset; // Element offset in the camera joint buffer; ShaderSkinningJointOffsetInvalid if unavailable.
    uint prevSkinningJointOffset; // Previous rendered pose; aliases the current offset when history is unavailable.
};

#ifdef __cplusplus
static_assert(sizeof(ShaderSkinningJoint) == 48, "ShaderSkinningJoint must match the joint buffer stride");
static_assert(sizeof(ShaderSurface) == 128, "ShaderSurface must match the HLSL structured-buffer layout");
#endif

struct ShaderMaterial {
    uint baseColorTextureIndex;
    uint normalTextureIndex;
    uint emissiveTextureIndex;
    uint metallicTextureIndex;
    uint roughnessTextureIndex;
    uint aoTextureIndex;
    uint clearCoatTextureIndex;
    uint clearCoatRoughnessTextureIndex;
    uint clearCoatNormalTextureIndex;
    uint heightTextureIndex;
    uint samplerIndex;
    uint streamingMaterialIndex;

    uint packedBaseColorFactor;
    float alphaCutoff;
    uint2 packedEmissionColorFactor;
    uint packedMetallicRoughnessFactors;
    float aoStrength;
    uint packedClearCoatFactors;
    float heightScale;
    uint2 packedTexCoordTransform;
    uint flags; // ShaderMaterialFlag* bitmask and dither pattern offset.

#ifndef __cplusplus
    inline float4 GetBaseColorFactor() { return UnpackRGBA8UNormToFloat4(packedBaseColorFactor); }
    inline float4 GetEmissionColorFactor() { return UnpackRGBA16FToFloat4(packedEmissionColorFactor); }
    inline float2 GetMetallicRoughnessFactors() { return UnpackRG16UNormToFloat2(packedMetallicRoughnessFactors); }
    inline float2 GetClearCoatFactors() { return UnpackRG16UNormToFloat2(packedClearCoatFactors); }
    inline float2 TransformTexCoord(float2 texCoord) {
        if ((flags & ShaderMaterialFlagTexCoordTransform) == 0u) {
            return texCoord;
        }

        const float4 texCoordTransform = UnpackRGBA16FToFloat4(packedTexCoordTransform);
        return texCoord * texCoordTransform.xy + texCoordTransform.zw;
    }
#endif
};

#ifdef __cplusplus
static_assert(sizeof(ShaderMaterial) == 92, "ShaderMaterial must remain a tightly packed StructuredBuffer element");
#endif

struct ShaderLightRef {
    uint packedLightRef;   // low 2 bits: light type, remaining bits: type-local light index
    uint packedTileBounds; // 8 bits each, low to high: min X, min Y, max X, max Y
};

#ifdef __cplusplus
static_assert(sizeof(ShaderLightRef) == 8, "ShaderLightRef must remain a tightly packed StructuredBuffer element");
#endif

#endif // SHADERINTEROP_H
