#ifndef __LOCAL_FOG_COMMON_HLSLI__
#define __LOCAL_FOG_COMMON_HLSLI__

#include "FogInterop.h"
#include "../CommonBindless.hlsli"

float3 LocalFogWorldOffsetToLocal(float3 worldOffset, ShaderLocalFogVolume volume) {
    return float3(dot(worldOffset, volume.axisX), dot(worldOffset, volume.axisY), dot(worldOffset, volume.axisZ));
}

float LocalFogSphereSampleDensity(float3 localPos, float radius, float boundaryFadeDistanceRcp) {
    const float distanceSquared = dot(localPos, localPos);
    if (distanceSquared > radius * radius) {
        return 0.0f;
    }

    [branch]
    if (boundaryFadeDistanceRcp == 0.0f) {
        return 1.0f;
    }

    const float distanceToBoundary = radius - sqrt(distanceSquared);
    return smoothstep(0.0f, 1.0f, distanceToBoundary * boundaryFadeDistanceRcp);
}

float LocalFogBoxSampleDensity(float3 localPos, float3 halfExtents, float boundaryFadeDistanceRcp) {
    const float3 distancesToFaces = halfExtents - abs(localPos);
    const float distanceToBoundary = min(distancesToFaces.x, min(distancesToFaces.y, distancesToFaces.z));
    if (distanceToBoundary < 0.0f) {
        return 0.0f;
    }

    if (boundaryFadeDistanceRcp == 0.0f) {
        return 1.0f;
    }

    return smoothstep(0.0f, 1.0f, distanceToBoundary * boundaryFadeDistanceRcp);
}

float4 LocalFogSampleColorDensity(float3 worldPos, float froxelDiagonalSquared, ShaderLocalFogVolume volume, SamplerState linearWrap, SamplerState linearClamp) {
    const float3 offset = worldPos - volume.origin;
    float3 localPos = 0.0f;
    float density;
    if (volume.shape == ShaderLocalFogVolumeShapeSphere) {
        density = LocalFogSphereSampleDensity(offset, volume.halfExtents.x, volume.boundaryFadeDistanceRcp);
    } else {
        localPos = LocalFogWorldOffsetToLocal(offset, volume);
        density = LocalFogBoxSampleDensity(localPos, volume.halfExtents, volume.boundaryFadeDistanceRcp);
    }

    [branch]
    if (volume.heightFogFalloff > 0.0f && density > 0.0f) {
        const float heightDelta = max(offset.z - volume.heightFogOffset, 0.0f);
        const float exponent = max(-volume.heightFogFalloff * heightDelta, -80.0f);
        density *= exp2(exponent);
    }

    float4 colorDensity = float4(1.0f, 1.0f, 1.0f, density);
    [branch]
    if (volume.maskTextureIndex != 0 && density > 0.0f) {
        if (volume.shape == ShaderLocalFogVolumeShapeSphere) {
            localPos = LocalFogWorldOffsetToLocal(offset, volume);
        }

        const float3 uvw = localPos * volume.maskUVWScale + volume.maskUVWBias;

        // 프록셀 대각선과 최대 texel 밀도로 회전에 무관한 보수적 footprint 를 구한다.
        // 제곱 길이를 유지해 sqrt 를 생략하고, 1 texel 이하의 footprint 는 mip 0 을 사용한다.
        const float footprintTexelsSquared = froxelDiagonalSquared * volume.maskTexelsPerMeter * volume.maskTexelsPerMeter;
        const float mipLevel = 0.25f * log2(max(footprintTexelsSquared, 1.0f));

        // NOTE: 텍스처 인덱스가 wave 내에서 다를 수 있으므로, NonUniformResourceIndex 를 붙여준다.
        const Texture3D<float4> maskTexture = GetTexture3D(NonUniformResourceIndex(volume.maskTextureIndex));
        float4 maskColorDensity;
        [branch]
        if (volume.maskAddressMode == ShaderLocalFogVolumeMaskAddressClamp) {
            maskColorDensity = maskTexture.SampleLevel(linearClamp, uvw, mipLevel);
        } else {
            maskColorDensity = maskTexture.SampleLevel(linearWrap, uvw, mipLevel);
        }
        colorDensity *= saturate(maskColorDensity);
    }
    return colorDensity;
}

#endif // __LOCAL_FOG_COMMON_HLSLI__
