#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../Octahedral.hlsli"
#include "SSRDepth.hlsli"
#include "SSRRadiance.hlsli"
#include "SSRResolution.hlsli"

#define COMMON_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0, flags = DATA_VOLATILE), SRV(t1), SRV(t2), SRV(t3), UAV(u0)), " \
    "RootConstants(num32BitConstants=3, b0), " \
    "CBV(b1), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer SSRUpsampleCB : register(b0) {
    SSRUpsampleParams ssrUpsampleParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

Texture2D<float> inputDepthTexture : register(t0);
Texture2D<float4> inputNormalRoughnessTexture : register(t1); // RG: UNORM octahedral view-space shading normal, B: perceptual roughness.
Texture2D<float4> inputReflectionTexture : register(t2); // RGB: 압축된 confidence-premultiplied radiance, A: confidence.
Texture2D<float4> inputReceiverSurfaceTexture : register(t3); // B: normalized block pixel index, A: receiver perceptual roughness.
RWTexture2D<float4> outputReflectionTexture : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSRUpsampleBlockSizeX, SSRUpsampleBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= ssrUpsampleParams.outputSize)) {
        return;
    }

#if SSR_RESOLUTION_DIVISOR == 1
    // inputReflectionTexture 가 이미 full 해상도이므로 업샘플링이 필요 없다.
    const float4 reflection = inputReflectionTexture.Load(int3(pixelCoord, 0));
    outputReflectionTexture[pixelCoord] = ssrUpsampleParams.debugMode == SSRDebugModeUpsampledReflection ? float4(DecompressSSRRadiance(reflection.rgb), 1.0f) : reflection;
#else
    const float depth = inputDepthTexture.Load(int3(pixelCoord, 0));
    const float clearDepth = cameraConstants.depthClearValue;
    if (depth == clearDepth) {
        outputReflectionTexture[pixelCoord] = 0.0f;
        return;
    }

    const float linearDepth = cameraConstants.GetLinearDepth(depth);
    const float2 depthGradient = ComputeSSRDepthGradient(inputDepthTexture, ssrUpsampleParams.outputSize, pixelCoord, depth, clearDepth);
    const float4 normalRoughness = inputNormalRoughnessTexture.Load(int3(pixelCoord, 0));
    const float3 viewNormal = DecodeUnitVectorOctahedralUNorm(normalRoughness.xy);
    const float roughness = normalRoughness.z;

    uint traceWidth;
    uint traceHeight;
    inputReflectionTexture.GetDimensions(traceWidth, traceHeight);
    const int2 traceSize = int2(traceWidth, traceHeight);

    const float2 traceCoord = (float2(pixelCoord) + 0.5f) * (1.0f / float(SSR_RESOLUTION_DIVISOR)) - 0.5f;
    const int2 baseCoord = int2(floor(traceCoord));
    const float2 blend = frac(traceCoord);
    const float depthSigma = max(linearDepth * 0.01f, 0.01f);
    const float roughnessSigma = 0.1f;
    const bool debugUpsampledReflection = ssrUpsampleParams.debugMode == SSRDebugModeUpsampledReflection;
    float3 premulRadianceSum = 0.0f;
    float confidenceSum = 0.0f;
    float weightSum = 0.0f;
    float baseWeightSum = 0.0f;

    [unroll]
    for (uint y = 0; y < 2; ++y) {
        [unroll]
        for (uint x = 0; x < 2; ++x) {
            // 주변 2x2 block 의 각 receiver 표면에 대해, 실제 trace 로 선택한 좌표를 얻는다.
            const int2 sampleCoord = clamp(baseCoord + int2(x, y), int2(0, 0), traceSize - 1);
            const float4 receiverSurface = inputReceiverSurfaceTexture.Load(int3(sampleCoord, 0));
            const uint2 blockPixelOffset = DecodeSSRBlockPixelOffset(receiverSurface.b);
            const uint2 receiverPixelCoord = min(uint2(sampleCoord) * SSR_RESOLUTION_DIVISOR + blockPixelOffset, ssrUpsampleParams.outputSize - 1u);

            // 현재 xy block 의 receiver 표면 depth 를 얻는다.
            const float receiverDepth = inputDepthTexture.Load(int3(receiverPixelCoord, 0));
            if (receiverDepth == clearDepth) {
                continue;
            }

            const float receiverLinearDepth = cameraConstants.GetLinearDepth(receiverDepth);
            if (receiverLinearDepth <= 0.0f || !isfinite(receiverLinearDepth)) {
                continue;
            }

            // pixelCoord 가 일정한 기울기로 receiverPixelCoord 까지 이어진다면 예상되는 depth 를 구한다.
            const float2 receiverPixelDelta = float2(receiverPixelCoord) - float2(pixelCoord);
            const float expectedDepth = depth + dot(depthGradient, receiverPixelDelta);
            if (expectedDepth < 0.0f || expectedDepth > 1.0f || expectedDepth == clearDepth) {
                continue;
            }

            const float expectedLinearDepth = cameraConstants.GetLinearDepth(expectedDepth);
            if (expectedLinearDepth <= 0.0f || !isfinite(expectedLinearDepth)) {
                continue;
            }

            // 주변 block 의 depth, roughness, normal 가 원본 픽셀의 것과 얼마나 비슷한지 계산하여 reflection 의 가중 평균을 낸다.
            const float depthError = (receiverLinearDepth - expectedLinearDepth) / depthSigma;
            const float roughnessError = (receiverSurface.a - roughness) / roughnessSigma;
            const float spatialWeight = (x == 0 ? 1.0f - blend.x : blend.x) * (y == 0 ? 1.0f - blend.y : blend.y);
            const float bilateralError = depthError * depthError + roughnessError * roughnessError;
            const float baseWeight = spatialWeight * exp(-0.5f * bilateralError);

            const float3 receiverViewNormal = DecodeUnitVectorOctahedralUNorm(inputNormalRoughnessTexture.Load(int3(receiverPixelCoord, 0)).xy);
            const float normalSimilarity = saturate(dot(viewNormal, receiverViewNormal));
            const float normalWeight = lerp(1.0f / 16.0f, 1.0f, normalSimilarity * normalSimilarity);
            const float weight = baseWeight * normalWeight;

            const float4 reflection = inputReflectionTexture.Load(int3(sampleCoord, 0));
            const float3 premulRadiance = DecompressSSRRadiance(reflection.rgb);
            const float confidence = saturate(reflection.a);

            premulRadianceSum += premulRadiance * weight;
            confidenceSum += confidence * weight;
            weightSum += weight;
            baseWeightSum += baseWeight;
        }
    }

    // normal 감쇠로 기존에 유효했던 표면이 탈락하지 않도록 원래 가중치 합으로 판정한다.
    if (baseWeightSum <= 1.0e-6f) {
        outputReflectionTexture[pixelCoord] = 0.0f;
        return;
    }

    const float invWeightSum = rcp(weightSum);
    const float3 premulRadiance = premulRadianceSum * invWeightSum;
    const float confidence = confidenceSum * invWeightSum;

    outputReflectionTexture[pixelCoord] = debugUpsampledReflection ? float4(premulRadiance, 1.0f) : float4(CompressSSRRadiance(premulRadiance), confidence);
#endif
}
