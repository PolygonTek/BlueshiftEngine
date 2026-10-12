///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Portions adapted from XeGTAO.
// Copyright (C) 2016-2021, Intel Corporation
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "GTAOCommon.hlsli"

#define GTAO_PREFILTER_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0, flags = DATA_VOLATILE), UAV(u0, numDescriptors = 6)), " \
    "RootConstants(num32BitConstants=12, b0), " \
    "CBV(b1)"

cbuffer GTAOPrefilterCB : register(b0) {
    GTAOPrefilterParams gtaoPrefilterParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

Texture2D<float> inputDepthTexture : register(t0);
RWTexture2D<float> outputDepthMip0 : register(u0);
RWTexture2D<float> outputDepthMip1 : register(u1);
RWTexture2D<float> outputDepthMip2 : register(u2);
RWTexture2D<float> outputDepthMip3 : register(u3);
RWTexture2D<float> outputDepthMip4 : register(u4);
RWTexture2D<float> outputSelectedDepthTexture : register(u5);

groupshared float gtaoSharedDepths[8][8];

bool GTAOIsCloserDeviceDepth(float lhs, float rhs) {
    return gtaoPrefilterParams.useMaxDepth != 0u ? lhs > rhs : lhs < rhs;
}

float GTAOLoadProcessingDepth(uint2 outputCoord, out float selectedDepth) {
    const uint2 blockOrigin = outputCoord * gtaoPrefilterParams.resolutionScale;
    selectedDepth = gtaoPrefilterParams.farPlaneDeviceDepth;
    bool foundSurface = false;

    [unroll]
    for (uint y = 0u; y < 2u; ++y) {
        [unroll]
        for (uint x = 0u; x < 2u; ++x) {
            if (x >= gtaoPrefilterParams.resolutionScale || y >= gtaoPrefilterParams.resolutionScale) {
                continue;
            }

            const uint2 inputCoord = blockOrigin + uint2(x, y);
            if (any(inputCoord >= gtaoPrefilterParams.inputSize)) {
                continue;
            }

            const float depth = inputDepthTexture.Load(int3(inputCoord, 0));
            if (depth == gtaoPrefilterParams.farPlaneDeviceDepth) {
                continue;
            }

            if (!foundSurface || GTAOIsCloserDeviceDepth(depth, selectedDepth)) {
                selectedDepth = depth;
                foundSurface = true;
            }
        }
    }

    float linearDepth = 0.0f;
    if (foundSurface) {
        const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;
        linearDepth = ToLinearDepth(selectedDepth, cameraConstants.deviceDepthToNdcZScaleBias, cameraConstants.projectionScaleBiasZ, orthogonal);
    }
    return linearDepth;
}

[RootSignature(GTAO_PREFILTER_ROOT_SIGNATURE)]
[numthreads(GTAOPrefilterBlockSizeX, GTAOPrefilterBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupThreadId : SV_GroupThreadID) {
    // Each thread covers a 2x2 mip-0 block, so one 8x8 group generates a 16x16 block and all five depth mips.
    const uint2 baseCoord = dispatchThreadId.xy * 2u;
    const uint2 maxOutputCoord = gtaoPrefilterParams.outputSize - 1u;
    const uint2 coord0 = min(baseCoord + uint2(0u, 0u), maxOutputCoord);
    const uint2 coord1 = min(baseCoord + uint2(1u, 0u), maxOutputCoord);
    const uint2 coord2 = min(baseCoord + uint2(0u, 1u), maxOutputCoord);
    const uint2 coord3 = min(baseCoord + uint2(1u, 1u), maxOutputCoord);

    float selectedDepth0;
    float selectedDepth1;
    float selectedDepth2;
    float selectedDepth3;
    const float depth0 = GTAOLoadProcessingDepth(coord0, selectedDepth0);
    const float depth1 = GTAOLoadProcessingDepth(coord1, selectedDepth1);
    const float depth2 = GTAOLoadProcessingDepth(coord2, selectedDepth2);
    const float depth3 = GTAOLoadProcessingDepth(coord3, selectedDepth3);

    if (baseCoord.x < gtaoPrefilterParams.outputSize.x && baseCoord.y < gtaoPrefilterParams.outputSize.y) {
        outputDepthMip0[baseCoord] = depth0;
        if (gtaoPrefilterParams.resolutionScale > 1u) {
            outputSelectedDepthTexture[baseCoord] = selectedDepth0;
        }
    }
    if (baseCoord.x + 1u < gtaoPrefilterParams.outputSize.x && baseCoord.y < gtaoPrefilterParams.outputSize.y) {
        outputDepthMip0[baseCoord + uint2(1u, 0u)] = depth1;
        if (gtaoPrefilterParams.resolutionScale > 1u) {
            outputSelectedDepthTexture[baseCoord + uint2(1u, 0u)] = selectedDepth1;
        }
    }
    if (baseCoord.x < gtaoPrefilterParams.outputSize.x && baseCoord.y + 1u < gtaoPrefilterParams.outputSize.y) {
        outputDepthMip0[baseCoord + uint2(0u, 1u)] = depth2;
        if (gtaoPrefilterParams.resolutionScale > 1u) {
            outputSelectedDepthTexture[baseCoord + uint2(0u, 1u)] = selectedDepth2;
        }
    }
    if (baseCoord.x + 1u < gtaoPrefilterParams.outputSize.x && baseCoord.y + 1u < gtaoPrefilterParams.outputSize.y) {
        outputDepthMip0[baseCoord + uint2(1u, 1u)] = depth3;
        if (gtaoPrefilterParams.resolutionScale > 1u) {
            outputSelectedDepthTexture[baseCoord + uint2(1u, 1u)] = selectedDepth3;
        }
    }

    const float depthMip1 = GTAOFilterDepthMip(float4(depth0, depth1, depth2, depth3), gtaoPrefilterParams);
    const uint2 mip1Size = max(gtaoPrefilterParams.outputSize >> 1u, 1u);
    if (gtaoPrefilterParams.depthMipCount > 1u && all(dispatchThreadId.xy < mip1Size)) {
        outputDepthMip1[dispatchThreadId.xy] = depthMip1;
    }
    gtaoSharedDepths[groupThreadId.x][groupThreadId.y] = depthMip1;

    GroupMemoryBarrierWithGroupSync();

    if (all((groupThreadId.xy & 1u) == 0u)) {
        const float depthMip2 = GTAOFilterDepthMip(float4(
            gtaoSharedDepths[groupThreadId.x + 0u][groupThreadId.y + 0u],
            gtaoSharedDepths[groupThreadId.x + 1u][groupThreadId.y + 0u],
            gtaoSharedDepths[groupThreadId.x + 0u][groupThreadId.y + 1u],
            gtaoSharedDepths[groupThreadId.x + 1u][groupThreadId.y + 1u]), gtaoPrefilterParams);
        const uint2 mip2Coord = dispatchThreadId.xy >> 1u;
        const uint2 mip2Size = max(gtaoPrefilterParams.outputSize >> 2u, 1u);
        if (gtaoPrefilterParams.depthMipCount > 2u && all(mip2Coord < mip2Size)) {
            outputDepthMip2[mip2Coord] = depthMip2;
        }
        gtaoSharedDepths[groupThreadId.x][groupThreadId.y] = depthMip2;
    }

    GroupMemoryBarrierWithGroupSync();

    if (all((groupThreadId.xy & 3u) == 0u)) {
        const float depthMip3 = GTAOFilterDepthMip(float4(
            gtaoSharedDepths[groupThreadId.x + 0u][groupThreadId.y + 0u],
            gtaoSharedDepths[groupThreadId.x + 2u][groupThreadId.y + 0u],
            gtaoSharedDepths[groupThreadId.x + 0u][groupThreadId.y + 2u],
            gtaoSharedDepths[groupThreadId.x + 2u][groupThreadId.y + 2u]), gtaoPrefilterParams);
        const uint2 mip3Coord = dispatchThreadId.xy >> 2u;
        const uint2 mip3Size = max(gtaoPrefilterParams.outputSize >> 3u, 1u);
        if (gtaoPrefilterParams.depthMipCount > 3u && all(mip3Coord < mip3Size)) {
            outputDepthMip3[mip3Coord] = depthMip3;
        }
        gtaoSharedDepths[groupThreadId.x][groupThreadId.y] = depthMip3;
    }

    GroupMemoryBarrierWithGroupSync();

    if (all(groupThreadId.xy == 0u)) {
        const float depthMip4 = GTAOFilterDepthMip(float4(
            gtaoSharedDepths[0][0],
            gtaoSharedDepths[4][0],
            gtaoSharedDepths[0][4],
            gtaoSharedDepths[4][4]), gtaoPrefilterParams);
        const uint2 mip4Coord = dispatchThreadId.xy >> 3u;
        const uint2 mip4Size = max(gtaoPrefilterParams.outputSize >> 4u, 1u);
        if (gtaoPrefilterParams.depthMipCount > 4u && all(mip4Coord < mip4Size)) {
            outputDepthMip4[mip4Coord] = depthMip4;
        }
    }
}
