#ifndef __STANDARD_BRDF_HLSLI__
#define __STANDARD_BRDF_HLSLI__

#include "BRDFLibrary.hlsli"
#include "ShadingParams.hlsli"

#define PBR_DIFFUSE_LAMBERT             0
#define PBR_DIFFUSE_WRAPPED             1
#define PBR_DIFFUSE_BURLEY              2
#define PBR_DIFFUSE_OREN_NAYAR          3

#define PBR_SPECULAR_D_BLINN            0
#define PBR_SPECULAR_D_BECKMANN         1
#define PBR_SPECULAR_D_GGX              2

#define PBR_SPECULAR_G_NEUMANN          0
#define PBR_SPECULAR_G_KELEMEN          1
#define PBR_SPECULAR_G_COOK_TORRANCE    2
#define PBR_SPECULAR_G_SCHLICK_GGX      3
#define PBR_SPECULAR_G_SMITH_GGX        4
#define PBR_SPECULAR_G_SMITH_GGX_FAST   5

#ifndef PBR_DIFFUSE
    #define PBR_DIFFUSE PBR_DIFFUSE_BURLEY
#endif

#ifndef PBR_SPECULAR_D
    #define PBR_SPECULAR_D PBR_SPECULAR_D_GGX
#endif

#ifndef PBR_SPECULAR_G
    #define PBR_SPECULAR_G PBR_SPECULAR_G_SCHLICK_GGX
#endif

#ifndef PBR_CLEARCOAT_D
    #define PBR_CLEARCOAT_D PBR_SPECULAR_D_GGX
#endif

#ifndef PBR_CLEARCOAT_G
    #define PBR_CLEARCOAT_G PBR_SPECULAR_G_KELEMEN
#endif

// Evaluates the selected diffuse BRDF.
float3 DiffuseBRDF(float NdotL, float NdotV, float LdotV, float VdotH, float3 diffuseColor, float linearRoughness, float diffuseWrap) {
#if PBR_DIFFUSE == PBR_DIFFUSE_LAMBERT
    return diffuseColor * Fd_Lambert();
#elif PBR_DIFFUSE == PBR_DIFFUSE_WRAPPED
    return diffuseColor * Fd_Wrap(NdotL, diffuseWrap);
#elif PBR_DIFFUSE == PBR_DIFFUSE_BURLEY
    return diffuseColor * Fd_Burley(NdotL, NdotV, VdotH, linearRoughness);
#elif PBR_DIFFUSE == PBR_DIFFUSE_OREN_NAYAR
    return diffuseColor * Fd_OrenNayar(NdotL, NdotV, LdotV, linearRoughness);
#endif
}

// Evaluates an anisotropic GGX microfacet specular BRDF.
float3 AnisotropicBRDF(float3 H, float3 L, float3 V, float3 T, float3 B, float NdotL, float NdotV, float NdotH, float VdotH, float anisotropy, float linearRoughness, float3 F0, float fresnelSecondFactor, out float3 F) {
    const float TdotH = dot(T, H);
    const float BdotH = dot(B, H);
    const float TdotL = dot(T, L);
    const float BdotL = dot(B, L);
    const float TdotV = dot(T, V);
    const float BdotV = dot(B, V);

    float roughnessT;
    float roughnessB;
    RoughnessToAnisotropyRoughness(anisotropy, linearRoughness, roughnessT, roughnessB);

    const float D = D_GGXAniso(NdotH, TdotH, BdotH, roughnessT, roughnessB);
    const float G = G_SmithGGXCorrelatedAniso(NdotV, NdotL, TdotV, BdotV, TdotL, BdotL, roughnessT, roughnessB);

    F = F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * fresnelSecondFactor;

    // The geometric visibility term is already divided by 4 * NdotL * NdotV.
    return D * G * F;
}

// Evaluates an isotropic microfacet specular BRDF.
float3 IsotropicBRDF(float NdotL, float NdotV, float NdotH, float VdotH, float roughness, float linearRoughness, float3 F0, float fresnelSecondFactor, out float3 F) {
#if PBR_SPECULAR_D == PBR_SPECULAR_D_BLINN
    const float D = D_Blinn(NdotH, linearRoughness);
#elif PBR_SPECULAR_D == PBR_SPECULAR_D_BECKMANN
    const float D = D_Beckmann(NdotH, linearRoughness);
#elif PBR_SPECULAR_D == PBR_SPECULAR_D_GGX
    const float D = D_GGX(NdotH, linearRoughness);
#endif

#if PBR_SPECULAR_G == PBR_SPECULAR_G_NEUMANN
    const float G = G_Neumann(NdotV, NdotL);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_KELEMEN
    const float G = G_Kelemen(VdotH);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_COOK_TORRANCE
    const float G = G_CookTorrance(NdotV, NdotL, NdotH, VdotH);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_SCHLICK_GGX
    // Disney's modification reduces hotness by remapping roughness with (roughness + 1) / 2 before squaring.
    const float remappedRoughness = roughness + 1.0f;
    const float k = remappedRoughness * remappedRoughness * 0.125f;
    const float G = G_SchlickGGX(NdotV, NdotL, k);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_SMITH_GGX
    const float G = G_SmithGGXCorrelated(NdotV, NdotL, linearRoughness);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_SMITH_GGX_FAST
    const float G = G_SmithGGXCorrelatedFast(NdotV, NdotL, linearRoughness);
#endif

    F = F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * fresnelSecondFactor;

    // The geometric visibility term is already divided by 4 * NdotL * NdotV.
    return D * G * F;
}

