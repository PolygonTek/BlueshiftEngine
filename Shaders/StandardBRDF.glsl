#ifndef STANDARD_BRDF_INCLUDED
#define STANDARD_BRDF_INCLUDED

$include "StandardConfig.glsl"
$include "BRDFLibrary.glsl"

#if defined(DIRECT_LIGHTING)

//----------------------------------
// Diffuse BRDF
//----------------------------------
vec3 DiffuseBRDF(float NdotL, float NdotV, float VdotH, vec3 albedo, float roughness, float linearRoughness) {
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

#if _ANISO != 0
vec3 AnisotropicBRDF(vec3 H, float NdotL, float NdotV, float NdotH, float VdotH, float roughness, float linearRoughness, vec3 F0, float FSecondFactor, out vec3 F) {
    float TdotH = dot(shading.anisotropicT, H);
    float BdotH = dot(shading.anisotropicB, H);
    float TdotL = dot(shading.anisotropicT, shading.l);
    float BdotL = dot(shading.anisotropicB, shading.l);
    float TdotV = dot(shading.anisotropicT, shading.v);
    float BdotV = dot(shading.anisotropicB, shading.v);

    float at, ab;
    RoughnessToAnisotropyRoughness(shading.anisotropy, linearRoughness, at, ab);

    // Normal distribution term
    float D = D_GGXAniso(NdotH, TdotH, BdotH, at, ab);
    
    // Geometric visibility term
    float G = G_SmithGGXCorrelatedAniso(NdotV, NdotL, TdotV, BdotV, TdotL, BdotL, at, ab);

    // Fresnel reflection term
    F = F0 + (vec3(1.0) - F0) * FSecondFactor;

    // Microfacets specular BRDF = D * G * F (G term is already divided by (4 * NdotL * NdotV))
    return D * G * F;
}
#endif

vec3 IsotropicBRDF(float NdotL, float NdotV, float NdotH, float VdotH, float roughness, float linearRoughness, vec3 F0, float FSecondFactor, out vec3 F) {
    // Normal distribution term
#if PBR_SPECULAR_D == PBR_SPECULAR_D_BLINN
    float D = D_Blinn(NdotH, linearRoughness);
#elif PBR_SPECULAR_D == PBR_SPECULAR_D_BECKMANN
    float D = D_Beckmann(NdotH, linearRoughness);
#elif PBR_SPECULAR_D == PBR_SPECULAR_D_GGX
    float D = D_GGX(NdotH, linearRoughness);
#endif

    // Geometric visibility term
#if PBR_SPECULAR_G == PBR_SPECULAR_G_NEUMANN
    float G = G_Neumann(NdotV, NdotL);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_KELEMEN
    float G = G_Kelemen(VdotH);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_COOK_TORRANCE
    float G = G_CookTorrance(NdotV, NdotL, NdotH, VdotH);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_SCHLICK_GGX
    // Disney's modification to reduce "hotness" by remapping roughness using (roughness + 1) / 2 before squaring.
    float k = roughness + 1.0;
    k = k * k * 0.25; // k for direct lighting
    float G = G_SchlickGGX(NdotV, NdotL, k);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_SMITH_GGX
    float G = G_SmithGGXCorrelated(NdotV, NdotL, linearRoughness);
#elif PBR_SPECULAR_G == PBR_SPECULAR_G_SMITH_GGX_FAST
    float G = G_SmithGGXCorrelatedFast(NdotV, NdotL, linearRoughness);
#endif

    // Fresnel reflection term
    F = F0 + (vec3(1.0) - F0) * FSecondFactor;

    // Microfacets specular BRDF = D * G * F (G term is already divided by (4 * NdotL * NdotV))
    return D * G * F;
}

//----------------------------------
// Specular BRDF
//----------------------------------
vec3 SpecularBRDF(vec3 H, float NdotL, float NdotV, float NdotH, float VdotH, float roughness, float linearRoughness, vec3 F0, float FSecondFactor, out vec3 F) {
#if _ANISO != 0
    return AnisotropicBRDF(H, NdotL, NdotV, NdotH, VdotH, roughness, linearRoughness, F0, FSecondFactor, F);
#else
    return IsotropicBRDF(NdotL, NdotV, NdotH, VdotH, roughness, linearRoughness, F0, FSecondFactor, F);
#endif
}

//----------------------------------
// ClearCoat BRDF
//----------------------------------
float ClearCoatBRDF(float NdotH, float VdotH, float clearCoatReflectivity, float clearCoatRoughness, float clearCoatLinearRoughness, float FSecondFactor, out float F) {
#if PBR_CLEARCOAT_D == PBR_SPECULAR_D_GGX
    float D = D_GGX(NdotH, clearCoatLinearRoughness);
#endif

#if PBR_CLEARCOAT_G == PBR_SPECULAR_G_KELEMEN
    float G = G_Kelemen(VdotH);
#endif

    // IOR of clear coatted layer is 1.5
    float F0 = 0.04; // 0.04 == IorToF0(1.5)
    // F_Schlick
    F = F0 + (1.0 - F0) * FSecondFactor;
    F *= clearCoatReflectivity;

    return D * G * F;
}

vec2 GetPrefilteredDFG(float NdotV, float roughness) {
#if 0
    // Zioma's approximation based on Karis
    return vec2(1.0, pow(1.0 - max(roughness, NdotV), 3.0));
#else
    // Karis' approximation based on Lazarov's
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572,  0.022);
    const vec4 c1 = vec4( 1.0,  0.0425,  1.040, -0.040);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
#endif
}

