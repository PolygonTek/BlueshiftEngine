#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "GTAOCommon.hlsli"

#define GTAO_DENOISE_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer GTAODenoiseCB : register(b0) {
    GTAODenoiseParams gtaoDenoiseParams;
}

Texture2D<float> inputAOTexture : register(t0);
Texture2D<float> packedEdgesTexture : register(t1);
RWTexture2D<float> outputAOTexture : register(u0);

[RootSignature(GTAO_DENOISE_ROOT_SIGNATURE)]
[numthreads(GTAODenoiseBlockSizeX, GTAODenoiseBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    GTAODenoisePixelPair(dispatchThreadId.xy * uint2(2u, 1u), gtaoDenoiseParams, inputAOTexture, packedEdgesTexture, pointClampSampler, outputAOTexture);
}