// Evaluates the currently supported isotropic specular BRDF.
float3 SpecularBRDF(float NdotL, float NdotV, float NdotH, float VdotH, float roughness, float linearRoughness, float3 F0, float fresnelSecondFactor, out float3 F) {
    return IsotropicBRDF(NdotL, NdotV, NdotH, VdotH, roughness, linearRoughness, F0, fresnelSecondFactor, F);
}

// Evaluates the clear coat microfacet BRDF and returns its Fresnel term.
float ClearCoatBRDF(float NdotH, float VdotH, float clearCoatReflectivity, float clearCoatLinearRoughness, float fresnelSecondFactor, out float F) {
#if PBR_CLEARCOAT_D == PBR_SPECULAR_D_GGX
    const float D = D_GGX(NdotH, clearCoatLinearRoughness);
#endif

#if PBR_CLEARCOAT_G == PBR_SPECULAR_G_KELEMEN
    const float G = G_Kelemen(VdotH);
#endif

    // A clear coat layer uses an IOR of 1.5, whose F0 is 0.04.
    F = (0.04f + (1.0f - 0.04f) * fresnelSecondFactor) * clearCoatReflectivity;
    return D * G * F;
}

#if USE_MULTIPLE_SCATTERING_COMPENSATION
// Compensates for energy lost by single-scattering microfacet BRDFs.
// Use a scalar F0 for achromatic lobes such as clear coat.
float GetMultipleScatteringEnergyCompensation(float dfgFirstSum, float F0) {
    return 1.0f + F0 * (rcp(dfgFirstSum) - 1.0f);
}

// Use an RGB F0 for colored lobes such as the standard material base layer.
float3 GetMultipleScatteringEnergyCompensation(float dfgFirstSum, float3 F0) {
    return 1.0f + F0 * (rcp(dfgFirstSum) - 1.0f);
}
#endif

// 광선에 수직인 면의 단위 조도에 대한 standard material 의 direct lighting 응답을 계산한다.
float3 EvaluateStandardDirectBRDF(in ShadingParams shading, float3 lightDir) {
    const float3 halfVectorUnnormalized = lightDir + shading.viewDir;
    const float halfVectorLengthSquared = dot(halfVectorUnnormalized, halfVectorUnnormalized);
    if (halfVectorLengthSquared <= FLT_MIN) {
        return 0.0f;
    }

    const float3 halfVector = halfVectorUnnormalized * rsqrt(halfVectorLengthSquared);
    const float NdotL = saturate(dot(shading.normal, lightDir));
    const float NdotH = saturate(dot(shading.normal, halfVector));
    const float VdotH = saturate(dot(shading.viewDir, halfVector));
    const float LdotV = saturate(dot(lightDir, shading.viewDir));
    const float fresnelSecondFactor = F_SecondFactorSchlickSG(VdotH);

    float3 specularFresnel;
    float3 specular = SpecularBRDF(NdotL, shading.normalDotView, NdotH, VdotH, shading.roughness, shading.linearRoughness, shading.specularF0, fresnelSecondFactor, specularFresnel);
    const float3 diffuse = DiffuseBRDF(NdotL, shading.normalDotView, LdotV, VdotH, shading.diffuseColor, shading.linearRoughness, 0.0f) * (1.0f - specularFresnel);

#if USE_MULTIPLE_SCATTERING_COMPENSATION
    specular *= shading.energyCompensation;
#endif

#if _CLEARCOAT != 0
    const float clearCoatNdotL = saturate(dot(shading.clearCoatNormal, lightDir));
    const float clearCoatNdotH = saturate(dot(shading.clearCoatNormal, halfVector));

    float clearCoatFresnel;
    float clearCoatSpecular = ClearCoatBRDF(clearCoatNdotH, VdotH, shading.clearCoat, shading.clearCoatLinearRoughness, fresnelSecondFactor, clearCoatFresnel);
    const float baseLayerAttenuation = 1.0f - clearCoatFresnel;

#if USE_MULTIPLE_SCATTERING_COMPENSATION
    clearCoatSpecular *= shading.clearCoatEnergyCompensation;
#endif

    return (diffuse + specular) * baseLayerAttenuation * baseLayerAttenuation * NdotL + clearCoatSpecular * clearCoatNdotL;
#else
    return (diffuse + specular) * NdotL;
#endif
}

// Maps linear roughness to a prefiltered environment cubemap mip level.
float LinearRoughnessToMipLevel(float linearRoughness, float maxMipLevel) {
    return maxMipLevel * linearRoughness * (1.7f - 0.7f * linearRoughness);
}

// Computes the DFG second sum from a preintegrated DFG value and F0.
float3 GetSpecularEnvSecondSum(float2 preDFG, float3 F0) {
    return F0 * preDFG.x + float3(preDFG.y, preDFG.y, preDFG.y);
}

// Approximates the dominant specular reflection direction for a rough surface.
float3 GetSpecularDominantDir(float3 N, float3 S, float linearRoughness) {
    const float linearSmoothness = 1.0f - linearRoughness;
    return lerp(N, S, linearSmoothness * (sqrt(linearSmoothness) + linearRoughness));
}

// Computes specular occlusion from ambient occlusion and view-dependent surface properties.
float GetSpecularOcclusionFromAmbientOcclusion(float NdotV, float ambientOcclusion, float roughness) {
    return saturate(pow(NdotV + ambientOcclusion, exp2(-16.0f * roughness - 1.0f)) - 1.0f + ambientOcclusion);
}

#endif // __STANDARD_BRDF_HLSLI__
