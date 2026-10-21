#include "../ShaderInterop.h"
#include "SSRRadiance.hlsli"

#if SSR_CAPTURE_FULL_RESOLUTION_DEPTH
#define SSR_HISTORY_DESCRIPTOR_TABLE "DescriptorTable(SRV(t0), SRV(t1, flags = DATA_VOLATILE), UAV(u0), UAV(u1)), "
#else
#define SSR_HISTORY_DESCRIPTOR_TABLE "DescriptorTable(SRV(t0), UAV(u0)), "
#endif

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    SSR_HISTORY_DESCRIPTOR_TABLE \
    "RootConstants(num32BitConstants=9, b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

cbuffer SSRSceneColorHistoryMipCB : register(b0) {
    SSRSceneColorHistoryMipParams ssrSceneColorHistoryMipParams;
}

SamplerState linearClampSampler : register(s100);
Texture2D<float4> inputColorMipTexture : register(t0);
RWTexture2D<float4> outputColorMipTexture : register(u0);
#if SSR_CAPTURE_FULL_RESOLUTION_DEPTH
Texture2D<float> inputDepthTexture : register(t1);
RWTexture2D<float> outputDepthTexture : register(u1);
#endif

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSRSceneColorHistoryBlockSizeX, SSRSceneColorHistoryBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= ssrSceneColorHistoryMipParams.outputSize)) {
        return;
    }

#if SSR_CAPTURE_FULL_RESOLUTION_DEPTH
    // Capture opaque depth in the first color mip dispatch, before ocean/transparent depth writes.
    [unroll]
    for (uint y = 0; y < 2; ++y) {
        [unroll]
        for (uint x = 0; x < 2; ++x) {
            const uint2 coord = outputCoord * 2u + uint2(x, y);
            if (all(coord < ssrSceneColorHistoryMipParams.inputSize)) {
                outputDepthTexture[coord] = inputDepthTexture.Load(int3(coord, 0));
            }
        }
    }
#endif

    const float2 uv = (float2(outputCoord) + 0.5f) * ssrSceneColorHistoryMipParams.outputTexelSize;
    const float2 offset = ssrSceneColorHistoryMipParams.inputTexelSize * 0.5f;
    float3 color0 = inputColorMipTexture.SampleLevel(linearClampSampler, uv + float2(-offset.x, -offset.y), 0.0f).rgb;
    float3 color1 = inputColorMipTexture.SampleLevel(linearClampSampler, uv + float2( offset.x, -offset.y), 0.0f).rgb;
    float3 color2 = inputColorMipTexture.SampleLevel(linearClampSampler, uv + float2(-offset.x,  offset.y), 0.0f).rgb;
    float3 color3 = inputColorMipTexture.SampleLevel(linearClampSampler, uv + float2( offset.x,  offset.y), 0.0f).rgb;

    // 첫번째 패스에서만 raw HDR scene color 를 압축한다. 이후 mip 에서는 압축된 값을 사용한다.
    if (ssrSceneColorHistoryMipParams.compressInput != 0) {
        color0 = CompressSSRRadiance(color0);
        color1 = CompressSSRRadiance(color1);
        color2 = CompressSSRRadiance(color2);
        color3 = CompressSSRRadiance(color3);
    }

    outputColorMipTexture[outputCoord] = float4((color0 + color1 + color2 + color3) * 0.25f, 1.0f);
}