//----------------------------------
// Direct Lighting
//----------------------------------
vec3 DirectLit_Standard() {
    vec3 H = normalize(shading.l + shading.v);

    float NdotL = saturate(dot(shading.n, shading.l));
    float NdotV = shading.ndotv;
    float NdotH = saturate(dot(shading.n, H));
    float VdotH = saturate(dot(shading.v, H));

    float FSecondFactor = F_SecondFactorSchlickSG(VdotH);

    vec3 Fs;
    vec3 Cs = SpecularBRDF(H, NdotL, NdotV, NdotH, VdotH, shading.roughness, shading.linearRoughness, shading.specular.rgb, FSecondFactor, Fs);

#ifdef USE_MULTIPLE_SCATTERING_COMPENSATION
    Cs *= shading.energyCompensation;
#endif

    // From specular reflection term Fs, we can directly calculate the ratio of refraction.
    vec3 Cd = DiffuseBRDF(NdotL, NdotV, VdotH, shading.diffuse.rgb, shading.roughness, shading.linearRoughness) * (vec3(1.0) - Fs);

#if _CLEARCOAT == 0
    vec3 color = (Cd + Cs) * NdotL;
#else
    #if _CC_NORMAL == 1
        float clearCoatNdotL = saturate(dot(shading.clearCoatN, shading.l));
        float clearCoatNdotH = saturate(dot(shading.clearCoatN, H));
    #else
        float clearCoatNdotL = NdotL;
        float clearCoatNdotH = NdotH;
    #endif

    float Fcc;
    float Ccc = ClearCoatBRDF(clearCoatNdotH, VdotH, shading.clearCoat, shading.clearCoatRoughness, shading.clearCoatLinearRoughness, FSecondFactor, Fcc);

    float Fattenuation = 1.0 - Fcc;

    vec3 color = (Cd + Cs) * Fattenuation * Fattenuation * NdotL;

    color += vec3(Ccc * clearCoatNdotL);
#endif

    // Incident radiance is translated to PI * LightColor in direct lighting computation.
    return color * PI;
}

#endif

#if defined(INDIRECT_LIGHTING)

