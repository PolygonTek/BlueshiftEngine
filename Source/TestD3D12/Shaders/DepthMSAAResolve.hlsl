#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer DepthMSAAResolveCB : register(b0) {
    DepthMSAAResolveParams depthMSAAResolveParams;
}

Texture2DMS<float> inputDepthTexture : register(t0);
RWTexture2D<float> outputDepthTexture : register(u0);

float LoadResolvedDepth(uint2 pixelCoord) {
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    float depth = inputDepthTexture.Load(int2(pixelCoord), 0);
    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(pixelCoord), sampleIndex);
        depth = depthMSAAResolveParams.useMinDepth != 0 ? min(depth, sampleDepth) : max(depth, sampleDepth);
    }
    return depth;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(DepthMSAAResolveBlockSizeX, DepthMSAAResolveBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= depthMSAAResolveParams.outputSize)) {
        return;
    }

    outputDepthTexture[pixelCoord] = LoadResolvedDepth(pixelCoord);
}
