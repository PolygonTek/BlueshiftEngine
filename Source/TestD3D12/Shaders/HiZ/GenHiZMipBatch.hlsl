#include "../ShaderInterop.h"
#include "../Common.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0), UAV(u1), UAV(u2), UAV(u3)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

cbuffer HiZMipGenCB : register(b0) {
    HiZMipGenParams hiZMipGenParams;
}

Texture2D<float> inputTexture : register(t0);
SamplerState linearClampSampler : register(s100);
RWTexture2D<float> outputMip0 : register(u0);
RWTexture2D<float> outputMip1 : register(u1);
RWTexture2D<float> outputMip2 : register(u2);
RWTexture2D<float> outputMip3 : register(u3);

groupshared float sharedDepths[GenHiZMipBlockSizeY][GenHiZMipBlockSizeX];

uint2 GetOutputMipSize(uint mipLevel) {
    return max(hiZMipGenParams.outputSize >> mipLevel, uint2(1, 1));
}

// hiZ mipmap generation 에서 depth 를 reduce 할 때는 더 멀리 있는 depth 를 선택한다.
float ReduceDepth(float a, float b, float c, float d) {
    if (hiZMipGenParams.reverseZ != 0) {
        return min(min(a, b), min(c, d));
    }
    return max(max(a, b), max(c, d));
}

float ReduceQuadDepth(float depth) {
    return ReduceDepth(depth, QuadReadAcrossX(depth), QuadReadAcrossY(depth), QuadReadAcrossDiagonal(depth));
}

float ReduceInputDepth(uint2 outputCoord) {
    uint2 inputCoord = outputCoord << 1;
    float2 uv = (float2(inputCoord) + 1.0) * hiZMipGenParams.inputSizeRcp;
    float4 depths = inputTexture.GatherRed(linearClampSampler, uv);
    return ReduceDepth(depths.x, depths.y, depths.z, depths.w);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(GenHiZMipBlockSizeX * GenHiZMipBlockSizeY, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex) {
    const uint2 groupThreadId = RemapLane8x8(groupIndex);
    const uint2 dispatchThreadId = groupId.xy * uint2(GenHiZMipBlockSizeX, GenHiZMipBlockSizeY) + groupThreadId;
    const uint2 firstOutputCoord = min(dispatchThreadId, hiZMipGenParams.outputSize - 1);
    float depth = ReduceInputDepth(firstOutputCoord);

    if (all(dispatchThreadId < hiZMipGenParams.outputSize)) {
        outputMip0[dispatchThreadId] = depth;
    }

    if (hiZMipGenParams.outputMipCount > 1) {
        depth = ReduceQuadDepth(depth);

        if (all((groupThreadId.xy & 1) == 0)) {
            const uint2 mip1Coord = dispatchThreadId >> 1;
            if (all(mip1Coord < GetOutputMipSize(1))) {
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
        if (all(mip2Coord < GetOutputMipSize(2))) {
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
        if (all(mip3Coord < GetOutputMipSize(3))) {
            outputMip3[mip3Coord] = depth;
        }
    }
}
