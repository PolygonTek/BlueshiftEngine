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

#include <DirectXMath.h>

using float2 = DirectX::XMFLOAT2;
using float3 = DirectX::XMFLOAT3;
using float4 = DirectX::XMFLOAT4;
using float3x4 = DirectX::XMFLOAT3X4;
using float3x3 = DirectX::XMFLOAT3X3;
using float4x4 = DirectX::XMFLOAT4X4;
using int2 = DirectX::XMINT2;
using int3 = DirectX::XMINT3;
using int4 = DirectX::XMINT4;
using uint = uint32_t;
using uint2 = DirectX::XMUINT2;
using uint3 = DirectX::XMUINT3;
using uint4 = DirectX::XMUINT4;

#define column_major
#define row_major

#else

#endif // __cplusplus

static const uint TextureStreamingFeedbackResolution = 16384;
static const uint TextureStreamingFeedbackMaxLOD = 14;
static const uint TextureStreamingInvalidIndex = 0xFFFFFFFF;

static const uint GenMip2DBlockSizeX = 32;
static const uint GenMip2DBlockSizeY = 32;
static const uint GenMip3DBlockSizeX = 8;
static const uint GenMip3DBlockSizeY = 8;
static const uint GenMip3DBlockSizeZ = 8;
static const uint MipGenOptionBitPreserveCoverage = 1 << 0;
static const uint MipGenOptionBitSRGB = 1 << 1;

static const uint DepthHistoryResolveBlockSizeX = 16;
static const uint DepthHistoryResolveBlockSizeY = 16;
static const uint SurfaceOutputMSAAResolveBlockSizeX = 16;
static const uint SurfaceOutputMSAAResolveBlockSizeY = 16;

static const uint LuminanceBlockSize = 32;
static const uint LuminanceHistogramBinCount = LuminanceBlockSize * LuminanceBlockSize;
static const uint LuminanceBufferOffsetExposure = 0;
static const uint LuminanceBufferOffsetLuminance = LuminanceBufferOffsetExposure + 4;
static const uint LuminanceBufferOffsetHistogram = LuminanceBufferOffsetLuminance + 4;

static const uint FinalPostProcessBlockSizeX = 16;
static const uint FinalPostProcessBlockSizeY = 16;
static const uint ColorGradingLUTSize = 32;

static const uint FinalPostProcessFlagToneMapping = 1u << 0;
static const uint FinalPostProcessFlagDithering = 1u << 1;
static const uint FinalPostProcessFlagColorGrading = 1u << 2;

static const uint TAAResolveBlockSizeX = 16;
static const uint TAAResolveBlockSizeY = 16;

static const uint ToneMappingOperatorLinear = 0;
static const uint ToneMappingOperatorExponential = 1;
static const uint ToneMappingOperatorLogarithmic = 2;
static const uint ToneMappingOperatorDragoLogarithmic = 3;
static const uint ToneMappingOperatorReinhard = 4;
static const uint ToneMappingOperatorReinhardExtended = 5;
static const uint ToneMappingOperatorFilmicALU = 6;
static const uint ToneMappingOperatorFilmicACES = 7;
static const uint ToneMappingOperatorFilmicUnreal3 = 8;
static const uint ToneMappingOperatorFilmicUncharted2 = 9;
static const uint ToneMappingOperatorFilmicGT = 10;

static const uint BloomBlockSizeX = 16;
static const uint BloomBlockSizeY = 16;

static const uint ConvolutionBloomApertureEnergyReduceBlockSize = 256;

static const uint GenHiZMipBlockSizeX = 8;
static const uint GenHiZMipBlockSizeY = 8;
static const uint HiZDebugPreviewBlockSizeX = 16;
static const uint HiZDebugPreviewBlockSizeY = 16;

static const uint HiZCullThreadCount = 64;
static const uint HiZCullIndirectArgInstanceCountOffset = 4;
static const uint HiZCullIndirectArgSize = 20;

static const uint ShaderSurfaceFlagNonUniformScale = 1u << 0;

static const uint ShaderLightTypeBitCount = 2;
static const uint ShaderLightTypeMask = (1u << ShaderLightTypeBitCount) - 1u;
static const uint ShaderLightTypePoint = 0;
static const uint ShaderLightTypeSpot = 1;
static const uint ShaderLightTypeBoxDir = 2;
static const uint ShaderLightTileSize = 32;
static const uint ShaderLightTileCoordBitCount = 16;
static const uint ShaderLightTileCoordMask = (1u << ShaderLightTileCoordBitCount) - 1u;
static const uint ShaderLightMaskWordBits = 32;
static const uint ShaderLightMaskWordShift = 5;
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

static const uint ShaderCameraFlagProjectionOrthogonal = 1u << 0;
static const uint ShaderCameraFlagUseLightZBin = 1u << 1;

static const uint MultiplySpectrum2DBlockSize = 256;

