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
float glossinessToSpecularPower(float glossiness) {
    return exp2(11.0 * glossiness + 1.0); 
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
    return exp((NdotH2 - 1.0) / (a2 * NdotH2)) * INV_PI / (a2 * NdotH2 * NdotH2);
}

// Trowbridge-Reitz aka GGX
float D_GGX(float NdotH, float a) {
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 * INV_PI / (denom * denom + 1e-7);
}

float D_GGXAniso(float NdotH, float XdotH, float YdotH, float ax, float ay) {
    float ax2 = ax * ax;
    float ay2 = ay * ay;
    float axy = ax * ay;
    float denom = XdotH * XdotH / ax2 + YdotH * YdotH / ay2 + NdotH * NdotH;
    return INV_PI / (axy * denom * denom);
}

//---------------------------------------------------
// Geometric visibility functions - divided by (4 * NdotL * NdotV)
//---------------------------------------------------

float G_Neumann(float NdotV, float NdotL) {
    return 0.25 / max(NdotL, NdotV);
}

// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
float G_Kelemen(float VdotH) {
    return 0.25 / (VdotH * VdotH);
}

float G_CookTorrance(float NdotV, float NdotL, float NdotH, float VdotH) {
    float G = min(1.0, min(2.0 * NdotH * NdotV / VdotH, (2.0 * NdotH * NdotL) / VdotH));
    return G / (4.0 * NdotL * NdotV);
}

float G_SchlickGGX(float NdotV, float NdotL, float k) {
    float oneMinusK = 1.0 - k;
    float GV = NdotV * oneMinusK + k;
    float GL = NdotL * oneMinusK + k;
    return 0.25 / (GL * GV);
}

float G_SmithGGXCorrelatedAniso(float NdotV, float NdotL, float XdotV, float YdotV, float XdotL, float YdotL, float ax, float ay) {
    float lambdaV = NdotL * length(vec3(ax * XdotV, ay * YdotV, NdotV));
    float lambdaL = NdotV * length(vec3(ax * XdotL, ay * YdotL, NdotL));
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
    float r = (incidentIor - transmittedIor) / (incidentIor + transmittedIor);
    return r * r;
}

float IorToF0(float transmittedIor) {
    float r = (1.0 - transmittedIor) / (1.0 + transmittedIor);
    return r * r;
}

float F0ToIor(float f0) {
    float r = sqrt(f0);
    return (1.0 - r) / (1.0 + r);
}

vec3 F0ToClearCoatToSurfaceF0(vec3 airToSurfaceF0) {
    // Approximation of IorToF0(F0ToIor(airToSurfaceF0), 1.5)
    // This assumes that the clear coat layer has an IOR of 1.5
#if defined(GL_ES)
    return saturate(airToSurfaceF0 * (airToSurfaceF0 * 0.526868 + 0.529324) - 0.0482256);
#else
    return saturate(airToSurfaceF0 * (airToSurfaceF0 * (0.941892 - 0.263008 * airToSurfaceF0) + 0.346479) - 0.0285998);
#endif
}

#endif
