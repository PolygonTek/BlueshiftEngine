#ifndef STANDARD_BRDF_INCLUDED
#define STANDARD_BRDF_INCLUDED

$include "StandardConfig.glsl"
$include "BRDFLibrary.glsl"

float clearCoatReflectivity = 1.0;
float clearCoatRoughness = 0.1;

//----------------------------------
// Diffuse BRDF
//----------------------------------
vec3 DiffuseBRDF(float NdotL, float NdotV, float VdotH, vec3 albedo, float roughness) {
    // We adopted Disney's reparameterization of a = roughness^2
    // a means perceptual linear roughness.
    float linearRoughness = roughness * roughness;

#if PBR_DIFFUSE == PBR_DIFFUSE_LAMBERT
    return albedo * Fd_Lambert(NdotL);
#elif PBR_DIFFUSE == PBR_DIFFUSE_WRAPPED
    return albedo * Fd_Wrap(NdotL, diffuseWrap);
#elif PBR_DIFFUSE == PBR_DIFFUSE_BURLEY
    return albedo * Fd_Burley(NdotL, NdotV, VdotH, linearRoughness);
#elif PBR_DIFFUSE == PBR_DIFFUSE_OREN_NAYAR
    float LdotV = max(dot(L, V), 0);
    return albedo * Fd_OrenNayar(NdotL, NdotV, LdotV, linearRoughness);
#endif
}

//----------------------------------
// Specular BRDF
//----------------------------------
vec3 SpecularBRDF(float NdotL, float NdotH, float NdotV, float VdotH, vec3 F0, float roughness, out vec3 Fs) {
    // We adopted Disney's reparameterization of a = roughness^2
    // a means perceptual linear roughness.
    float linearRoughness = roughness * roughness;

    // Normal distribution term
#if PBR_SPECULAR_D == PBR_SPECULAR_D_BLINN
    float D = D_Blinn(NdotH, linearRoughness);
#elif PBR_SPECULAR_D == PBR_SPECULAR_D_BECKMANN
    float D = D_Beckmann(NdotH, linearRoughness);
#elif PBR_SPECULAR_D == PBR_SPECULAR_D_GGX
    float D = D_GGX(NdotH, linearRoughness);
#elif PBR_SPECULAR_D == PBR_SPECULAR_D_GGX_ANISO
    float D = D_GGXAniso(NdotH, XdotH, YdotH, ax, ay);
#endif

    // Geometric visibility term
#if PBR_SPECULAR_G == PBR_SPECULAR_G_NEUMANN
    float G = G_Neumann(NdotV, NdotL);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_KELEMEN
    float G = G_Kelemen(VdotH);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_COOK_TORRANCE
    float G = G_CookTorrance(NdotV, NdotL, NdotH, VdotH);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_GGX
    // Disney's modification to reduce "hotness" by remapping roughness using (roughness + 1) / 2 before squaring.
    float k = roughness + 1.0; // k for direct lighting
    float G = G_SchlickGGX(NdotV, NdotL, (k * k) * 0.125);
#endif

    // Fresnel reflection term
    Fs = F_SchlickSG(F0, VdotH);

    // Microfacets specular BRDF = D * G * F / 4 (G term is already divided by (NdotL * NdotV))
    return D * G * Fs * 0.25;
}

//----------------------------------
// ClearCoat BRDF
//----------------------------------
vec3 ClearCoatBRDF(vec3 H, float NdotL, float NdotH, float NdotV, float VdotH, float clearCoatReflectivity, float clearCoatRoughness, out vec3 Fc) {
#if CLEARCOAT_NORMAL == 1
    float clearCoatNdotH = saturate(dot(clearCoatN, H));
#else
    float clearCoatNdotH = NdotH;
#endif

    float clearCoatLinearRoughness = clearCoatRoughness * clearCoatRoughness;

#if PBR_CLEARCOAT_D == PBR_SPECULAR_D_GGX
    float D = D_GGX(clearCoatNdotH, clearCoatLinearRoughness);
#endif

#if PBR_CLEARCOAT_G == PBR_SPECULAR_G_KELEMEN
    float G = G_Kelemen(VdotH);
#elif PBR_CLEARCOAT_G == PBR_SPECULAR_G_GGX
    // Disney's modification to reduce "hotness" by remapping roughness using (roughness + 1) / 2 before squaring.
    float k = clearCoatRoughness + 1.0; // k for direct lighting
    float G = G_SchlickGGX(NdotV, NdotL, (k * k) * 0.125);
#endif

    // IOR of clear coatted layer is 1.5.
    // 0.04 = IorToF0(1.5)
    Fc = F_SchlickSG(vec3(0.04), VdotH) * clearCoatReflectivity;

    return D * G * Fc * 0.25;
}

