#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../Colors.hlsli"
#include "SSRDepth.hlsli"
#include "SSRRadiance.hlsli"
#include "SSRResolution.hlsli"

#define COMMON_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), SRV(t5, flags = DATA_VOLATILE), SRV(t6), UAV(u0)), " \
    "RootConstants(num32BitConstants=10, b0), " \
    "CBV(b1), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer SSRTemporalCB : register(b0) {
    SSRTemporalParams ssrTemporalParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

Texture2D<float4> inputCurrReflectionTexture : register(t0); // RGB: 압축된 confidence-premultiplied radiance, A: confidence.
Texture2D<float4> inputPrevReflectionTexture : register(t1);
Texture2D<float4> inputTraceTexture : register(t2); // A: trace validity.
Texture2D<float4> inputReceiverSurfaceTexture : register(t3); // B: normalized block pixel index.
Texture2D<float2> inputPrevDepthTexture : register(t4); // Opaque reduced-resolution SSR: half-res max/min. Otherwise: full-res device depth in R.
Texture2D<float> inputCurrDepthTexture : register(t5);
Texture2D<float2> inputVelocityTexture : register(t6); // Unjittered receiver velocity.

RWTexture2D<float4> outputReflectionTexture : register(u0);

// 현재 프레임의 depth gradient 를 이전 프레임의 depth gradient 로 재투영한다.
float2 ReprojectDepthGradient(float2 currUV, float currDepth, float2 depthGradient) {
    // currDepth 위치에서 depthGradient 로 기울어진 clip-space 평면을 구한다.
    const float2 ndcXY = currUV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    const float2 depthPerNDC = depthGradient * float2(ssrTemporalParams.inputSize) * float2(0.5f, -0.5f);
    const float4 clipPlane = float4(-depthPerNDC, 1.0f, dot(depthPerNDC, ndcXY) - currDepth);

    // dot(clipPlane, mul(viewProjMatrix, worldPos)) == 0
    // dot(mul(clipPlane, viewProjMatrix), worldPos) == 0
    // dot(worldPlane, worldPos) == 0
    const float4 worldPlane = mul(clipPlane, cameraConstants.viewProjMatrix);

    // dot(worldPlane, mul(prevViewProjMatrix, prevClipPos)) == 0
    // dot(mul(worldPlane, prevInvViewProjMatrix), prevClipPos) == 0
    // dot(prevClipPlane, prevClipPos) == 0
    const float3 prevClipPlane = mul(worldPlane, cameraConstants.prevInvViewProjMatrix).xyz; // 기울기만 알면 된다.

    const float planeScale = max(max(abs(prevClipPlane.x), abs(prevClipPlane.y)), abs(prevClipPlane.z));
    if (!all(isfinite(prevClipPlane)) || abs(prevClipPlane.z) <= planeScale * 1.0e-6f) {
        return 0.0f;
    }

    const float2 prevDepthPerNDC = -prevClipPlane.xy / prevClipPlane.z;
    const float2 prevDepthGradient = prevDepthPerNDC * float2(2.0f, -2.0f) / float2(ssrTemporalParams.inputSize);
    return all(isfinite(prevDepthGradient)) ? prevDepthGradient : 0.0f;
}

