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

#ifndef __OCEAN_INTEROP_H__
#define __OCEAN_INTEROP_H__

#include "../ShaderTypes.h"

#ifdef __cplusplus
#include <cstddef>
#endif

static const uint OceanInitialSpectrumBlockSizeX = 8;
static const uint OceanInitialSpectrumBlockSizeY = 8;
static const uint OceanTimeSpectrumBlockSizeX = 8;
static const uint OceanTimeSpectrumBlockSizeY = 8;
static const uint OceanResolveFieldsBlockSizeX = 8;
static const uint OceanResolveFieldsBlockSizeY = 8;
static const uint OceanSurfaceQueryBlockSize = 64;
static const uint OceanSurfaceQueryFixedPointIterationCount = 3;

static const uint OceanSpectrumComplexPlaneCount = 4;
static const uint OceanMaxCascadeCount = 4;
static const uint OceanTimeSpectrumOptionBitCrossDerivative = 1u << 0;
static const uint OceanFoamOptionBitEnabled = 1u << 0;
static const uint OceanFoamOptionBitResetHistory = 1u << 1;
static const uint OceanSurfaceOptionBitFoamEnabled = 1u << 0;
static const uint OceanSurfaceOptionBitRefractionEnabled = 1u << 1;

#ifdef __cplusplus
static_assert(OceanMaxCascadeCount == 4, "Ocean cascade parameters use float4 packing");
#endif

struct OceanInitialSpectrumCascadeParams {
    uint randomSeed;
    float patchSize;
    float minWaveNumber;
    float maxWaveNumber;
};

#ifdef __cplusplus
static_assert(sizeof(OceanInitialSpectrumCascadeParams) == 16, "OceanInitialSpectrumCascadeParams must match the HLSL constant-buffer layout");
#endif

struct OceanPhillipsInitialSpectrumParams {
    float spectrumScale;
    float windSpeed;
    float2 windDirection;
    float gravity;
    float smallWaveDamping;
    float oppositeWaveDamping;
    float padding0;
    OceanInitialSpectrumCascadeParams cascadeParams[OceanMaxCascadeCount];
};

#ifdef __cplusplus
static_assert(sizeof(OceanPhillipsInitialSpectrumParams) == 32 + sizeof(OceanInitialSpectrumCascadeParams) * OceanMaxCascadeCount,
    "OceanPhillipsInitialSpectrumParams must match the HLSL constant-buffer layout");
#endif

struct OceanTMAInitialSpectrumParams {
    float spectrumScale;
    float windSpeed;
    float2 windDirection;
    float gravity;
    float smallWaveDamping;
    float alpha;
    float peakOmega;
    float depth;
    float swell;
    float spread;
    float gamma;
    OceanInitialSpectrumCascadeParams cascadeParams[OceanMaxCascadeCount];
};

#ifdef __cplusplus
static_assert(sizeof(OceanTMAInitialSpectrumParams) == 48 + sizeof(OceanInitialSpectrumCascadeParams) * OceanMaxCascadeCount,
    "OceanTMAInitialSpectrumParams must match the HLSL constant-buffer layout");
#endif

struct OceanTimeSpectrumParams {
    float time;
    float gravity;
    float depth;
    float choppiness;
    float patchSize;
    uint cascadeIndex;
    uint options;
};

#ifdef __cplusplus
static_assert(sizeof(OceanTimeSpectrumParams) == 28, "OceanTimeSpectrumParams must match the seven HLSL root constants");
#endif

struct OceanResolveFieldsParams {
    float deltaTime;
    float displacementScale;
    float foamThreshold;
    float foamGenerationRate;
    float foamDecayRate;
    uint foamOptions;
    uint cascadeIndex;
};

#ifdef __cplusplus
static_assert(sizeof(OceanResolveFieldsParams) == 28, "OceanResolveFieldsParams must match the seven HLSL root constants");
#endif

struct OceanSurfaceQueryInput {
    float2 worldPositionXY;     ///< horizontal world position at which the displaced surface is requested
    float sampleDiameter;       ///< world-space filter diameter in meters; zero samples mip 0
    uint queryId;               ///< result 에 그대로 전달되는 caller-owned opaque ID
};

#ifdef __cplusplus
static_assert(sizeof(OceanSurfaceQueryInput) == 16, "OceanSurfaceQueryInput must match the HLSL structured-buffer layout");
#endif

struct OceanSurfaceQueryResult {
    float3 surfacePosition;
    uint queryId;
    float3 surfaceNormal;
    float sampleTime;           ///< ocean simulation time represented by this result
};

#ifdef __cplusplus
static_assert(sizeof(OceanSurfaceQueryResult) == 32, "OceanSurfaceQueryResult must match the HLSL structured-buffer layout");
#endif

struct OceanSurfaceQueryParams {
    float4 cascadePatchSizes;
    float surfaceHeight;
    float displacementScale;
    float sampleTime;
    uint queryCount;
    uint cascadeCount;
};

#ifdef __cplusplus
static_assert(sizeof(OceanSurfaceQueryParams) == 36, "OceanSurfaceQueryParams must match the HLSL root constants");
#endif

struct OceanSurfaceParams {
    float4 cascadePatchSizes;
    float4 cascadeVertexMipLevels;
    float surfaceHeight;
    float roughness;
    float displacementScale;
    uint surfaceOptions;
    float3 waterColor;
    float fresnelF0;
    float3 scatterColor;
    float scatterHeight;
    float3 foamColor;
    float cascadeFadeScale;
    float foamDetailMapSize;
    float foamDetailStrength;
    float refractionDistance;
    float absorptionDistance;
    float airToWaterEta;
    float waterToAirEta;
    float2 sceneSizeRcp;
    float2 sceneSnapshotSize;
    float2 sceneSnapshotSizeRcp;
    uint cascadeCount;
};

#ifdef __cplusplus
static_assert(sizeof(OceanSurfaceParams) == 148, "OceanSurfaceParams must match the HLSL constant-buffer layout");
static_assert(offsetof(OceanSurfaceParams, sceneSizeRcp) == 120, "Scene size reciprocal must match the HLSL offset");
static_assert(offsetof(OceanSurfaceParams, cascadeCount) == 144, "Cascade count must follow the scene size parameters");
#endif

#endif // __OCEAN_INTEROP_H__
