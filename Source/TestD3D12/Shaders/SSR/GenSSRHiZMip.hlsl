#include "../ShaderInterop.h"
#include "SSRHiZCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0)"

cbuffer SSRHiZMipCB : register(b0) {
    SSRHiZMipParams ssrHiZMipParams;
}

Texture2D<float2> inputSSRHiZMipTexture : register(t0);

// R: max depth, G: min depth
RWTexture2D<float2> outputSSRHiZMipTexture : register(u0);

// 하위 mip 의 2x2 cell 을 하나의 depth extrema (max/min) 로 합친다.
float2 ReduceDepthExtrema(uint2 outputCoord) {
    const uint2 inputCoordMin = outputCoord * 2;
    const uint2 inputCoordMax = min(inputCoordMin + 1, ssrHiZMipParams.inputSize - 1);

    float2 extrema = inputSSRHiZMipTexture.Load(int3(inputCoordMin, 0));
    extrema = MergeDepthExtrema(extrema, inputSSRHiZMipTexture.Load(int3(uint2(inputCoordMax.x, inputCoordMin.y), 0)));
    extrema = MergeDepthExtrema(extrema, inputSSRHiZMipTexture.Load(int3(uint2(inputCoordMin.x, inputCoordMax.y), 0)));
    extrema = MergeDepthExtrema(extrema, inputSSRHiZMipTexture.Load(int3(inputCoordMax, 0)));
    return extrema;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSRHiZBlockSizeX, SSRHiZBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= ssrHiZMipParams.outputSize)) {
        return;
    }

    outputSSRHiZMipTexture[outputCoord] = ReduceDepthExtrema(outputCoord);
}
