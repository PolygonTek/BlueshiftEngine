#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(CBV(b0), SRV(t0), UAV(u0)), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_LINEAR, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

MipGenParams mipGenParams : register(b0);

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

RWTexture3D<MIP_OUTPUT_FORMAT> outputTexture : register(u0);
Texture3D<MIP_OUTPUT_FORMAT> inputTexture : register(t0);
SamplerState linearSampler : register(s0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(GENMIP_3D_BLOCK_SIZE_X, GENMIP_3D_BLOCK_SIZE_Y, GENMIP_3D_BLOCK_SIZE_Z)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    // 출력 텍셀의 좌표
    uint3 dstCoord = dispatchThreadId.xyz;

    // 출력 좌표가 출력 크기를 넘어가면 조기 종료
    if (dstCoord.x >= mipGenParams.dstSize.x || dstCoord.y >= mipGenParams.dstSize.y || dstCoord.z >= mipGenParams.dstSize.z) {
        return;
    }

    // 텍셀의 중앙값으로 uv 를 계산하여 2x2x2 블럭의 중심으로 선형 샘플링을 한다.
    float3 uvw = ((float3)dstCoord + 0.5) * (float3)mipGenParams.dstSizeRcp.xyz;

    outputTexture[dstCoord] = inputTexture.SampleLevel(linearSampler, uvw, 0);
}