static const uint FFTRadix2_256StageCount = 8;
static const uint FFTRadix2_256ButterflyCount = 256 / 2;
static const uint FFTRadix2_256BlockSize = FFTRadix2_256ButterflyCount;
static const uint FFTRadix4_256StageCount = 4;
static const uint FFTRadix4_256ButterflyCount = 256 / 4;
static const uint FFTRadix4_256BlockSize = FFTRadix4_256ButterflyCount;
static const uint FFTRadix8_512StageCount = 3;
static const uint FFTRadix8_512ButterflyCount = 512 / 8;
static const uint FFTRadix8_512BlockSize = FFTRadix8_512ButterflyCount;
static const uint FFTRadix8_1024StageCount = 3;
static const uint FFTRadix8_1024ButterflyCount = 1024 / 8;
static const uint FFTRadix8_1024BlockSize = FFTRadix8_1024ButterflyCount;
static const uint FFTRadix2_1024ButterflyCount = 1024 / 2;
static const uint FFTRadix2_1024BlockSize = FFTRadix2_1024ButterflyCount;

static const uint TransposeComplexTileSize = 16;

struct MipGenParams {
    uint3 dstSize;
    uint padding;
    float3 dstSizeRcp;
    uint flags;
};

struct LuminancePass1Params {
    uint2 sampleSize;
    float2 sampleSizeRcp;
    float luminanceLogMin;
    float luminanceLogRangeRcp;
};

struct LuminancePass2Params {
    float luminanceLogMin;
    float luminanceLogRange;
    float luminancePixelCount;
    float luminanceAdaptationRate;
    float middleGray;
    float3 padding0;
};

struct FinalPostProcessParams {
    uint2 outputSize;
    float2 outputTexelSize;
    uint flags;
    uint toneMappingOperator;
    float bloomIntensity;
    float chromaticAberration;
    float vignetteIntensity;
    float lensDistortion;
    float lensDistortionFitScale;
    float lensDistortionRadiusSqScale;
    float3 whiteBalance;
    float bloomDirtIntensity;
    float2 bloomDirtUvScale;
    float2 padding0;
};

struct TAAResolveParams {
    uint2 outputSize;
    float2 outputTexelSize;
    float historyWeight;
    uint historyValid;
    uint useMinDepth;
    uint orthogonal;
    float2 projectionDepthParams;
    float2 ndcDepthScaleBias;
    float depthFallbackThreshold;
    float maxSubpixelCurrentWeight;
    float luminanceWeight;
    float padding0;
};

struct DepthHistoryResolveParams {
    uint2 outputSize;
    uint useMinDepth;
    uint padding0;
};

struct SurfaceOutputMSAAResolveParams {
    uint2 outputSize;
    uint useMinDepth;
    uint padding0;
};

struct FullscreenImageParams {
    float2 texCoordScale;
    float2 texCoordOffset;
};

struct BloomParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 inputTexelSize;
    float2 outputTexelSize;
    float4 params0;
    float4 params1;
};

#define BP_BLOOM_THRESHOLD bloomParams.params0.x
#define BP_BLOOM_SOFT_THRESHOLD bloomParams.params0.y
#define BP_BLOOM_PREFILTER_QUALITY bloomParams.params0.z
#define BP_BLOOM_WEIGHT bloomParams.params0.z
#define BP_BLOOM_RADIUS bloomParams.params0.w
#define BP_BLOOM_CONTENT_OFFSET bloomParams.params1.xy
#define BP_BLOOM_CONTENT_SIZE bloomParams.params1.zw
#define BP_BLOOM_KERNEL_SCALE bloomParams.params0.x
#define BP_BLOOM_KERNEL_ROTATION_COS bloomParams.params0.x
#define BP_BLOOM_KERNEL_ROTATION_SIN bloomParams.params0.y
#define BP_BLOOM_KERNEL_SPATIAL_SCALE bloomParams.params0.zw
#define BP_BLOOM_DIFFRACTION_PSF_SCALE bloomParams.params0.x
#define BP_BLOOM_DISPERSION_AMOUNT bloomParams.params0.x
#define BP_BLOOM_DISPERSION_MIN_WAVELENGTH_NM bloomParams.params0.y
#define BP_BLOOM_DISPERSION_MAX_WAVELENGTH_NM bloomParams.params0.z
#define BP_BLOOM_DISPERSION_REFERENCE_WAVELENGTH_NM bloomParams.params0.w
#define BP_BLOOM_DISPERSION_SAMPLE_COUNT bloomParams.params1.x
#define BP_BLOOM_DISPERSION_INTENSITY_SCALE bloomParams.params1.y

struct HiZMipGenParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 inputSizeRcp;
    uint outputMipCount;
    uint useMinDepth;
};

