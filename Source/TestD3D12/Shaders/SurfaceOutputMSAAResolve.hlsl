#include "ShaderInterop.h"

#ifndef SURFACE_OUTPUT_MSAA_RESOLVE_MODE
#error SURFACE_OUTPUT_MSAA_RESOLVE_MODE must be defined
#endif

#if SURFACE_OUTPUT_MSAA_RESOLVE_MODE == 1 || SURFACE_OUTPUT_MSAA_RESOLVE_MODE == 3
#define RESOLVE_NORMAL 1
#else
#define RESOLVE_NORMAL 0
#endif

#if SURFACE_OUTPUT_MSAA_RESOLVE_MODE == 2 || SURFACE_OUTPUT_MSAA_RESOLVE_MODE == 3
#define RESOLVE_VELOCITY 1
#else
#define RESOLVE_VELOCITY 0
#endif

#if RESOLVE_NORMAL && RESOLVE_VELOCITY
#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), UAV(u0), UAV(u1)), " \
    "CBV(b0)"
#else
#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "CBV(b0)"
#endif

cbuffer SurfaceOutputMSAAResolveCB : register(b0) {
    SurfaceOutputMSAAResolveParams surfaceOutputMSAAResolveParams;
}

Texture2DMS<float> inputDepthTexture : register(t0);

#if RESOLVE_NORMAL
Texture2DMS<float2> inputNormalTexture : register(t1);
RWTexture2D<float2> outputNormalTexture : register(u0);
#endif

#if RESOLVE_VELOCITY
    #if RESOLVE_NORMAL
Texture2DMS<float2> inputVelocityTexture : register(t2);
RWTexture2D<float2> outputVelocityTexture : register(u1);
    #else
Texture2DMS<float2> inputVelocityTexture : register(t1);
RWTexture2D<float2> outputVelocityTexture : register(u0);
    #endif
#endif

uint FindClosestSampleIndex(uint2 pixelCoord) {
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    uint closestSampleIndex = 0;
    float closestDepth = inputDepthTexture.Load(int2(pixelCoord), 0);
    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(pixelCoord), sampleIndex);
        const bool isCloser = surfaceOutputMSAAResolveParams.useMinDepth != 0 ? sampleDepth < closestDepth : sampleDepth > closestDepth;
        if (isCloser) {
            closestDepth = sampleDepth;
            closestSampleIndex = sampleIndex;
        }
    }
    return closestSampleIndex;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SurfaceOutputMSAAResolveBlockSizeX, SurfaceOutputMSAAResolveBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= surfaceOutputMSAAResolveParams.outputSize)) {
        return;
    }

    const uint sampleIndex = FindClosestSampleIndex(pixelCoord);
#if RESOLVE_NORMAL
    outputNormalTexture[pixelCoord] = inputNormalTexture.Load(int2(pixelCoord), sampleIndex);
#endif
#if RESOLVE_VELOCITY
    outputVelocityTexture[pixelCoord] = inputVelocityTexture.Load(int2(pixelCoord), sampleIndex);
#endif
}
