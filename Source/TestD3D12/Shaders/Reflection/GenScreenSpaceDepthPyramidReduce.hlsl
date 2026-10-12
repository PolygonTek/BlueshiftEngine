#include "../ShaderInterop.h"
#include "ScreenSpaceDepthPyramidCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0)"

cbuffer ScreenSpaceDepthPyramidCB : register(b0) {
    ScreenSpaceDepthPyramidParams depthPyramidParams;
}

Texture2D<float2> inputDepthPyramidTexture : register(t0);

// R: max depth, G: min depth
RWTexture2D<float2> outputDepthPyramidTexture : register(u0);

// 이전 pyramid mip 의 2x2 영역을 하나의 max/min depth 범위로 합친다.
float2 ReduceInputDepth(uint2 outputCoord) {
    const uint2 inputMin = outputCoord * 2;
    const uint2 inputMax = min(inputMin + 1, depthPyramidParams.inputSize - 1);

    float2 extrema = inputDepthPyramidTexture.Load(int3(inputMin, 0));
    extrema = MergeDepthExtrema(extrema, inputDepthPyramidTexture.Load(int3(uint2(inputMax.x, inputMin.y), 0)));
    extrema = MergeDepthExtrema(extrema, inputDepthPyramidTexture.Load(int3(uint2(inputMin.x, inputMax.y), 0)));
    extrema = MergeDepthExtrema(extrema, inputDepthPyramidTexture.Load(int3(inputMax, 0)));
    return extrema;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(ScreenSpaceDepthPyramidBlockSizeX, ScreenSpaceDepthPyramidBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= depthPyramidParams.outputSize)) {
        return;
    }

    outputDepthPyramidTexture[outputCoord] = ReduceInputDepth(outputCoord);
}
