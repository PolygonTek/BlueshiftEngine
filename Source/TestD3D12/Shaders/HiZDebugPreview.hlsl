#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer HiZDebugPreviewCB : register(b0) {
    HiZDebugPreviewParams hiZDebugPreviewParams;
}

Texture2D<float> inputHiZTexture : register(t0);
RWTexture2D<float4> outputPreviewTexture : register(u0);

float SafeSignedDenominator(float value) {
    const float minAbsValue = 1.0e-6;
    if (abs(value) >= minAbsValue) {
        return value;
    }
    return value < 0.0 ? -minAbsValue : minAbsValue;
}

float LinearizeDepth(float deviceDepth) {
    const float ndcDepth = deviceDepth * hiZDebugPreviewParams.ndcDepthScaleBias.x + hiZDebugPreviewParams.ndcDepthScaleBias.y;
    const float projDepthA = hiZDebugPreviewParams.projectionDepthParams.x; // projection matrix [2][2]
    const float projDepthB = hiZDebugPreviewParams.projectionDepthParams.y; // projection matrix [2][3]

    if (hiZDebugPreviewParams.orthogonal != 0) {
        // ndcZ = (A * viewZ + B) / 1
        // viewZ = (ndcZ - B) / A
        // projection matrix 는 오른손 좌표계이므로 음수 방향이 카메라 앞쪽이다. 그러므로 preview 를 위해서 뒤집는다.
        // viewDistance = -viewZ = (B - ndcZ) / A
        return (projDepthB - ndcDepth) / SafeSignedDenominator(projDepthA);
    }

    // ndcZ = (A * viewZ + B) / (-viewZ)
    // viewZ = -B / (ndcZ + A)
    // projection matrix 는 오른손 좌표계이므로 음수 방향이 카메라 앞쪽이다. 그러므로 preview 를 위해서 뒤집는다.
    // viewDistance = -viewZ = B / (ndcZ + A)
    return projDepthB / SafeSignedDenominator(ndcDepth + projDepthA);
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
    const float linearDepth = max(LinearizeDepth(deviceDepth), 0.0);
    const float preview = saturate(linearDepth * hiZDebugPreviewParams.linearDepthScale);

    outputPreviewTexture[outputCoord] = float4(preview, preview, preview, 1.0);
}
