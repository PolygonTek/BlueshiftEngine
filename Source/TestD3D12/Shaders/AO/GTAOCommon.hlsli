///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Portions adapted from XeGTAO.
// Copyright (C) 2016-2021, Intel Corporation
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GTAO_COMMON_HLSLI__
#define __GTAO_COMMON_HLSLI__

#include "../ShaderInterop.h"
#include "../Common.hlsli"

static const float GTAOWorkingVisibilityScale = 1.5f;
static const uint GTAOHilbertLevel = 6u;
static const uint GTAOHilbertWidth = 1u << GTAOHilbertLevel;

uint GTAOHilbertIndex(uint2 pixelCoord) {
    uint posX = pixelCoord.x & (GTAOHilbertWidth - 1u);
    uint posY = pixelCoord.y & (GTAOHilbertWidth - 1u);
    uint index = 0u;

    [unroll]
    for (uint level = GTAOHilbertWidth >> 1u; level > 0u; level >>= 1u) {
        const uint regionX = (posX & level) != 0u;
        const uint regionY = (posY & level) != 0u;
        index += level * level * ((3u * regionX) ^ regionY);

        if (regionY == 0u) {
            if (regionX != 0u) {
                posX = GTAOHilbertWidth - 1u - posX;
                posY = GTAOHilbertWidth - 1u - posY;
            }
            const uint temp = posX;
            posX = posY;
            posY = temp;
        }
    }
    return index;
}

float2 GTAOSpatioTemporalNoise(uint2 pixelCoord, uint temporalIndex) {
    const uint index = GTAOHilbertIndex(pixelCoord) + 288u * (temporalIndex & 63u);
    // R2 sequence driven by the 64x64 Hilbert ordering.
    return frac(0.5f + float(index) * float2(0.75487766624669276005f, 0.56984029099805326591f));
}

float GTAOFilterDepthMip(float4 depths, GTAOPrefilterParams params) {
    const float maxDepth = max(max(depths.x, depths.y), max(depths.z, depths.w));
    const float effectRadius = 0.75f * params.radius * params.radiusMultiplier;
    const float falloffRange = max(params.falloffRange * effectRadius, 1.0e-6f);
    const float falloffFrom = effectRadius * (1.0f - params.falloffRange);
    const float falloffMul = -rcp(falloffRange);
    const float falloffAdd = falloffFrom / falloffRange + 1.0f;
    // Unlike the reference implementation, zero is our no-surface sentinel. Exclude it so
    // background samples cannot pull near-camera geometry toward the camera in coarser mips.
    const float4 validMask = float4(depths.x > 0.0f ? 1.0f : 0.0f,
        depths.y > 0.0f ? 1.0f : 0.0f,
        depths.z > 0.0f ? 1.0f : 0.0f,
        depths.w > 0.0f ? 1.0f : 0.0f);
    const float4 weights = saturate((maxDepth - depths) * falloffMul + falloffAdd) * validMask;
    return dot(weights, depths) / max(weights.x + weights.y + weights.z + weights.w, 1.0e-6f);
}

float4 GTAOCalculateEdges(float centerZ, float leftZ, float rightZ, float topZ, float bottomZ) {
    float4 edges = float4(leftZ, rightZ, topZ, bottomZ) - centerZ;
    const float slopeLR = (edges.y - edges.x) * 0.5f;
    const float slopeTB = (edges.w - edges.z) * 0.5f;
    const float4 slopeAdjusted = edges + float4(slopeLR, -slopeLR, slopeTB, -slopeTB);
    edges = min(abs(edges), abs(slopeAdjusted));
    return saturate(1.25f - edges / max(centerZ * 0.011f, 1.0e-6f));
}

float GTAOPackEdges(float4 edgesLRTB) {
    // Four 2-bit edge weights packed into one R8_UNORM value.
    const float4 quantized = round(saturate(edgesLRTB) * 2.9f);
    return dot(quantized, float4(64.0f, 16.0f, 4.0f, 1.0f) / 255.0f);
}

