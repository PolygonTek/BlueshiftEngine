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

#ifndef __SSR_INTEROP_H__
#define __SSR_INTEROP_H__

#include "../ShaderTypes.h"

static const uint SSRHiZBlockSizeX = 8;
static const uint SSRHiZBlockSizeY = 8;

static const uint SSRTraceBlockSizeX = 8;
static const uint SSRTraceBlockSizeY = 8;
static const uint SSRRadianceResolveBlockSizeX = 8;
static const uint SSRRadianceResolveBlockSizeY = 8;
static const uint SSRTemporalBlockSizeX = 8;
static const uint SSRTemporalBlockSizeY = 8;
static const uint SSRUpsampleBlockSizeX = 8;
static const uint SSRUpsampleBlockSizeY = 8;
static const uint SSRSceneColorHistoryBlockSizeX = 8;
static const uint SSRSceneColorHistoryBlockSizeY = 8;

static const uint ScreenSpaceRayTraceStatusMiss = 0;
static const uint ScreenSpaceRayTraceStatusHit = 1;
static const uint ScreenSpaceRayTraceStatusMaxIterations = 2;

// Positive values are valid trace confidence. Negative values retain invalid-result diagnostics.
static const float SSRTraceValidityMiss = 0.0f;
static const float SSRTraceValidityHit = 1.0f;
static const float SSRTraceValidityConfidenceRejected = -0.25f; // Hit with zero final confidence; retains the traced-surface temporal behavior.
static const float SSRTraceValidityHistoryRejected = -0.5f; // A traced surface; temporal history may still fill the miss.
static const float SSRTraceValidityMaxIterations = -1.0f;
static const float SSRTraceValidityNoSurface = -2.0f;
static const float SSRTraceValidityRoughnessRejected = -3.0f;

static const uint SSRDebugModeOff = 0;
static const uint SSRDebugModeRawReflection = 1;
static const uint SSRDebugModeUpsampledReflection = 2;
static const uint SSRDebugModeTraceStatus = 3;
static const uint SSRDebugModeHitUV = 4;
static const uint SSRDebugModeConfidence = 5;
static const uint SSRDebugModeIterations = 6;

struct SSRHiZMipParams {
    uint2 inputSize;
    uint2 outputSize;
    uint2 validOutputSize;
    float clearDepth;
    uint reverseZ;
};

#ifdef __cplusplus
static_assert(sizeof(SSRHiZMipParams) == 32, "SSRHiZMipParams must match the HLSL root constants");
#endif

struct SSRHiZMipBatchParams {
    uint2 inputSize;
    uint2 outputSize;
    uint outputMipCount;
};

#ifdef __cplusplus
static_assert(sizeof(SSRHiZMipBatchParams) == 20, "SSRHiZMipBatchParams must match the five HLSL root constants");
#endif

struct SSRSceneColorHistoryMipParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 inputTexelSize;
    float2 outputTexelSize;
    uint compressInput;
};

#ifdef __cplusplus
static_assert(sizeof(SSRSceneColorHistoryMipParams) == 36, "SSRSceneColorHistoryMipParams must match the HLSL root constants");
#endif

struct SSRTraceParams {
    uint2 inputSize;
    uint2 outputSize;
    uint reverseZ;
    uint debugMode;
    float2 uvJitterDelta; // currUVJitter - prevUVJitter
    uint ssrHiZMipCount;
    uint maxIterations;
    float thickness;
    float rayRoughnessScale;
    float screenEdgeFade;
    float roughnessFadeStart;
    float roughnessFadeEnd;
    uint noiseIndex;
};

#ifdef __cplusplus
static_assert(sizeof(SSRTraceParams) == 64, "SSRTraceParams must match the HLSL root constants");
#endif

struct SSRRadianceResolveParams {
    uint2 outputSize;
    uint debugMode;
    float coneRoughnessScale;
};

#ifdef __cplusplus
static_assert(sizeof(SSRRadianceResolveParams) == 16, "SSRRadianceResolveParams must match the HLSL root constants");
#endif

struct SSRTemporalParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 uvJitterDelta; // currUVJitter - prevUVJitter
    float historyWeight;
    uint historyValid;
    uint transparentReceiver;
};

#ifdef __cplusplus
static_assert(sizeof(SSRTemporalParams) == 36, "SSRTemporalParams must match the HLSL root constants");
#endif

struct SSRUpsampleParams {
    uint2 outputSize;
    uint debugMode;
};

#ifdef __cplusplus
static_assert(sizeof(SSRUpsampleParams) == 12, "SSRUpsampleParams must match the HLSL root constants");
#endif

#endif // __SSR_INTEROP_H__