// 현재 프레임 블록 좌표와 블록 내부 픽셀 인덱스로 이전 프레임 블록 좌표, 픽셀 위치, depth gradient 와 픽셀 단위 이동 거리의 제곱을 계산한다.
// prevPixelCoord.xy 는 full-resolution 픽셀 좌표이고, z 는 device depth 이다.
bool ReprojectReceiverSurface(uint2 blockCoord, float encodedBlockPixelIndex, out float2 prevBlockCoord, out float3 prevPixelCoord, out float2 prevDepthGradient, out float motionPixelsSquared) {
    prevBlockCoord = 0.0f;
    prevPixelCoord = 0.0f;
    prevDepthGradient = 0.0f;
    motionPixelsSquared = 0.0f;

    const uint2 blockPixelOffset = DecodeSSRBlockPixelOffset(encodedBlockPixelIndex);
    const uint2 pixelCoord = min(blockCoord * SSR_RESOLUTION_DIVISOR + blockPixelOffset, ssrTemporalParams.inputSize - 1u);
    const float currDepth = inputCurrDepthTexture.Load(int3(pixelCoord, 0));
    const float2 currUV = (float2(pixelCoord) + 0.5f) / float2(ssrTemporalParams.inputSize);

    const float3 worldPos = ReconstructWorldPosition(currUV, currDepth, cameraConstants.invViewProjMatrix);
    const float4 prevClipPos = mul(cameraConstants.prevViewProjMatrix, float4(worldPos, 1.0f));
    if (prevClipPos.w <= 0.0f) {
        return false;
    }

    const float3 prevNDC = prevClipPos.xyz / prevClipPos.w;
    const float prevDepth = prevNDC.z;

    const float2 velocity = inputVelocityTexture.Load(int3(pixelCoord, 0));
    const float2 prevUV = currUV - velocity - ssrTemporalParams.uvJitterDelta; // currUV 는 jittered UV 이므로, 이전 프레임의 jittered UV 를 얻으려면 uvJitterDelta 를 빼야 한다.

    // 이전 프레임 UV 나 예상 깊이가 NaN/Inf 이거나 [0, 1] 범위를 벗어나면 재투영을 거부한다.
    if (!all(isfinite(prevUV)) || any(prevUV < 0.0f) || any(prevUV > 1.0f) || !isfinite(prevDepth) || prevDepth < 0.0f || prevDepth > 1.0f) {
        return false;
    }

    // 이미 읽은 unjittered velocity 로 full-resolution 이동 거리의 제곱을 구해 TAA jitter 를 움직임에서 제외한다.
    const float2 velocityPixels = velocity * float2(ssrTemporalParams.inputSize);
    motionPixelsSquared = dot(velocityPixels, velocityPixels);

    // 각 프레임의 jitter 보정을 포함한 UV 이동량을 full-resolution 픽셀 이동량으로 변환해 이전 블록 좌표와 픽셀 위치를 계산한다.
    const float2 motionDeltaPixels = (prevUV - currUV) * float2(ssrTemporalParams.inputSize);
    prevBlockCoord = float2(blockCoord) + motionDeltaPixels * (1.0f / float(SSR_RESOLUTION_DIVISOR));
    prevPixelCoord = float3(float2(pixelCoord) + motionDeltaPixels, prevDepth);

    // 깊이 평면의 기준점은 reflection texel 중심과 구분하고, velocity 로 움직인 실제 픽셀 위치에 둔다.
    const float2 depthGradient = ComputeSSRDepthGradient(inputCurrDepthTexture, ssrTemporalParams.inputSize, pixelCoord, currDepth, cameraConstants.depthClearValue);
    prevDepthGradient = ReprojectDepthGradient(currUV, currDepth, depthGradient);
    return true;
}

float2 LoadPrevDepthExtrema(int2 prevBlockCoord) {
#if SSR_RESOLUTION_DIVISOR == 1
    const float depth = inputPrevDepthTexture.Load(int3(prevBlockCoord, 0)).x;
    return depth != cameraConstants.depthClearValue ? depth.xx : float2(0.0f, 1.0f);
#else
    if (ssrTemporalParams.transparentReceiver == 0u) {
#if SSR_RESOLUTION_DIVISOR == 2
        return inputPrevDepthTexture.Load(int3(prevBlockCoord, 0));
#else
        // A quarter-resolution receiver covers four existing half-resolution depth extrema.
        const uint2 halfSize = (ssrTemporalParams.inputSize + 1u) >> 1u;
        const uint2 coordMin = uint2(prevBlockCoord) * 2u;
        float2 extrema = float2(0.0f, 1.0f);
        [unroll]
        for (uint y = 0; y < 2; ++y) {
            [unroll]
            for (uint x = 0; x < 2; ++x) {
                const uint2 coord = min(coordMin + uint2(x, y), halfSize - 1u);
                const float2 sampleDepth = inputPrevDepthTexture.Load(int3(coord, 0));
                extrema = float2(max(extrema.x, sampleDepth.x), min(extrema.y, sampleDepth.y));
            }
        }
        return extrema;
#endif
    }

    const uint2 inputCoordMin = uint2(prevBlockCoord) * SSR_RESOLUTION_DIVISOR;
    const float clearDepth = cameraConstants.depthClearValue;
    float maxDepth = 0.0f;
    float minDepth = 1.0f;
    bool hasSurface = false;
    [unroll]
    for (uint blockPixelIndex = 0u; blockPixelIndex < SSR_BLOCK_PIXEL_COUNT; ++blockPixelIndex) {
        const uint2 offset = uint2(blockPixelIndex % SSR_RESOLUTION_DIVISOR, blockPixelIndex / SSR_RESOLUTION_DIVISOR);
        const uint2 coord = min(inputCoordMin + offset, ssrTemporalParams.inputSize - 1u);
        const float depth = inputPrevDepthTexture.Load(int3(coord, 0)).x;
        if (depth != clearDepth) {
            maxDepth = max(maxDepth, depth);
            minDepth = min(minDepth, depth);
            hasSurface = true;
        }
    }
    return hasSurface ? float2(maxDepth, minDepth) : float2(0.0f, 1.0f);
#endif
}

