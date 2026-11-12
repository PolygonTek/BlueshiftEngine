#ifndef __FOG_INTEROP_H__
#define __FOG_INTEROP_H__

#include "../ShaderTypes.h"

static const uint VFogFlagShadows = 2u;
static const uint VFogFlagSTBNJitter = 4u;

struct MediumParams {
    float baseExtinction;           // Extinction coefficient at unit relative density (1/m).
    float3 scatteringAlbedo;        // RGB fraction of baseExtinction due to scattering, [0, 1].
    float anisotropy;               // HenyeyGreensteinPhase's g parameter.
    float3 baseEmission;            // RGB radiance emitted per meter at unit relative density.
};

struct HeightFogParams {
    MediumParams medium;
    float baseHeight;
    float baseDensity;              // Relative density at baseHeight.
    float heightFalloff;            // exp2 height falloff (1/m); zero is a homogeneous medium.
    float maxDistance;              // World-space ray distance.
    float3 ambientRadiance;         // Spatially constant, isotropic incident radiance for height fog.
    float padding;
};

struct VFogLightingParams {
    float lightDistanceBiasScale;
    uint flags;
    uint stbnVec3TextureIndex;
    float padding;
};

struct VFogTemporalParams {
    uint historyValid;
    uint historyTextureIndex;
    float historyWeight;
    uint noiseFrameIndex;
};

struct VFogParams {
    uint2 tileCount;
    float2 tileCountRcp;

    uint sliceCount;
    float nearDepth;
    float farDepth;
    float depthScale;

    float exponentPerSlice;
    float slicesPerExponent;
    uint debugMode;
    uint debugSlice;
};

#ifdef __cplusplus
static_assert(sizeof(MediumParams) == 32, "Medium constants must match HLSL");
static_assert(sizeof(HeightFogParams) == 64, "Height fog constants must match HLSL");
static_assert(sizeof(VFogLightingParams) == 16, "VFog lighting root constants must match HLSL");
static_assert(sizeof(VFogTemporalParams) == 16, "VFog temporal constant buffer must match HLSL");
static_assert(sizeof(VFogParams) == 48, "VFog constants must match HLSL");
#endif

#endif
