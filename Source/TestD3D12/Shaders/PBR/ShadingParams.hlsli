#ifndef __SHADING_PARAMS_HLSLI__
#define __SHADING_PARAMS_HLSLI__

#include "../CommonBindless.hlsli"
#include "BRDFLibrary.hlsli"

struct ShadingParams {
    float4 baseColor;
    float3 diffuseColor;
    float3 specularF0;
    float metallic;
    float roughness;
    float linearRoughness;

    float3 viewDir;
    float3 normal;
    float normalDotView;
    float3 reflectionDir;

    float ambientOcclusion;
    float3 emission;

#if _CLEARCOAT != 0
    float clearCoat;
    float clearCoatRoughness;
    float clearCoatLinearRoughness;
    float3 clearCoatNormal;
#endif
};

// 저장된 두 성분으로 tangent space normal 을 디코딩하고 양의 Z 성분을 복원한다.
float3 DecodeTangentSpaceNormal(float2 encodedNormal) {
    const float2 normalXY = encodedNormal * 2.0f - 1.0f;
    const float normalZ = sqrt(saturate(1.0f - dot(normalXY, normalXY)));
    return float3(normalXY, normalZ);
}

// material texture 를 샘플링하여 direct lighting 과 indirect lighting 에서 공유할 입력값을 준비한다.
ShadingParams PrepareShadingParams(ShaderMaterial shaderMaterial, float4 baseColor, float2 texCoord, float3 viewDir, float3 normal,
#if _NORMAL != 0 || (_CLEARCOAT != 0 && _CLEARCOAT_NORMAL != 0)
    float3x3 tangentToWorld,
#endif
    SamplerState materialSampler) {
    ShadingParams shading;

    shading.baseColor = baseColor;
    shading.viewDir = normalize(viewDir);
    shading.normal = normalize(normal);

#if _NORMAL != 0
    const float2 encodedNormal = GetTexture2D(shaderMaterial.normalTextureIndex).Sample(materialSampler, texCoord).xy;
    const float3 normalTS = DecodeTangentSpaceNormal(encodedNormal);
    shading.normal = normalize(mul(normalTS, tangentToWorld));
#endif

    shading.normalDotView = max(dot(shading.normal, shading.viewDir), 1e-4f);
    shading.reflectionDir = reflect(-shading.viewDir, shading.normal);

    const float2 metallicRoughnessFactors = UnpackRG16UNormToFloat2(shaderMaterial.packedMetallicRoughnessFactors);

#if _METALLIC != 0 || _ROUGHNESS >= 3 || _AO >= 2 || _CLEARCOAT >= 3 || (_CLEARCOAT != 0 && _CLEARCOAT_ROUGHNESS >= 2)
    const float4 metallicSample = GetTexture2D(shaderMaterial.metallicTextureIndex).Sample(materialSampler, texCoord);
#endif

#if _METALLIC == 0
    shading.metallic = metallicRoughnessFactors.x;
#elif _METALLIC == 1
    shading.metallic = metallicSample.r * metallicRoughnessFactors.x;
#elif _METALLIC == 2
    shading.metallic = metallicSample.g * metallicRoughnessFactors.x;
#elif _METALLIC == 3
    shading.metallic = metallicSample.b * metallicRoughnessFactors.x;
#elif _METALLIC == 4
    shading.metallic = metallicSample.a * metallicRoughnessFactors.x;
#endif

#if _ROUGHNESS == 0
    shading.roughness = metallicRoughnessFactors.y;
#elif _ROUGHNESS == 1
    shading.roughness = GetTexture2D(shaderMaterial.roughnessTextureIndex).Sample(materialSampler, texCoord).r * metallicRoughnessFactors.y;
#elif _ROUGHNESS == 2
    shading.roughness = (1.0f - GetTexture2D(shaderMaterial.roughnessTextureIndex).Sample(materialSampler, texCoord).r) * metallicRoughnessFactors.y;
#elif _ROUGHNESS == 3
    shading.roughness = metallicSample.r * metallicRoughnessFactors.y;
#elif _ROUGHNESS == 4
    shading.roughness = metallicSample.g * metallicRoughnessFactors.y;
#elif _ROUGHNESS == 5
    shading.roughness = metallicSample.b * metallicRoughnessFactors.y;
#elif _ROUGHNESS == 6
    shading.roughness = metallicSample.a * metallicRoughnessFactors.y;
#endif

    shading.metallic = saturate(shading.metallic);
    shading.roughness = clamp(shading.roughness, MinRoughness, 1.0f);
    shading.linearRoughness = shading.roughness * shading.roughness;

    // 0.04 의 기본 반사율은 대부분의 dielectric material 에 적합하다.
    shading.specularF0 = lerp(float3(0.04f, 0.04f, 0.04f), shading.baseColor.rgb, shading.metallic);
    shading.diffuseColor = shading.baseColor.rgb * (1.0f - shading.metallic);

#if _CLEARCOAT != 0
    const float2 clearCoatFactors = UnpackRG16UNormToFloat2(shaderMaterial.packedClearCoatFactors);

    #if _CLEARCOAT == 1
        shading.clearCoat = clearCoatFactors.x;
    #elif _CLEARCOAT == 2
        shading.clearCoat = GetTexture2D(shaderMaterial.clearCoatTextureIndex).Sample(materialSampler, texCoord).r * clearCoatFactors.x;
    #elif _CLEARCOAT == 3
        shading.clearCoat = metallicSample.r * clearCoatFactors.x;
    #elif _CLEARCOAT == 4
        shading.clearCoat = metallicSample.g * clearCoatFactors.x;
    #elif _CLEARCOAT == 5
        shading.clearCoat = metallicSample.b * clearCoatFactors.x;
    #elif _CLEARCOAT == 6
        shading.clearCoat = metallicSample.a * clearCoatFactors.x;
    #endif

    #if _CLEARCOAT_ROUGHNESS == 0
        shading.clearCoatRoughness = clearCoatFactors.y;
    #elif _CLEARCOAT_ROUGHNESS == 1
        shading.clearCoatRoughness = GetTexture2D(shaderMaterial.clearCoatRoughnessTextureIndex).Sample(materialSampler, texCoord).r * clearCoatFactors.y;
    #elif _CLEARCOAT_ROUGHNESS == 2
        shading.clearCoatRoughness = metallicSample.r * clearCoatFactors.y;
    #elif _CLEARCOAT_ROUGHNESS == 3
        shading.clearCoatRoughness = metallicSample.g * clearCoatFactors.y;
    #elif _CLEARCOAT_ROUGHNESS == 4
        shading.clearCoatRoughness = metallicSample.b * clearCoatFactors.y;
    #elif _CLEARCOAT_ROUGHNESS == 5
        shading.clearCoatRoughness = metallicSample.a * clearCoatFactors.y;
    #endif

    shading.clearCoat = saturate(shading.clearCoat);
    shading.clearCoatRoughness = lerp(MinClearCoatRoughness, MaxClearCoatRoughness, saturate(shading.clearCoatRoughness));
    shading.clearCoatLinearRoughness = shading.clearCoatRoughness * shading.clearCoatRoughness;

#if _CLEARCOAT_NORMAL != 0
    const float2 encodedClearCoatNormal = GetTexture2D(shaderMaterial.clearCoatNormalTextureIndex).Sample(materialSampler, texCoord).xy;
    const float3 clearCoatNormalTS = DecodeTangentSpaceNormal(encodedClearCoatNormal);
    shading.clearCoatNormal = normalize(mul(clearCoatNormalTS, tangentToWorld));
#else
    shading.clearCoatNormal = shading.normal;
#endif

    // IOR 1.5 인 clear coat layer 아래에 놓인 base layer 의 F0 를 보정한다.
    shading.specularF0 = lerp(shading.specularF0, F0ForAirInterfaceToF0ForClearCoat15(shading.specularF0), shading.clearCoat);
#endif

#if _AO == 0
    shading.ambientOcclusion = 1.0f;
#elif _AO == 1
    shading.ambientOcclusion = GetTexture2D(shaderMaterial.aoTextureIndex).Sample(materialSampler, texCoord).r;
#elif _AO == 2
    shading.ambientOcclusion = metallicSample.r;
#elif _AO == 3
    shading.ambientOcclusion = metallicSample.g;
#elif _AO == 4
    shading.ambientOcclusion = metallicSample.b;
#elif _AO == 5
    shading.ambientOcclusion = metallicSample.a;
#endif

#if _AO != 0
    shading.ambientOcclusion = lerp(1.0f, shading.ambientOcclusion, shaderMaterial.aoStrength);
#endif

#if _EMISSION == 0
    shading.emission = 0.0f;
#else
    const float4 emissionColorFactor = UnpackRGBA16FToFloat4(shaderMaterial.packedEmissionColorFactor);
    shading.emission = emissionColorFactor.rgb * emissionColorFactor.a;

    #if _EMISSION == 2
        shading.emission *= GetTexture2D(shaderMaterial.emissiveTextureIndex).Sample(materialSampler, texCoord).rgb;
    #endif
#endif

    return shading;
}

#endif // __SHADING_PARAMS_HLSLI__
