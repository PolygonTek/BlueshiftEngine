#include "../ShaderInterop.h"
#include "../Colors.hlsli"

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

    float2 outputUv = (float2(outputCoord) + 0.5) * bloomParams.outputTexelSize;
    float apertureScale = max(BP_BLOOM_APERTURE_SCALE, 0.0001);
    float2 apertureUv = (outputUv - 0.5) / apertureScale + 0.5;

    // FFT 영역 중앙에 축소된 aperture 를 배치하고 그 바깥은 0 으로 채운다.
    float aperture = 0.0;
    if (all(apertureUv >= 0.0) && all(apertureUv <= 1.0)) {
        float4 apertureSample = apertureSourceTexture.SampleLevel(linearClampSampler, apertureUv, 0);

        // grayscale 로 바꿔서 complex 버퍼에 저장한다.
        aperture = GetLuma(max(apertureSample.rgb, float3(0.0, 0.0, 0.0))) * saturate(apertureSample.a);
    }

    uint outputIndex = outputCoord.y * bloomParams.outputSize.x + outputCoord.x;
    apertureSpatialBuffer[outputIndex] = float2(aperture, 0.0);
}
