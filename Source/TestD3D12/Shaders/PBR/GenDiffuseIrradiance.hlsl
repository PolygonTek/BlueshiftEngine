#include "../ShaderInterop.h"
#include "IBLIntegration.hlsli"
#include "../Cubemap.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_LINEAR, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

TextureCube<float4> radianceCubeMap : register(t0);
RWTexture2DArray<float4> outputIrradiance : register(u0);
SamplerState linearClampSampler : register(s0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(IBLIntegrationBlockSizeX, IBLIntegrationBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint outputWidth;
    uint outputHeight;
    uint outputFaceCount;
    outputIrradiance.GetDimensions(outputWidth, outputHeight, outputFaceCount);

    const uint3 outputCoord = dispatchThreadId;
    if (outputCoord.x >= outputWidth || outputCoord.y >= outputHeight || outputCoord.z >= min(outputFaceCount, 6u)) {
        return;
    }

    const float2 uv = (float2(outputCoord.xy) + 0.5f) / float2(outputWidth, outputHeight);
    const float3 N = normalize(CubeMapFaceToDirection(outputCoord.z, uv));
    const float3 irradiance = IntegrateDiffuseIrradiance(N, radianceCubeMap, linearClampSampler);

    outputIrradiance[outputCoord] = float4(irradiance, 1.0f);
}
