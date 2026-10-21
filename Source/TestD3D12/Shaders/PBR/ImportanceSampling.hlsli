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

// Returns a cosine-weighted incident light direction about local +Z for Lambert diffuse reflectance.
// xi must be in the [0, 1) range.
float3 ImportanceSampleLambert(float2 xi) {
    return SampleHemisphereCosine(xi);
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

// Returns an importance-sampled halfway direction for isotropic GGX specular VNDF with respect to N (local +Z).
// xi must be in the [0, 1) range. linearRoughness is clamped to at least 1.0e-4.
// tangentView points from the surface toward the viewer and must lie in the local +Z hemisphere.
float3 ImportanceSampleGGXVNDF(float2 xi, float3 tangentView, float linearRoughness) {
    const float alpha = max(linearRoughness, 1.0e-4f);
    const float3 stretchedView = normalize(float3(alpha * tangentView.xy, tangentView.z));

    const float projectedLengthSquared = dot(stretchedView.xy, stretchedView.xy);
    const float3 tangentX = projectedLengthSquared > 0.0f ? float3(-stretchedView.y, stretchedView.x, 0.0f) * rsqrt(projectedLengthSquared) : float3(1.0f, 0.0f, 0.0f);
    const float3 tangentY = cross(stretchedView, tangentX);

    const float radius = sqrt(xi.x);
    const float phi = TWO_PI * xi.y;
    float sinPhi;
    float cosPhi;
    sincos(phi, sinPhi, cosPhi);

    const float diskX = radius * cosPhi;
    float diskY = radius * sinPhi;
    const float viewBlend = 0.5f * (1.0f + stretchedView.z);
    diskY = lerp(sqrt(saturate(1.0f - diskX * diskX)), diskY, viewBlend);

    const float normalZ = sqrt(saturate(1.0f - diskX * diskX - diskY * diskY));
    const float3 stretchedNormal = diskX * tangentX + diskY * tangentY + normalZ * stretchedView;
    return normalize(float3(alpha * stretchedNormal.xy, max(stretchedNormal.z, 0.0f)));
}

#endif // __IMPORTANCE_SAMPLING_HLSLI__
