#ifndef __LOCAL_FOG_COMMON_HLSLI__
#define __LOCAL_FOG_COMMON_HLSLI__

#include "FogInterop.h"

float LocalFogSphereSampleDensity(float3 localPos, float radius, float density, float boundaryFadeDistanceRcp) {
    const float distanceSquared = dot(localPos, localPos);
    if (distanceSquared > radius * radius) {
        return 0.0f;
    }

    [branch]
    if (boundaryFadeDistanceRcp == 0.0f) {
        return density;
    }

    const float distanceToBoundary = radius - sqrt(distanceSquared);
    return density * smoothstep(0.0f, 1.0f, distanceToBoundary * boundaryFadeDistanceRcp);
}

float LocalFogBoxSampleDensity(float3 localPos, float3 halfExtents, float density, float boundaryFadeDistanceRcp) {
    const float3 distancesToFaces = halfExtents - abs(localPos);
    const float distanceToBoundary = min(distancesToFaces.x, min(distancesToFaces.y, distancesToFaces.z));
    if (distanceToBoundary < 0.0f) {
        return 0.0f;
    }

    if (boundaryFadeDistanceRcp == 0.0f) {
        return density;
    }

    return density * smoothstep(0.0f, 1.0f, distanceToBoundary * boundaryFadeDistanceRcp);
}

float LocalFogSampleDensity(float3 worldPos, ShaderLocalFogVolume volume) {
    const float3 offset = worldPos - volume.origin;
    float density;
    if (volume.shape == ShaderLocalFogVolumeShapeSphere) {
        density = LocalFogSphereSampleDensity(offset, volume.halfExtents.x, volume.density, volume.boundaryFadeDistanceRcp);
    } else {
        const float3 localPos = float3(dot(offset, volume.axisX), dot(offset, volume.axisY), dot(offset, volume.axisZ));
        density = LocalFogBoxSampleDensity(localPos, volume.halfExtents, volume.density, volume.boundaryFadeDistanceRcp);
    }

    [branch]
    if (volume.heightFogFalloff > 0.0f && density > 0.0f) {
        const float heightDelta = max(offset.z - volume.heightFogOffset, 0.0f);
        const float exponent = max(-volume.heightFogFalloff * heightDelta, -80.0f);
        density *= exp2(exponent);
    }
    return density;
}

#endif // __LOCAL_FOG_COMMON_HLSLI__