vec3 GetDiffuseEnv(vec3 N, vec3 albedo) {
    vec3 d1 = texCUBE(irradianceEnvCubeMap0, N).rgb;
    //vec3 d2 = texCUBE(irradianceEnvCubeMap1, N).rgb;

#if USE_SRGB_TEXTURE == 0
    d1 = LinearToGamma(d1);
    //d2 = LinearToGamma(d2);
#endif

    return albedo * d1;//mix(d1, d2, ambientLerp);
}

vec3 GetSpecularEnvFirstSum(vec3 S, float linearRoughness) {
    vec4 sampleVec;
    sampleVec.xyz = S;

    // Convert linear roughness to mip level
    sampleVec.w = 7.0 * linearRoughness * (1.7 - 0.7 * linearRoughness);

    vec3 s1 = texCUBElod(prefilteredEnvCubeMap0, sampleVec).rgb;
    //vec3 s2 = texCUBElod(prefilteredEnvCubeMap1, sampleVec).rgb;

#if USE_SRGB_TEXTURE == 0
    s1 = LinearToGamma(s1);
    //s2 = LinearToGamma(s2);
#endif

    return s1;
}

vec2 GetPrefilteredDFG_LUT(float NdotV, float roughness) {
    return tex2D(prefilteredDfgMap, vec2(NdotV, roughness)).xy;
}

vec3 GetSpecularEnvSecondSum(vec2 prefilteredDfg, vec3 F0) {
    return F0 * prefilteredDfg.x + prefilteredDfg.yyy;
}

//----------------------------------
// Indirect Lighting
//----------------------------------
vec3 IndirectLit_Standard(vec3 S) {
    vec3 specularEnvSum1 = GetSpecularEnvFirstSum(S, shading.linearRoughness);
    vec3 specularEnvSum2 = GetSpecularEnvSecondSum(shading.dfg, shading.specular.rgb);

    vec3 Cs = specularEnvSum1 * specularEnvSum2 * shading.energyCompensation;

    float FSecondFactor = F_SecondFactorSchlickSG(shading.ndotv);

    // F_SchlickRoughness
    vec3 Fs = shading.specular.rgb + (max(vec3(1.0 - shading.roughness), shading.specular.rgb) - shading.specular.rgb) * FSecondFactor;
    //vec3 Fs = shading.specular.rgb + (vec3(1.0) - shading.specular.rgb) * FSecondFactor;

    vec3 diffuseEnv = GetDiffuseEnv(shading.n, shading.diffuse.rgb);

    vec3 Cd = diffuseEnv * (vec3(1.0) - Fs);

#if _CLEARCOAT == 0
    vec3 color = Cd + Cs;
#else
    #if _CC_NORMAL == 1
        float clearCoatNdotV = max(dot(shading.clearCoatN, shading.v), 0.0);

        vec3 clearCoatS = reflect(-shading.v, shading.clearCoatN);
    #else
        float clearCoatNdotV = shading.ndotv;

        vec3 clearCoatS = S;
    #endif

    vec2 ccDfg = GetPrefilteredDFG_LUT(clearCoatNdotV, shading.clearCoatRoughness);
    
    vec3 clearCoatEnvSum1 = GetSpecularEnvFirstSum(S, shading.clearCoatLinearRoughness);
    vec3 clearCoatEnvSum2 = GetSpecularEnvSecondSum(ccDfg, vec3(0.04));

    vec3 Ccc = clearCoatEnvSum1 * clearCoatEnvSum2 * shading.clearCoat;
    
    // IOR of clear coatted layer is 1.5
    float F0 = 0.04; // 0.04 == IorToF0(1.5)
    float Fcc = F0 + (1.0 - F0) * FSecondFactor;
    Fcc *= shading.clearCoat;

    float Fattenuation = 1.0 - Fcc;

    vec3 color = (Cd + Cs) * Fattenuation * Fattenuation + Ccc;
#endif

    return color;
}

#endif

#endif
