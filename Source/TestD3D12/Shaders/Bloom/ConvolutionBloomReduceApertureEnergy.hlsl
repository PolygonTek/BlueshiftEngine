#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

StructuredBuffer<float2> inputBuffer : register(t0);
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float sharedEnergy[ConvolutionBloomApertureEnergyReduceBlockSize];

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(ConvolutionBloomApertureEnergyReduceBlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    const uint inputElementCount = bloomParams.inputSize.x;
    const bool inputIsPartialEnergy = bloomParams.inputSize.y != 0;
    const uint inputIndex = groupId.x * ConvolutionBloomApertureEnergyReduceBlockSize + groupThreadId.x;

    float energy = 0.0;
    if (inputIndex < inputElementCount) {
        float2 value = inputBuffer[inputIndex];
        energy = inputIsPartialEnergy ? value.x : dot(value, value);
    }

    sharedEnergy[groupThreadId.x] = energy;
    GroupMemoryBarrierWithGroupSync();

    [unroll]
    for (uint stride = ConvolutionBloomApertureEnergyReduceBlockSize >> 1; stride > 0; stride >>= 1) {
        if (groupThreadId.x < stride) {
            sharedEnergy[groupThreadId.x] += sharedEnergy[groupThreadId.x + stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (groupThreadId.x == 0) {
        outputBuffer[groupId.x] = float2(sharedEnergy[0], 0.0);
    }
}
