#include "../Common.hlsli"
#include "../Cubemap.hlsli"
#include "../SphericalHarmonics.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

StructuredBuffer<float4> faceSHCoefficients : register(t0);
RWTexture2DArray<float4> outputIrradiance : register(u0);

groupshared float3 incidentSHCoefficients[SH3CoefficientCount];

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(IBLIntegrationBlockSizeX, IBLIntegrationBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID, uint groupThreadIndex : SV_GroupIndex) {
    // Load and sum the six per-face projections once per output thread group.
    if (groupThreadIndex < SH3CoefficientCount) {
        float3 coefficient = 0.0f;
        [unroll]
        for (uint faceIndex = 0u; faceIndex < 6u; ++faceIndex) {
            coefficient += faceSHCoefficients[faceIndex * SH3CoefficientCount + groupThreadIndex].rgb;
        }
        incidentSHCoefficients[groupThreadIndex] = coefficient;
    }
    GroupMemoryBarrierWithGroupSync();

    uint outputWidth;
    uint outputHeight;
    uint outputFaceCount;
    outputIrradiance.GetDimensions(outputWidth, outputHeight, outputFaceCount);

    const uint3 outputCoord = dispatchThreadId;
    if (outputCoord.x >= outputWidth || outputCoord.y >= outputHeight || outputCoord.z >= min(outputFaceCount, 6u)) {
        return;
    }

    const float2 uv = (float2(outputCoord.xy) + 0.5f) / float2(outputWidth, outputHeight);
    const float3 direction = normalize(CubeMapFaceToDirection(outputCoord.z, uv));

    float basis[SH3CoefficientCount];
    EvaluateSH3Basis(direction, basis);

    float3 irradiance = 0.0f;
    [unroll]
    for (uint coefficientIndex = 0u; coefficientIndex < SH3CoefficientCount; ++coefficientIndex) {
        irradiance += incidentSHCoefficients[coefficientIndex] * basis[coefficientIndex] * LambertSHConvolutionFactor(coefficientIndex);
    }

    outputIrradiance[outputCoord] = float4(irradiance, 1.0f);
}
