#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

half3 ApplyBloomThreshold(half3 color) {
    half threshold = half(BP_BLOOM_THRESHOLD);
    if (threshold <= half(0.0)) {
        return color;
    }

    half luminance = dot(color, half3(0.2127, 0.7152, 0.0722));
    half contribution = max(luminance - threshold, half(0.0));
    half softThreshold = half(BP_BLOOM_SOFT_THRESHOLD);
    if (softThreshold > half(0.0)) {
        half soft = clamp(luminance - threshold + softThreshold, half(0.0), half(2.0) * softThreshold);
        contribution = max(contribution, soft * soft / max(half(4.0) * softThreshold, half(0.0001)));
    }

    return color * (contribution / max(luminance, half(0.0001)));
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= bloomParams.outputSize.x || pixelCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    float2 uv = (float2(pixelCoord) + 0.5) * bloomParams.outputTexelSize;
    half3 color = half3(inputTexture.SampleLevel(linearClampSampler, uv, 0).rgb);
    color = ApplyBloomThreshold(max(color, half3(0.0, 0.0, 0.0)));

    outputTexture[pixelCoord] = float4(float3(color), 1.0);
}
