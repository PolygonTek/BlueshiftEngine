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

#ifndef __AO_INTEROP_H__
#define __AO_INTEROP_H__

#include "../ShaderTypes.h"

static const uint AOUpsampleBlockSizeX = 8;
static const uint AOUpsampleBlockSizeY = 8;

static const uint GTAODepthMipLevelCount = 5;
static const uint GTAOPrefilterBlockSizeX = 8;
static const uint GTAOPrefilterBlockSizeY = 8;
static const uint GTAOMainBlockSizeX = 8;
static const uint GTAOMainBlockSizeY = 8;
static const uint GTAODenoiseBlockSizeX = 8;
static const uint GTAODenoiseBlockSizeY = 8;

static const uint GTAOQualityLevelCount = 4;
static const uint GTAOQualityLowSliceCount = 1;
static const uint GTAOQualityLowStepsPerSlice = 3;
static const uint GTAOQualityMediumSliceCount = 2;
static const uint GTAOQualityMediumStepsPerSlice = 3;
static const uint GTAOQualityHighSliceCount = 3;
static const uint GTAOQualityHighStepsPerSlice = 3;
static const uint GTAOQualityUltraSliceCount = 9;
static const uint GTAOQualityUltraStepsPerSlice = 3;

struct GTAOPrefilterParams {
    uint2 inputSize;
    uint2 outputSize;
    uint resolutionScale;
    uint reverseZ;
    uint depthMipCount;
    float radius;
    float falloffRange;
    float radiusMultiplier;
    float clearDepth;
};

#ifdef __cplusplus
static_assert(sizeof(GTAOPrefilterParams) == 44, "GTAOPrefilterParams must match the HLSL root constants");
#endif

struct GTAOMainParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 outputSizeRcp;
    uint resolutionScale;
    uint noiseIndex;
    uint depthMipCount;
    float radius;
    float falloffRange;
    float radiusMultiplier;
    float sampleDistributionPower;
    float thinOccluderCompensation;
    float depthMipSamplingOffset;
    float power;
    float clearDepth;
};

#ifdef __cplusplus
static_assert(sizeof(GTAOMainParams) == 68, "GTAOMainParams must match the HLSL root constants");
#endif

struct GTAODenoiseParams {
    uint2 outputSize;
    float2 outputSizeRcp;
    float blurBeta;
    uint finalPass;
};

#ifdef __cplusplus
static_assert(sizeof(GTAODenoiseParams) == 24, "GTAODenoiseParams must match the HLSL root constants");
#endif

struct GTAODebugParams {
    uint2 inputSize;
    uint2 outputSize;
    uint sourceType;
    uint mipLevel;
    float linearDepthScale;
};

#ifdef __cplusplus
static_assert(sizeof(GTAODebugParams) == 28, "GTAODebugParams must match the HLSL root constants");
#endif

struct AOUpsampleParams {
    uint2 inputSize;
    uint2 outputSize;
    float depthSigmaRcp;
    float clearDepth;
};

#ifdef __cplusplus
static_assert(sizeof(AOUpsampleParams) == 24, "AOUpsampleParams must match the HLSL root constants");
#endif

#endif // __AO_INTEROP_H__
