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

Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

half3 ApplyBloomThreshold(half3 color) {
    half threshold = half(BP_BLOOM_THRESHOLD);
    if (threshold <= half(0.0)) {
        return color;
    }

    half luminance = GetLuma(color);
    half contribution = max(luminance - threshold, half(0.0));
    half softThreshold = half(BP_BLOOM_SOFT_THRESHOLD);
    if (softThreshold > half(0.0)) {
        half soft = clamp(luminance - threshold + softThreshold, half(0.0), half(2.0) * softThreshold);
        contribution = max(contribution, soft * soft / max(half(4.0) * softThreshold, half(0.0001)));
    }

    return color * (contribution / max(luminance, half(0.0001)));
}

// 컬러 샘플링 후 threshold 를 적용하여 리턴한다, Karis weight 를 함께 얻는다.
half3 SampleBloomPrefilterTap(float2 uv, out half w) {
    half3 color = half3(inputTexture.SampleLevel(linearClampSampler, uv, 0).rgb);
    color = ApplyBloomThreshold(max(color, half3(0.0, 0.0, 0.0)));

    // Karis average weight reduces firefly-like highlights before the bloom pyramid amplifies them.
    half luminance = GetLuma(color);
    w = rcp(luminance + half(1.0));
    return color;
}

// 4-samples 가중 평균
half3 WeightedAverage4(half3 sample0, half3 sample1, half3 sample2, half3 sample3, half w0, half w1, half w2, half w3) {
    half weightSum = w0 + w1 + w2 + w3;
    return (sample0 * w0 + sample1 * w1 + sample2 * w2 + sample3 * w3) / max(weightSum, half(0.0001));
}

// 5-samples 가중 평균
half3 WeightedAverage5(half3 sample0, half3 sample1, half3 sample2, half3 sample3, half3 sample4, half w0, half w1, half w2, half w3, half w4) {
    half weightSum = w0 + w1 + w2 + w3 + w4;
    return (sample0 * w0 + sample1 * w1 + sample2 * w2 + sample3 * w3 + sample4 * w4) / max(weightSum, half(0.0001));
}

// Use a rotated grid to minimize artifacts from horizontal and vertical boundaries.
// "High Quality Antialiasing" [Lorach07]
half3 SampleBloomPrefilterRotatedGrid5Tap(float2 uv, float2 offset) {
    static const float2 taps[5] = {
        float2( 0.0,  0.0),
        float2( 0.9, -0.4),
        float2(-0.9,  0.4),
        float2( 0.4,  0.9),
        float2(-0.4, -0.9)
    };

    half w0, w1, w2, w3, w4;
    half3 sample0 = SampleBloomPrefilterTap(uv + taps[0] * offset, w0);
    half3 sample1 = SampleBloomPrefilterTap(uv + taps[1] * offset, w1);
    half3 sample2 = SampleBloomPrefilterTap(uv + taps[2] * offset, w2);
    half3 sample3 = SampleBloomPrefilterTap(uv + taps[3] * offset, w3);
    half3 sample4 = SampleBloomPrefilterTap(uv + taps[4] * offset, w4);

    return WeightedAverage5(sample0, sample1, sample2, sample3, sample4, w0, w1, w2, w3, w4);
}

// "Post-Processing in CoD Advanced Warfare" [Jimenez14]
half3 SampleBloomPrefilterJimenez13Tap(float2 uv, float2 offset) {
    half wa, wb, wc, wd, we, wf, wg, wh, wi, wj, wk, wl, wm;
    half3 a = SampleBloomPrefilterTap(uv + offset * float2(-1.0, -1.0), wa);
    half3 b = SampleBloomPrefilterTap(uv + offset * float2( 0.0, -1.0), wb);
    half3 c = SampleBloomPrefilterTap(uv + offset * float2( 1.0, -1.0), wc);
    half3 d = SampleBloomPrefilterTap(uv + offset * float2(-0.5, -0.5), wd);
    half3 e = SampleBloomPrefilterTap(uv + offset * float2( 0.5, -0.5), we);
    half3 f = SampleBloomPrefilterTap(uv + offset * float2(-1.0,  0.0), wf);
    half3 g = SampleBloomPrefilterTap(uv, wg);
    half3 h = SampleBloomPrefilterTap(uv + offset * float2( 1.0,  0.0), wh);
    half3 i = SampleBloomPrefilterTap(uv + offset * float2(-0.5,  0.5), wi);
    half3 j = SampleBloomPrefilterTap(uv + offset * float2( 0.5,  0.5), wj);
    half3 k = SampleBloomPrefilterTap(uv + offset * float2(-1.0,  1.0), wk);
    half3 l = SampleBloomPrefilterTap(uv + offset * float2( 0.0,  1.0), wl);
    half3 m = SampleBloomPrefilterTap(uv + offset * float2( 1.0,  1.0), wm);

    half3 color = WeightedAverage4(d, e, i, j, wd, we, wi, wj) * half(0.5);
    color += WeightedAverage4(a, b, f, g, wa, wb, wf, wg) * half(0.125);
    color += WeightedAverage4(b, c, g, h, wb, wc, wg, wh) * half(0.125);
    color += WeightedAverage4(f, g, k, l, wf, wg, wk, wl) * half(0.125);
    color += WeightedAverage4(g, h, l, m, wg, wh, wl, wm) * half(0.125);

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
    float2 offset = bloomParams.inputTexelSize * max(BP_BLOOM_RADIUS, 0.0);
    half3 color;
    if (BP_BLOOM_PREFILTER_QUALITY > 0.5) {
        color = SampleBloomPrefilterJimenez13Tap(uv, offset);
    } else {
        color = SampleBloomPrefilterRotatedGrid5Tap(uv, offset);
    }

    outputTexture[pixelCoord] = float4(float3(color), 1.0);
}
