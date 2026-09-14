#include "../ShaderInterop.h"
#include "../Colors.hlsli"
#include "../Cubemap.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_LINEAR, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

#ifndef MIP_OUTPUT_COMPONENT_COUNT
#define MIP_OUTPUT_COMPONENT_COUNT 4
#endif

#if MIP_OUTPUT_COMPONENT_COUNT != 1 && MIP_OUTPUT_COMPONENT_COUNT != 4
#error Unsupported MIP_OUTPUT_COMPONENT_COUNT.
#endif

cbuffer MipGenCB : register(b0) {
    MipGenParams mipGenParams;
}

RWTexture2DArray<MIP_OUTPUT_FORMAT> outputTexture : register(u0);
TextureCube<MIP_OUTPUT_FORMAT> inputTexture : register(t0);
SamplerState linearSampler : register(s0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(GenMip2DBlockSizeX, GenMip2DBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    // 출력 텍셀의 좌표 (z 는 큐브맵 페이스 인덱스)
    uint3 dstCoord = dispatchThreadId.xyz;

    // 출력 좌표가 출력 크기를 넘어가면 조기 종료
    if (dstCoord.x >= mipGenParams.dstSize.x || dstCoord.y >= mipGenParams.dstSize.y) {
        return;
    }

    // 텍셀의 중앙값으로 uv 를 계산하여 2x2 블럭의 중심으로 선형 샘플링을 한다.
    float2 uv = ((float2)dstCoord.xy + 0.5) * mipGenParams.dstSizeRcp.xy;
    float3 direction = CubeMapFaceToDirection(dstCoord.z, uv);

#if MIP_OUTPUT_COMPONENT_COUNT == 1
    float value = inputTexture.SampleLevel(linearSampler, direction, 0);
    if (mipGenParams.flags & MipGenOptionBitSRGB) {
        value = LinearToGammaApprox(value.xxx).x;
    }
    outputTexture[dstCoord] = value;
#else
    float4 color = inputTexture.SampleLevel(linearSampler, direction, 0);

    // sRGB SRV 에서 선형화된 RGB 를 UNorm UAV 에 저장하기 전에 다시 인코딩한다.
    if (mipGenParams.flags & MipGenOptionBitSRGB) {
        color.rgb = LinearToGammaApprox(color.rgb);
    }
    outputTexture[dstCoord] = color;
#endif
}
