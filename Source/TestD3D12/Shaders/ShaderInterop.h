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

static const uint LuminanceBlockSize = 32;
static const uint LuminanceHistogramBinCount = LuminanceBlockSize * LuminanceBlockSize;
static const uint LuminanceBufferOffsetExposure = 0;
static const uint LuminanceBufferOffsetLuminance = LuminanceBufferOffsetExposure + 4;
static const uint LuminanceBufferOffsetHistogram = LuminanceBufferOffsetLuminance + 4;

static const uint ToneMappingBlockSizeX = 16;
static const uint ToneMappingBlockSizeY = 16;

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

struct ToneMappingParams {
    uint2 outputSize;
    float2 outputTexelSize;
    float exposureScale;
    uint toneMappingOperator;
    float bloomIntensity;
    float padding0;
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

struct FrameConstants {
    float frameTime;
    float elapsedTime;
    float elapsedPrevTime;
    uint frameCount;
    uint shaderMaterialTableIndex;
    uint feedbackBufferIndex;
    uint padding0;
    uint padding1;
};

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
    uint padding1;
    uint padding2;
};

struct SurfaceDrawParams {
    uint instanceBaseOffset;
};

struct ShaderSurface {
    row_major float3x4 worldMatrix;
    row_major float3x4 prevWorldMatrix;

    float4 localBoundingSphere;
    uint shaderMaterialIndex;
    uint padding0;
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

#endif // SHADERINTEROP_H
