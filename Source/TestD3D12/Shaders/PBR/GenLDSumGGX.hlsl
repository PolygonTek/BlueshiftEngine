#include "IBLIntegration.hlsli"
#include "../Cubemap.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=2, b0), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_LINEAR, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

cbuffer GenLDSumGGXCB : register(b0) {
    GenLDSumGGXParams genLDSumGGXParams;
}

TextureCube<float4> radianceCubeMap : register(t0);
RWTexture2DArray<float4> outputLD : register(u0);
SamplerState linearClampSampler : register(s0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(IBLIntegrationBlockSizeX, IBLIntegrationBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint outputWidth;
    uint outputHeight;
    uint outputFaceCount;
    outputLD.GetDimensions(outputWidth, outputHeight, outputFaceCount);

    const uint3 outputCoord = dispatchThreadId;
    if (outputCoord.x >= outputWidth || outputCoord.y >= outputHeight || outputCoord.z >= min(outputFaceCount, 6u)) {
        return;
    }

    // Mip 0 is copied from the source cubemap. For the remaining mips, the caller sets
    // roughness to mipLevel / (mipLevelCount - 1) and binds that mip as the output UAV.
    const float2 uv = (float2(outputCoord.xy) + 0.5f) / float2(outputWidth, outputHeight);
    const float3 N = normalize(CubeMapFaceToDirection(outputCoord.z, uv));
    const float linearRoughness = genLDSumGGXParams.roughness * genLDSumGGXParams.roughness;
    const float3 integratedLD = IntegrateLD(N, linearRoughness, radianceCubeMap, linearClampSampler, (float)genLDSumGGXParams.radianceCubeMapSize);

    outputLD[outputCoord] = float4(integratedLD, 1.0f);
}
