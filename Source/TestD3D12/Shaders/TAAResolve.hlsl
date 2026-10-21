#include "ShaderInterop.h"
#include "Common.hlsli"
#include "Colors.hlsli"

#ifndef USE_NEIGHBORHOOD_COLOR_BOUNDS
#define USE_NEIGHBORHOOD_COLOR_BOUNDS 1
#endif

#define TAA_COLOR_BOUNDS_MODE_MIN_MAX 0
#define TAA_COLOR_BOUNDS_MODE_VARIANCE 1

#ifndef TAA_COLOR_BOUNDS_MODE
#define TAA_COLOR_BOUNDS_MODE TAA_COLOR_BOUNDS_MODE_MIN_MAX
#endif

#if TAA_COLOR_BOUNDS_MODE != TAA_COLOR_BOUNDS_MODE_MIN_MAX && TAA_COLOR_BOUNDS_MODE != TAA_COLOR_BOUNDS_MODE_VARIANCE
#error Invalid TAA_COLOR_BOUNDS_MODE.
#endif

#define TAA_HISTORY_FILTER_MODE_BILINEAR 0
#define TAA_HISTORY_FILTER_MODE_CATMULL_ROM 1

#ifndef TAA_HISTORY_FILTER_MODE
#define TAA_HISTORY_FILTER_MODE TAA_HISTORY_FILTER_MODE_BILINEAR
#endif

#if TAA_HISTORY_FILTER_MODE != TAA_HISTORY_FILTER_MODE_BILINEAR && TAA_HISTORY_FILTER_MODE != TAA_HISTORY_FILTER_MODE_CATMULL_ROM
#error Invalid TAA_HISTORY_FILTER_MODE.
#endif

#ifndef USE_PLUS_NEIGHBORHOOD
#define USE_PLUS_NEIGHBORHOOD 0
#endif

#ifndef USE_DIAGONAL_NEIGHBORHOOD
#define USE_DIAGONAL_NEIGHBORHOOD 0
#endif

#if USE_PLUS_NEIGHBORHOOD && USE_DIAGONAL_NEIGHBORHOOD
#error USE_PLUS_NEIGHBORHOOD and USE_DIAGONAL_NEIGHBORHOOD are mutually exclusive.
#endif

#ifndef USE_YCOCG_CLAMP
#define USE_YCOCG_CLAMP 1
#endif

#ifndef USE_ADAPTIVE_WEIGHT
#define USE_ADAPTIVE_WEIGHT 1
#endif

#ifndef USE_NEAREST_DEPTH_VELOCITY
#define USE_NEAREST_DEPTH_VELOCITY 1
#endif

#ifndef USE_SUBPIXEL_CORRECTION
#define USE_SUBPIXEL_CORRECTION 0
#endif

#ifndef USE_LUMINANCE_WEIGHTING
#define USE_LUMINANCE_WEIGHTING 0
#endif

