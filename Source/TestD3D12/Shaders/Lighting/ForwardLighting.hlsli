#ifndef __FORWARD_LIGHTING_HLSLI__
#define __FORWARD_LIGHTING_HLSLI__

#include "../ShaderInterop.h"
#include "LightCullingCommon.hlsli"
#include "ShadowLibrary.hlsli"
#include "../PBR/StandardBRDF.hlsli"

bool GetLightVolumePosition(float4 projectedPosition, out float3 volumePosition) {
    if (projectedPosition.w <= 0.0f) {
        volumePosition = 0.0f;
        return false;
    }

    volumePosition = projectedPosition.xyz / projectedPosition.w;
    return all(volumePosition >= 0.0f) && all(volumePosition <= 1.0f);
}

bool IsInsideLightVolume(float4 projectedPosition) {
    float3 volumePosition;
    return GetLightVolumePosition(projectedPosition, volumePosition);
}

float CalcLightFallOff(row_major float3x4 fallOffMatrix, float fallOffExponent, float3 worldPosition) {
    const float3 lightFallOff = mul(fallOffMatrix, float4(worldPosition, 1.0f));
    const float fallOff = saturate(1.0f - dot(lightFallOff, lightFallOff));
    return pow(fallOff, fallOffExponent);
}

float3 EvaluateLocalLight(float3 lightOrigin, float3 lightColor, row_major float3x4 fallOffMatrix, float fallOffExponent, float3 worldPosition, in ShadingParams shading) {
    const float attenuation = CalcLightFallOff(fallOffMatrix, fallOffExponent, worldPosition);
    if (attenuation <= 0.0f) {
        return 0.0f;
    }

    const float3 lightVector = lightOrigin - worldPosition;
    const float lightDistanceSquared = dot(lightVector, lightVector);
    if (lightDistanceSquared <= FLT_MIN) {
        return 0.0f;
    }

    const float3 lightDir = lightVector * rsqrt(lightDistanceSquared);
    return lightColor * attenuation * EvaluateStandardDirectBRDF(shading, lightDir);
}

float2 CalcPointLightProjectionUV(ShaderPointLight pointLight, float3 worldPosition) {
    const float4 position = float4(worldPosition, 1.0f);
    return float2(dot(pointLight.worldToProjectionU, position), dot(pointLight.worldToProjectionV, position));
}

float3 EvaluatePointLight(ShaderPointLight pointLight, float3 worldPosition, in ShadingParams shading) {
    const float3 lightVector = pointLight.origin - worldPosition;
    const float lightDistanceSquared = dot(lightVector, lightVector);
    if (lightDistanceSquared <= FLT_MIN) {
        return 0.0f;
    }

    const float fallOff = saturate(1.0f - lightDistanceSquared * pointLight.invRadiusSquared);
    const float attenuation = pow(fallOff, pointLight.fallOffExponent);
    if (attenuation <= 0.0f) {
        return 0.0f;
    }

    const float3 lightDir = lightVector * rsqrt(lightDistanceSquared);
    return pointLight.lightColor * attenuation * EvaluateStandardDirectBRDF(shading, lightDir);
}

float3 EvaluateSpotLight(ShaderSpotLight spotLight, float3 worldPosition, in ShadingParams shading) {
    const float4 projectedPosition = mul(spotLight.viewProjScaleBiasMatrix, float4(worldPosition, 1.0f));
    if (!IsInsideLightVolume(projectedPosition)) {
        return 0.0f;
    }
    return EvaluateLocalLight(spotLight.origin, spotLight.lightColor, spotLight.fallOffMatrix, spotLight.fallOffExponent, worldPosition, shading);
}

float3 EvaluateBoxDirLight(ShaderBoxDirLight boxDirLight, float3 worldPosition, in ShadingParams shading) {
    const float4 projectedPosition = mul(boxDirLight.worldToLightVolumeMatrix, float4(worldPosition, 1.0f));
    float3 volumePosition;
    if (!GetLightVolumePosition(projectedPosition, volumePosition)) {
        return 0.0f;
    }

    const float3 centeredVolumePosition = volumePosition * 2.0f - 1.0f;
    const float3 boxDistance3 = abs(centeredVolumePosition);
    const float boxDistance = max(boxDistance3.x, max(boxDistance3.y, boxDistance3.z));
    const float fallOff = pow(saturate(1.0f - boxDistance * boxDistance), boxDirLight.fallOffExponent);

    const float3 lightDir = -boxDirLight.direction;
    return boxDirLight.lightColor * fallOff * EvaluateStandardDirectBRDF(shading, lightDir);
}

float3 EvaluateDirectionalLight(ShaderDirectionalLight directionalLight, in ShadingParams shading) {
    const float3 lightDir = -directionalLight.direction;
    return directionalLight.lightColor * EvaluateStandardDirectBRDF(shading, lightDir);
}

