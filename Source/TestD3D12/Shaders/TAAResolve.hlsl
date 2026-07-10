#include "Common.hlsli"
#include "Colors.hlsli"

#ifndef USE_NEIGHBORHOOD_COLOR_BOUNDS
#define USE_NEIGHBORHOOD_COLOR_BOUNDS 1
#endif

#ifndef USE_PLUS_NEIGHBORHOOD
#define USE_PLUS_NEIGHBORHOOD 0
#endif

#ifndef USE_DIAGONAL_NEIGHBORHOOD
#define USE_DIAGONAL_NEIGHBORHOOD 1
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
#define USE_SUBPIXEL_CORRECTION 1
#endif

#ifndef USE_DEPTH_DISOCCLUSION_FALLBACK
#define USE_DEPTH_DISOCCLUSION_FALLBACK 1
#endif

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer TAAResolveCB : register(b0) {
    TAAResolveParams taaResolveParams;
}

static const float TAAResolveDepthFallbackVelocityThreshold = 0.01;

Texture2D<float4> currentColorTexture : register(t0);
Texture2D<float2> velocityTexture : register(t1);
Texture2D<float> depthTexture : register(t2);
Texture2D<float4> previousColorHistoryTexture : register(t3);
Texture2D<float> previousDepthTexture : register(t4);
RWTexture2D<float4> outputColorHistoryTexture : register(u0);

#if USE_NEIGHBORHOOD_COLOR_BOUNDS
static const float TAAResolveMinColorBoxRange = 1e-5;

void AccumulateNeighborhoodColorBounds(int2 sampleCoord, int2 maxPixelCoord, inout float3 colorMin, inout float3 colorMax) {
    const int2 clampedCoord = clamp(sampleCoord, int2(0, 0), maxPixelCoord);
    float3 sampleColor = currentColorTexture.Load(int3(clampedCoord, 0)).rgb;
#if USE_YCOCG_CLAMP
    sampleColor = RGBToYCoCg(sampleColor);
#endif

    colorMin = min(colorMin, sampleColor);
    colorMax = max(colorMax, sampleColor);
}

// pixelCoord 주변 neighborhood 영역의 color bounds 를 구한다.
void ComputeNeighborhoodColorBounds(uint2 pixelCoord, float3 centerColor, out float3 colorMin, out float3 colorMax) {
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
#endif

float LinearizeDepth(float deviceDepth) {
    const float ndcDepth = deviceDepth * taaResolveParams.ndcDepthScaleBias.x + taaResolveParams.ndcDepthScaleBias.y;

    if (taaResolveParams.orthogonal != 0) {
        return LinearizeOrthogonalDepth(ndcDepth, taaResolveParams.projectionDepthParams.x, taaResolveParams.projectionDepthParams.y);
    }
    return LinearizePerspectiveDepth(ndcDepth, taaResolveParams.projectionDepthParams.x, taaResolveParams.projectionDepthParams.y);
}

float LoadCurrentDepth(int2 pixelCoord, int2 maxPixelCoord) {
    return depthTexture.Load(int3(clamp(pixelCoord, int2(0, 0), maxPixelCoord), 0));
}

float LoadPreviousDepth(float2 previousUV) {
    const int2 maxPixelCoord = int2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y) - 1;
    const float2 previousPixelCoord = previousUV * float2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y);
    const int2 pixelCoord = clamp(int2(previousPixelCoord.x, previousPixelCoord.y), int2(0, 0), maxPixelCoord);
    return previousDepthTexture.Load(int3(pixelCoord, 0));
}

