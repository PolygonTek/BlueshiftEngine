#include "../Common.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=6, b1), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer LuminancePass1CB : register(b1) {
    LuminancePass1Params luminancePass1Params;
}

Texture2D<float4> inputTexture : register(t0);
RWByteAddressBuffer outputHistogram : register(u0);

// local histogram bin table
groupshared uint shared_bins[LuminanceHistogramBinCount];

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(LuminanceBlockSize, LuminanceBlockSize, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex) {
    shared_bins[groupIndex] = 0;
    GroupMemoryBarrierWithGroupSync();

    if (dispatchThreadId.x < luminancePass1Params.sampleSize.x && dispatchThreadId.y < luminancePass1Params.sampleSize.y) {
        // 절반 크기로 중앙을 샘플링하여 downsample 된 inputTexture 를 샘플링하는 효과
        const float2 sampleUV = (dispatchThreadId.xy + 0.5) * luminancePass1Params.sampleSizeRcp;
        half3 color = half3(inputTexture.SampleLevel(linearClampSampler, sampleUV, 0).rgb);
        half luminance = dot(color, half3(0.2127h, 0.7152h, 0.0722h));

        // luminance 값에 log 를 취하고 적절한 범위로 clamp 한 후 양자화하여 binIndex 를 구한다.
        uint binIndex = 0;
        if (luminance > half(0.001)) {
            float logLuminance = saturate((log2(float(luminance)) - luminancePass1Params.luminanceLogMin) * luminancePass1Params.luminanceLogRangeRcp);
            binIndex = (uint)(logLuminance * (LuminanceHistogramBinCount - 2) + 1);
        }
        
        // local histogram bin table 에 누적한다.
        InterlockedAdd(shared_bins[binIndex], 1);
    }

    // global histogram bin table 에 누적한다.
    GroupMemoryBarrierWithGroupSync();
    outputHistogram.InterlockedAdd(LuminanceBufferOffsetHistogram + groupIndex * 4, shared_bins[groupIndex]);
}
