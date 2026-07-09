#include "ShaderInterop.h"

#ifndef DEPTH_HISTORY_RESOLVE_MSAA
#define DEPTH_HISTORY_RESOLVE_MSAA 0
#endif

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer DepthHistoryResolveCB : register(b0) {
    DepthHistoryResolveParams depthHistoryResolveParams;
}

#if DEPTH_HISTORY_RESOLVE_MSAA
Texture2DMS<float> inputDepthTexture : register(t0);
#else
Texture2D<float> inputDepthTexture : register(t0);
#endif
RWTexture2D<float> outputDepthTexture : register(u0);

float LoadResolvedDepth(uint2 pixelCoord) {
#if DEPTH_HISTORY_RESOLVE_MSAA
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    float depth = inputDepthTexture.Load(int2(pixelCoord), 0);
    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(pixelCoord), sampleIndex);
        depth = depthHistoryResolveParams.useMinDepth != 0 ? min(depth, sampleDepth) : max(depth, sampleDepth);
    }
    return depth;
#else
    return inputDepthTexture.Load(int3(pixelCoord, 0));
#endif
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(DepthHistoryResolveBlockSizeX, DepthHistoryResolveBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= depthHistoryResolveParams.outputSize)) {
        return;
    }

    outputDepthTexture[pixelCoord] = LoadResolvedDepth(pixelCoord);
}
