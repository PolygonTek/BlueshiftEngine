#include "../ShaderInterop.h"
#include "SSRHiZCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), UAV(u0), UAV(u1), UAV(u2), UAV(u3)), " \
    "RootConstants(num32BitConstants=8, b0)"

cbuffer SSRHiZMipCB : register(b0) {
    SSRHiZMipParams ssrHiZMipParams;
}

Texture2DMS<float> inputDepthTexture : register(t0);
Texture2DMS<float4> inputNormalRoughnessTexture : register(t1);
Texture2DMS<float2> inputVelocityTexture : register(t2);

// R: max depth, G: min depth
RWTexture2D<float2> outputSSRHiZMipTexture : register(u0);

RWTexture2D<float> outputResolvedDepthTexture : register(u1);
RWTexture2D<float4> outputResolvedNormalRoughnessTexture : register(u2);
RWTexture2D<float2> outputResolvedVelocityTexture : register(u3);

float2 LoadDepthExtrema(uint2 inputCoord) {
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    float depth = inputDepthTexture.Load(int2(inputCoord), 0);
    float2 extrema = EncodeDepthExtrema(depth, ssrHiZMipParams.clearDepth);
    float closestDepth = depth;
    uint closestSampleIndex = 0;

    // 모든 MSAA sample 을 순회하며 depth 의 max/min 을 구하고, closest depth 와 그 sample index 를 찾는다.
    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        depth = inputDepthTexture.Load(int2(inputCoord), sampleIndex);
        extrema = MergeDepthExtrema(extrema, EncodeDepthExtrema(depth, ssrHiZMipParams.clearDepth));

        const bool isCloser = ssrHiZMipParams.reverseZ != 0 ? depth > closestDepth : depth < closestDepth;
        if (isCloser) {
            closestDepth = depth;
            closestSampleIndex = sampleIndex;
        }
    }

    // first SSRHiZ pass 는 모든 MSAA sample 들을 순회하므로, 이 작업을 이용해서
    // 가장 가까운 depth 에 대응되는 normal/roughness/velocity texture 를 resolve 한다.
    outputResolvedDepthTexture[inputCoord] = closestDepth;
    outputResolvedNormalRoughnessTexture[inputCoord] = inputNormalRoughnessTexture.Load(int2(inputCoord), closestSampleIndex);
    outputResolvedVelocityTexture[inputCoord] = inputVelocityTexture.Load(int2(inputCoord), closestSampleIndex);

    return extrema;
}

// 원본 MSAA depth 의 2x2 cell 을 하나의 depth extrema (max/min) 로 합친다.
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