#ifndef USE_DEPTH_DISOCCLUSION_FALLBACK
#define USE_DEPTH_DISOCCLUSION_FALLBACK 1
#endif

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), SRV(t5), UAV(u0)), " \
    "CBV(b0), " \
    "CBV(b1), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer TAAResolveCB : register(b0) {
    TAAResolveParams taaResolveParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

static const float TAAResolveDepthFallbackVelocityThreshold = 0.01;

Texture2D<float4> colorTexture : register(t0);
Texture2D<float2> velocityTexture : register(t1);
Texture2D<float> depthTexture : register(t2);
Texture2D<float4> colorHistoryTexture : register(t3);
Texture2D<float> prevDepthTexture : register(t4);
Texture2D<float> reactiveMaskTexture : register(t5);
RWTexture2D<float4> outputColorHistoryTexture : register(u0);

#if TAA_HISTORY_FILTER_MODE == TAA_HISTORY_FILTER_MODE_CATMULL_ROM
// 4x4 Catmull-Rom filter 를 bilinear 9 tap 으로 축약하여 history 를 재구성한다.
float3 SampleColorHistoryCatmullRom(float2 uv) {
    const float2 textureSize = float2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y);
    const float2 samplePos = uv * textureSize;
    const float2 centerTexelPos = floor(samplePos - 0.5) + 0.5;
    const float2 f = samplePos - centerTexelPos;
    const float2 f2 = f * f;
    const float2 f3 = f2 * f;

    const float2 weight0 = f2 - 0.5 * (f3 + f);
    const float2 weight1 = 1.5 * f3 - 2.5 * f2 + 1.0;
    const float2 weight3 = 0.5 * (f3 - f2);
    const float2 weight2 = 1.0 - weight0 - weight1 - weight3;
    const float2 weight12 = weight1 + weight2;

    const float2 uv0 = (centerTexelPos - 1.0) * taaResolveParams.outputTexelSize;
    const float2 uv12 = (centerTexelPos + weight2 / weight12) * taaResolveParams.outputTexelSize;
    const float2 uv3 = (centerTexelPos + 2.0) * taaResolveParams.outputTexelSize;

    float3 historyColor = colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv0.x, uv0.y), 0).rgb * (weight0.x * weight0.y);
    historyColor += colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv0.x, uv12.y), 0).rgb * (weight0.x * weight12.y);
    historyColor += colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv0.x, uv3.y), 0).rgb * (weight0.x * weight3.y);
    historyColor += colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv12.x, uv0.y), 0).rgb * (weight12.x * weight0.y);
    historyColor += colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv12.x, uv12.y), 0).rgb * (weight12.x * weight12.y);
    historyColor += colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv12.x, uv3.y), 0).rgb * (weight12.x * weight3.y);
    historyColor += colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv3.x, uv0.y), 0).rgb * (weight3.x * weight0.y);
    historyColor += colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv3.x, uv12.y), 0).rgb * (weight3.x * weight12.y);
    historyColor += colorHistoryTexture.SampleLevel(linearClampSampler, float2(uv3.x, uv3.y), 0).rgb * (weight3.x * weight3.y);
    return historyColor;
}
#endif

float3 SampleColorHistory(float2 uv) {
#if TAA_HISTORY_FILTER_MODE == TAA_HISTORY_FILTER_MODE_BILINEAR
    return colorHistoryTexture.SampleLevel(linearClampSampler, uv, 0).rgb;
#elif TAA_HISTORY_FILTER_MODE == TAA_HISTORY_FILTER_MODE_CATMULL_ROM
    return SampleColorHistoryCatmullRom(uv);
#endif
}

#if USE_NEIGHBORHOOD_COLOR_BOUNDS
static const float TAAResolveMinColorBoxRange = 1e-5;
static const float TAAResolveRelativeColorTolerance = 0.02;
static const float TAAResolveColorToleranceMaxMotionPixels = 1.0;

float3 LoadNeighborhoodColor(int2 sampleCoord, int2 maxPixelCoord) {
    const int2 clampedCoord = clamp(sampleCoord, int2(0, 0), maxPixelCoord);
    float3 sampleColor = colorTexture.Load(int3(clampedCoord, 0)).rgb;
#if USE_YCOCG_CLAMP
    sampleColor = RGBToYCoCg(sampleColor);
#endif
    return sampleColor;
}

void AccumulateNeighborhoodColorBounds(int2 sampleCoord, int2 maxPixelCoord, inout float3 colorMin, inout float3 colorMax) {
    const float3 sampleColor = LoadNeighborhoodColor(sampleCoord, maxPixelCoord);
    colorMin = min(colorMin, sampleColor);
    colorMax = max(colorMax, sampleColor);
}

void ComputeMinMaxNeighborhoodColorBounds(uint2 pixelCoord, float3 centerColor, out float3 colorMin, out float3 colorMax) {
    const int2 baseCoord = int2(pixelCoord);
    const int2 maxPixelCoord = int2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y) - 1;

    colorMin = centerColor;
    colorMax = centerColor;

#if USE_PLUS_NEIGHBORHOOD
    AccumulateNeighborhoodColorBounds(baseCoord + int2(-1, 0), maxPixelCoord, colorMin, colorMax);
    AccumulateNeighborhoodColorBounds(baseCoord + int2( 1, 0), maxPixelCoord, colorMin, colorMax);
    AccumulateNeighborhoodColorBounds(baseCoord + int2( 0,-1), maxPixelCoord, colorMin, colorMax);
    AccumulateNeighborhoodColorBounds(baseCoord + int2( 0, 1), maxPixelCoord, colorMin, colorMax);
