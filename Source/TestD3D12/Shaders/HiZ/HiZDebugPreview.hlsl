#include "../ShaderInterop.h"
#include "../Common.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer HiZDebugPreviewCB : register(b0) {
    HiZDebugPreviewParams hiZDebugPreviewParams;
}

Texture2D<float> inputHiZTexture : register(t0);
RWTexture2D<float4> outputPreviewTexture : register(u0);

float LinearizeDepth(float deviceDepth) {
    return DeviceDepthToLinearDepth(deviceDepth, hiZDebugPreviewParams.ndcDepthScaleBias, hiZDebugPreviewParams.projectionDepthParams, hiZDebugPreviewParams.orthogonal != 0);
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
