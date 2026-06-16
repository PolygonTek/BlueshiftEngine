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

#ifndef _SHADERINTEROP_H_
#define _SHADERINTEROP_H_

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

static const uint LuminanceBlockSize = 32;
static const uint LuminanceHistogramBinCount = LuminanceBlockSize * LuminanceBlockSize;
static const uint LuminanceBufferOffsetExposure = 0;
static const uint LuminanceBufferOffsetLuminance = LuminanceBufferOffsetExposure + 4;
static const uint LuminanceBufferOffsetHistogram = LuminanceBufferOffsetLuminance + 4;

static const uint ToneMappingBlockSizeX = 16;
static const uint ToneMappingBlockSizeY = 16;
static const uint BloomBlockSizeX = 16;
static const uint BloomBlockSizeY = 16;
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

struct MipGenParams {
    uint3 dstSize;
    uint padding;
    float3 dstSizeRcp;
    uint flags;
};

struct PostProcessParams {
    uint2 resolution;
    float2 resolutionRcp;
    float4 params0;
    float4 params1;
};

#define PPP_LUMINANCE_LOG_MIN postProcessParams.params0.x
#define PPP_LUMINANCE_LOG_RANGE_RCP postProcessParams.params0.y
#define PPP_LUMINANCE_PIXEL_COUNT postProcessParams.params0.z
#define PPP_LUMINANCE_ADAPTATION_RATE postProcessParams.params0.w
#define PPP_LUMINANCE_MIDDLE_GRAY postProcessParams.params1.x
#define PPP_LUMINANCE_LOG_RANGE postProcessParams.params1.y

struct ToneMappingParams {
    uint2 outputSize;
    float exposureScale;
    uint toneMappingOperator;
    float bloomIntensity;
    float3 padding0;
};

struct BloomParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 inputTexelSize;
    float2 outputTexelSize;
    float4 params0;
};

#define BP_BLOOM_THRESHOLD bloomParams.params0.x
#define BP_BLOOM_SOFT_THRESHOLD bloomParams.params0.y
#define BP_BLOOM_PREFILTER_QUALITY bloomParams.params0.z
#define BP_BLOOM_WEIGHT bloomParams.params0.z
#define BP_BLOOM_RADIUS bloomParams.params0.w

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
    uint surfaceTableIndex;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct ShaderSurface {
    row_major float3x4 worldMatrix;
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
};

#endif // SHADERINTEROP_H
