#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "CBV(b0)"

cbuffer MSAAResolveVelocityCB : register(b0) {
    MSAAResolveVelocityParams msaaResolveVelocityParams;
}

Texture2DMS<float> inputDepthTexture : register(t0);
Texture2DMS<float2> inputVelocityTexture : register(t1);
RWTexture2D<float2> outputVelocityTexture : register(u0);

uint FindClosestSampleIndex(uint2 pixelCoord) {
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    uint closestSampleIndex = 0;
    float closestDepth = inputDepthTexture.Load(int2(pixelCoord), 0);

    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(pixelCoord), sampleIndex);
        const bool isCloser = msaaResolveVelocityParams.useMinDepth != 0 ? sampleDepth < closestDepth : sampleDepth > closestDepth;
        if (isCloser) {
            closestDepth = sampleDepth;
            closestSampleIndex = sampleIndex;
        }
    }
    return closestSampleIndex;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(MSAAResolveVelocityBlockSizeX, MSAAResolveVelocityBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= msaaResolveVelocityParams.outputSize)) {
        return;
    }

    const uint sampleIndex = FindClosestSampleIndex(pixelCoord);
    outputVelocityTexture[pixelCoord] = inputVelocityTexture.Load(int2(pixelCoord), sampleIndex);
}
