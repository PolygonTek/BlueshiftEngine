#ifndef __SPECULAR_AA_HLSLI__
#define __SPECULAR_AA_HLSLI__

float ComputeSpecularAAKernelRoughness(float3 normal, float screenSpaceVariance, float limit) {
    const float3 normalDx = ddx(normal);
    const float3 normalDy = ddy(normal);
    const float normalVariance = screenSpaceVariance * (dot(normalDx, normalDx) + dot(normalDy, normalDy));
    return min(2.0f * normalVariance, limit);
}

float ApplySpecularAA(float roughness, float kernelRoughness) {
    return sqrt(saturate(roughness * roughness + kernelRoughness));
}

#endif // __SPECULAR_AA_HLSLI__
