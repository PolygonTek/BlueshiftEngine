#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "CBV(b0)"

cbuffer MSAAResolveVelocityCB : register(b0) {
    MSAAResolveVelocityParams msaaResolveVelocityParams;
}

Texture2DMS<float> msDepthTexture : register(t0);
Texture2DMS<float2> msVelocityTexture : register(t1);
RWTexture2D<float2> resolvedVelocityTexture : register(u0);

// 서브 샘플 중 가장 가까운 깊이 값을 갖는 인덱스를 가져온다.
uint FindClosestSampleIndex(uint2 pixelCoord) {
    uint width;
    uint height;
    uint sampleCount;
    msDepthTexture.GetDimensions(width, height, sampleCount);

    uint closestSampleIndex = 0;
    float closestDepth = msDepthTexture.Load(int2(pixelCoord), 0);

    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = msDepthTexture.Load(int2(pixelCoord), sampleIndex);
        const bool isCloser = msaaResolveVelocityParams.reverseZ != 0 ? sampleDepth > closestDepth : sampleDepth < closestDepth;
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
    resolvedVelocityTexture[pixelCoord] = msVelocityTexture.Load(int2(pixelCoord), sampleIndex);
}
