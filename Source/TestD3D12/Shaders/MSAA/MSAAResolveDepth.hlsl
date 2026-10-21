#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer MSAAResolveDepthCB : register(b0) {
    MSAAResolveDepthParams msaaResolveDepthParams;
}

Texture2DMS<float> msDepthTexture : register(t0);
RWTexture2D<float> resolvedDepthTexture : register(u0);

// 서브 샘플 중 가장 가까운 깊이 값을 가져온다.
float LoadResolvedDepth(uint2 pixelCoord) {
    uint width;
    uint height;
    uint sampleCount;
    msDepthTexture.GetDimensions(width, height, sampleCount);

    float closestDepth = msDepthTexture.Load(int2(pixelCoord), 0);

    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = msDepthTexture.Load(int2(pixelCoord), sampleIndex);
        closestDepth = msaaResolveDepthParams.reverseZ != 0 ? max(closestDepth, sampleDepth) : min(closestDepth, sampleDepth);
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

    resolvedDepthTexture[pixelCoord] = LoadResolvedDepth(pixelCoord);
}
