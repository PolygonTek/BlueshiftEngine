#include "ShaderInterop.h"

#ifndef RESOLVE_MSAA_NORMAL
#define RESOLVE_MSAA_NORMAL 0
#endif

#if RESOLVE_MSAA_NORMAL
#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0), UAV(u1)), " \
    "CBV(b0)"
#else
#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"
#endif

cbuffer DepthMSAAResolveCB : register(b0) {
    DepthMSAAResolveParams depthMSAAResolveParams;
}

Texture2DMS<float> inputDepthTexture : register(t0);
RWTexture2D<float> outputDepthTexture : register(u0);
#if RESOLVE_MSAA_NORMAL
Texture2DMS<float2> inputNormalTexture : register(t1);
RWTexture2D<float2> outputNormalTexture : register(u1);
#endif

#if RESOLVE_MSAA_NORMAL
float LoadResolvedDepth(uint2 pixelCoord, out uint closestSampleIndex) {
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    closestSampleIndex = 0;
    float closestDepth = inputDepthTexture.Load(int2(pixelCoord), 0);

    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(pixelCoord), sampleIndex);
        const bool isCloser = depthMSAAResolveParams.useMinDepth != 0 ? sampleDepth < closestDepth : sampleDepth > closestDepth;
        if (isCloser) {
            closestDepth = sampleDepth;
            closestSampleIndex = sampleIndex;
        }
    }
    return closestDepth;
}
#else
float LoadResolvedDepth(uint2 pixelCoord) {
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    float closestDepth = inputDepthTexture.Load(int2(pixelCoord), 0);

    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(pixelCoord), sampleIndex);
        closestDepth = depthMSAAResolveParams.useMinDepth != 0 ? min(closestDepth, sampleDepth) : max(closestDepth, sampleDepth);
    }
    return closestDepth;
}
#endif

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(DepthMSAAResolveBlockSizeX, DepthMSAAResolveBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= depthMSAAResolveParams.outputSize)) {
        return;
    }

#if RESOLVE_MSAA_NORMAL
    uint closestSampleIndex;
    outputDepthTexture[pixelCoord] = LoadResolvedDepth(pixelCoord, closestSampleIndex);
    outputNormalTexture[pixelCoord] = inputNormalTexture.Load(int2(pixelCoord), closestSampleIndex);
#else
    outputDepthTexture[pixelCoord] = LoadResolvedDepth(pixelCoord);
#endif
}
