#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0), UAV(u1)), " \
    "CBV(b0)"

cbuffer MSAAResolveDepthNormalCB : register(b0) {
    MSAAResolveDepthParams msaaResolveDepthParams;
}

Texture2DMS<float> inputDepthTexture : register(t0);
Texture2DMS<float2> inputNormalTexture : register(t1);
RWTexture2D<float> outputDepthTexture : register(u0);
RWTexture2D<float2> outputNormalTexture : register(u1);

float LoadResolvedDepth(uint2 pixelCoord, out uint closestSampleIndex) {
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    closestSampleIndex = 0;
    float closestDepth = inputDepthTexture.Load(int2(pixelCoord), 0);

    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(pixelCoord), sampleIndex);
        const bool isCloser = msaaResolveDepthParams.useMinDepth != 0 ? sampleDepth < closestDepth : sampleDepth > closestDepth;
        if (isCloser) {
            closestDepth = sampleDepth;
            closestSampleIndex = sampleIndex;
        }
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

    uint closestSampleIndex;
    outputDepthTexture[pixelCoord] = LoadResolvedDepth(pixelCoord, closestSampleIndex);
    outputNormalTexture[pixelCoord] = inputNormalTexture.Load(int2(pixelCoord), closestSampleIndex);
}
