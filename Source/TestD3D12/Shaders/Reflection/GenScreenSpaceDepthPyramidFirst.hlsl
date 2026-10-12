#include "../ShaderInterop.h"
#include "ScreenSpaceDepthPyramidCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0)"

cbuffer ScreenSpaceDepthPyramidCB : register(b0) {
    ScreenSpaceDepthPyramidParams depthPyramidParams;
}

Texture2D<float> inputDepthTexture : register(t0);

// R: max depth, G: min depth
RWTexture2D<float2> outputDepthPyramidTexture : register(u0);

float2 LoadDepthExtrema(uint2 coord) {
    const float depth = inputDepthTexture.Load(int3(coord, 0));
    return EncodeDepthExtrema(depth, depthPyramidParams.farPlaneDeviceDepth);
}

// 원본 depth 의 2x2 영역을 하나의 max/min depth 범위로 합친다.
float2 ReduceInputDepth(uint2 outputCoord) {
    const uint2 inputMin = outputCoord * 2;
    const uint2 inputMax = min(inputMin + 1, depthPyramidParams.inputSize - 1);

    float2 extrema = LoadDepthExtrema(inputMin);
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(uint2(inputMax.x, inputMin.y)));
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(uint2(inputMin.x, inputMax.y)));
    extrema = MergeDepthExtrema(extrema, LoadDepthExtrema(inputMax));
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