#elif USE_DIAGONAL_NEIGHBORHOOD
    AccumulateNeighborhoodColorBounds(baseCoord + int2(-1,-1), maxPixelCoord, colorMin, colorMax);
    AccumulateNeighborhoodColorBounds(baseCoord + int2( 1,-1), maxPixelCoord, colorMin, colorMax);
    AccumulateNeighborhoodColorBounds(baseCoord + int2(-1, 1), maxPixelCoord, colorMin, colorMax);
    AccumulateNeighborhoodColorBounds(baseCoord + int2( 1, 1), maxPixelCoord, colorMin, colorMax);
#else
    [unroll]
    for (int sampleY = -1; sampleY <= 1; ++sampleY) {
        [unroll]
        for (int sampleX = -1; sampleX <= 1; ++sampleX) {
            if (sampleX != 0 || sampleY != 0) {
                AccumulateNeighborhoodColorBounds(baseCoord + int2(sampleX, sampleY), maxPixelCoord, colorMin, colorMax);
            }
        }
    }
#endif
}

void ComputeVarianceNeighborhoodColorBounds(uint2 pixelCoord, float3 centerColor, out float3 colorMin, out float3 colorMax) {
    const int2 baseCoord = int2(pixelCoord);
    const int2 maxPixelCoord = int2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y) - 1;

    float3 colorSum = centerColor;
    float3 colorSquareSum = centerColor * centerColor;

    [unroll]
    for (int sampleY = -1; sampleY <= 1; ++sampleY) {
        [unroll]
        for (int sampleX = -1; sampleX <= 1; ++sampleX) {
            if (sampleX != 0 || sampleY != 0) {
                const float3 sampleColor = LoadNeighborhoodColor(baseCoord + int2(sampleX, sampleY), maxPixelCoord);
                colorSum += sampleColor;
                colorSquareSum += sampleColor * sampleColor;
            }
        }
    }

    static const float TAAResolveNeighborhoodSampleCountRcp = 1.0 / 9.0;
    const float3 colorMean = colorSum * TAAResolveNeighborhoodSampleCountRcp;
    const float3 colorVariance = max(colorSquareSum * TAAResolveNeighborhoodSampleCountRcp - colorMean * colorMean, float3(0.0, 0.0, 0.0));
    const float3 colorSigma = sqrt(colorVariance);
    const float colorBoxSigma = max(taaResolveParams.colorBoxSigma, 0.0);

    colorMin = colorMean - colorBoxSigma * colorSigma;
    colorMax = colorMean + colorBoxSigma * colorSigma;
}

// pixelCoord 주변 neighborhood 영역의 color bounds 를 선택한 방식으로 구한다.
void ComputeNeighborhoodColorBounds(uint2 pixelCoord, float3 centerColor, out float3 colorMin, out float3 colorMax) {
#if TAA_COLOR_BOUNDS_MODE == TAA_COLOR_BOUNDS_MODE_MIN_MAX
    ComputeMinMaxNeighborhoodColorBounds(pixelCoord, centerColor, colorMin, colorMax);
#elif TAA_COLOR_BOUNDS_MODE == TAA_COLOR_BOUNDS_MODE_VARIANCE
    ComputeVarianceNeighborhoodColorBounds(pixelCoord, centerColor, colorMin, colorMax);
#endif
}
#endif

float LoadDepth(int2 pixelCoord, int2 maxPixelCoord) {
    return depthTexture.Load(int3(clamp(pixelCoord, int2(0, 0), maxPixelCoord), 0));
}

float LoadPrevDepth(float2 prevUV) {
    const int2 maxPixelCoord = int2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y) - 1;
    const float2 prevPixelCoord = prevUV * float2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y);
    const int2 pixelCoord = clamp(int2(prevPixelCoord.x, prevPixelCoord.y), int2(0, 0), maxPixelCoord);
    return prevDepthTexture.Load(int3(pixelCoord, 0));
}

