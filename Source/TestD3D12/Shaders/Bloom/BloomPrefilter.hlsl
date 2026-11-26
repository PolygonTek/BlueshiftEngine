#include "../ShaderInterop.h"
#include "../Colors.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=10, b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer BloomCB : register(b0) {
    BloomPrefilterParams bloomParams;
}

Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

// Keep HDR thresholding and Karis weights in full precision.
float3 ApplyBloomThreshold(float3 color) {
    float threshold = float(bloomParams.threshold);
    if (threshold <= float(0.0)) {
        return color;
    }

    float luminance = GetLuma(color);
    float contribution = max(luminance - threshold, float(0.0));
    float softThreshold = float(bloomParams.softThreshold);
    if (softThreshold > float(0.0)) {
        float soft = clamp(luminance - threshold + softThreshold, float(0.0), float(2.0) * softThreshold);
        contribution = max(contribution, soft * soft / max(float(4.0) * softThreshold, float(0.0001)));
    }

    return color * (contribution / max(luminance, float(0.0001)));
}

// 컬러 샘플링 후 threshold 를 적용하여 리턴한다, Karis weight 를 함께 얻는다.
float3 SampleBloomPrefilterTap(float2 uv, out float w) {
    float3 color = float3(inputTexture.SampleLevel(linearClampSampler, uv, 0).rgb);
    color = ApplyBloomThreshold(max(color, float3(0.0, 0.0, 0.0)));

    // Karis average weight reduces firefly-like highlights before the bloom pyramid amplifies them.
    float luminance = GetLuma(color);
    w = rcp(luminance + float(1.0));
    return color;
}

// 4-samples 가중 평균
float3 WeightedAverage4(float3 sample0, float3 sample1, float3 sample2, float3 sample3, float w0, float w1, float w2, float w3) {
    float weightSum = w0 + w1 + w2 + w3;
    return (sample0 * w0 + sample1 * w1 + sample2 * w2 + sample3 * w3) / max(weightSum, float(0.0001));
}

// 5-samples 가중 평균
float3 WeightedAverage5(float3 sample0, float3 sample1, float3 sample2, float3 sample3, float3 sample4, float w0, float w1, float w2, float w3, float w4) {
    float weightSum = w0 + w1 + w2 + w3 + w4;
    return (sample0 * w0 + sample1 * w1 + sample2 * w2 + sample3 * w3 + sample4 * w4) / max(weightSum, float(0.0001));
}

// Use a rotated grid to minimize artifacts from horizontal and vertical boundaries.
// "High Quality Antialiasing" [Lorach07]
float3 SampleBloomPrefilterRotatedGrid5Tap(float2 uv, float2 offset) {
    static const float2 taps[5] = {
        float2( 0.0,  0.0),
        float2( 0.9, -0.4),
        float2(-0.9,  0.4),
        float2( 0.4,  0.9),
        float2(-0.4, -0.9)
    };

    float w0, w1, w2, w3, w4;
    float3 sample0 = SampleBloomPrefilterTap(uv + taps[0] * offset, w0);
    float3 sample1 = SampleBloomPrefilterTap(uv + taps[1] * offset, w1);
    float3 sample2 = SampleBloomPrefilterTap(uv + taps[2] * offset, w2);
    float3 sample3 = SampleBloomPrefilterTap(uv + taps[3] * offset, w3);
    float3 sample4 = SampleBloomPrefilterTap(uv + taps[4] * offset, w4);

    return WeightedAverage5(sample0, sample1, sample2, sample3, sample4, w0, w1, w2, w3, w4);
}

// "Post-Processing in CoD Advanced Warfare" [Jimenez14]
float3 SampleBloomPrefilterJimenez13Tap(float2 uv, float2 offset) {
    float wa, wb, wc, wd, we, wf, wg, wh, wi, wj, wk, wl, wm;
    float3 a = SampleBloomPrefilterTap(uv + offset * float2(-1.0, -1.0), wa);
    float3 b = SampleBloomPrefilterTap(uv + offset * float2( 0.0, -1.0), wb);
    float3 c = SampleBloomPrefilterTap(uv + offset * float2( 1.0, -1.0), wc);
    float3 d = SampleBloomPrefilterTap(uv + offset * float2(-0.5, -0.5), wd);
    float3 e = SampleBloomPrefilterTap(uv + offset * float2( 0.5, -0.5), we);
    float3 f = SampleBloomPrefilterTap(uv + offset * float2(-1.0,  0.0), wf);
    float3 g = SampleBloomPrefilterTap(uv, wg);
    float3 h = SampleBloomPrefilterTap(uv + offset * float2( 1.0,  0.0), wh);
    float3 i = SampleBloomPrefilterTap(uv + offset * float2(-0.5,  0.5), wi);
    float3 j = SampleBloomPrefilterTap(uv + offset * float2( 0.5,  0.5), wj);
    float3 k = SampleBloomPrefilterTap(uv + offset * float2(-1.0,  1.0), wk);
    float3 l = SampleBloomPrefilterTap(uv + offset * float2( 0.0,  1.0), wl);
    float3 m = SampleBloomPrefilterTap(uv + offset * float2( 1.0,  1.0), wm);

    float3 color = WeightedAverage4(d, e, i, j, wd, we, wi, wj) * float(0.5);
    color += WeightedAverage4(a, b, f, g, wa, wb, wf, wg) * float(0.125);
    color += WeightedAverage4(b, c, g, h, wb, wc, wg, wh) * float(0.125);
    color += WeightedAverage4(f, g, k, l, wf, wg, wk, wl) * float(0.125);
    color += WeightedAverage4(g, h, l, m, wg, wh, wl, wm) * float(0.125);

    return color;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= bloomParams.outputSize.x || pixelCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    float2 uv = (float2(pixelCoord) + 0.5) * bloomParams.outputTexelSize;
    float2 offset = bloomParams.inputTexelSize * max(bloomParams.radius, 0.0);
    float3 color;
    if (bloomParams.prefilterQuality > 0.5) {
        color = SampleBloomPrefilterJimenez13Tap(uv, offset);
    } else {
        color = SampleBloomPrefilterRotatedGrid5Tap(uv, offset);
    }

    outputTexture[pixelCoord] = float4(float3(color), 1.0);
}