// 표면 경사를 보정한 예상 깊이와 이전 프레임의 실제 깊이를 비교해 history 재사용 여부를 판단한다.
bool IsPreviousSurfaceCompatible(int2 sampleBlockCoord, float3 prevPixelCoord, float2 prevDepthGradient) {
    const float2 extrema = LoadPrevDepthExtrema(sampleBlockCoord);
    if (extrema.y > extrema.x || !all(isfinite(extrema))) {
        return false;
    }
    const float farDepth = cameraConstants.depthClearValue;
    const float closestDepth = farDepth == 0.0f ? extrema.x : extrema.y;

    // History 는 block 의 가장 가까운 표면에 속한다. 같은 경사면도 재투영된 픽셀과 block 의 위치가 다르면
    // 깊이가 크게 달라지므로, 그 block 안에서 평면이 가장 가까워지는 픽셀 중심의 예상 깊이와 비교한다.
    // Min/max 범위 전체를 허용하지 않아 foreground 가 섞인 block 의 history 는 계속 거부한다.
    const float2 coordMin = float2(sampleBlockCoord * SSR_RESOLUTION_DIVISOR);
    const float2 coordMax = min(coordMin + float(SSR_RESOLUTION_DIVISOR - 1), float2(ssrTemporalParams.inputSize - 1u));
    const float2 nearDirection = prevDepthGradient * (farDepth == 0.0f ? 1.0f : -1.0f);
    const float2 closestPixelCoord = float2(nearDirection.x > 0.0f ? coordMax.x : coordMin.x, nearDirection.y > 0.0f ? coordMax.y : coordMin.y);
    const float expectedDepth = prevPixelCoord.z + dot(prevDepthGradient, closestPixelCoord - prevPixelCoord.xy);
    if (!isfinite(expectedDepth) || expectedDepth < 0.0f || expectedDepth > 1.0f || expectedDepth == farDepth) {
        return false;
    }

    // 경사를 보정한 추정 linear depth 의 1% 까지 오차를 허용한다.
    const float expectedLinearDepth = cameraConstants.GetLinearDepth(expectedDepth);
    const float closestLinearDepth = cameraConstants.GetLinearDepth(closestDepth);
    const float relativeDepthTolerance = 0.01f;
    return isfinite(expectedLinearDepth) && expectedLinearDepth > 0.0f &&
           isfinite(closestLinearDepth) && closestLinearDepth > 0.0f &&
           abs(expectedLinearDepth - closestLinearDepth) <= expectedLinearDepth * relativeDepthTolerance;
}

// 재투영된 prevBlockCoord 주변의 2x2 history 샘플을 깊이 검증한 뒤 bilinear 보간한다.
// prevPixelCoord 와 prevDepthGradient 로 표면 경사를 보정해 같은 표면으로 판정된 샘플만 사용한다.
// RGB 는 압축을 해제한 상태에서 confidence 와 함께 보간하고, 유효한 샘플의 가중치 합으로 정규화한다.
// 보간된 RGB 는 다시 압축하며, 유효한 샘플의 가중치 합이 너무 작으면 false 를 반환한다.
bool LoadValidatedPreviousReflection(float2 prevBlockCoord, float3 prevPixelCoord, float2 prevDepthGradient, out float4 prevReflection) {
    const int2 baseCoord = int2(floor(prevBlockCoord));
    const float2 blend = frac(prevBlockCoord);
    const int2 maxCoord = int2(ssrTemporalParams.outputSize) - 1;

    prevReflection = 0.0f;
    float weightSum = 0.0f;

    // History color 의 bilinear 샘플마다 깊이를 검사하고, 통과한 샘플만 보간한다.
    [unroll]
    for (uint y = 0u; y < 2u; ++y) {
        [unroll]
        for (uint x = 0u; x < 2u; ++x) {
            const int2 sampleCoord = clamp(baseCoord + int2(x, y), int2(0, 0), maxCoord);
            const float spatialWeight = (x == 0u ? 1.0f - blend.x : blend.x) * (y == 0u ? 1.0f - blend.y : blend.y);
            if (spatialWeight <= 0.0f) {
                continue;
            }
            
            if (IsPreviousSurfaceCompatible(sampleCoord, prevPixelCoord, prevDepthGradient)) {
                const float4 reflection = inputPrevReflectionTexture.Load(int3(sampleCoord, 0));
                prevReflection += float4(DecompressSSRRadiance(reflection.rgb), reflection.a) * spatialWeight;
                weightSum += spatialWeight;
            }
        }
    }

    if (weightSum <= 1.0e-5f) {
        return false;
    }

    prevReflection *= rcp(weightSum);
    prevReflection.rgb = CompressSSRRadiance(prevReflection.rgb);
    return true;
}