float4 GTAOUnpackEdges(float packedValue) {
    const uint packed = uint(packedValue * 255.5f);
    return saturate(float4((packed >> 6u) & 3u, (packed >> 4u) & 3u, (packed >> 2u) & 3u, packed & 3u) / 3.0f);
}

// Match the nearest surface selected by the GTAO depth prefilter at half resolution.
uint2 GTAOSelectInputCoord(uint2 outputCoord, uint resolutionScale, uint2 inputSize, float farPlaneDeviceDepth, Texture2D<float> depthTexture) {
    const uint2 blockOrigin = outputCoord * resolutionScale;
    uint2 selectedCoord = min(blockOrigin, inputSize - 1u);
    float selectedDepth = farPlaneDeviceDepth;
    bool foundSurface = false;

    [unroll]
    for (uint y = 0u; y < 2u; ++y) {
        [unroll]
        for (uint x = 0u; x < 2u; ++x) {
            if (x >= resolutionScale || y >= resolutionScale) {
                continue;
            }

            const uint2 inputCoord = blockOrigin + uint2(x, y);
            if (any(inputCoord >= inputSize)) {
                continue;
            }

            const float depth = depthTexture.Load(int3(inputCoord, 0));
            if (depth != farPlaneDeviceDepth && (!foundSurface || (farPlaneDeviceDepth == 0.0f ? depth > selectedDepth : depth < selectedDepth))) {
                selectedCoord = inputCoord;
                selectedDepth = depth;
                foundSurface = true;
            }
        }
    }
    return selectedCoord;
}

float GTAOFastSqrt(float value) {
    return asfloat(0x1fbd1df5 + (asint(max(value, 0.0f)) >> 1));
}

float GTAOFastAcos(float value) {
    const float x = abs(clamp(value, -1.0f, 1.0f));
    float result = (-0.156583f * x + HALF_PI) * GTAOFastSqrt(1.0f - x);
    return value >= 0.0f ? result : PI - result;
}

