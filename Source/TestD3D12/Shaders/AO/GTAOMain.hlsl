#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../NormalEncoding.hlsli"
#include "GTAOCommon.hlsli"

#define GTAO_MAIN_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0, flags = DATA_VOLATILE), SRV(t1, flags = DATA_VOLATILE), SRV(t2), UAV(u0), UAV(u1)), " \
    "RootConstants(num32BitConstants=18, b0), " \
    "CBV(b1), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer GTAOMainCB : register(b0) {
    GTAOMainParams gtaoMainParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

Texture2D<float> inputDepthTexture : register(t0);
Texture2D<float2> inputNormalTexture : register(t1);
Texture2D<float> prefilteredDepthTexture : register(t2);
RWTexture2D<float> outputAOTexture : register(u0);
RWTexture2D<float> outputPackedEdgesTexture : register(u1);

#ifndef GTAO_SLICE_COUNT
#define GTAO_SLICE_COUNT GTAOQualityHighSliceCount
#endif

#ifndef GTAO_STEPS_PER_SLICE
#define GTAO_STEPS_PER_SLICE GTAOQualityHighStepsPerSlice
#endif

bool GTAOIsCloserDeviceDepth(float lhs, float rhs) {
    return gtaoMainParams.farPlaneDeviceDepth == 0.0f ? lhs > rhs : lhs < rhs;
}

uint2 GTAOSelectInputCoord(uint2 outputCoord) {
    const uint2 blockOrigin = outputCoord * gtaoMainParams.resolutionScale;
    uint2 selectedCoord = min(blockOrigin, gtaoMainParams.inputSize - 1u);
    float selectedDepth = gtaoMainParams.farPlaneDeviceDepth;
    bool foundSurface = false;

    [unroll]
    for (uint y = 0u; y < 2u; ++y) {
        [unroll]
        for (uint x = 0u; x < 2u; ++x) {
            if (x >= gtaoMainParams.resolutionScale || y >= gtaoMainParams.resolutionScale) {
                continue;
            }

            const uint2 inputCoord = blockOrigin + uint2(x, y);
            if (any(inputCoord >= gtaoMainParams.inputSize)) {
                continue;
            }

            const float depth = inputDepthTexture.Load(int3(inputCoord, 0));
            if (depth != gtaoMainParams.farPlaneDeviceDepth && (!foundSurface || GTAOIsCloserDeviceDepth(depth, selectedDepth))) {
                selectedCoord = inputCoord;
                selectedDepth = depth;
                foundSurface = true;
            }
        }
    }
    return selectedCoord;
}

[RootSignature(GTAO_MAIN_ROOT_SIGNATURE)]
[numthreads(GTAOMainBlockSizeX, GTAOMainBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= gtaoMainParams.outputSize)) {
        return;
    }

    const uint2 inputCoord = GTAOSelectInputCoord(pixelCoord);
    const float3 viewNormal = DecodeOctahedralNormal(inputNormalTexture.Load(int3(inputCoord, 0)));
    float packedEdges;
    const float visibility = GTAOEvaluateVisibility(pixelCoord, viewNormal, gtaoMainParams, GTAO_SLICE_COUNT, GTAO_STEPS_PER_SLICE, cameraConstants, prefilteredDepthTexture, pointClampSampler, packedEdges);
    outputAOTexture[pixelCoord] = visibility;
    outputPackedEdgesTexture[pixelCoord] = packedEdges;
}