#if USE_NEAREST_DEPTH_VELOCITY
// pixelCoord 주변 3x3 box 영역에서 가장 가까운 depth 픽셀 좌표를 찾는다.
int2 FindClosestDepthPixelCoord(uint2 pixelCoord, out float centerDepth) {
    const int2 baseCoord = int2(pixelCoord);
    const int2 maxPixelCoord = int2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y) - 1;
    const int2 centerCoord = clamp(baseCoord, int2(0, 0), maxPixelCoord);

    centerDepth = LoadDepth(centerCoord, maxPixelCoord);
    float closestDepth = centerDepth;
    int2 closestCoord = centerCoord;

    [unroll]
    for (int sampleY = -1; sampleY <= 1; ++sampleY) {
        [unroll]
        for (int sampleX = -1; sampleX <= 1; ++sampleX) {
            if (sampleX != 0 || sampleY != 0) {
                const int2 sampleCoord = clamp(baseCoord + int2(sampleX, sampleY), int2(0, 0), maxPixelCoord);
                const float sampleDepth = LoadDepth(sampleCoord, maxPixelCoord);

                if (taaResolveParams.reverseZ != 0 ? sampleDepth > closestDepth : sampleDepth < closestDepth) {
                    closestDepth = sampleDepth;
                    closestCoord = sampleCoord;
                }
            }
        }
    }

    return closestCoord;
}
#endif

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(TAAResolveBlockSizeX, TAAResolveBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= taaResolveParams.outputSize)) {
        return;
    }

    const float4 currColor = colorTexture.Load(int3(pixelCoord, 0));
    float4 resolvedColor = currColor;

#if USE_NEAREST_DEPTH_VELOCITY
    float currDepth;
    const int2 velocityCoord = FindClosestDepthPixelCoord(pixelCoord, currDepth);
#else
    const int2 maxPixelCoord = int2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y) - 1;
    const float currDepth = LoadDepth(int2(pixelCoord), maxPixelCoord);
    const int2 velocityCoord = int2(pixelCoord);
#endif

    if (taaResolveParams.historyValid != 0 && currDepth >= 0.0 && currDepth <= 1.0) {
        const float2 uv = (float2(pixelCoord) + 0.5) * taaResolveParams.outputTexelSize;
        const float2 velocity = velocityTexture.Load(int3(velocityCoord, 0));
        const float2 prevUV = uv - velocity;

        if (all(prevUV >= 0.0) && all(prevUV <= 1.0)) {
#if USE_DEPTH_DISOCCLUSION_FALLBACK
            if (length(velocity) > TAAResolveDepthFallbackVelocityThreshold) {
                const float prevDepth = LoadPrevDepth(prevUV);
                const float clearDepth = taaResolveParams.reverseZ != 0 ? 0.0f : 1.0f;
                // 현재/이전 깊이는 보간 없이 읽으므로 clear 값만 배경으로 취급한다.
                const bool currBackground = currDepth == clearDepth;
                const bool prevBackground = prevDepth == clearDepth;

                // 배경과 표면 사이의 전환에서는 이전 프레임 결과를 사용하지 않는다.
                // 양쪽 모두 배경이면 깊이 비교를 생략해 무한대끼리 빼는 것을 피한다.
                bool rejectHistory = currBackground != prevBackground;
                if (!currBackground && !prevBackground) {
                    // 현재/이전 표면의 깊이 차이가 임계값을 넘으면 이전 프레임 결과를 제외한다.
                    const float currLinearDepth = max(cameraConstants.GetLinearDepth(currDepth), 0.0f);
                    const float prevLinearDepth = max(cameraConstants.GetLinearDepth(prevDepth), 0.0f);
                    rejectHistory = abs(currLinearDepth - prevLinearDepth) > taaResolveParams.depthFallbackThreshold;
                }

                if (rejectHistory) {
                    outputColorHistoryTexture[pixelCoord] = currColor;
                    return;
                }
            }
#endif

            float3 historyColor = SampleColorHistory(prevUV);

            float historyWeight = saturate(taaResolveParams.historyWeight);

            // 표면 velocity 로 추적할 수 없는 굴절 변화는 현재/이전 색 차이로 추정한다.
            // 정지한 굴절과 반사/거품 위주의 픽셀은 기존 누적을 유지하고, 변화가 커도
            // 이 제한만으로 history 를 완전히 버리지 않아 jitter 가 그대로 노출되지 않게 한다.
            const float reactiveMask = reactiveMaskTexture.Load(int3(pixelCoord, 0));
            if (reactiveMask > 0.0f) {
                const float3 colorDifference = abs(currColor.rgb - historyColor);
                const float3 colorMagnitude = max(abs(currColor.rgb), abs(historyColor));
                const float relativeColorDifference = max(colorDifference.r, max(colorDifference.g, colorDifference.b)) / max(max(colorMagnitude.r, max(colorMagnitude.g, colorMagnitude.b)), 0.01f);
                const float temporalChange = smoothstep(0.1f, 0.5f, relativeColorDifference);
                const float reactiveHistoryWeight = 0.5f;
                historyWeight = min(historyWeight, lerp(1.0f, reactiveHistoryWeight, reactiveMask * temporalChange));
            }

#if USE_SUBPIXEL_CORRECTION
            // subpixel 이동이 클수록 current color 비중을 높여 history 재구성 sampling 으로 생기는 blur 를 줄인다.
            const float2 velocityPixels = abs(velocity) * float2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y);
            const float subpixelCorrection = frac(max(velocityPixels.x, velocityPixels.y)) * 0.5;
            const float baseCurrentWeight = 1.0 - historyWeight;
            const float currentWeight = max(baseCurrentWeight, lerp(baseCurrentWeight, taaResolveParams.maxSubpixelCurrentWeight, subpixelCorrection));
            historyWeight = 1.0 - currentWeight;
#endif

#if USE_LUMINANCE_WEIGHTING
            // history 와 current 의 luminance 차이가 클수록 history 비중을 낮춘다.
            const float currentLuma = GetLuma(max(currColor.rgb, float3(0.0, 0.0, 0.0)));
            const float historyLuma = GetLuma(max(historyColor.rgb, float3(0.0, 0.0, 0.0)));
            const float luminanceMismatch = saturate(abs(historyLuma - currentLuma) / max(max(currentLuma, historyLuma), 1.0e-5));
            historyWeight *= lerp(1.0, 1.0 - luminanceMismatch, saturate(taaResolveParams.luminanceWeight));
#endif

#if USE_NEIGHBORHOOD_COLOR_BOUNDS
#if USE_YCOCG_CLAMP
            float3 centerColor = RGBToYCoCg(currColor.rgb);
            historyColor = RGBToYCoCg(historyColor);
#else
            float3 centerColor = currColor.rgb;
#endif
            float3 colorMin;
            float3 colorMax;
            ComputeNeighborhoodColorBounds(pixelCoord, centerColor, colorMin, colorMax);

            // 화면상 이동량이 작을수록 color bounds 에 밝기에 비례한 여유를 더한다.
            // 이동량이 커질수록 여유를 줄이고 1 pixel/frame 부터는 적용하지 않는다.
            const float2 boundsVelocityPixels = abs(velocity) * float2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y);
            const float boundsStability = saturate(1.0 - max(boundsVelocityPixels.x, boundsVelocityPixels.y) / TAAResolveColorToleranceMaxMotionPixels);