// 현재 프레임의 3x3 이웃에서 history 제한용 YCoCg 색상 범위 (colorMin, colorMax) 와 최대 confidence 를 계산한다.
void ComputeCurrentNeighborhoodBounds(uint2 pixelCoord, out float3 colorMin, out float3 colorMax, out float confidenceMax) {
    const int2 maxCoord = int2(ssrTemporalParams.outputSize) - 1;
    colorMin = float3(FLT_MAX, FLT_MAX, FLT_MAX);
    colorMax = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    confidenceMax = 0.0f;
    float3 colorSum = 0.0f;
    float3 colorSquareSum = 0.0f;

    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            const int2 sampleCoord = clamp(int2(pixelCoord) + int2(x, y), int2(0, 0), maxCoord);
            const float4 sampleReflection = inputCurrReflectionTexture.Load(int3(sampleCoord, 0));
            const float3 sampleColor = RGBToYCoCg(sampleReflection.rgb);
            colorMin = min(colorMin, sampleColor);
            colorMax = max(colorMax, sampleColor);
            confidenceMax = max(confidenceMax, sampleReflection.a);
            colorSum += sampleColor;
            colorSquareSum += sampleColor * sampleColor;
        }
    }

    // 기존 3x3 샘플을 재사용해 compressed premultiplied YCoCg 공간에서 variance clamp 를 계산한다.
    const float3 colorMean = colorSum * (1.0f / 9.0f);
    const float3 colorVariance = max(colorSquareSum * (1.0f / 9.0f) - colorMean * colorMean, 0.0f);
    const float3 colorExtent = 2.0f * sqrt(colorVariance);

    // 균일한 색의 반올림 오차로 bounds 가 뒤집히지 않도록 평균을 기존 min/max 안에 둔다.
    const float3 boundedMean = clamp(colorMean, colorMin, colorMax);

    // 밝은 이웃 하나가 오래된 반사색을 계속 허용하지 않게 min/max 와 mean +/- 2 sigma 의 교집합을 사용한다.
    colorMin = max(colorMin, boundedMean - colorExtent);
    colorMax = min(colorMax, boundedMean + colorExtent);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSRTemporalBlockSizeX, SSRTemporalBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= ssrTemporalParams.outputSize)) {
        return;
    }

    const float4 currReflection = inputCurrReflectionTexture.Load(int3(pixelCoord, 0));
    if (ssrTemporalParams.historyValid == 0u) {
        outputReflectionTexture[pixelCoord] = currReflection;
        return;
    }

    const float4 traceRecord = inputTraceTexture.Load(int3(pixelCoord, 0));
    // NoSurface or RoughnessRejected 픽셀은 이전 프레임의 reflection 을 누적하지 않는다.
    if (traceRecord.a <= SSRTraceValidityNoSurface) {
        outputReflectionTexture[pixelCoord] = currReflection;
        return;
    }

    const float4 receiverSurface = inputReceiverSurfaceTexture.Load(int3(pixelCoord, 0));
    float2 prevBlockCoord;
    float3 prevPixelCoord;
    float2 prevDepthGradient;
    float motionPixelsSquared;
    if (!ReprojectReceiverSurface(pixelCoord, receiverSurface.b, prevBlockCoord, prevPixelCoord, prevDepthGradient, motionPixelsSquared)) {
        outputReflectionTexture[pixelCoord] = currReflection;
        return;
    }

    float4 prevReflection;
    if (!LoadValidatedPreviousReflection(prevBlockCoord, prevPixelCoord, prevDepthGradient, prevReflection)) {
        outputReflectionTexture[pixelCoord] = currReflection;
        return;
    }

    float3 colorMin;
    float3 colorMax;
    float confidenceMax;
    ComputeCurrentNeighborhoodBounds(pixelCoord, colorMin, colorMax, confidenceMax);

    // 이전 reflection 의 압축된 RGB 를 현재 프레임의 3x3 이웃 색상 범위로 clamp 한다.
    const float3 clampedPrevColor = max(YCoCgToRGB(clamp(RGBToYCoCg(prevReflection.rgb), colorMin, colorMax)), 0.0f);
    float historyWeight = saturate(ssrTemporalParams.historyWeight);

    // confidence 가 곱해진 현재/이전 radiance 와 confidence 를 각각 누적한다.
    const float3 currPremulRadiance = DecompressSSRRadiance(currReflection.rgb);
    const float3 prevPremulRadiance = DecompressSSRRadiance(confidenceMax > 0.0f ? clampedPrevColor : prevReflection.rgb);
    const float3 resolvedPremulRadiance = lerp(currPremulRadiance, prevPremulRadiance, historyWeight);
    const float resolvedConfidence = lerp(saturate(currReflection.a), saturate(prevReflection.a), historyWeight);
    outputReflectionTexture[pixelCoord] = float4(CompressSSRRadiance(resolvedPremulRadiance), resolvedConfidence);
}
