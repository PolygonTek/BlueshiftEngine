#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer MSAAResolveDepthCB : register(b0) {
    MSAAResolveDepthParams msaaResolveDepthParams;
}

Texture2DMS<float> inputDepthTexture : register(t0);
RWTexture2D<float> outputDepthTexture : register(u0);

float LoadResolvedDepth(uint2 pixelCoord) {
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    float closestDepth = inputDepthTexture.Load(int2(pixelCoord), 0);

    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(pixelCoord), sampleIndex);
        closestDepth = msaaResolveDepthParams.useMinDepth != 0 ? min(closestDepth, sampleDepth) : max(closestDepth, sampleDepth);
    }
    return closestDepth;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(MSAAResolveDepthBlockSizeX, MSAAResolveDepthBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= msaaResolveDepthParams.outputSize)) {
        return;
    }

    outputDepthTexture[pixelCoord] = LoadResolvedDepth(pixelCoord);
}
