#ifndef STANDARD_BRDF_INCLUDED
#define STANDARD_BRDF_INCLUDED

$include "StandardConfig.glsl"

float pow4(float f) {
    float f2 = f * f;
    return f2 * f2;
}

float pow5(float f) {
    float f2 = f * f;
    return f2 * f2 * f;
}

float glossinessToSpecularPower(float glossiness) {
    return exp2(10.0 * glossiness + 1.0); 
}

float litDiffuseLambert(in float NdotL) {
    return NdotL;
}

float litDiffuseBurley(in float NdotL, in float NdotV, in float VdotH, in float roughness) {
    float Fd90 = 0.5 + 2.0 * VdotH * VdotH * roughness;
    float FdL = 1.0 + (Fd90 - 1.0) * pow5(1.0 - NdotL);
    float FdV = 1.0 + (Fd90 - 1.0) * pow5(1.0 - NdotV);
    return NdotL * FdL * FdV;
}

float litDiffuseOrenNayar(in float NdotL, in float NdotV, in float LdotV, in float roughness) { 
    float sigma2 = roughness * roughness;
    float A = 1.0 - sigma2 * (0.5 / (sigma2 + 0.33) + 0.17 / (sigma2 + 0.13));
    float B = 0.45 * sigma2 / (sigma2  + 0.09);

    // A tiny improvement of Oren-Nayar [Yasuhiro Fujii]
    // http://mimosa-pudica.net/improved-oren-nayar.html
    //float A = 1.0 / (PI + 0.90412966 * roughness);
    //float B = roughness / (PI + 0.90412966 * roughness);

    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));
    
    return NdotL * (A + B * s / t);
}

//---------------------------------------------------
// Normal distribution functions
//---------------------------------------------------

float D_Blinn(float NdotH, float m) {
    float m2 = m * m;
    float n = 2.0 / m2 - 2.0;
    n = max(n, 1e-4); // prevent possible zero
    return pow(NdotH, n) * (n + 2.0) * INV_TWO_PI;
}

float D_Beckmann(float NdotH, float m) {
    float m2 = m * m;
    float NdotH2 = NdotH * NdotH;
    return exp((NdotH2 - 1.0) / (m2 * NdotH2)) * INV_PI / (m2 * NdotH2 * NdotH2);
}

// Trowbridge-Reitz aka GGX
float D_GGX(float NdotH, float m) {
    float m2 = m * m;
    float denom = NdotH * NdotH * (m2 - 1.0) + 1.0;
    return m2 * INV_PI / (denom * denom + 1e-7);
}

float D_GGXAniso(float NdotH, float XdotH, float YdotH, float mx, float my) {
    float denom = XdotH * XdotH / (mx * mx) + YdotH * YdotH / (my * my) + NdotH * NdotH;
    return INV_PI * mx * my * denom * denom;
}

//---------------------------------------------------
// Geometric visibility functions - divided by (NdotL * NdotV)
//---------------------------------------------------

float G_Neumann(float NdotV, float NdotL) {
    return 1.0 / max(NdotL, NdotV);
}

float G_Kelemen(float VdotH) {
    return 1.0 / (VdotH * VdotH);
}

float G_CookTorrance(float NdotV, float NdotL, float NdotH, float VdotH) {
    float G = min(1.0, min(2.0 * NdotH * NdotV / VdotH, (2.0 * NdotH * NdotL) / VdotH));
    return G / (NdotL * NdotV);
}

float G_SchlickGGX(float NdotV, float NdotL, float k) {
    float oneMinusK = 1.0 - k;
    float GV = NdotV * oneMinusK + k;
    float GL = NdotL * oneMinusK + k;
    return 1.0 / (GL * GV);
}

//---------------------------------------------------
// Fresnel functions
//---------------------------------------------------

// Fresnel using Schlick's approximation
vec3 F_Schlick(vec3 F0, float NdotV) {
    return F0 + (vec3(1.0) - F0) * pow5(1.0 - NdotV);
}

