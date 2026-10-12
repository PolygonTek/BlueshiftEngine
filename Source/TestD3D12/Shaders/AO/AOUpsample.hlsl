#include "../ShaderInterop.h"
#include "../Common.hlsli"

// Full-resolution depth becomes writable again for opaque rendering after AO.
#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2, flags = DATA_VOLATILE), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0), " \
    "CBV(b1)"

cbuffer AOUpsampleCB : register(b0) {
    AOUpsampleParams aoUpsampleParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

Texture2D<float> inputHalfAOTexture : register(t0);
Texture2D<float> inputHalfDepthTexture : register(t1);
Texture2D<float> inputDepthTexture : register(t2);
RWTexture2D<float> outputAOTexture : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(AOUpsampleBlockSizeX, AOUpsampleBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= aoUpsampleParams.outputSize)) {
        return;
    }

    const float depth = inputDepthTexture.Load(int3(pixelCoord, 0));
    if (depth == aoUpsampleParams.farPlaneDeviceDepth) {
        outputAOTexture[pixelCoord] = 1.0f;
        return;
    }

    const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;
    const float linearDepth = ToLinearDepth(depth, cameraConstants.deviceDepthToNdcZScaleBias, cameraConstants.projectionScaleBiasZ, orthogonal);

    // 2x2 블록 중심 기준으로 보간한다. 홀수 해상도에서도 정확히 2:1 좌표 대응을 유지한다.
    const float2 halfCoord = (float2(pixelCoord) + 0.5f) * 0.5f - 0.5f;
    const int2 baseCoord = int2(floor(halfCoord));
    const float2 blend = frac(halfCoord);
    float valueSum = 0.0f;
    float weightSum = 0.0f;

    [unroll]
    for (uint y = 0; y < 2; ++y) {
        [unroll]
        for (uint x = 0; x < 2; ++x) {
            const int2 sampleCoord = clamp(baseCoord + int2(x, y), int2(0, 0), int2(aoUpsampleParams.inputSize) - 1);
            const float sampleDepth = inputHalfDepthTexture.Load(int3(sampleCoord, 0));

            if (sampleDepth != aoUpsampleParams.farPlaneDeviceDepth) {
                const float sampleLinearDepth = ToLinearDepth(sampleDepth, cameraConstants.deviceDepthToNdcZScaleBias, cameraConstants.projectionScaleBiasZ, orthogonal);
                const float depthError = (sampleLinearDepth - linearDepth) * aoUpsampleParams.depthSigmaRcp;
                const float spatialWeight = (x == 0 ? 1.0f - blend.x : blend.x) * (y == 0 ? 1.0f - blend.y : blend.y);
                const float weight = spatialWeight * exp2(-0.721347520f * depthError * depthError);

                valueSum += weight * inputHalfAOTexture.Load(int3(sampleCoord, 0));
                weightSum += weight;
            }
        }
    }

    // 주변 half-resolution texel 에 같은 표면이 없으면 다른 표면의 AO 를 가져오지 않는다.
    outputAOTexture[pixelCoord] = weightSum > 1.0e-5f ? saturate(valueSum / weightSum) : 1.0f;
}
