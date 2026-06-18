#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);
#define SAMPLING_LINEAR_CLAMP_SAMPLER linearClampSampler
#include "Sampling.hlsli"

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= bloomParams.outputSize.x || pixelCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    float2 uv = (float2(pixelCoord) + 0.5) * bloomParams.outputTexelSize;
    float2 offset = bloomParams.inputTexelSize * max(BP_BLOOM_RADIUS, 0.0);
    half3 color = SampleKawase4Tap(inputTexture, uv, offset);

    outputTexture[pixelCoord] = float4(float3(max(color, half3(0.0, 0.0, 0.0))), 1.0);
}
