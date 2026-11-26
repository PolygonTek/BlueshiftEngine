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

#ifndef __HIZ_INTEROP_H__
#define __HIZ_INTEROP_H__

#include "../ShaderTypes.h"

static const uint GenHiZMipBlockSizeX = 8;
static const uint GenHiZMipBlockSizeY = 8;
static const uint HiZDebugPreviewBlockSizeX = 16;
static const uint HiZDebugPreviewBlockSizeY = 16;

static const uint HiZCullThreadCount = 64;
static const uint HiZCullIndirectArgInstanceCountOffset = 4;
static const uint HiZCullIndirectArgSize = 20;

struct HiZMipGenParams {
    uint2 inputSize;
    float2 inputSizeRcp;
    uint2 outputSize;
    uint outputMipCount;
    uint reverseZ;
};

#ifdef __cplusplus
static_assert(sizeof(HiZMipGenParams) == 32, "HiZMipGenParams must match the eight HLSL root constants");
#endif

struct HiZDebugPreviewParams {
    uint2 inputSize;
    uint2 outputSize;
    float linearDepthScale;
};

#ifdef __cplusplus
static_assert(sizeof(HiZDebugPreviewParams) == 20, "HiZDebugPreviewParams must match the HLSL root constants");
#endif

struct HiZCullInstancesParams {
    uint inputInstanceCount;
    uint inputInstanceBaseOffset;
    uint hiZWidth;
    uint hiZHeight;
    uint hiZMipCount;
    uint reverseZ;
    uint instanceBaseOffset;
    uint indirectArgOffset;
};

#ifdef __cplusplus
static_assert(sizeof(HiZCullInstancesParams) == 32, "Hi-Z culling parameters must match the eight HLSL root constants");
#endif

#endif // __HIZ_INTEROP_H__
