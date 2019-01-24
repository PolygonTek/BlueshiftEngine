#ifndef BRDF_LIBRARY_INCLUDED
#define BRDF_LIBRARY_INCLUDED

#if defined(GL_ES)
// min roughness such that (MIN_ROUGHNESS^4) > 0 in fp16 (i.e. 2^(-14/4), slightly rounded up)
#define MIN_ROUGHNESS           0.089
#define MIN_LINEAR_ROUGHNESS    0.007921
#else
#define MIN_ROUGHNESS           0.045
#define MIN_LINEAR_ROUGHNESS    0.002025
#endif

// Clear coat layers are almost always glossy.
#define MIN_CLEARCOAT_ROUGHNESS MIN_ROUGHNESS
#define MAX_CLEARCOAT_ROUGHNESS 0.6

// Convert perceptual glossiness to specular power from [0, 1] to [2, 8192]
float GlossinessToSpecularPower(float glossiness) {
    return exp2(11.0 * glossiness + 1.0); 
}

// Anisotropic parameters: at and ab are the roughness along the tangent and bitangent.
// to simplify materials, we derive them from a single roughness parameter.
// Kulla 2017, "Revisiting Physically Based Shading at Imageworks"
void RoughnessToAnisotropyRoughness(float anisotropy, float linearRoughness, out float roughnessT, out float roughnessB) {
    roughnessT = max(linearRoughness * (1.0 + anisotropy), MIN_LINEAR_ROUGHNESS);
    roughnessB = max(linearRoughness * (1.0 - anisotropy), MIN_LINEAR_ROUGHNESS);
}

//---------------------------------------------------
// diffuse BRDF function
//---------------------------------------------------

float Fd_Lambert(float NdotL) {
    return INV_PI;
}

// Energy conserving wrap diffuse term, does *not* include the divide by PI
float Fd_Wrap(float NdotL, float w) {
    float onePlusW = 1.0 + w;
    return saturate((NdotL + w) / (onePlusW * onePlusW));
}

float Fd_Burley(float NdotL, float NdotV, float VdotH, float roughness) {
    float F90 = 0.5 + 2.0 * VdotH * VdotH * roughness;
    float FdL = 1.0 + (F90 - 1.0) * pow5(1.0 - NdotL);
    float FdV = 1.0 + (F90 - 1.0) * pow5(1.0 - NdotV);
    return FdL * FdV * INV_PI;
}

float Fd_OrenNayar(float NdotL, float NdotV, float LdotV, float roughness) { 
    float sigma2 = roughness * roughness;
    float A = 1.0 - sigma2 * (0.5 / (sigma2 + 0.33) + 0.17 / (sigma2 + 0.13));
    float B = 0.45 * sigma2 / (sigma2  + 0.09);

    // A tiny improvement of Oren-Nayar [Yasuhiro Fujii]
    // http://mimosa-pudica.net/improved-oren-nayar.html
    //float A = 1.0 / (PI + 0.90412966 * roughness);
    //float B = roughness / (PI + 0.90412966 * roughness);

    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));
    
    return (A + B * s / t) * INV_PI;
}

//---------------------------------------------------
// Normal distribution functions
//---------------------------------------------------

float D_Blinn(float NdotH, float a) {
    float a2 = a * a;
    float n = 2.0 / a2 - 2.0;
    n = max(n, 1e-4); // prevent possible zero
    return pow(NdotH, n) * (n + 2.0) * INV_TWO_PI;
}

float D_Beckmann(float NdotH, float a) {
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    return exp((NdotH2 - 1.0) / (a2 * NdotH2 + 1e-7)) * INV_PI / (a2 * NdotH2 * NdotH2 + 1e-7);
}

// Trowbridge-Reitz aka GGX
float D_GGX(float NdotH, float a) {
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 * INV_PI / (denom * denom + 1e-7);
}