#if USE_YCOCG_CLAMP
            const float boundsLuma = max(centerColor.x, 0.0);
#else
            const float boundsLuma = GetLuma(max(centerColor, float3(0.0, 0.0, 0.0)));
#endif
            const float colorTolerance = boundsLuma * TAAResolveRelativeColorTolerance * boundsStability;
            colorMin -= colorTolerance;
            colorMax += colorTolerance;

#if USE_ADAPTIVE_WEIGHT
            // history color 가 허용 여유를 포함한 color bounds 밖으로 벗어난 정도에 따라 historyWeight 를 줄인다.
            const float3 unclampedHistoryColor = historyColor;
            historyColor = clamp(historyColor, colorMin, colorMax);
            const float3 colorRange = max(colorMax - colorMin, float3(TAAResolveMinColorBoxRange, TAAResolveMinColorBoxRange, TAAResolveMinColorBoxRange));
            const float3 normalizedClampDistance = abs(historyColor - unclampedHistoryColor) / colorRange;
            const float clampAmount = saturate(max(max(normalizedClampDistance.x, normalizedClampDistance.y), normalizedClampDistance.z));
            historyWeight *= (1.0 - clampAmount);
#else
            historyColor = clamp(historyColor, colorMin, colorMax);
#endif

#if USE_YCOCG_CLAMP
            historyColor = max(YCoCgToRGB(historyColor), float3(0.0, 0.0, 0.0));
#endif
#endif // USE_NEIGHBORHOOD_COLOR_BOUNDS
            resolvedColor.rgb = lerp(currColor.rgb, historyColor, historyWeight);
        }
    }

    outputColorHistoryTexture[pixelCoord] = resolvedColor;
}
