#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../Cubemap.hlsli"
#include "SphericalHarmonics.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=3, b0)"

cbuffer ReconstructCubemapSHCB : register(b0) {
    ReconstructCubemapSHParams reconstructCubemapSHParams;
}

StructuredBuffer<float4> inputSHCoefficients : register(t0);
RWTexture2DArray<float4> outputCubeMap : register(u0);

groupshared float3 bandWeightedSHCoefficients[SHCoefficientCount];

float SHBandFactor(uint coefficientIndex) {
    if (coefficientIndex == 0u) {
        return reconstructCubemapSHParams.bandFactors.x;
    }
    if (coefficientIndex < 4u) {
        return reconstructCubemapSHParams.bandFactors.y;
    }
    return reconstructCubemapSHParams.bandFactors.z;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(IBLIntegrationBlockSizeX, IBLIntegrationBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID, uint groupThreadIndex : SV_GroupIndex) {
    if (groupThreadIndex < SHCoefficientCount) {
        bandWeightedSHCoefficients[groupThreadIndex] = inputSHCoefficients[groupThreadIndex].rgb * SHBandFactor(groupThreadIndex);
    }
    GroupMemoryBarrierWithGroupSync();

    uint outputWidth;
    uint outputHeight;
    uint outputFaceCount;
    outputCubeMap.GetDimensions(outputWidth, outputHeight, outputFaceCount);

    const uint3 outputCoord = dispatchThreadId;
    if (outputCoord.x >= outputWidth || outputCoord.y >= outputHeight || outputCoord.z >= min(outputFaceCount, 6u)) {
        return;
    }

    const float2 uv = (float2(outputCoord.xy) + 0.5f) / float2(outputWidth, outputHeight);
    const float3 direction = normalize(CubeMapFaceToDirection(outputCoord.z, uv));

    float basis[SHCoefficientCount];
    EvaluateSHBasis(direction, basis);

    float3 outputValue = 0.0f;
    [unroll]
    for (uint coefficientIndex = 0u; coefficientIndex < SHCoefficientCount; ++coefficientIndex) {
        outputValue += bandWeightedSHCoefficients[coefficientIndex] * basis[coefficientIndex];
    }

    outputCubeMap[outputCoord] = float4(outputValue, 1.0f);
}
