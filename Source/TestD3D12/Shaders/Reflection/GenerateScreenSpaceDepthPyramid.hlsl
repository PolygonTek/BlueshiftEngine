#include "../ShaderInterop.h"

#ifndef SCREEN_SPACE_DEPTH_PYRAMID_FIRST_MIP
#define SCREEN_SPACE_DEPTH_PYRAMID_FIRST_MIP 0
#endif

#ifndef SCREEN_SPACE_DEPTH_PYRAMID_DEPTH_MSAA
#define SCREEN_SPACE_DEPTH_PYRAMID_DEPTH_MSAA 0
#endif

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=6, b0)"

cbuffer ScreenSpaceDepthPyramidCB : register(b0) {
    ScreenSpaceDepthPyramidParams depthPyramidParams;
}

#if SCREEN_SPACE_DEPTH_PYRAMID_FIRST_MIP
    #if SCREEN_SPACE_DEPTH_PYRAMID_DEPTH_MSAA
Texture2DMS<float> inputDepthTexture : register(t0);
    #else
Texture2D<float> inputDepthTexture : register(t0);
    #endif
#else
Texture2D<float2> inputDepthPyramid : register(t0);
#endif

// R: max depth, G: min depth
RWTexture2D<float2> outputDepthPyramid : register(u0);

float2 MergeDepthExtrema(float2 a, float2 b) {
    return float2(max(a.x, b.x), min(a.y, b.y));
}

#if SCREEN_SPACE_DEPTH_PYRAMID_FIRST_MIP
float2 LoadDepthExtrema(uint2 coord) {
    coord = min(coord, depthPyramidParams.inputSize - 1);

#if SCREEN_SPACE_DEPTH_PYRAMID_DEPTH_MSAA
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    float depth = inputDepthTexture.Load(int2(coord), 0);
    float2 extrema = float2(depth, depth);
    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        depth = inputDepthTexture.Load(int2(coord), sampleIndex);
        extrema = MergeDepthExtrema(extrema, float2(depth, depth));
    }
    return extrema;
#else
    const float depth = inputDepthTexture.Load(int3(coord, 0));
    return float2(depth, depth);
#endif
}
#endif

#if SCREEN_SPACE_DEPTH_PYRAMID_FIRST_MIP
float2 ReduceInputDepth(uint2 outputCoord) {
    const uint2 srcMin = outputCoord * 2;

    float2 extrema = LoadDepthExtrema(srcMin);
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(srcMin + uint2(1, 0)));
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(srcMin + uint2(0, 1)));
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(srcMin + uint2(1, 1)));
    return extrema;
}
#else
float2 ReduceInputDepth(uint2 outputCoord) {
    const uint2 inputSize = depthPyramidParams.inputSize;
    const uint2 srcMin = outputCoord * 2;
    const uint2 srcMax = min(srcMin + 1, inputSize - 1);

    float2 extrema = inputDepthPyramid.Load(int3(srcMin, 0));
    extrema = MergeDepthExtrema(extrema, inputDepthPyramid.Load(int3(uint2(srcMax.x, srcMin.y), 0)));
    extrema = MergeDepthExtrema(extrema, inputDepthPyramid.Load(int3(uint2(srcMin.x, srcMax.y), 0)));
    extrema = MergeDepthExtrema(extrema, inputDepthPyramid.Load(int3(srcMax, 0)));
    return extrema;
}
#endif

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(ScreenSpaceDepthPyramidBlockSizeX, ScreenSpaceDepthPyramidBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= depthPyramidParams.outputSize)) {
        return;
    }

#if SCREEN_SPACE_DEPTH_PYRAMID_FIRST_MIP
    // 패딩 영역은 이후 mip 의 최대/최소 depth 계산에 영향을 주지 않도록 (0, 1) 로 채운다.
    if (any(outputCoord >= depthPyramidParams.validOutputSize)) {
        outputDepthPyramid[outputCoord] = float2(0.0, 1.0);
        return;
    }
#endif

    outputDepthPyramid[outputCoord] = ReduceInputDepth(outputCoord);
}