// Fresnel using Schlick's approximation with spherical Gaussian approximation
vec3 F_SchlickSG(vec3 F0, float NdotV) {
    return F0 + (vec3(1.0) - F0) * exp2((-5.55473 * NdotV - 6.98316) * NdotV);
}

// Fresnel injected roughness term
vec3 F_SchlickRoughness(vec3 F0, float roughness, float NdotV) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow5(1.0 - NdotV);
}

//---------------------------------------------------

vec3 DirectLit_Standard(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 F0, float roughness) {
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    //----------------------------------
    // Diffuse BRDF
    //----------------------------------
#if PBR_DIFFUSE == 0
    vec3 Cd = albedo * litDiffuseLambert(NdotL);
#elif PBR_DIFFUSE == 1
    vec3 Cd = albedo * litDiffuseBurley(NdotL, NdotV, VdotH, sqrt(roughness));
#elif PBR_DIFFUSE == 2
    float LdotV = max(dot(L, V), 0);
    vec3 Cd = albedo * litDiffuseOrenNayar(NdotL, NdotV, LdotV, roughness);
#endif

    //----------------------------------
    // Specular BRDF
    //----------------------------------
    float m = roughness * roughness;

    // Normal distribution term
#if PBR_SPEC_D == 0
    float D = D_Blinn(NdotH, m);
#elif PBR_SPEC_D == 1
    float D = D_Beckmann(NdotH, m);
#elif PBR_SPEC_D == 2
    float D = D_GGX(NdotH, m);
#elif PBR_SPEC_D == 3
    float D = D_GGXAniso(NdotH, XdotH, YdotH, mx, my);
#endif

    // Geometric visibility term
#if PBR_SPEC_G == 0
    float G = G_Neumann(NdotV, NdotL);
#elif PBR_SPEC_G == 1
    float G = G_Kelemen(VdotH);
#elif PBR_SPEC_G == 2
    float G = G_CookTorrance(NdotV, NdotL, NdotH, VdotH);
#elif PBR_SPEC_G == 3
    float k = roughness + 1.0; // k for direct lighting
    float G = G_SchlickGGX(NdotV, NdotL, (k * k) * 0.125);
#endif

    // Specular Fresnel term
    vec3 F = F_SchlickSG(F0, VdotH);

    // Microfacets specular BRDF = D * G * F / 4 (G term is divided by (NdotL * NdotV))
    vec3 BRDFspec = D * G * F * 0.25;

    // Final specular lighting
    vec3 Cs = PI * NdotL * BRDFspec;

    // Final diffuse lighting with fresnel
    Cd *= (vec3(1.0) - F);

    return Cd + Cs;
}

uniform float wrappedDiffuse;

// Phong/Blinn-Phong lighting
vec3 DirectLit_Phong(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 specular, float specularPower) {
    float NdotL = dot(N, L);

#if defined(_WRAPPED_DIFFUSE)
    float oneMinusW = 1.0 + wrappedDiffuse;
    vec3 Cd = albedo.rgb * (NdotL + wrappedDiffuse) / (oneMinusW * oneMinusW);
#else // Lambertian
    vec3 Cd = albedo.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    #ifdef USE_BLINN_PHONG
        vec3 H = normalize(L + V);

        float NdotH = max(dot(N, H), 0.0);

        float normFactor = specularPower * 0.125 + 1.0;

        vec3 Cs = specular.rgb * normFactor * pow(NdotH, specularPower);
    #else
        vec3 R = reflect(-L, N);

        float RdotV = max(dot(R, V), 0.0);

        float normFactor = specularPower * 0.5 + 1.0;

        vec3 Cs = specular.rgb * normFactor * pow(RdotV, specularPower);
    #endif
    
    return Cd * (vec3(1.0) - specular.rgb) + Cs;
#else
    return Cd;
#endif
}

