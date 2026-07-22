#include "../Common.hlsli"
#include "../Cubemap.hlsli"
#include "../SphericalHarmonics.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_POINT, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

static const uint SHProjectionBlockSize = 128u;

TextureCube<float4> radianceCubeMap : register(t0);
RWStructuredBuffer<float4> faceSHCoefficients : register(u0);
SamplerState pointClampSampler : register(s0);

groupshared float3 partialSHCoefficients[SH3CoefficientCount][SHProjectionBlockSize];

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SHProjectionBlockSize, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupThreadIndex : SV_GroupIndex) {
    const uint faceIndex = groupId.x;

    uint radianceWidth;
    uint radianceHeight;
    uint radianceMipLevelCount;
    radianceCubeMap.GetDimensions(0u, radianceWidth, radianceHeight, radianceMipLevelCount);

    float3 projectedCoefficients[SH3CoefficientCount];
    [unroll]
    for (uint coefficientIndex = 0u; coefficientIndex < SH3CoefficientCount; ++coefficientIndex) {
        projectedCoefficients[coefficientIndex] = 0.0f;
    }

    const uint texelCount = radianceWidth * radianceHeight;
    const float radianceSizeRcp = rcp((float)radianceWidth);
    for (uint texelIndex = groupThreadIndex; texelIndex < texelCount; texelIndex += SHProjectionBlockSize) {
        const uint2 texelCoord = uint2(texelIndex % radianceWidth, texelIndex / radianceWidth);
        const float2 uv = (float2(texelCoord) + 0.5f) * radianceSizeRcp;
        const float3 direction = normalize(CubeMapFaceToDirection(faceIndex, uv));
        const float3 radiance = radianceCubeMap.SampleLevel(pointClampSampler, direction, 0.0f).rgb;
        const float solidAngle = CubeMapTexelSolidAngle(uv, radianceSizeRcp);

        float basis[SH3CoefficientCount];
        EvaluateSH3Basis(direction, basis);

        [unroll]
        for (uint coefficientIndex = 0u; coefficientIndex < SH3CoefficientCount; ++coefficientIndex) {
            projectedCoefficients[coefficientIndex] += radiance * (basis[coefficientIndex] * solidAngle);
        }
    }

    [unroll]
    for (uint coefficientIndex = 0u; coefficientIndex < SH3CoefficientCount; ++coefficientIndex) {
        partialSHCoefficients[coefficientIndex][groupThreadIndex] = projectedCoefficients[coefficientIndex];
    }
    GroupMemoryBarrierWithGroupSync();

    // Each group reduces one cubemap face. The second pass sums the six face results.
    for (uint stride = SHProjectionBlockSize >> 1u; stride > 0u; stride >>= 1u) {
        if (groupThreadIndex < stride) {
            [unroll]
            for (uint coefficientIndex = 0u; coefficientIndex < SH3CoefficientCount; ++coefficientIndex) {
                partialSHCoefficients[coefficientIndex][groupThreadIndex] += partialSHCoefficients[coefficientIndex][groupThreadIndex + stride];
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (groupThreadIndex == 0u) {
        [unroll]
        for (uint coefficientIndex = 0u; coefficientIndex < SH3CoefficientCount; ++coefficientIndex) {
            faceSHCoefficients[faceIndex * SH3CoefficientCount + coefficientIndex] = float4(partialSHCoefficients[coefficientIndex][0], 0.0f);
        }
    }
}