struct HiZDebugPreviewParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 projectionDepthParams;
    float2 ndcDepthScaleBias;
    float linearDepthScale;
    uint orthogonal;
    uint padding0;
    uint padding1;
};

struct HiZCullInstancesParams {
    uint inputInstanceCount;
    uint surfaceTableIndex;
    uint hiZWidth;
    uint hiZHeight;
    uint hiZMipCount;
    uint useMinDepth;
    uint instanceBaseOffset;
    uint indirectArgOffset;
};

struct MultiplySpectrum2DParams {
    uint fftSize;
    uint planeCount;
    float scale;
    uint padding0;
};

struct FFTStageParams {
    float sign;
    float scale;
    uint padding0;
    uint padding1;
};

struct ShaderLightCullingParams {
    uint numLights;
    uint lightRefTableOffset;
    uint tileMaskWordTableOffset;
    uint tileCountX;
    uint tileCountY;
    uint wordCountPerTile;
    uint padding0;
    uint padding1;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderLightCullingParams) == 32, "ShaderLightCullingParams must match the HLSL constant-buffer layout");
#endif

struct FrameConstants {
    float frameTime;
    float elapsedTime;
    float elapsedPrevTime;
    uint frameCount;
    uint shaderMaterialTableIndex;
    uint shaderPointLightTableIndex;
    uint shaderSpotLightTableIndex;
    uint shaderBoxDirLightTableIndex;
    uint shaderLightRefTableIndex;
    uint shaderLightZBinTableIndex;
    uint shaderLightTileMaskWordTableIndex;
    uint feedbackBufferIndex;
};

#ifdef __cplusplus
static_assert(sizeof(FrameConstants) == 48, "FrameConstants must match the HLSL constant-buffer layout");
#endif

struct CameraConstants {
    row_major float4x4 viewProjMatrix;
    row_major float4x4 invViewProjMatrix;
    row_major float4x4 unjitteredViewProjMatrix;

    row_major float4x4 prevViewProjMatrix;
    row_major float4x4 prevInvViewProjMatrix;
    row_major float4x4 prevUnjitteredViewProjMatrix;

    row_major float4x4 hiZCullViewProjMatrix;

    uint surfaceTableIndex;
    float ndcFarZ;
    float projectionDepthA;
    float projectionDepthB;
    uint flags;

    uint numLights;
    uint lightCullingLightRefTableOffset;
    uint lightCullingTileMaskWordTableOffset;
    uint lightCullingZBinTableOffset;
    float lightCullingZBinScale;
    float lightCullingZBinBias;
    uint lightCullingTileCountX;
    uint lightCullingTileCountY;
    uint lightCullingWordCountPerTile;
    uint lightCullingDebugMode;
    uint padding0;
};

#ifdef __cplusplus
static_assert(sizeof(CameraConstants) == 512, "CameraConstants must match the HLSL constant-buffer layout");
#endif

struct SurfaceDrawParams {
    uint instanceBaseOffset;
};

struct ShaderSurface {
    row_major float3x4 worldMatrix;
    row_major float3x4 prevWorldMatrix;

    float4 localBoundingSphere;
    uint shaderMaterialIndex;
    uint flags;
    uint padding1;
    uint padding2;
};

struct ShaderMaterial {
    uint baseColorTextureIndex;
    uint normalTextureIndex;
    uint emissiveTextureIndex;
    uint streamingMaterialIndex;
    float4 baseColorFactor;
    float alphaCutoff;
    float3 padding0;
};

struct ShaderLightRef {
    uint packedLightRef; // low 2 bits: light type, remaining bits: type-local light index
    uint packedTileMin;  // low 16 bits: tile X, high 16 bits: tile Y
    uint packedTileMax;  // low 16 bits: tile X, high 16 bits: tile Y
};

#ifdef __cplusplus
static_assert(sizeof(ShaderLightRef) == 12, "ShaderLightRef must remain a tightly packed StructuredBuffer element");
#endif

struct ShaderPointLight {
    float4 worldToProjectionU;
    float4 worldToProjectionV;
    float3 origin;
    float invRadiusSquared;
    float3 lightColor;
    float fallOffExponent;
    uint flags;
    float3 padding0;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderPointLight) == 80, "ShaderPointLight must remain a tightly packed StructuredBuffer element");
#endif

struct ShaderSpotLight {
    row_major float4x4 viewProjScaleBiasMatrix;
    row_major float3x4 fallOffMatrix;
    float3 origin;
    float fallOffExponent;
    float3 direction;
    float zNear;
    float3 size;
    uint flags;
    float3 lightColor;
    float padding0;
};

struct ShaderBoxDirLight {
    row_major float4x4 worldToLightVolumeMatrix;
    float3 origin;
    uint flags;
    float3 direction;
    float fallOffExponent;
    float3 size;
    float padding1;
    float3 lightColor;
    float padding2;
};

#endif // SHADERINTEROP_H
