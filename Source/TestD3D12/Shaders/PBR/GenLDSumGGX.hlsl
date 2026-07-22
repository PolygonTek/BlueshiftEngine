#include "IBLIntegration.hlsli"
#include "../Cubemap.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=3, b0), " \
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

    const float2 uv = (float2(outputCoord.xy) + 0.5f) / float2(outputWidth, outputHeight);
    const float3 N = normalize(CubeMapFaceToDirection(outputCoord.z, uv));
    const float linearRoughness = genLDSumGGXParams.roughness * genLDSumGGXParams.roughness;

    float3 integratedLD;
    if (linearRoughness == 0.0f) {
        // Roughness 0 에서 LD 는 N 방향의 source radiance 와 같다. 출력 해상도가 더 낮을 때
        // mip 0 을 직접 sampling 하면 aliasing 이 발생하므로, 출력 texel footprint 이상을
        // 덮는 source mip 을 사용한다. 입출력 해상도가 같으면 mip 0 이 선택된다.
        // 한 번만 sampling 하므로 roughness 0 에서 특이점이 되는 GGX PDF 계산도 피할 수 있다.
        integratedLD = radianceCubeMap.SampleLevel(linearClampSampler, N, genLDSumGGXParams.minRadianceMipLevel).rgb;
    } else {
        integratedLD = IntegrateLD(N, linearRoughness, radianceCubeMap, linearClampSampler, (float)genLDSumGGXParams.radianceCubeMapSize, genLDSumGGXParams.minRadianceMipLevel);
    }

    outputLD[outputCoord] = float4(integratedLD, 1.0f);
}