// Phong/Blinn-Phong lighting with Fresnel
vec3 DirectLit_PhongFresnel(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 specular, float specularPower) {
    float NdotL = dot(N, L);

#if defined(_WRAPPED_DIFFUSE)
    float oneMinusW = 1.0 + wrappedDiffuse;
    vec3 Cd = albedo.rgb * (NdotL + wrappedDiffuse) / (oneMinusW * oneMinusW);
#else // Lambertian
    vec3 Cd = albedo.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    #ifdef USE_BLINN_PHONG
        vec3 H = normalize(L + V);
    
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);
    
        vec3 F = F_SchlickSG(specular.rgb, VdotH);

        float normFactor = specularPower * 0.125 + 1.0;

        vec3 Cs = F * normFactor * pow(NdotH, specularPower);
    #else
        vec3 R = reflect(-L, N);

        float RdotV = max(dot(R, V), 0.0);
        float NdotV = max(dot(N, V), 0.0);

        vec3 F = F_SchlickSG(specular.rgb, NdotV);

        float normFactor = specularPower * 0.5 + 1.0;

        vec3 Cs = F * normFactor * pow(RdotV, specularPower);
    #endif

    return Cd * (vec3(1.0) - F) + Cs;
#else
    return Cd;
#endif
}

uniform samplerCube irradianceEnvCubeMap0;
uniform samplerCube prefilteredEnvCubeMap0;
uniform vec3 probePosition0;
uniform vec3 probeMins0;
uniform vec3 probeMaxs0;

uniform samplerCube irradianceEnvCubeMap1;
uniform samplerCube prefilteredEnvCubeMap1;
uniform vec3 probePosition1;
uniform vec3 probeMins1;
uniform vec3 probeMaxs1;

uniform sampler2D integrationLUTMap;
uniform float ambientLerp;

vec3 IndirectLit_Standard(vec3 worldN, vec3 worldS, float NdotV, vec3 albedo, vec3 F0, float roughness) {
    vec3 d1 = texCUBE(irradianceEnvCubeMap0, worldN).rgb;
    //vec3 d2 = texCUBE(irradianceEnvCubeMap1, worldN).rgb;

    vec3 Cd = albedo * d1;//mix(d1, d2, ambientLerp);

    vec4 sampleVec;
    sampleVec.xyz = worldS;
    sampleVec.w = roughness * 7.0; // FIXME: 7.0 == maximum mip level

    vec3 s1 = texCUBElod(prefilteredEnvCubeMap0, sampleVec).rgb;
    //vec3 s2 = texCUBElod(prefilteredEnvCubeMap1, sampleVec).rgb;

    vec2 envBRDF = tex2D(integrationLUTMap, vec2(NdotV, roughness)).xy;

    vec3 F = F_SchlickRoughness(F0, roughness, NdotV);
    vec3 Cs = (F * envBRDF.x + envBRDF.y) * s1;

    return Cd * (vec3(1.0) - F) + Cs;
}

vec3 IndirectLit_PhongFresnel(vec3 worldN, vec3 worldS, float NdotV, vec3 albedo, vec3 specular, float specularPower, float roughness) {
    vec3 d1 = texCUBE(irradianceEnvCubeMap0, worldN).rgb;
    //vec3 d2 = texCUBE(irradianceEnvCubeMap1, worldN).rgb;

    vec3 Cd = albedo * d1;//mix(d1, d2, ambientLerp);

    // (log2(specularPower) - log2(maxSpecularPower)) / log2(pow(maxSpecularPower, -1/numMipmaps))
    // (log2(specularPower) - 11) / (-11/8)
    float specularMipLevel = -(8.0 / 11.0) * log2(specularPower) + 8.0;

    vec4 sampleVec;
    sampleVec.xyz = worldS;
    sampleVec.w = specularMipLevel;

    vec3 s1 = texCUBElod(prefilteredEnvCubeMap0, sampleVec).rgb;
    //vec3 s2 = texCUBElod(prefilteredEnvCubeMap1, sampleVec).rgb;

    vec3 F = F_SchlickRoughness(specular, roughness, NdotV);
    vec3 Cs = F * s1;

    return Cd * (vec3(1.0) - F) + Cs;
}

#endif