float GTAOEvaluateVisibility(uint2 pixelCoord, float3 viewNormal, GTAOMainParams params, uint sliceCount, uint stepsPerSlice, CameraConstants cameraConstants, Texture2D<float> prefilteredDepthTexture, SamplerState pointClampSampler, out float packedEdges) {
    const int2 maxPixelCoord = int2(params.outputSize) - 1;
    const int2 centerCoord = int2(pixelCoord);
    const int2 leftCoord = clamp(centerCoord + int2(-1, 0), 0, maxPixelCoord);
    const int2 rightCoord = clamp(centerCoord + int2(1, 0), 0, maxPixelCoord);
    const int2 topCoord = clamp(centerCoord + int2(0, -1), 0, maxPixelCoord);
    const int2 bottomCoord = clamp(centerCoord + int2(0, 1), 0, maxPixelCoord);

    float centerZ = prefilteredDepthTexture.Load(int3(centerCoord, 0));
    const float leftZ = prefilteredDepthTexture.Load(int3(leftCoord, 0));
    const float rightZ = prefilteredDepthTexture.Load(int3(rightCoord, 0));
    const float topZ = prefilteredDepthTexture.Load(int3(topCoord, 0));
    const float bottomZ = prefilteredDepthTexture.Load(int3(bottomCoord, 0));

    const float4 edgesLRTB = GTAOCalculateEdges(centerZ, leftZ, rightZ, topZ, bottomZ);
    packedEdges = GTAOPackEdges(edgesLRTB);

    if (centerZ <= 0.0f) {
        return rcp(GTAOWorkingVisibilityScale);
    }

    // Pull the receiver slightly toward the camera to suppress depth quantization self-occlusion.
    centerZ *= 0.99999f;
    const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;
    const float2 uv = (float2(pixelCoord) + 0.5f) * params.outputSizeRcp;
    const float3 centerPosition = ReconstructViewPosition(uv, centerZ, cameraConstants.uvToViewScaleBiasXY, orthogonal);
    const float3 viewVector = orthogonal ? float3(0.0f, 0.0f, 1.0f) : normalize(-centerPosition);
    viewNormal = normalize(viewNormal);

    const float3 rightPosition = ReconstructViewPosition(uv + float2(params.outputSizeRcp.x, 0.0f), centerZ, cameraConstants.uvToViewScaleBiasXY, orthogonal);
    const float pixelViewSize = max(length(rightPosition - centerPosition), 1.0e-6f);
    const float effectRadius = params.radius * params.radiusMultiplier;
    const float screenSpaceRadius = effectRadius / pixelViewSize;
    const float minSampleDistance = 1.3f / max(screenSpaceRadius, 1.0e-6f);

    const float falloffRange = max(params.falloffRange * effectRadius, 1.0e-6f);
    const float falloffFrom = effectRadius * (1.0f - params.falloffRange);
    const float falloffMul = -rcp(falloffRange);
    const float falloffAdd = falloffFrom / falloffRange + 1.0f;
    const float2 localNoise = GTAOSpatioTemporalNoise(pixelCoord, params.noiseIndex);

    float visibility = saturate((10.0f - screenSpaceRadius) / 100.0f) * 0.5f;

    [loop]
    for (uint slice = 0u; slice < sliceCount; ++slice) {
        const float sliceK = (float(slice) + localNoise.x) / float(sliceCount);
        const float phi = sliceK * PI;
        float sinPhi;
        float cosPhi;
        sincos(phi, sinPhi, cosPhi);

        const float2 omega = float2(cosPhi, -sinPhi) * screenSpaceRadius;
        const float3 direction = float3(cosPhi, sinPhi, 0.0f);
        const float3 orthogonalDirection = direction - dot(direction, viewVector) * viewVector;
        const float3 unnormalizedAxis = cross(orthogonalDirection, viewVector);
        const float3 axis = unnormalizedAxis * rsqrt(max(dot(unnormalizedAxis, unnormalizedAxis), 1.0e-12f));
        float3 projectedNormal = viewNormal - axis * dot(viewNormal, axis);
        float projectedNormalLength = max(length(projectedNormal), 1.0e-6f);
        const float signNormal = sign(dot(orthogonalDirection, projectedNormal));
        const float cosNormal = saturate(dot(projectedNormal, viewVector) / projectedNormalLength);
        const float normalAngle = signNormal * GTAOFastAcos(cosNormal);

        const float lowHorizonCos0 = cos(normalAngle + HALF_PI);
        const float lowHorizonCos1 = cos(normalAngle - HALF_PI);
        float horizonCos0 = lowHorizonCos0;
        float horizonCos1 = lowHorizonCos1;

        [loop]
        for (uint step = 0u; step < stepsPerSlice; ++step) {
            const float stepBaseNoise = float(slice + step * stepsPerSlice) * 0.6180339887498948482f;
            const float stepNoise = frac(localNoise.y + stepBaseNoise);
            float sampleScale = (float(step) + stepNoise) / float(stepsPerSlice);
            sampleScale = pow(sampleScale, params.sampleDistributionPower) + minSampleDistance;

            float2 sampleOffsetPixels = sampleScale * omega;
            const float sampleOffsetLength = max(length(sampleOffsetPixels), 1.0e-6f);
            const float maxMipLevel = float(max(params.depthMipCount, 1u) - 1u);
            const float mipLevel = clamp(log2(sampleOffsetLength) - params.depthMipSamplingOffset, 0.0f, maxMipLevel);
            const float2 sampleOffsetUV = round(sampleOffsetPixels) * params.outputSizeRcp;

            const float2 sampleUV0 = uv + sampleOffsetUV;
            const float2 sampleUV1 = uv - sampleOffsetUV;
            const bool valid0 = all(sampleUV0 >= 0.0f) && all(sampleUV0 < 1.0f);
            const bool valid1 = all(sampleUV1 >= 0.0f) && all(sampleUV1 < 1.0f);

            if (valid0) {
                const float sampleZ = prefilteredDepthTexture.SampleLevel(pointClampSampler, sampleUV0, mipLevel);
                const float3 delta = sampleZ > 0.0f ? ReconstructViewPosition(sampleUV0, sampleZ, cameraConstants.uvToViewScaleBiasXY, orthogonal) - centerPosition : 0.0f;
                const float distance = length(delta);
                if (distance > 1.0e-6f) {
                    const float falloffDistance = length(float3(delta.xy, delta.z * (1.0f + params.thinOccluderCompensation)));
                    const float weight = saturate(falloffDistance * falloffMul + falloffAdd);
                    const float sampleHorizonCos = dot(delta / distance, viewVector);
                    horizonCos0 = max(horizonCos0, lerp(lowHorizonCos0, sampleHorizonCos, weight));
                }
            }

            if (valid1) {
                const float sampleZ = prefilteredDepthTexture.SampleLevel(pointClampSampler, sampleUV1, mipLevel);
                const float3 delta = sampleZ > 0.0f ? ReconstructViewPosition(sampleUV1, sampleZ, cameraConstants.uvToViewScaleBiasXY, orthogonal) - centerPosition : 0.0f;
                const float distance = length(delta);
                if (distance > 1.0e-6f) {
                    const float falloffDistance = length(float3(delta.xy, delta.z * (1.0f + params.thinOccluderCompensation)));
                    const float weight = saturate(falloffDistance * falloffMul + falloffAdd);
                    const float sampleHorizonCos = dot(delta / distance, viewVector);
                    horizonCos1 = max(horizonCos1, lerp(lowHorizonCos1, sampleHorizonCos, weight));
                }
            }
        }

        projectedNormalLength = lerp(projectedNormalLength, 1.0f, 0.05f);
        const float horizon0 = -GTAOFastAcos(horizonCos1);
        const float horizon1 = GTAOFastAcos(horizonCos0);
        const float integral0 = (cosNormal + 2.0f * horizon0 * sin(normalAngle) - cos(2.0f * horizon0 - normalAngle)) * 0.25f;
        const float integral1 = (cosNormal + 2.0f * horizon1 * sin(normalAngle) - cos(2.0f * horizon1 - normalAngle)) * 0.25f;
        visibility += projectedNormalLength * (integral0 + integral1);
    }

    visibility = pow(max(visibility / float(sliceCount), 0.0f), params.power);
    visibility = max(0.03f, visibility);
    return saturate(visibility / GTAOWorkingVisibilityScale);
}

