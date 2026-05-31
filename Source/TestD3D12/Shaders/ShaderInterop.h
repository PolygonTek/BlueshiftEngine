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

static const uint MAX_DRAW_INSTANCE_COUNT = 1024;

static const uint TEXTURE_STREAMING_FEEDBACK_RESOLUTION = 16384;
static const uint TEXTURE_STREAMING_FEEDBACK_MAX_LOD = 14;
static const uint TEXTURE_STREAMING_INVALID_INDEX = 0xFFFFFFFF;

static const uint GENMIP_2D_BLOCK_SIZE_X = 32;
static const uint GENMIP_2D_BLOCK_SIZE_Y = 32;
static const uint GENMIP_3D_BLOCK_SIZE_X = 8;
static const uint GENMIP_3D_BLOCK_SIZE_Y = 8;
static const uint GENMIP_3D_BLOCK_SIZE_Z = 8;
static const uint MIPGEN_OPTION_BIT_PRESERVE_COVERAGE = 1 << 0;
static const uint MIPGEN_OPTION_BIT_SRGB = 1 << 1;

struct MipGenParams {
    uint3 dstSize;
    uint padding;
    float3 dstSizeRcp;
    uint flags;
};

struct BindlessMaterialData {
    uint baseColorTextureIndex;
    uint streamingTextureIndex;
    uint flags;
    uint padding;
    float4 baseColorFactor;
};

struct DrawMaterialConstants {
    uint materialIndex;
};

#endif // SHADERINTEROP_H