#if USE_NEAREST_DEPTH_VELOCITY
// pixelCoord 주변 3x3 box 영역에서 가장 가까운 depth 픽셀 좌표를 찾는다.
int2 FindClosestDepthPixelCoord(uint2 pixelCoord, out float centerDepth) {
    const int2 baseCoord = int2(pixelCoord);
    const int2 maxPixelCoord = int2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y) - 1;
    const int2 centerCoord = clamp(baseCoord, int2(0, 0), maxPixelCoord);

    centerDepth = LoadCurrentDepth(centerCoord, maxPixelCoord);
    float closestDepth = centerDepth;
    int2 closestCoord = centerCoord;

    [unroll]
    for (int sampleY = -1; sampleY <= 1; ++sampleY) {
        [unroll]
        for (int sampleX = -1; sampleX <= 1; ++sampleX) {
            if (sampleX != 0 || sampleY != 0) {
                const int2 sampleCoord = clamp(baseCoord + int2(sampleX, sampleY), int2(0, 0), maxPixelCoord);
                const float sampleDepth = LoadCurrentDepth(sampleCoord, maxPixelCoord);

                if (taaResolveParams.useMinDepth != 0 ? sampleDepth < closestDepth : sampleDepth > closestDepth) {
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

    const float4 currentColor = currentColorTexture.Load(int3(pixelCoord, 0));
    float4 resolvedColor = currentColor;

#if USE_NEAREST_DEPTH_VELOCITY
    float currentDepth;
    const int2 velocityCoord = FindClosestDepthPixelCoord(pixelCoord, currentDepth);
#else
    const int2 maxPixelCoord = int2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y) - 1;
    const float currentDepth = LoadCurrentDepth(int2(pixelCoord), maxPixelCoord);
    const int2 velocityCoord = int2(pixelCoord);
#endif

    if (taaResolveParams.historyValid != 0 && currentDepth >= 0.0 && currentDepth <= 1.0) {
        const float2 uv = (float2(pixelCoord) + 0.5) * taaResolveParams.outputTexelSize;
        const float2 velocity = velocityTexture.Load(int3(velocityCoord, 0));
        const float2 previousUV = uv - velocity;

        if (all(previousUV >= 0.0) && all(previousUV <= 1.0)) {
#if USE_DEPTH_DISOCCLUSION_FALLBACK
            if (length(velocity) > TAAResolveDepthFallbackVelocityThreshold) {
                // 현재 픽셀의 depth 와 이전 프레임 depth 차이가 threshold 이상으로 차이가 나면 TAA 를 그만둔다.
                const float currentLinearDepth = max(LinearizeDepth(currentDepth), 0.0);
                const float previousLinearDepth = max(LinearizeDepth(LoadPreviousDepth(previousUV)), 0.0);
                if (abs(currentLinearDepth - previousLinearDepth) > taaResolveParams.depthFallbackThreshold) {
                    outputColorHistoryTexture[pixelCoord] = currentColor;
                    return;
                }
            }
#endif

            float3 historyColor = previousColorHistoryTexture.SampleLevel(linearClampSampler, previousUV, 0).rgb;

            float historyWeight = saturate(taaResolveParams.historyWeight);

#if USE_SUBPIXEL_CORRECTION
            // subpixel 이동이 클수록 current color 비중을 높여 history linear sampling 으로 생기는 blur 를 줄인다.
            const float2 velocityPixels = abs(velocity) * float2(taaResolveParams.outputSize.x, taaResolveParams.outputSize.y);
            const float subpixelCorrection = frac(max(velocityPixels.x, velocityPixels.y)) * 0.5;
            const float baseCurrentWeight = 1.0 - historyWeight;
            const float currentWeight = max(baseCurrentWeight, lerp(baseCurrentWeight, taaResolveParams.maxSubpixelCurrentWeight, subpixelCorrection));
            historyWeight = 1.0 - currentWeight;
#endif

#if USE_NEIGHBORHOOD_COLOR_BOUNDS
#if USE_YCOCG_CLAMP
            float3 centerColor = RGBToYCoCg(currentColor.rgb);
            historyColor = RGBToYCoCg(historyColor);
#else
            float3 centerColor = currentColor.rgb;
#endif
            float3 colorMin;
            float3 colorMax;
            ComputeNeighborhoodColorBounds(pixelCoord, centerColor, colorMin, colorMax);

#if USE_ADAPTIVE_WEIGHT
            // history color 가 color bounds 밖으로 벗어난 정도에 따라 historyWeight 를 줄인다.
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
            resolvedColor.rgb = lerp(currentColor.rgb, historyColor, historyWeight);
        }
    }

    outputColorHistoryTexture[pixelCoord] = resolvedColor;
}
