#include "../ShaderInterop.h"
#include "GTAOCommon.hlsli"

#define GTAO_DEBUG_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0)"

cbuffer GTAODebugCB : register(b0) {
    GTAODebugParams gtaoDebugParams;
}

Texture2D<float> inputTexture : register(t0);
RWTexture2D<float> outputTexture : register(u0);

[RootSignature(GTAO_DEBUG_ROOT_SIGNATURE)]
[numthreads(GTAOMainBlockSizeX, GTAOMainBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= gtaoDebugParams.outputSize)) {
        return;
    }

    const uint2 inputCoord = min(outputCoord * gtaoDebugParams.inputSize / gtaoDebugParams.outputSize, gtaoDebugParams.inputSize - 1u);
    const float value = inputTexture.Load(int3(inputCoord, 0));

    float preview = value * GTAOWorkingVisibilityScale;
    if (gtaoDebugParams.sourceType == 1u) {
        const float4 edges = GTAOUnpackEdges(value);
        preview = dot(edges, 0.25f);
    } else if (gtaoDebugParams.sourceType == 2u) {
        preview = value > 0.0f ? 1.0f - exp2(-value * gtaoDebugParams.linearDepthScale) : 0.0f;
    }
    outputTexture[outputCoord] = saturate(preview);
}
