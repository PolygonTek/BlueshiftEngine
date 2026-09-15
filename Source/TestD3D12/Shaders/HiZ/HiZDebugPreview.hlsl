#include "../ShaderInterop.h"
#include "../Common.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "CBV(b1)"

cbuffer HiZDebugPreviewCB : register(b0) {
    HiZDebugPreviewParams hiZDebugPreviewParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

Texture2D<float> inputHiZTexture : register(t0);
RWTexture2D<float4> outputPreviewTexture : register(u0);

float LinearizeDepth(float deviceDepth) {
    const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;
    return ToLinearDepth(deviceDepth, cameraConstants.ndcDepthScaleBias, cameraConstants.projectionZ, orthogonal);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(HiZDebugPreviewBlockSizeX, HiZDebugPreviewBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= hiZDebugPreviewParams.outputSize)) {
        return;
    }

    const uint2 inputCoord = min((outputCoord * hiZDebugPreviewParams.inputSize) / hiZDebugPreviewParams.outputSize, hiZDebugPreviewParams.inputSize - 1);
    const float deviceDepth = inputHiZTexture.Load(int3(inputCoord, 0));
    const float linearDepth = LinearizeDepth(deviceDepth);
    // 무한 원거리 투영의 배경은 흰색으로 표시하고, 무한대와 0 의 곱을 피한다.
    const float preview = isfinite(linearDepth) ? saturate(max(linearDepth, 0.0f) * hiZDebugPreviewParams.linearDepthScale) : 1.0f;

    outputPreviewTexture[outputCoord] = float4(preview, preview, preview, 1.0);
}
