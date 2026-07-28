#ifndef __IMAGE_BASED_LIGHTING_HLSLI__
#define __IMAGE_BASED_LIGHTING_HLSLI__

#include "StandardBRDF.hlsli"

// Environment Light 나 Reflection Probe 에서 샘플링한 간접광을 저장한다.
struct ImageBasedLightingSample {
    float3 diffuseIrradiance;
    float3 specularRadiance;

#if _CLEARCOAT != 0
    float3 clearCoatSpecularRadiance;
#endif
};

// material 과 view 방향에 따라 미리 적분된 DFG 값을 저장한다.
struct ImageBasedLightingDFG {
    float2 baseLayer;

#if _CLEARCOAT != 0
    float2 clearCoat;
#endif
};

// 미리 적분된 DFG LUT 에서 base layer 와 clear coat 의 DFG 값을 샘플링한다.
ImageBasedLightingDFG SampleImageBasedLightingDFG(in ShadingParams shading) {
    const Texture2D<float4> dfgSumGGXTexture = GetTexture2D(frameConstants.dfgSumGGXTextureIndex);

    ImageBasedLightingDFG dfg;
    dfg.baseLayer = dfgSumGGXTexture.SampleLevel(linearClampSampler, float2(shading.normalDotView, shading.roughness), 0.0f).xy;

#if _CLEARCOAT != 0
    const float clearCoatNormalDotView = saturate(dot(shading.clearCoatNormal, shading.viewDir));
    dfg.clearCoat = dfgSumGGXTexture.SampleLevel(linearClampSampler, float2(clearCoatNormalDotView, shading.clearCoatRoughness), 0.0f).xy;
#endif

    return dfg;
}

// Environment Light cubemap 에서 diffuse irradiance 와 prefiltered specular radiance 를 샘플링한다.
ImageBasedLightingSample SampleEnvironmentLight(in ShadingParams shading) {
    const TextureCube<float4> diffuseIrradianceCubeMap = GetTextureCube(cameraConstants.environmentLightDiffuseIrradianceCubeMapIndex);
    const TextureCube<float4> specularCubeMap = GetTextureCube(cameraConstants.environmentLightSpecularCubeMapIndex);
    const float maxSpecularMipLevel = (float)(cameraConstants.environmentLightSpecularMipCount - 1u);
    const float3 radianceScale = cameraConstants.environmentLightColor * cameraConstants.environmentLightIntensity;

    // Environment Light axis 의 역회전으로 world space 방향을 cubemap local space 로 변환한다.
    const float3 diffuseDirection = mul(cameraConstants.environmentLightAxis, float4(shading.normal, 0.0f));
    const float3 specularDominantDirection = GetSpecularDominantDir(shading.normal, shading.reflectionDir, shading.linearRoughness);
    const float3 specularDirection = mul(cameraConstants.environmentLightAxis, float4(specularDominantDirection, 0.0f));

    ImageBasedLightingSample sample;
    sample.diffuseIrradiance = diffuseIrradianceCubeMap.SampleLevel(linearClampSampler, diffuseDirection, 0.0f).rgb * radianceScale;
    sample.specularRadiance = specularCubeMap.SampleLevel(linearClampSampler, specularDirection, LinearRoughnessToMipLevel(shading.linearRoughness, maxSpecularMipLevel)).rgb * radianceScale;

#if _CLEARCOAT != 0
    const float3 clearCoatReflectionDirection = reflect(-shading.viewDir, shading.clearCoatNormal);
    const float3 clearCoatDominantDirection = GetSpecularDominantDir(shading.clearCoatNormal, clearCoatReflectionDirection, shading.clearCoatLinearRoughness);
    const float3 clearCoatDirection = mul(cameraConstants.environmentLightAxis, float4(clearCoatDominantDirection, 0.0f));

    sample.clearCoatSpecularRadiance = specularCubeMap.SampleLevel(linearClampSampler, clearCoatDirection, LinearRoughnessToMipLevel(shading.clearCoatLinearRoughness, maxSpecularMipLevel)).rgb * radianceScale;
#endif

    return sample;
}

// 광원 소스에서 샘플링한 간접광에 standard material 의 BRDF 응답을 적용한다.
float3 EvaluateStandardImageBasedLighting(in ShadingParams shading, in ImageBasedLightingSample sample, in ImageBasedLightingDFG dfg) {
    const float fresnelSecondFactor = F_SecondFactorSchlickSG(shading.normalDotView);
    const float oneMinusRoughness = 1.0f - shading.roughness;
    const float3 fresnel = shading.specularF0 + (max(float3(oneMinusRoughness, oneMinusRoughness, oneMinusRoughness), shading.specularF0) - shading.specularF0) * fresnelSecondFactor;

    float3 diffuse = sample.diffuseIrradiance * shading.diffuseColor * (1.0f - fresnel);
    float3 specular = sample.specularRadiance * GetSpecularEnvSecondSum(dfg.baseLayer, shading.specularF0);

#if USE_MULTIPLE_SCATTERING_COMPENSATION
    specular *= shading.energyCompensation;
#endif

    diffuse *= shading.ambientOcclusion;
    specular *= GetSpecularOcclusionFromAmbientOcclusion(shading.normalDotView, shading.ambientOcclusion, shading.roughness);

#if _CLEARCOAT != 0
    const float clearCoatNormalDotView = saturate(dot(shading.clearCoatNormal, shading.viewDir));
    const float clearCoatFresnelSecondFactor = F_SecondFactorSchlickSG(clearCoatNormalDotView);
    const float clearCoatFresnel = (0.04f + (1.0f - 0.04f) * clearCoatFresnelSecondFactor) * shading.clearCoat;
    const float baseLayerAttenuation = 1.0f - clearCoatFresnel;

    float3 clearCoatSpecular = sample.clearCoatSpecularRadiance * GetSpecularEnvSecondSum(dfg.clearCoat, float3(0.04f, 0.04f, 0.04f)) * shading.clearCoat;

#if USE_MULTIPLE_SCATTERING_COMPENSATION
    clearCoatSpecular *= shading.clearCoatEnergyCompensation;
#endif

    clearCoatSpecular *= GetSpecularOcclusionFromAmbientOcclusion(clearCoatNormalDotView, shading.ambientOcclusion, shading.clearCoatRoughness);

    return (diffuse + specular) * baseLayerAttenuation * baseLayerAttenuation + clearCoatSpecular;
#else
    return diffuse + specular;
#endif
}

#endif // __IMAGE_BASED_LIGHTING_HLSLI__
