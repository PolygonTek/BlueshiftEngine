#include "../ShaderInterop.h"
#include "../Common.hlsli"

#ifndef GEN_HIZ_DEPTH_MSAA
#define GEN_HIZ_DEPTH_MSAA 0
#endif

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0), UAV(u1), UAV(u2), UAV(u3)), " \
    "CBV(b0)"

cbuffer HiZMipGenCB : register(b0) {
    HiZMipGenParams hiZMipGenParams;
}

#if GEN_HIZ_DEPTH_MSAA
Texture2DMS<float> inputDepthTexture : register(t0);
#else
Texture2D<float> inputDepthTexture : register(t0);
#endif
RWTexture2D<float> outputMip0 : register(u0);
RWTexture2D<float> outputMip1 : register(u1);
RWTexture2D<float> outputMip2 : register(u2);
RWTexture2D<float> outputMip3 : register(u3);

groupshared float sharedDepths[GenHiZMipBlockSizeY][GenHiZMipBlockSizeX];

uint2 GetMipSize(uint mipLevel) {
    return max(hiZMipGenParams.inputSize >> mipLevel, uint2(1, 1));
}

// hiZ mipmap generation 에서 depth 를 reduce 할 때는 더 멀리 있는 depth 를 선택한다.
float ReduceDepth(float a, float b, float c, float d) {
    if (hiZMipGenParams.useMinDepth != 0) {
        return min(min(a, b), min(c, d));
    }
    return max(max(a, b), max(c, d));
}

float ReduceQuadDepth(float depth) {
    return ReduceDepth(depth, QuadReadAcrossX(depth), QuadReadAcrossY(depth), QuadReadAcrossDiagonal(depth));
}

float LoadInputDepth(uint2 coord) {
    coord = min(coord, hiZMipGenParams.inputSize - 1);

#if GEN_HIZ_DEPTH_MSAA
    uint width;
    uint height;
    uint sampleCount;
    inputDepthTexture.GetDimensions(width, height, sampleCount);

    float depth = inputDepthTexture.Load(int2(coord), 0);
    for (uint sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
        const float sampleDepth = inputDepthTexture.Load(int2(coord), sampleIndex);
        depth = hiZMipGenParams.useMinDepth != 0 ? min(depth, sampleDepth) : max(depth, sampleDepth);
    }
    return depth;
#else
    return inputDepthTexture.Load(int3(coord, 0));
#endif
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(GenHiZMipBlockSizeX * GenHiZMipBlockSizeY, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex) {
    const uint2 groupThreadId = RemapLane8x8(groupIndex);
    const uint2 dispatchThreadId = groupId.xy * uint2(GenHiZMipBlockSizeX, GenHiZMipBlockSizeY) + groupThreadId;
    const uint2 mip0Coord = min(dispatchThreadId, hiZMipGenParams.inputSize - 1);
    float depth = LoadInputDepth(mip0Coord);

    if (all(dispatchThreadId < hiZMipGenParams.inputSize)) {
        outputMip0[dispatchThreadId] = depth;
    }

    if (hiZMipGenParams.outputMipCount > 1) {
        depth = ReduceQuadDepth(depth);

        if (all((groupThreadId.xy & 1) == 0)) {
            const uint2 mip1Coord = dispatchThreadId >> 1;
            if (all(mip1Coord < GetMipSize(1))) {
                outputMip1[mip1Coord] = depth;
            }
            sharedDepths[groupThreadId.y][groupThreadId.x] = depth;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (hiZMipGenParams.outputMipCount > 2 && all((groupThreadId.xy & 3) == 0)) {
        depth = ReduceDepth(
            sharedDepths[groupThreadId.y    ][groupThreadId.x    ],
            sharedDepths[groupThreadId.y    ][groupThreadId.x + 2],
            sharedDepths[groupThreadId.y + 2][groupThreadId.x    ],
            sharedDepths[groupThreadId.y + 2][groupThreadId.x + 2]);

        const uint2 mip2Coord = dispatchThreadId >> 2;
        if (all(mip2Coord < GetMipSize(2))) {
            outputMip2[mip2Coord] = depth;
        }
        sharedDepths[groupThreadId.y][groupThreadId.x] = depth;
    }

    GroupMemoryBarrierWithGroupSync();

    if (hiZMipGenParams.outputMipCount > 3 && groupIndex == 0) {
        depth = ReduceDepth(
            sharedDepths[0][0],
            sharedDepths[0][4],
            sharedDepths[4][0],
            sharedDepths[4][4]);

        const uint2 mip3Coord = dispatchThreadId >> 3;
        if (all(mip3Coord < GetMipSize(3))) {
            outputMip3[mip3Coord] = depth;
        }
    }
}
