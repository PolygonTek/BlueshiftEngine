#include "../Common.hlsli"
#include "SphericalHarmonics.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

StructuredBuffer<float4> faceSHCoefficients : register(t0);
RWStructuredBuffer<float4> outputSHCoefficients : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SHCoefficientCount, 1, 1)]
void CSMain(uint groupThreadIndex : SV_GroupIndex) {
    float3 coefficient = 0.0f;

    [unroll]
    for (uint faceIndex = 0u; faceIndex < 6u; ++faceIndex) {
        coefficient += faceSHCoefficients[faceIndex * SHCoefficientCount + groupThreadIndex].rgb;
    }
    outputSHCoefficients[groupThreadIndex] = float4(coefficient, 0.0f);
}
