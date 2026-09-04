#include "../ShaderInterop.h"
#include "ScreenSpaceDepthPyramidCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0), UAV(u1), UAV(u2)), " \
    "RootConstants(num32BitConstants=8, b0)"

cbuffer ScreenSpaceDepthPyramidCB : register(b0) {
    ScreenSpaceDepthPyramidParams depthPyramidParams;
}

Texture2DMS<float> inputDepthTexture : register(t0);
Texture2DMS<float2> inputNormalTexture : register(t1);

// R: max depth, G: min depth
RWTexture2D<float2> outputDepthPyramidTexture : register(u0);
RWTexture2D<float> outputResolvedDepthTexture : register(u1);
RWTexture2D<float2> outputResolvedNormalTexture : register(u2);

float2 LoadDepthExtrema(uint2 coord) {
    coord = min(coord, depthPyramidParams.inputSize - 1);

    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    float depth = inputDepthTexture.Load(int2(coord), 0);
    float2 extrema = EncodeDepthExtrema(depth, depthPyramidParams.farPlaneDeviceDepth);
    float closestDepth = depth;
    uint closestSampleIndex = 0;

    // 모든 MSAA sample 을 순회하며 depth 의 max/min 을 구하고, closest depth 와 그 sample index 를 찾는다.
    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        depth = inputDepthTexture.Load(int2(coord), sampleIndex);
        extrema = MergeDepthExtrema(extrema, EncodeDepthExtrema(depth, depthPyramidParams.farPlaneDeviceDepth));

        const bool isCloser = depthPyramidParams.useMinDepth != 0 ? depth < closestDepth : depth > closestDepth;
        if (isCloser) {
            closestDepth = depth;
            closestSampleIndex = sampleIndex;
        }
    }

    // 첫 번째 pyramid pass 는 모든 MSAA sample 을 순회하므로, 이 작업을 이용해서
    // 이후 SSR depth 검사에 사용할 가장 가까운 depth 와 대응하는 normal 을 resolve 한다.
    outputResolvedDepthTexture[coord] = closestDepth;
    outputResolvedNormalTexture[coord] = inputNormalTexture.Load(int2(coord), closestSampleIndex);

    return extrema;
}

// 원본 MSAA depth 의 2x2 영역을 하나의 max/min depth 범위로 축소하여 pyramid mip 0 값을 만든다.
float2 ReduceInputDepth(uint2 outputCoord) {
    const uint2 srcMin = outputCoord * 2;

    float2 extrema = LoadDepthExtrema(srcMin);
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(srcMin + uint2(1, 0)));
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(srcMin + uint2(0, 1)));
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(srcMin + uint2(1, 1)));
    return extrema;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(ScreenSpaceDepthPyramidBlockSizeX, ScreenSpaceDepthPyramidBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= depthPyramidParams.outputSize)) {
        return;
    }

    // 유효 영역 밖의 padding cell 은 상위 mip reduction 에 영향을 주지 않도록 EmptyDepthExtrema 로 채운다.
    if (any(outputCoord >= depthPyramidParams.validOutputSize)) {
        outputDepthPyramidTexture[outputCoord] = EmptyDepthExtrema;
        return;
    }

    outputDepthPyramidTexture[outputCoord] = ReduceInputDepth(outputCoord);
}
