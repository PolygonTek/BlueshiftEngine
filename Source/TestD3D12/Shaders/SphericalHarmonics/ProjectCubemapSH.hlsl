#include "../Common.hlsli"
#include "../Cubemap.hlsli"
#include "SphericalHarmonics.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_POINT, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

static const uint SHProjectionBlockSize = 128u;

TextureCube<float4> inputCubeMap : register(t0);
RWStructuredBuffer<float4> faceSHCoefficients : register(u0);
SamplerState pointClampSampler : register(s0);

groupshared float3 partialSHCoefficients[SHCoefficientCount][SHProjectionBlockSize];

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SHProjectionBlockSize, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupThreadIndex : SV_GroupIndex) {
    const uint faceIndex = groupId.x;

    uint inputWidth;
    uint inputHeight;
    uint inputMipLevelCount;
    inputCubeMap.GetDimensions(0u, inputWidth, inputHeight, inputMipLevelCount);

    float3 projectedCoefficients[SHCoefficientCount];
    [unroll]
    for (uint coefficientIndex = 0u; coefficientIndex < SHCoefficientCount; ++coefficientIndex) {
        projectedCoefficients[coefficientIndex] = 0.0f;
    }

    const uint texelCount = inputWidth * inputHeight;
    const float inputSizeRcp = rcp((float)inputWidth);
    for (uint texelIndex = groupThreadIndex; texelIndex < texelCount; texelIndex += SHProjectionBlockSize) {
        const uint2 texelCoord = uint2(texelIndex % inputWidth, texelIndex / inputWidth);
        const float2 uv = (float2(texelCoord) + 0.5f) * inputSizeRcp;
        const float3 direction = normalize(CubeMapFaceToDirection(faceIndex, uv));
        const float3 inputValue = inputCubeMap.SampleLevel(pointClampSampler, direction, 0.0f).rgb;
        const float solidAngle = CubeMapTexelSolidAngle(uv, inputSizeRcp);

        float basis[SHCoefficientCount];
        EvaluateSHBasis(direction, basis);

        [unroll]
        for (uint coefficientIndex = 0u; coefficientIndex < SHCoefficientCount; ++coefficientIndex) {
            projectedCoefficients[coefficientIndex] += inputValue * (basis[coefficientIndex] * solidAngle);
        }
    }

    [unroll]
    for (uint coefficientIndex = 0u; coefficientIndex < SHCoefficientCount; ++coefficientIndex) {
        partialSHCoefficients[coefficientIndex][groupThreadIndex] = projectedCoefficients[coefficientIndex];
    }
    GroupMemoryBarrierWithGroupSync();

    // 각 thread group 이 cubemap face 하나의 계수를 합산한다.
    for (uint stride = SHProjectionBlockSize >> 1u; stride > 0u; stride >>= 1u) {
        if (groupThreadIndex < stride) {
            [unroll]
            for (uint coefficientIndex = 0u; coefficientIndex < SHCoefficientCount; ++coefficientIndex) {
                partialSHCoefficients[coefficientIndex][groupThreadIndex] += partialSHCoefficients[coefficientIndex][groupThreadIndex + stride];
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (groupThreadIndex == 0u) {
        [unroll]
        for (uint coefficientIndex = 0u; coefficientIndex < SHCoefficientCount; ++coefficientIndex) {
            faceSHCoefficients[faceIndex * SHCoefficientCount + coefficientIndex] = float4(partialSHCoefficients[coefficientIndex][0], 0.0f);
        }
    }
}
