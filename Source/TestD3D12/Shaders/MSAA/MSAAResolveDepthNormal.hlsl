#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0), UAV(u1)), " \
    "CBV(b0)"

cbuffer MSAAResolveDepthNormalCB : register(b0) {
    MSAAResolveDepthParams msaaResolveDepthParams;
}

Texture2DMS<float> msDepthTexture : register(t0);
Texture2DMS<float4> msNormalRoughnessTexture : register(t1);
RWTexture2D<float> resolvedDepthTexture : register(u0);
RWTexture2D<float4> resolvedNormalRoughnessTexture : register(u1);

// 서브 샘플 중 가장 가까운 깊이 값과 그 서브 샘플 인덱스를 가져온다.
float LoadResolvedDepth(uint2 pixelCoord, out uint closestSampleIndex) {
    uint width;
    uint height;
    uint sampleCount;
    msDepthTexture.GetDimensions(width, height, sampleCount);

    closestSampleIndex = 0;
    float closestDepth = msDepthTexture.Load(int2(pixelCoord), 0);

    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = msDepthTexture.Load(int2(pixelCoord), sampleIndex);
        const bool isCloser = msaaResolveDepthParams.reverseZ != 0 ? sampleDepth > closestDepth : sampleDepth < closestDepth;
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
    resolvedDepthTexture[pixelCoord] = LoadResolvedDepth(pixelCoord, closestSampleIndex);
    resolvedNormalRoughnessTexture[pixelCoord] = msNormalRoughnessTexture.Load(int2(pixelCoord), closestSampleIndex);
}