float3 EvaluateShaderLight(uint packedLightRef, float3 worldPosition, in ShadingParams shading) {
    const uint lightType = packedLightRef & ShaderLightTypeMask;
    const uint lightTypeIndex = packedLightRef >> ShaderLightTypeBitCount;

    if (lightType == ShaderLightTypePoint) {
        const ShaderPointLight pointLight = GetShaderPointLightTable(frameConstants.shaderPointLightTableIndex)[lightTypeIndex];
        return EvaluatePointLight(pointLight, worldPosition, shading);
    }

    if (lightType == ShaderLightTypeSpot) {
        const ShaderSpotLight spotLight = GetShaderSpotLightTable(frameConstants.shaderSpotLightTableIndex)[lightTypeIndex];
        return EvaluateSpotLight(spotLight, worldPosition, shading);
    }

    if (lightType == ShaderLightTypeBoxDir) {
        const ShaderBoxDirLight boxDirLight = GetShaderBoxDirLightTable(frameConstants.shaderBoxDirLightTableIndex)[lightTypeIndex];
        return EvaluateBoxDirLight(boxDirLight, worldPosition, shading);
    }

    return 0.0f;
}

#if SHADER_LIGHT_USE_WAVE_OPS
// 모든 Wave 가 같은 lightRef 만 읽는다면 Wave 당 한번만 읽는다.
uint LoadWaveShaderLightRef(uint lightIndex) {
    uint packedLightRef = 0;
    if (WaveIsFirstLane()) {
        packedLightRef = GetShaderLightRefTable(frameConstants.shaderLightRefTableIndex)[cameraConstants.lightCullingLightRefTableOffset + lightIndex].packedLightRef;
    }
    return WaveReadLaneFirst(packedLightRef);
}
#endif

float3 EvaluateDirectLighting(uint2 pixelCoord, float ndcDepth, float3 worldPosition, in ShadingParams shading) {
#if SHADER_LIGHT_USE_WAVE_OPS
    // Helper lane 은 wave 연산에 참여하지 않으므로 wave 결과에 의존하는 반복문에서 제외한다.
    if (IsHelperLane()) {
        return 0.0f;
    }
#endif

    float3 directLighting = 0.0f;
    if ((cameraConstants.flags & ShaderCameraFlagUseDirectionalLight) != 0) {
        const ShadowLighting shadowLighting = EvaluateDirectionalShadowLighting(pixelCoord, worldPosition);
        directLighting = shadowLighting * EvaluateDirectionalLight(frameConstants.directionalLight, shading);
    }

#if SHADER_LIGHT_USE_WAVE_OPS
    const bool useZBin = (cameraConstants.flags & ShaderCameraFlagUseLightZBin) != 0;
    ShaderLightIterator lightIterator = BeginShaderLightIteration(pixelCoord, ndcDepth, useZBin, true);

    const bool hasLights = lightIterator.nextWordIndex < lightIterator.endWordIndex;

    // 모든 lane 이 동일한 반복을 수행하도록 각 lane 의 light mask word 범위를 모두 포함하는 Wave 단위 반복 범위를 구한다.
    const uint laneFirstWordIndex = hasLights ? lightIterator.nextWordIndex : 0xFFFFFFFFu;
    const uint laneEndWordIndex = hasLights ? lightIterator.endWordIndex : 0;
    const uint waveFirstWordIndex = WaveActiveMin(laneFirstWordIndex);
    const uint waveEndWordIndex = WaveActiveMax(laneEndWordIndex);

    [loop]
    for (uint wordIndex = waveFirstWordIndex; wordIndex < waveEndWordIndex; ++wordIndex) {
        // 모든 lane 의 light mask 를 합친다.
        const uint laneLightMask = LoadShaderLightMask(lightIterator, wordIndex, true);
        uint waveLightMask = WaveActiveBitOr(laneLightMask);

        [loop]
        while (waveLightMask != 0) {
            const uint bitIndex = firstbitlow(waveLightMask);
            const uint lightBit = 1u << bitIndex;
            const uint lightIndex = (wordIndex << ShaderLightWordShift) + bitIndex;
            const uint packedLightRef = LoadWaveShaderLightRef(lightIndex);

            // 현재 lane 의 tile 및 Z-bin 에 포함된 light 만 실제로 계산한다.
            if ((laneLightMask & lightBit) != 0) {
                directLighting += EvaluateShaderLight(packedLightRef, worldPosition, shading);
            }

            waveLightMask &= ~lightBit;
        }
    }

    return directLighting;
#else
    const bool useZBin = (cameraConstants.flags & ShaderCameraFlagUseLightZBin) != 0;
    ShaderLightIterator lightIterator = BeginShaderLightIteration(pixelCoord, ndcDepth, useZBin, false);
    ShaderLightRef shaderLightRef;

    [loop]
    while (NextShaderLight(lightIterator, shaderLightRef)) {
        directLighting += EvaluateShaderLight(shaderLightRef.packedLightRef, worldPosition, shading);
    }

    return directLighting;
#endif
}

#endif // __FORWARD_LIGHTING_HLSLI__
