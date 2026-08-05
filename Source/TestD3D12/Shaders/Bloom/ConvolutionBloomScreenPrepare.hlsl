#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

Texture2D<float4> screenTexture : register(t0);
RWStructuredBuffer<float2> fftScreenBuffer : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= bloomParams.outputSize.x || pixelCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    uint outputIndex = pixelCoord.y * bloomParams.outputSize.x + pixelCoord.x;
    uint outputPixelCount = bloomParams.outputSize.x * bloomParams.outputSize.y;

    // content 영역 밖은 검은색으로 칠한다.
    float2 contentSampleCoord = float2(pixelCoord) + 0.5 - BP_BLOOM_CONTENT_OFFSET;
    if (contentSampleCoord.x < 0.0 || contentSampleCoord.y < 0.0 || contentSampleCoord.x >= BP_BLOOM_CONTENT_SIZE.x || contentSampleCoord.y >= BP_BLOOM_CONTENT_SIZE.y) {
        fftScreenBuffer[outputIndex] = float2(0.0, 0.0);
        fftScreenBuffer[outputPixelCount + outputIndex] = float2(0.0, 0.0);
        return;
    }

    float2 uv = contentSampleCoord / BP_BLOOM_CONTENT_SIZE;
    half3 color = max(half3(screenTexture.SampleLevel(linearClampSampler, uv, BP_BLOOM_SCREEN_MIP_LEVEL).rgb), half3(0.0, 0.0, 0.0));

    // FFT screen buffer 의 앞쪽에는 rg 배열만 담고, 이후에 b0 배열을 담는다. (패킹된 FFTDualReal 셰이더를 사용하기에 유리)
    float3 outputColor = float3(color);
    fftScreenBuffer[outputIndex] = outputColor.rg;
    fftScreenBuffer[outputPixelCount + outputIndex] = float2(outputColor.b, 0.0);
}
