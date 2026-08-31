#ifndef __SPECULAR_AA_HLSLI__
#define __SPECULAR_AA_HLSLI__

static const float SpecularAAVariance = 0.15f;
static const float SpecularAAThreshold = 0.2f;

float ComputeSpecularAAKernelRoughness(float3 normal) {
    const float3 normalDx = ddx(normal);
    const float3 normalDy = ddy(normal);
    const float normalVariance = SpecularAAVariance * (dot(normalDx, normalDx) + dot(normalDy, normalDy));
    return min(2.0f * normalVariance, SpecularAAThreshold);
}

float ApplySpecularAA(float roughness, float kernelRoughness) {
    return sqrt(saturate(roughness * roughness + kernelRoughness));
}

#endif // __SPECULAR_AA_HLSLI__
