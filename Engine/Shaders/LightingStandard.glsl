#ifndef LIGHTING_STANDARD_INCLUDED
#define LIGHTING_STANDARD_INCLUDED

#define PBR_DIFFUSE 1
#define PBR_SPEC_D 2
#define PBR_SPEC_G 3

float pow5(float f) {
    float f2 = f * f;
    return f2 * f2 * f;
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
    return pow(NdotH, n) * (n + 2.0) * INV_TWO_PI;
}

float D_Beckmann(float NdotH, float m) {
    float m2 = m * m;
    float NdotH2 = NdotH * NdotH;
    return exp((NdotH2 - 1.0) / (m2 * NdotH2)) / (PI * m2 * NdotH2 * NdotH2);
}

// Trowbridge-Reitz aka GGX
float D_GGX(float NdotH, float m) {
    float m2 = m * m;
    float denom = NdotH * NdotH * (m2 - 1.0) + 1.0;
    return m2 / (PI * denom * denom);
}

float D_GGXAniso(float NdotH, float XdotH, float YdotH, float mx, float my) {
    float denom = XdotH * XdotH / (mx * mx) + YdotH * YdotH / (my * my) + NdotH * NdotH;
    return 1.0 / (PI * mx * my * denom * denom);
}

//---------------------------------------------------
// Geometric visibility functions
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

float G_SchlickGGX(float NdotV, float NdotL, float m) {
    float k = m * 0.5f;
    float invK = 1.0 - k;
    float GV = NdotV * invK + k;
    float GL = NdotL * invK + k;
    return 1.0 / (GL * GV);
}

//---------------------------------------------------
// Fresnel functions
//---------------------------------------------------

// Fresnel using Schlick's approximation
vec3 F_Schlick(vec3 F0, float VdotH) {
    return F0 + (vec3(1.0) - F0) * pow5(1.0 - VdotH);
}

// Fresnel using Schlick's approximation with spherical Gaussian approximation
vec3 F_SchlickSG(vec3 F0, float VdotH) {
    return F0 + (vec3(1.0) - F0) * exp2(-5.55473 * VdotH - 6.98316) * VdotH;
}

// Fresnel injected roughness term
vec3 F_SchlickRoughness(vec3 F0, float roughness, float VdotH) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow5(1.0 - VdotH);
}

//---------------------------------------------------

void litStandard(in vec3 L, in vec3 N, in vec3 V, in vec3 albedo, in float roughness, in float metalness, out vec3 color) {
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
    vec3 Cd = albedo * litDiffuseBurley(NdotL, NdotV, VdotH, roughness);
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
    float G = G_SchlickGGX(NdotV, NdotL, m);
#endif

    // Specular Fresnel term
    vec3 F0 = mix(vec3(0.04), albedo, metalness);
    vec3 F = F_SchlickSG(F0, VdotH); 

    // Microfacets specular BRDF = D * G * F / 4 (G term is divided by (NdotL * NdotV))
    vec3 BRDFspec = D * G * F * 0.25;

    // Final specular lighting
    vec3 Cs = PI * NdotL * BRDFspec;

    // Final diffuse lighting with fresnel
    Cd *= (vec3(1.0) - F) * (1.0 - metalness);
    //Cd *= (vec3(1.0) - F_SchlickSG(F0, NdotL));

    color = Cd + Cs;
}

#endif
