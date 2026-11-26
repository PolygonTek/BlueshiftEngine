#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);
#define SAMPLING_LINEAR_CLAMP_SAMPLER linearClampSampler
#include "../Sampling.hlsli"

cbuffer BloomCB : register(b0) {
    BloomUpsampleParams bloomParams;
}

Texture2D<float4> currentMipTexture : register(t0);
Texture2D<float4> lowerMipTexture : register(t1);
RWTexture2D<float4> outputTexture : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= bloomParams.outputSize.x || pixelCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    float2 uv = (float2(pixelCoord) + 0.5) * bloomParams.outputTexelSize;
    float2 offset = bloomParams.inputTexelSize * max(bloomParams.radius, 0.0);

    float3 upsampled = SampleTent9Tap(lowerMipTexture, uv, offset);
    float3 current = float3(currentMipTexture.SampleLevel(linearClampSampler, uv, 0).rgb);
    float3 color = max(current + upsampled * float(bloomParams.weight), float3(0.0, 0.0, 0.0));
    outputTexture[pixelCoord] = float4(float3(color), 1.0);
}