void GTAODecodeGather(float4 packedValues, out float values[4]) {
    [unroll]
    for (uint index = 0u; index < 4u; ++index) {
        values[index] = packedValues[index];
    }
}

void GTAOAddDenoiseSample(float value, float edgeWeight, inout float sum, inout float weightSum) {
    sum += edgeWeight * value;
    weightSum += edgeWeight;
}

void GTAODenoisePixelPair(uint2 basePixelCoord, GTAODenoiseParams params, Texture2D<float> sourceAOTexture, Texture2D<float> packedEdgesTexture, SamplerState pointClampSampler, RWTexture2D<float> outputAOTexture) {
    if (any(basePixelCoord >= params.outputSize)) {
        return;
    }

    const float2 gatherCenter = float2(basePixelCoord) * params.outputSizeRcp;
    const float4 edgesQ0 = packedEdgesTexture.GatherRed(pointClampSampler, gatherCenter, int2(0, 0));
    const float4 edgesQ1 = packedEdgesTexture.GatherRed(pointClampSampler, gatherCenter, int2(2, 0));
    const float4 edgesQ2 = packedEdgesTexture.GatherRed(pointClampSampler, gatherCenter, int2(1, 2));

    float aoQ0[4];
    float aoQ1[4];
    float aoQ2[4];
    float aoQ3[4];
    GTAODecodeGather(sourceAOTexture.GatherRed(pointClampSampler, gatherCenter, int2(0, 0)), aoQ0);
    GTAODecodeGather(sourceAOTexture.GatherRed(pointClampSampler, gatherCenter, int2(2, 0)), aoQ1);
    GTAODecodeGather(sourceAOTexture.GatherRed(pointClampSampler, gatherCenter, int2(0, 2)), aoQ2);
    GTAODecodeGather(sourceAOTexture.GatherRed(pointClampSampler, gatherCenter, int2(2, 2)), aoQ3);

    const float blurAmount = params.finalPass != 0u ? params.blurBeta : params.blurBeta / 5.0f;
    const float diagonalWeight = 0.425f;

    [unroll]
    for (uint side = 0u; side < 2u; ++side) {
        const uint2 pixelCoord = basePixelCoord + uint2(side, 0u);
        if (any(pixelCoord >= params.outputSize)) {
            continue;
        }

        const float4 edgesLeft = GTAOUnpackEdges(side == 0u ? edgesQ0.x : edgesQ0.y);
        const float4 edgesTop = GTAOUnpackEdges(side == 0u ? edgesQ0.z : edgesQ1.w);
        const float4 edgesRight = GTAOUnpackEdges(side == 0u ? edgesQ1.x : edgesQ1.y);
        const float4 edgesBottom = GTAOUnpackEdges(side == 0u ? edgesQ2.w : edgesQ2.z);
        float4 centerEdges = GTAOUnpackEdges(side == 0u ? edgesQ0.y : edgesQ1.x);

        // Enforce symmetric edges, then allow a controlled amount of leakage at highly disconnected pixels.
        centerEdges *= float4(edgesLeft.y, edgesRight.x, edgesTop.w, edgesBottom.z);
        const float edgeSum = centerEdges.x + centerEdges.y + centerEdges.z + centerEdges.w;
        const float edgeLeak = saturate(1.5f - edgeSum) / 1.5f * 0.5f;
        centerEdges = saturate(centerEdges + edgeLeak);

        const float weightTL = diagonalWeight * (centerEdges.x * edgesLeft.z + centerEdges.z * edgesTop.x);
        const float weightTR = diagonalWeight * (centerEdges.z * edgesTop.y + centerEdges.y * edgesRight.z);
        const float weightBL = diagonalWeight * (centerEdges.w * edgesBottom.x + centerEdges.x * edgesLeft.w);
        const float weightBR = diagonalWeight * (centerEdges.y * edgesRight.w + centerEdges.w * edgesBottom.y);

        const float center = side == 0u ? aoQ0[1] : aoQ1[0];
        const float left = side == 0u ? aoQ0[0] : aoQ0[1];
        const float top = side == 0u ? aoQ0[2] : aoQ1[3];
        const float right = side == 0u ? aoQ1[0] : aoQ1[1];
        const float bottom = side == 0u ? aoQ2[2] : aoQ3[3];
        const float topLeft = side == 0u ? aoQ0[3] : aoQ0[2];
        const float topRight = side == 0u ? aoQ1[3] : aoQ1[2];
        const float bottomLeft = side == 0u ? aoQ2[3] : aoQ2[2];
        const float bottomRight = side == 0u ? aoQ3[3] : aoQ3[2];

        float weightSum = blurAmount;
        float sum = center * weightSum;
        GTAOAddDenoiseSample(left, centerEdges.x, sum, weightSum);
        GTAOAddDenoiseSample(right, centerEdges.y, sum, weightSum);
        GTAOAddDenoiseSample(top, centerEdges.z, sum, weightSum);
        GTAOAddDenoiseSample(bottom, centerEdges.w, sum, weightSum);
        GTAOAddDenoiseSample(topLeft, weightTL, sum, weightSum);
        GTAOAddDenoiseSample(topRight, weightTR, sum, weightSum);
        GTAOAddDenoiseSample(bottomLeft, weightBL, sum, weightSum);
        GTAOAddDenoiseSample(bottomRight, weightBR, sum, weightSum);

        float result = sum / max(weightSum, 1.0e-6f);
        if (params.finalPass != 0u) {
            result = saturate(result * GTAOWorkingVisibilityScale);
        }
        outputAOTexture[pixelCoord] = result;
    }
}

#endif // __GTAO_COMMON_HLSLI__
