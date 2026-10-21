#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "SSRRadiance.hlsli"

#define COMMON_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), UAV(u0)), " \
    "RootConstants(num32BitConstants=4, b0), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer SSRRadianceResolveCB : register(b0) {
    SSRRadianceResolveParams ssrRadianceResolveParams;
}

Texture2D<float4> inputRadianceTexture : register(t0); // Previous-frame full-resolution HDR radiance; logical mip 0.
Texture2D<float4> inputRadiancePyramidTexture : register(t1); // Previous-frame physical mip 0 is logical scene-color mip 1.
Texture2D<float4> inputTraceTexture : register(t2);
Texture2D<float4> inputReceiverSurfaceTexture : register(t3); // RG: FP16 screen-space ray length in pixels (low/high bytes), B: normalized block pixel index, A: perceptual roughness.
RWTexture2D<float4> outputReflectionTexture : register(u0); // Trace 해상도. 일반 출력: RGB = Compress(radiance * confidence), A = confidence.

float4 GetTraceStatusDebugColor(float validity) {
    if (validity > SSRTraceValidityMiss) {
        return float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    if (validity <= SSRTraceValidityNoSurface) {
        return 0.0f;
    }
    if (validity == SSRTraceValidityHistoryRejected) {
        return float4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    if (validity == SSRTraceValidityConfidenceRejected) {
        return float4(1.0f, 0.0f, 1.0f, 1.0f);
    }
    if (validity <= SSRTraceValidityMaxIterations) {
        return float4(0.0f, 0.0f, 1.0f, 1.0f);
    }
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

float3 GetIterationDebugColor(float iterationRatio) {
    const float ratio = saturate(iterationRatio);
    if (ratio < (1.0f / 3.0f)) {
        return lerp(float3(0.02f, 0.05f, 0.4f), float3(0.0f, 0.8f, 1.0f), ratio * 3.0f);
    }
    if (ratio < (2.0f / 3.0f)) {
        return lerp(float3(0.0f, 0.8f, 1.0f), float3(1.0f, 0.9f, 0.0f), ratio * 3.0f - 1.0f);
    }
    return lerp(float3(1.0f, 0.9f, 0.0f), float3(1.0f, 0.0f, 0.0f), ratio * 3.0f - 2.0f);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSRRadianceResolveBlockSizeX, SSRRadianceResolveBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= ssrRadianceResolveParams.outputSize)) {
        return;
    }

    const float4 traceRecord = inputTraceTexture.Load(int3(pixelCoord, 0));
    if (ssrRadianceResolveParams.debugMode == SSRDebugModeTraceStatus) {
        outputReflectionTexture[pixelCoord] = GetTraceStatusDebugColor(traceRecord.a);
        return;
    }
    
    if (ssrRadianceResolveParams.debugMode == SSRDebugModeIterations) {
        const bool tracedSurface = traceRecord.a > SSRTraceValidityNoSurface && traceRecord.b > 0.0f;
        outputReflectionTexture[pixelCoord] = tracedSurface ? float4(GetIterationDebugColor(traceRecord.b), 1.0f) : 0.0f;
        return;
    }

    const bool validHit = traceRecord.a > 0.0f;
    if (ssrRadianceResolveParams.debugMode == SSRDebugModeHitUV) {
        outputReflectionTexture[pixelCoord] = validHit ? float4(traceRecord.xy, 0.0f, 1.0f) : 0.0f;
        return;
    }
    
    if (ssrRadianceResolveParams.debugMode == SSRDebugModeConfidence) {
        const float confidence = max(traceRecord.a, 0.0f);
        outputReflectionTexture[pixelCoord] = float4(confidence.xxx, 1.0f);
        return;
    }

    if (!validHit) {
        outputReflectionTexture[pixelCoord] = 0.0f;
        return;
    }

    if (ssrRadianceResolveParams.debugMode == SSRDebugModeRawReflection) {
        const float3 reflectedRadiance = inputRadianceTexture.SampleLevel(linearClampSampler, traceRecord.xy, 0.0f).rgb;
        outputReflectionTexture[pixelCoord] = float4(reflectedRadiance, traceRecord.a);
        return;
    }

    const float4 receiverSurface = inputReceiverSurfaceTexture.Load(int3(pixelCoord, 0));

    uint pyramidWidth;
    uint pyramidHeight;
    uint pyramidMipCount;
    inputRadiancePyramidTexture.GetDimensions(0, pyramidWidth, pyramidHeight, pyramidMipCount);

    // trace 에서 재투영 전 hit UV 로 계산한 현재 프레임 ray 길이를 RG8 UNORM 의 FP16 비트에서 복원한다.
    const float rayLengthPixels = UnpackRG8UNormToFP16(receiverSurface.rg);

    // roughness cone 내부에 끝점이 닿는 구를 맞춰 hit 너머의 radiance 가 과도하게 섞이지 않게 한다.
    // 단순 box mip pyramid 의 footprint 에 맞춰 screen-space cone radius 를 경험적으로 보정한다.
    // Material roughness is perceptual; use the GGX linear roughness for the cone angle so nearly smooth surfaces do not over-blur.
    // Preserve the approximate total GGX width while moving variance between ray jitter and cone filtering.
    const float linearRoughness = receiverSurface.a * receiverSurface.a;
    const float filterLinearRoughness = linearRoughness * ssrRadianceResolveParams.coneRoughnessScale;
    const float coneTangent = tan(min(filterLinearRoughness, 0.999f) * HALF_PI);
    const float coneRadius = rayLengthPixels * coneTangent / (sqrt(coneTangent * coneTangent + 1.0f) + coneTangent);
    const float maxRadianceMipLevel = float(pyramidMipCount);
    const float radianceMipLevel = min(log2(coneRadius * (1.0f / 16.0f) + 1.0f), maxRadianceMipLevel);

    float3 compressedRadiance;
    if (radianceMipLevel < 1.0f) {
        const float3 compressedMip0 = CompressSSRRadiance(inputRadianceTexture.SampleLevel(linearClampSampler, traceRecord.xy, 0.0f).rgb);
        const float3 compressedMip1 = inputRadiancePyramidTexture.SampleLevel(linearClampSampler, traceRecord.xy, 0.0f).rgb;
        compressedRadiance = lerp(compressedMip0, compressedMip1, radianceMipLevel);
    } else {
        compressedRadiance = inputRadiancePyramidTexture.SampleLevel(linearClampSampler, traceRecord.xy, radianceMipLevel - 1.0f).rgb;
    }

    const float3 reflectedRadiance = DecompressSSRRadiance(compressedRadiance);
    const float3 premulRadiance = reflectedRadiance * traceRecord.a;

    // bilateral upsample 중 HDR 밝기 차이가 weight 와 분리되도록 premultiplied radiance 를 가역 압축한다.
    outputReflectionTexture[pixelCoord] = float4(CompressSSRRadiance(premulRadiance), traceRecord.a);
}
