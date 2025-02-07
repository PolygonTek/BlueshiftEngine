#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_LINEAR, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

MipGenParams mipGenParams : register(b0);

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

RWTexture2DArray<MIP_OUTPUT_FORMAT> outputTexture : register(u0);
TextureCube<MIP_OUTPUT_FORMAT> inputTexture : register(t0);
SamplerState linearSampler : register(s0);

inline float3 ToCubeMapCoords(in uint faceIndex, in float2 uv) {
    // get uv in [-1, 1] range:
    uv = uv * 2 - 1;

    switch (faceIndex) {
    case 0:
        return float3(1.0, -uv.y, -uv.x);
    case 1:
        return float3(-1.0, -uv.y, uv.x);
    case 2:
        return float3(uv.x, 1.0, uv.y);
    case 3:
        return float3(uv.x, -1.0, -uv.y);
    case 4:
        return float3(uv.x, -uv.y, 1.0);
    case 5:
        return float3(-uv.x, -uv.y, -1.0);
    default:
        return float3(0, 0, 0);
    }
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(GENMIP_2D_BLOCK_SIZE_X, GENMIP_2D_BLOCK_SIZE_Y, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    // 출력 텍셀의 좌표 (z 는 큐브맵 페이스 인덱스)
    uint3 dstCoord = dispatchThreadId.xyz;

    // 출력 좌표가 출력 크기를 넘어가면 조기 종료
    if (dstCoord.x >= mipGenParams.dstSize.x || dstCoord.y >= mipGenParams.dstSize.y) {
        return;
    }

    // 텍셀의 중앙값으로 uv 를 계산하여 2x2 블럭의 중심으로 선형 샘플링을 한다.
    float2 uv = ((float2)dstCoord.xy + 0.5) * mipGenParams.dstSizeRcp.xy;
    float3 n = ToCubeMapCoords(dstCoord.z, uv);

    outputTexture[dstCoord] = inputTexture.SampleLevel(linearSampler, n, 0);
}