// Anisotropic GGX
// Burley 2012, Physically-Based Shading at Disney
float D_GGXAniso(float NdotH, float TdotH, float BdotH, float roughnessT, float roughnessB) {
#if 0
    // scalar mul: 9
    // scalar div: 3
    float ax2 = roughnessT * roughnessT;
    float ay2 = roughnessB * roughnessB;
    float denom = TdotH * TdotH / ax2 + BdotH * BdotH / ay2 + NdotH * NdotH;
    return 1.0 / (PI * roughnessT * roughnessB * denom * denom);
#else
    // scalar mul: 7
    // scalar div: 1
    // vec3 dot: 1
    float a2 = roughnessT * roughnessB;
    vec3 v = vec3(roughnessB * TdotH, roughnessT * BdotH, a2 * NdotH);
    float a2_v2 = a2 / dot(v, v);
    return INV_PI * a2 * a2_v2 * a2_v2;
#endif
}

//---------------------------------------------------
// Geometric visibility functions - divided by (4 * NdotL * NdotV)
//---------------------------------------------------

float G_Implicit() {
    return 0.25;
}

float G_Neumann(float NdotV, float NdotL) {
    return 0.25 / (max(NdotL, NdotV) + 1e-7);
}

// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
float G_Kelemen(float VdotH) {
    return 0.25 / (VdotH * VdotH + 1e-7);
}

float G_CookTorrance(float NdotV, float NdotL, float NdotH, float VdotH) {
    float k = 2.0 * NdotH / VdotH;
    float G = min(1.0, min(k * NdotV, k * NdotL));
    return G / (4.0 * NdotL * NdotV + 1e-7);
}

float G_Schlick(float NdotV, float NdotL, float k) {
    float oneMinusK = 1.0 - k;
    vec2 G = vec2(NdotV, NdotL) * oneMinusK + k;
    return 0.25 / (G.x * G.y + 1e-7);
}

// Smith Joint GGX Anisotropic Visibility
// Taken from https://cedec.cesa.or.jp/2015/session/ENG/14698.html
float G_SmithGGXCorrelatedAniso(float NdotV, float NdotL, float TdotV, float BdotV, float TdotL, float BdotL, float roughnessT, float roughnessB) {
    float lambdaV = NdotL * length(vec3(roughnessT * TdotV, roughnessB * BdotV, NdotV));
    float lambdaL = NdotV * length(vec3(roughnessT * TdotL, roughnessB * BdotL, NdotL));
    return 0.5 / (lambdaV + lambdaL);
}

//---------------------------------------------------
// Fresnel functions
//---------------------------------------------------

float F_SecondFactorSchlick(float cosTheta) {
    return pow5(1.0 - cosTheta);
}

// Fresnel using Schlick's approximation
vec3 F_Schlick(vec3 F0, float cosTheta) {
    return F0 + (vec3(1.0) - F0) * F_SecondFactorSchlick(cosTheta);
}

float F_SecondFactorSchlickSG(float cosTheta) {
    return exp2((-5.55473 * cosTheta - 6.98316) * cosTheta);
}

// Fresnel using Schlick's approximation with spherical Gaussian approximation
vec3 F_SchlickSG(vec3 F0, float cosTheta) {
    return F0 + (vec3(1.0) - F0) * F_SecondFactorSchlickSG(cosTheta);
}

// Fresnel injected roughness term
vec3 F_SchlickRoughness(vec3 F0, float roughness, float cosTheta) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * F_SecondFactorSchlick(cosTheta);
}

//---------------------------------------------------
// IOR
//---------------------------------------------------

float IorToF0(float transmittedIor, float incidentIor) {
    float r = (transmittedIor - incidentIor) / (transmittedIor + incidentIor);
    return r * r;
}

float IorToF0(float transmittedIor) {
    float r = (1.0 + transmittedIor) / (1.0 - transmittedIor);
    return r * r;
}

float F0ToIor(float f0) {
    float r = sqrt(f0);
    return (1.0 + r) / (1.0 - r);
}

vec3 F0ForAirInterfaceToF0ForClearCoat15(vec3 f0) {
    // Approximation of IorToF0(F0ToIor(f0), 1.5)
    // This assumes that the clear coat layer has an IOR of 1.5
#if defined(GL_ES)
    return saturate(f0 * (f0 * 0.526868 + 0.529324) - 0.0482256);
#else
    return saturate(f0 * (f0 * (0.941892 - 0.263008 * f0) + 0.346479) - 0.0285998);
#endif
}

#endif
