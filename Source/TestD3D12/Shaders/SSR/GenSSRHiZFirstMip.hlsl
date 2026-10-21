#include "../ShaderInterop.h"
#include "SSRHiZCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0)"

cbuffer SSRHiZMipCB : register(b0) {
    SSRHiZMipParams ssrHiZMipParams;
}

Texture2D<float> inputDepthTexture : register(t0);

// R: max depth, G: min depth
RWTexture2D<float2> outputSSRHiZMipTexture : register(u0);

float2 LoadDepthExtrema(uint2 inputCoord) {
    const float depth = inputDepthTexture.Load(int3(inputCoord, 0));
    return EncodeDepthExtrema(depth, ssrHiZMipParams.clearDepth);
}

// 원본 depth 의 2x2 cell 을 하나의 depth extrema (max/min) 로 합친다.
float2 ReduceDepthExtrema(uint2 outputCoord) {
    const uint2 inputCoordMin = outputCoord * 2;
    const uint2 inputCoordMax = min(inputCoordMin + 1, ssrHiZMipParams.inputSize - 1);

    float2 extrema = LoadDepthExtrema(inputCoordMin);
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(uint2(inputCoordMax.x, inputCoordMin.y)));
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(uint2(inputCoordMin.x, inputCoordMax.y)));
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(inputCoordMax));
    return extrema;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSRHiZBlockSizeX, SSRHiZBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= ssrHiZMipParams.outputSize)) {
        return;
    }

    // 유효 영역 밖의 padding cell 은 상위 mip reduction 에 영향을 주지 않도록 EmptyDepthExtrema 로 채운다.
    if (any(outputCoord >= ssrHiZMipParams.validOutputSize)) {
        outputSSRHiZMipTexture[outputCoord] = EmptyDepthExtrema;
        return;
    }

    outputSSRHiZMipTexture[outputCoord] = ReduceDepthExtrema(outputCoord);
}
