#ifndef __BRDF_LIBRARY_HLSLI__
#define __BRDF_LIBRARY_HLSLI__

#include "../Common.hlsli"

// D3D12 uses the desktop roughness limits from BlueshiftRuntime.
static const float MinRoughness = 0.045f;
static const float MinLinearRoughness = 0.002025f;

// Clear coat layers are almost always glossy.
static const float MinClearCoatRoughness = MinRoughness;
static const float MaxClearCoatRoughness = 0.6f;

// Convert perceptual glossiness to specular power from [0, 1] to [2, 8192].
float GlossinessToSpecularPower(float glossiness) {
    return exp2(11.0f * glossiness + 1.0f);
}

// Anisotropic parameters: roughnessT and roughnessB are the roughness along the tangent and bitangent.
// To simplify materials, we derive them from a single roughness parameter.
// Kulla 2017, "Revisiting Physically Based Shading at Imageworks"
void RoughnessToAnisotropyRoughness(float anisotropy, float linearRoughness, out float roughnessT, out float roughnessB) {
    roughnessT = max(linearRoughness * (1.0f + anisotropy), MinLinearRoughness);
    roughnessB = max(linearRoughness * (1.0f - anisotropy), MinLinearRoughness);
}

/*
-------------------------------------------------------------------------------
 Diffuse BRDF functions
-------------------------------------------------------------------------------
*/

float Fd_Lambert() {
    return INV_PI;
}

// Energy conserving wrap diffuse term, does *not* include the divide by PI.
float Fd_Wrap(float NdotL, float w) {
    const float onePlusW = 1.0f + w;
    return saturate((NdotL + w) / (onePlusW * onePlusW));
}

// Disney diffuse
float Fd_Burley(float NdotL, float NdotV, float VdotH, float linearRoughness) {
#if 1
    // Renormalized version
    const float energyBias = lerp(0.0f, 0.5f, linearRoughness);
    const float energyFactor = lerp(1.0f, 1.0f / 1.51f, linearRoughness);
    const float F90 = energyBias + 2.0f * VdotH * VdotH * linearRoughness;
    const float FdL = 1.0f + (F90 - 1.0f) * Pow5(1.0f - NdotL);
    const float FdV = 1.0f + (F90 - 1.0f) * Pow5(1.0f - NdotV);
    return FdL * FdV * energyFactor * INV_PI;
#else
    const float F90 = 0.5f + 2.0f * VdotH * VdotH * linearRoughness;
    const float FdL = 1.0f + (F90 - 1.0f) * Pow5(1.0f - NdotL);
    const float FdV = 1.0f + (F90 - 1.0f) * Pow5(1.0f - NdotV);
    return FdL * FdV * INV_PI;
#endif
}

float Fd_OrenNayar(float NdotL, float NdotV, float LdotV, float linearRoughness) {
    const float sigma2 = linearRoughness * linearRoughness;
    const float A = 1.0f - sigma2 * (0.5f / (sigma2 + 0.33f) + 0.17f / (sigma2 + 0.13f));
    const float B = 0.45f * sigma2 / (sigma2 + 0.09f);

    // A tiny improvement of Oren-Nayar [Yasuhiro Fujii]
    // http://mimosa-pudica.net/improved-oren-nayar.html
    // const float A = 1.0f / (PI + 0.90412966f * roughness);
    // const float B = roughness / (PI + 0.90412966f * roughness);

    const float s = LdotV - NdotL * NdotV;
    const float t = lerp(1.0f, max(NdotL, NdotV), step(0.0f, s));

    return (A + B * s / t) * INV_PI;
}

/*
-------------------------------------------------------------------------------
 Normal distribution functions
-------------------------------------------------------------------------------
*/

float D_Blinn(float NdotH, float linearRoughness) {
    const float a2 = linearRoughness * linearRoughness;
    float n = 2.0f / a2 - 2.0f;
    n = max(n, 1e-4f); // Prevent possible zero.
    return pow(NdotH, n) * (n + 2.0f) * INV_TWO_PI;
}

float D_Beckmann(float NdotH, float linearRoughness) {
    const float a2 = linearRoughness * linearRoughness;
    const float NdotH2 = NdotH * NdotH;
    return exp((NdotH2 - 1.0f) / (a2 * NdotH2)) * INV_PI / (a2 * NdotH2 * NdotH2);
}

// Trowbridge-Reitz aka GGX
// NdotH 는 [0, 1], linearRoughness 는 MinLinearRoughness 이상이어야 한다.
float D_GGX(float NdotH, float linearRoughness) {
#if 0
    const float a2 = linearRoughness * linearRoughness;
    const float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 * INV_PI / (denom * denom + 1e-7f);
#else
    const float a = NdotH * linearRoughness;
    const float denom = (1.0f - NdotH * NdotH) + a * a;
    const float k = linearRoughness / denom;
    return k * k * INV_PI;
#endif
}

// Anisotropic GGX
// Burley 2012, "Physically-Based Shading at Disney"
float D_GGXAniso(float NdotH, float TdotH, float BdotH, float roughnessT, float roughnessB) {
    // The values roughnessT and roughnessB are roughness^2, a2 is therefore roughness^4.
    // The dot product below computes roughness^8. We cannot fit in fp16 without clamping
    // the roughness to too high values so we perform the dot product and the division in fp32.
#if 0
    // Scalar mul: 9
    // Scalar div: 3
    const float ax2 = roughnessT * roughnessT;
    const float ay2 = roughnessB * roughnessB;
    const float denom = TdotH * TdotH / ax2 + BdotH * BdotH / ay2 + NdotH * NdotH;
    return 1.0f / (3.14159265358979323846f * roughnessT * roughnessB * denom * denom);
#else
    // Scalar mul: 7
    // Scalar div: 1
    // float3 dot: 1
    const float a2 = roughnessT * roughnessB;
    const float3 v = float3(roughnessB * TdotH, roughnessT * BdotH, a2 * NdotH);
    const float v2 = dot(v, v);
    const float a2_v2 = a2 / v2;
    return INV_PI * a2 * a2_v2 * a2_v2;
#endif
}

