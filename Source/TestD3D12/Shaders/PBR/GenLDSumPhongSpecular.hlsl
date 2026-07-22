#include "IBLIntegration.hlsli"
#include "../Cubemap.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=1, b0), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_LINEAR, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

cbuffer GenLDSumPhongSpecularCB : register(b0) {
    GenLDSumPhongSpecularParams genLDSumPhongSpecularParams;
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

    const float2 uv = (float2(outputCoord.xy) + 0.5f) / float2(outputWidth, outputHeight);
    const float3 S = normalize(CubeMapFaceToDirection(outputCoord.z, uv));
    const float3 integratedLD = IntegrateLDPhongSpecular(S, genLDSumPhongSpecularParams.specularPower, radianceCubeMap, linearClampSampler);

    outputLD[outputCoord] = float4(integratedLD, 1.0f);
}
