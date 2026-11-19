#ifndef __FOG_INTEROP_H__
#define __FOG_INTEROP_H__

#include "../ShaderTypes.h"

struct MediumParams {
    float baseExtinction;           // Extinction coefficient before spatial weighting (1/m).
    float3 scatteringAlbedo;        // RGB fraction of baseExtinction due to scattering, [0, 1].
    float anisotropy;               // HenyeyGreensteinPhase's g parameter.
    float3 baseEmission;            // RGB radiance emitted per meter before spatial weighting.
};

static const uint ShaderLocalFogVolumeShapeSphere = 0u;
static const uint ShaderLocalFogVolumeShapeBox = 1u;

static const uint ShaderLocalFogVolumeMaskAddressRepeat = 0u;
static const uint ShaderLocalFogVolumeMaskAddressClamp = 1u;

struct ShaderLocalFogVolume {
    MediumParams medium;
    float3 origin;
    uint shape;
    float3 halfExtents;             // Box half extents, or sphere radius in all components (m).
    float heightFogFalloff;         // exp2 height falloff above the reference plane along world Z (1/m); zero disables it.
    float3 axisX;                   // World-space local axes for both shapes; dot(worldPosition - origin, axis) gives local meters.
    uint maskAddressMode;           // Common U/V/W addressing for the volume mask.
    float3 axisY;
    float boundaryFadeDistanceRcp;  // CPU converts the fade ratio to reciprocal world distance (1/m). Zero means no fade.
    float3 axisZ;
    float heightFogOffset;          // Height where falloff starts, relative to origin.z (m). Density stays constant below it.
    float3 maskUVWScale;
    uint maskTextureIndex;
    float3 maskUVWBias;
    float maskTexelsPerMeter;        // max(abs(UVW scale) * mip-0 dimensions), for conservative mip selection.
};

struct HeightFogParams {
    MediumParams medium;
    float baseHeight;
    float padding0;
    float heightFalloff;            // exp2 height falloff (1/m); zero is a homogeneous medium.
    float maxDistance;              // World-space ray distance.
    float3 ambientRadiance;         // Spatially constant, isotropic incident radiance for height fog.
    float padding1;
};

static const uint VFogFlagShadows = 2u;
static const uint VFogFlagSTBNJitter = 4u;

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
#include <cstddef>

static_assert(sizeof(MediumParams) == 32, "Medium constants must match HLSL");
static_assert(sizeof(ShaderLocalFogVolume) == 144, "Local fog volumes must match the HLSL StructuredBuffer layout");
static_assert(offsetof(ShaderLocalFogVolume, maskAddressMode) == 76, "Mask addressing must match HLSL");
static_assert(offsetof(ShaderLocalFogVolume, maskUVWScale) == 112, "Mask UVW scale must match HLSL");
static_assert(offsetof(ShaderLocalFogVolume, maskTextureIndex) == 124, "Mask texture index must match HLSL");
static_assert(offsetof(ShaderLocalFogVolume, maskUVWBias) == 128, "Mask UVW bias must match HLSL");
static_assert(offsetof(ShaderLocalFogVolume, maskTexelsPerMeter) == 140, "Mask texel density must match HLSL");
static_assert(sizeof(HeightFogParams) == 64, "Height fog constants must match HLSL");
static_assert(sizeof(VFogLightingParams) == 16, "VFog lighting root constants must match HLSL");
static_assert(sizeof(VFogTemporalParams) == 16, "VFog temporal constant buffer must match HLSL");
static_assert(sizeof(VFogParams) == 48, "VFog constants must match HLSL");
#endif

#endif