/*
-------------------------------------------------------------------------------
 Geometric visibility functions - divided by (4 * NdotL * NdotV)
-------------------------------------------------------------------------------
*/

float G_Implicit() {
    return 0.25f;
}

float G_Neumann(float NdotV, float NdotL) {
    return 0.25f / max(NdotL, NdotV);
}

// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
float G_Kelemen(float VdotH) {
    return 0.25f / (VdotH * VdotH);
}

float G_CookTorrance(float NdotV, float NdotL, float NdotH, float VdotH) {
    const float k = 2.0f * NdotH / VdotH;
    const float G = min(1.0f, min(k * NdotV, k * NdotL));
    return G / (4.0f * NdotL * NdotV);
}

// k_direct = (roughness + 1)^2 / 8
// k_indirect = roughness^2 / 2
float G_SchlickGGX(float NdotV, float NdotL, float k) {
    const float oneMinusK = 1.0f - k;
    const float2 G = float2(NdotV, NdotL) * oneMinusK + k;
    return 0.25f / (G.x * G.y);
}

// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
float G_SmithGGXCorrelated(float NdotV, float NdotL, float linearRoughness) {
    const float a2 = linearRoughness * linearRoughness;
    // TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function.
    const float lambdaV = NdotL * sqrt((NdotV - a2 * NdotV) * NdotV + a2);
    const float lambdaL = NdotV * sqrt((NdotL - a2 * NdotL) * NdotL + a2);
    // a2 = 0 => v = 1 / (4 * NdotL * NdotV)   => min = 1/4, max = +inf
    // a2 = 1 => v = 1 / (2 * (NdotL + NdotV)) => min = 1/4, max = +inf
    return 0.5f / (lambdaV + lambdaL);
}

// Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
float G_SmithGGXCorrelatedFast(float NdotV, float NdotL, float linearRoughness) {
    return 0.5f / lerp(2.0f * NdotL * NdotV, NdotL + NdotV, linearRoughness);
}

// Smith Joint GGX Anisotropic Visibility
// Taken from https://cedec.cesa.or.jp/2015/session/ENG/14698.html
float G_SmithGGXCorrelatedAniso(float NdotV, float NdotL, float TdotV, float BdotV, float TdotL, float BdotL, float roughnessT, float roughnessB) {
    const float lambdaV = NdotL * length(float3(roughnessT * TdotV, roughnessB * BdotV, NdotV));
    const float lambdaL = NdotV * length(float3(roughnessT * TdotL, roughnessB * BdotL, NdotL));
    return 0.5f / (lambdaV + lambdaL);
}

/*
-------------------------------------------------------------------------------
 Fresnel functions
-------------------------------------------------------------------------------
*/

float F_SecondFactorSchlick(float cosTheta) {
    return Pow5(1.0f - cosTheta);
}

// Fresnel using Schlick's approximation
float3 F_Schlick(float3 F0, float cosTheta) {
    return F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * F_SecondFactorSchlick(cosTheta);
}

float F_SecondFactorSchlickSG(float cosTheta) {
    return exp2((-5.55473f * cosTheta - 6.98316f) * cosTheta);
}

// Fresnel using Schlick's approximation with spherical Gaussian approximation
float3 F_SchlickSG(float3 F0, float cosTheta) {
    return F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * F_SecondFactorSchlickSG(cosTheta);
}

// Fresnel injected roughness term
float3 F_SchlickRoughness(float3 F0, float roughness, float cosTheta) {
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * F_SecondFactorSchlick(cosTheta);
}

/*
-------------------------------------------------------------------------------
 IOR (Index of Refraction)
-------------------------------------------------------------------------------
*/

float IorToF0(float transmittedIor, float incidentIor) {
    const float r = (transmittedIor - incidentIor) / (transmittedIor + incidentIor);
    return r * r;
}

float IorToF0(float transmittedIor) {
    const float r = (transmittedIor - 1.0f) / (transmittedIor + 1.0f);
    return r * r;
}

float F0ToIor(float f0) {
    const float r = sqrt(f0);
    return (1.0f + r) / (1.0f - r);
}

float3 F0ForAirInterfaceToF0ForClearCoat15(float3 f0) {
    // Approximation of IorToF0(F0ToIor(f0), 1.5).
    // This assumes that the clear coat layer has an IOR of 1.5.
    return saturate(f0 * (f0 * (0.941892f - 0.263008f * f0) + 0.346479f) - 0.0285998f);
}

float2 GetPrefilteredDFG(float NdotV, float roughness) {
#if 0
    // Zioma's approximation based on Karis.
    return float2(1.0f, pow(1.0f - max(roughness, NdotV), 3.0f));
#else
    // Karis' approximation based on Lazarov's.
    // https://www.unrealengine.com/blog/physically-based-shading-on-mobile
    const float4 c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
    const float4 c1 = float4(1.0f, 0.0425f, 1.040f, -0.040f);
    const float4 r = roughness * c0 + c1;
    const float a004 = min(r.x * r.x, exp2(-9.28f * NdotV)) * r.x + r.y;
    return float2(-1.04f, 1.04f) * a004 + r.zw;
#endif
}

#endif // __BRDF_LIBRARY_HLSLI__
