#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

Texture2D<float4> apertureSourceTexture : register(t0);
RWStructuredBuffer<float2> apertureSpatialBuffer : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 outputCoord = dispatchThreadId.xy;
    if (outputCoord.x >= bloomParams.outputSize.x || outputCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    float2 uv = (float2(outputCoord) + 0.5) * bloomParams.outputTexelSize;
    float4 apertureSample = apertureSourceTexture.SampleLevel(linearClampSampler, uv, 0);

    // grayscale 로 바꿔서 complex 버퍼에 저장한다.
    float aperture = dot(max(apertureSample.rgb, float3(0.0, 0.0, 0.0)), float3(0.2126, 0.7152, 0.0722)) * saturate(apertureSample.a);

    uint outputIndex = outputCoord.y * bloomParams.outputSize.x + outputCoord.x;
    apertureSpatialBuffer[outputIndex] = float2(aperture, 0.0);
}
