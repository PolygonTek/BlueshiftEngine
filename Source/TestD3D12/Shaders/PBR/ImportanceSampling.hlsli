#ifndef __IMPORTANCE_SAMPLING_HLSLI__
#define __IMPORTANCE_SAMPLING_HLSLI__

#include "../Common.hlsli"

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// Efficient Van der Corput calculation.
float RadicalInverseVdC(uint bits) {
    return (float)reversebits(bits) * 2.3283064365386963e-10f; // 2^-32
}

// Requires sampleCount > 0 and sampleIndex < sampleCount.
float2 Hammersley(uint sampleIndex, uint sampleCount) {
    return float2((float)sampleIndex / (float)sampleCount, RadicalInverseVdC(sampleIndex));
}

// Builds a local frame whose basis vectors are stored in rows.
// Transform a tangent-space direction with mul(tangentDirection, localFrame).
// tangentZ must be normalized.
float3x3 GetLocalFrame(float3 tangentZ) {
    float3 tangentY = abs(tangentZ.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    const float3 tangentX = normalize(cross(tangentY, tangentZ));
    tangentY = cross(tangentZ, tangentX);
    return float3x3(tangentX, tangentY, tangentZ);
}

// Returns an importance-sampled incident light direction for Lambert diffuse reflectance with respect to N.
// xi must be in the [0, 1) range.
float3 ImportanceSampleLambert(float2 xi) {
    const float cosTheta = sqrt(1.0f - xi.x);
    const float sinTheta = sqrt(xi.x);
    const float phi = TWO_PI * xi.y;

    float sinPhi;
    float cosPhi;
    sincos(phi, sinPhi, cosPhi);
    return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

// Returns an importance-sampled incident light direction for Phong specular reflectance with respect to S.
// xi must be in the [0, 1) range and power must be greater than or equal to 0.
float3 ImportanceSamplePhongSpecular(float2 xi, float power) {
    const float oneMinusXiX = 1.0f - xi.x;
    const float cosTheta = pow(oneMinusXiX, 1.0f / (power + 1.0f));
    const float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    const float phi = TWO_PI * xi.y;

    float sinPhi;
    float cosPhi;
    sincos(phi, sinPhi, cosPhi);
    return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

// Returns an importance-sampled halfway direction for Beckmann specular NDF with respect to N.
// xi must be in the [0, 1) range and linearRoughness must be greater than or equal to MinLinearRoughness.
float3 ImportanceSampleBeckmann(float2 xi, float linearRoughness) {
    const float oneMinusXiX = 1.0f - xi.x;
    const float cosThetaSquared = 1.0f / (1.0f - linearRoughness * linearRoughness * log(oneMinusXiX));
    const float cosTheta = sqrt(cosThetaSquared);
    const float sinTheta = sqrt(1.0f - cosThetaSquared);
    const float phi = TWO_PI * xi.y;

    float sinPhi;
    float cosPhi;
    sincos(phi, sinPhi, cosPhi);
    return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

// Returns an importance-sampled halfway direction for GGX specular NDF with respect to N.
// xi must be in the [0, 1) range and linearRoughness must be greater than or equal to MinLinearRoughness.
float3 ImportanceSampleGGX(float2 xi, float linearRoughness) {
    const float linearRoughnessSquared = linearRoughness * linearRoughness;
    const float oneMinusXiX = 1.0f - xi.x;
    const float cosThetaSquared = oneMinusXiX / (linearRoughnessSquared * xi.x + oneMinusXiX);
    const float cosTheta = sqrt(cosThetaSquared);
    const float sinTheta = sqrt(1.0f - cosThetaSquared);
    const float phi = TWO_PI * xi.y;

    float sinPhi;
    float cosPhi;
    sincos(phi, sinPhi, cosPhi);
    return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

#endif // __IMPORTANCE_SAMPLING_HLSLI__
