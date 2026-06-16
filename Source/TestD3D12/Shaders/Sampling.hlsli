#ifndef __SAMPLING_HLSLI__
#define __SAMPLING_HLSLI__

#ifndef SAMPLING_LINEAR_CLAMP_SAMPLER
#define SAMPLING_LINEAR_CLAMP_SAMPLER linearClampSampler
#endif

// Equal-weight 4-tap box sampling pattern.
half3 SampleBox4Tap(Texture2D<float4> texture, float2 uv, float2 offset) {
    float4 d = offset.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

    half3 color = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.xy, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.zy, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.xw, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.zw, 0).rgb);

    return color * half(0.25);
}

// Center-weighted quincunx 5-tap filter.
// Samples the center with 4x weight and the four diagonal taps with 1x weight.
// The weights sum to 8, so this adds a mild blur while sampling.
half3 SampleQuincunx5Tap(Texture2D<float4> texture, float2 uv, float2 offset) {
    float4 d = offset.xyxy * float4(-1.0, -1.0, 1.0, 1.0);

    half3 color = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv, 0).rgb) * half(4.0);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.xy, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.zy, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.xw, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.zw, 0).rgb);

    return color * half(0.125);
}

// Weighted Jimenez 13-tap filter using bilinear sampling.
// [Jimenez14] http://goo.gl/eomGso
// . . . . . . .
// . A . B . C .
// . . D . E . .
// . F . G . H .
// . . I . J . .
// . K . L . M .
// . . . . . . .
half3 SampleJimenez13Tap(Texture2D<float4> texture, float2 uv, float2 offset) {
    half3 a = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2(-1.0, -1.0), 0).rgb);
    half3 b = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2( 0.0, -1.0), 0).rgb);
    half3 c = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2( 1.0, -1.0), 0).rgb);
    half3 d = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2(-0.5, -0.5), 0).rgb);
    half3 e = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2( 0.5, -0.5), 0).rgb);
    half3 f = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2(-1.0,  0.0), 0).rgb);
    half3 g = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv, 0).rgb);
    half3 h = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2( 1.0,  0.0), 0).rgb);
    half3 i = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2(-0.5,  0.5), 0).rgb);
    half3 j = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2( 0.5,  0.5), 0).rgb);
    half3 k = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2(-1.0,  1.0), 0).rgb);
    half3 l = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2( 0.0,  1.0), 0).rgb);
    half3 m = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + offset * float2( 1.0,  1.0), 0).rgb);

    half3 color = (d + e + i + j) * half(0.5);
    color += (a + b + g + f) * half(0.125);
    color += (b + c + h + g) * half(0.125);
    color += (f + g + l + k) * half(0.125);
    color += (g + h + m + l) * half(0.125);

    return color * half(0.25);
}

// 3x3 tent filter.
// Uses weights [1 2 1; 2 4 2; 1 2 1] normalized by 16.
half3 SampleTent9Tap(Texture2D<float4> texture, float2 uv, float2 offset) {
    float4 d = offset.xyxy * float4(1.0, 1.0, -1.0, 0.0);

    half3 color = half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv - d.xy, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv - d.wy, 0).rgb) * half(2.0);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv - d.zy, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv - d.xw, 0).rgb) * half(2.0);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv, 0).rgb) * half(4.0);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.xw, 0).rgb) * half(2.0);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.zy, 0).rgb);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.wy, 0).rgb) * half(2.0);
    color += half3(texture.SampleLevel(SAMPLING_LINEAR_CLAMP_SAMPLER, uv + d.xy, 0).rgb);

    return color * half(1.0 / 16.0);
}

#endif // __SAMPLING_HLSLI__