//----------------------------------
// Direct Lighting
//----------------------------------
vec3 DirectLit_Standard(vec3 L, vec3 N, vec3 V) {
    vec3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float NdotV = saturate(dot(N, V));
    float VdotH = saturate(dot(V, H));

    vec3 Fs;
    vec3 Cs = SpecularBRDF(NdotL, NdotH, NdotV, VdotH, material.specular.rgb, material.roughness, Fs);

    // From specular reflection term Fs, we can directly calculate the ratio of refraction.
    vec3 Cd = DiffuseBRDF(NdotL, NdotV, VdotH, material.diffuse.rgb, material.roughness) * (vec3(1.0) - Fs);

#if _CLEARCOAT == 0
    vec3 color = (Cd + Cs) * NdotL;
#else
    vec3 Fc;
    vec3 Cc = ClearCoatBRDF(H, NdotL, NdotH, NdotV, VdotH, material.clearCoat, material.clearCoatRoughness, Fc);

    vec3 color = (Cd + Cs) * (vec3(1.0) - Fc) * NdotL;

    #if _CLEARCOAT_NORMAL == 1
        float clearCoatNdotL = saturate(dot(material.clearCoatN, L));
    #else
        float clearCoatNdotL = NdotL;
    #endif

    color += Cc * clearCoatNdotL;
#endif

    // Incident radiance is translated to PI * LightColor in direct lighting computation.
    return color * PI;
}

vec3 GetSpecularEnvFirstSum(vec3 S, float roughness) {
    vec4 sampleVec;
    sampleVec.xyz = S;
    sampleVec.w = roughness * 7.0; // FIXME: 7.0 == maximum mip level

    vec3 s1 = texCUBElod(prefilteredEnvCubeMap0, sampleVec).rgb;
    //vec3 s2 = texCUBElod(prefilteredEnvCubeMap1, sampleVec).rgb;

#if USE_SRGB_TEXTURE == 0
    s1 = linearToGammaSpace(s1);
    //s2 = linearToGammaSpace(s2);
#endif

    return s1;
}

vec3 GetSpecularEnvSecondSum(float NdotV, float roughness, vec3 F0, float FScale, out vec3 F) {
    vec2 envBRDFs = tex2D(integrationLUTMap, vec2(NdotV, roughness)).xy;

    F = F_SchlickRoughness(F0, roughness, NdotV) * FScale;

    return F * envBRDFs.x + envBRDFs.yyy;
}

vec3 GetDiffuseEnv(vec3 N, vec3 albedo) {
    vec3 d1 = texCUBE(irradianceEnvCubeMap0, N).rgb;
    //vec3 d2 = texCUBE(irradianceEnvCubeMap1, N).rgb;

#if USE_SRGB_TEXTURE == 0
    d1 = linearToGammaSpace(d1);
    //d2 = linearToGammaSpace(d2);
#endif

    return albedo * d1;//mix(d1, d2, ambientLerp);
}

//----------------------------------
// Indirect Lighting
//----------------------------------
vec3 IndirectLit_Standard(vec3 N, vec3 V, vec3 S) {
    float NdotV = max(dot(N, V), 0.0);

    vec3 specularEnvSum1 = GetSpecularEnvFirstSum(S, material.roughness);
    vec3 Fs;
    vec3 specularEnvSum2 = GetSpecularEnvSecondSum(NdotV, material.roughness, material.specular.rgb, 1.0, Fs);

    vec3 Cs = specularEnvSum1 * specularEnvSum2;

    vec3 Cd = GetDiffuseEnv(N, material.diffuse.rgb) * (vec3(1.0) - Fs);

#if _CLEARCOAT == 0
    vec3 color = Cd + Cs;
#else
    #if _CLEARCOAT_NORMAL == 1
        float clearCoatNdotV = max(dot(material.clearCoatN, V), 0.0);
    #else
        float clearCoatNdotV = NdotV;
    #endif

    vec3 clearCoatEnvSum1 = GetSpecularEnvFirstSum(S, material.clearCoatRoughness);
    vec3 Fc;
    vec3 clearCoatEnvSum2 = GetSpecularEnvSecondSum(clearCoatNdotV, material.clearCoatRoughness, vec3(0.04), material.clearCoat, Fc);

    vec3 Cc = clearCoatEnvSum1 * clearCoatEnvSum2;

    vec3 color = (Cd + Cs) * (vec3(1.0) - Fc) + Cc;
#endif

    return color;
}

#endif
