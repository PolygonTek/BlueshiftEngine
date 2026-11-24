#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "SSRHiZCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0), UAV(u1), UAV(u2), UAV(u3)), " \
    "RootConstants(num32BitConstants=5, b0)"

cbuffer SSRHiZMipCB : register(b0) {
    SSRHiZMipBatchParams ssrHiZMipParams;
}

Texture2D<float2> inputSSRHiZMipTexture : register(t0);

// R: max depth, G: min depth
RWTexture2D<float2> outputMip0 : register(u0);
RWTexture2D<float2> outputMip1 : register(u1);
RWTexture2D<float2> outputMip2 : register(u2);
RWTexture2D<float2> outputMip3 : register(u3);

groupshared float2 sharedExtrema[SSRHiZBlockSizeY][SSRHiZBlockSizeX];

uint2 GetOutputMipSize(uint mipLevel) {
    return max(ssrHiZMipParams.outputSize >> mipLevel, uint2(1, 1));
}

float2 ReduceQuadExtrema(float2 extrema) {
    return MergeDepthExtrema(
        MergeDepthExtrema(extrema, QuadReadAcrossX(extrema)),
        MergeDepthExtrema(QuadReadAcrossY(extrema), QuadReadAcrossDiagonal(extrema)));
}

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
[numthreads(SSRHiZBlockSizeX * SSRHiZBlockSizeY, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex) {
    const uint2 groupThreadId = RemapLane8x8(groupIndex);
    const uint2 outputCoord = groupId.xy * uint2(SSRHiZBlockSizeX, SSRHiZBlockSizeY) + groupThreadId;

    // 경계 밖 lane 도 quad 연산과 그룹 동기화에 참여한다. 1xN/Nx1 mip 은 경계 값을 반복해서 reduce 한다.
    float2 extrema = ReduceDepthExtrema(min(outputCoord, ssrHiZMipParams.outputSize - 1));
    if (all(outputCoord < ssrHiZMipParams.outputSize)) {
        outputMip0[outputCoord] = extrema;
    }

    // outputMipCount 는 그룹 전체에 동일하므로 필요한 출력까지 생성한 뒤 함께 종료할 수 있다.
    if (ssrHiZMipParams.outputMipCount <= 1) {
        return;
    }

    extrema = ReduceQuadExtrema(extrema);
    if (all((groupThreadId & 1) == 0)) {
        const uint2 mipCoord = outputCoord >> 1;
        if (all(mipCoord < GetOutputMipSize(1))) {
            outputMip1[mipCoord] = extrema;
        }
        sharedExtrema[groupThreadId.y][groupThreadId.x] = extrema;
    }

    if (ssrHiZMipParams.outputMipCount <= 2) {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    if (all((groupThreadId & 3) == 0)) {
        extrema = MergeDepthExtrema(
            MergeDepthExtrema(sharedExtrema[groupThreadId.y][groupThreadId.x], sharedExtrema[groupThreadId.y][groupThreadId.x + 2]),
            MergeDepthExtrema(sharedExtrema[groupThreadId.y + 2][groupThreadId.x], sharedExtrema[groupThreadId.y + 2][groupThreadId.x + 2]));

        const uint2 mipCoord = outputCoord >> 2;
        if (all(mipCoord < GetOutputMipSize(2))) {
            outputMip2[mipCoord] = extrema;
        }
        sharedExtrema[groupThreadId.y][groupThreadId.x] = extrema;
    }

    if (ssrHiZMipParams.outputMipCount <= 3) {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    if (groupIndex == 0) {
        extrema = MergeDepthExtrema(
            MergeDepthExtrema(sharedExtrema[0][0], sharedExtrema[0][4]),
            MergeDepthExtrema(sharedExtrema[4][0], sharedExtrema[4][4]));

        const uint2 mipCoord = outputCoord >> 3;
        if (all(mipCoord < GetOutputMipSize(3))) {
            outputMip3[mipCoord] = extrema;
        }
    }
}
