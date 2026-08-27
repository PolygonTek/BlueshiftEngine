#include "../ShaderInterop.h"
#include "../Colors.hlsli"

#define GEN_MIP_WITH_GATHER

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "RootConstants(num32BitConstants=8, b0), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_POINT, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

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

#ifdef MIP_TEXTURE_ARRAY
RWTexture2DArray<MIP_OUTPUT_FORMAT> outputTexture : register(u0);
Texture2DArray<MIP_OUTPUT_FORMAT> inputTexture : register(t0);
#else
RWTexture2D<MIP_OUTPUT_FORMAT> outputTexture : register(u0);
Texture2D<MIP_OUTPUT_FORMAT> inputTexture : register(t0);
#endif
SamplerState pointSampler : register(s0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(GenMip2DBlockSizeX, GenMip2DBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    // 출력 텍셀의 좌표
#ifdef MIP_TEXTURE_ARRAY
    uint3 dstCoord = dispatchThreadId;
#else
    uint2 dstCoord = dispatchThreadId.xy;
#endif

    // 출력 좌표가 출력 크기를 넘어가면 조기 종료
    if (dstCoord.x >= mipGenParams.dstSize.x || dstCoord.y >= mipGenParams.dstSize.y
#ifdef MIP_TEXTURE_ARRAY
        || dstCoord.z >= mipGenParams.dstSize.z
#endif
    ) {
        return;
    }

    // 상위 Mip 레벨에서의 픽셀의 평균값을 계산한다.
#if MIP_OUTPUT_COMPONENT_COUNT == 1
#ifdef GEN_MIP_WITH_GATHER
#ifdef MIP_TEXTURE_ARRAY
    float3 uv = float3(((float2)dstCoord.xy + 0.5) * mipGenParams.dstSizeRcp.xy, dstCoord.z);
#else
    float2 uv = ((float2)dstCoord.xy + 0.5) * mipGenParams.dstSizeRcp.xy;
#endif
    float4 values = inputTexture.GatherRed(pointSampler, uv);
    float value = dot(values, 0.25f);
#else
    uint2 srcCoord = dstCoord.xy * 2;
#ifdef MIP_TEXTURE_ARRAY
    float value = inputTexture.Load(int4(srcCoord, dstCoord.z, 0));
    value += inputTexture.Load(int4(srcCoord + uint2(1, 0), dstCoord.z, 0));
    value += inputTexture.Load(int4(srcCoord + uint2(0, 1), dstCoord.z, 0));
    value += inputTexture.Load(int4(srcCoord + uint2(1, 1), dstCoord.z, 0));
#else
    float value = inputTexture.Load(int3(srcCoord, 0));
    value += inputTexture.Load(int3(srcCoord + uint2(1, 0), 0));
    value += inputTexture.Load(int3(srcCoord + uint2(0, 1), 0));
    value += inputTexture.Load(int3(srcCoord + uint2(1, 1), 0));
#endif
    value *= 0.25f;
#endif

    if (mipGenParams.flags & MipGenOptionBitSRGB) {
        value = LinearToGammaApprox(value.xxx).x;
    }

    outputTexture[dstCoord] = value;
#else
#ifdef GEN_MIP_WITH_GATHER
    // UV 좌표 계산 (픽셀 중앙값)
#ifdef MIP_TEXTURE_ARRAY
    float3 uv = float3(((float2)dstCoord.xy + 0.5) * mipGenParams.dstSizeRcp.xy, dstCoord.z);
#else
    float2 uv = ((float2)dstCoord.xy + 0.5) * mipGenParams.dstSizeRcp.xy;
#endif

    // Gather 를 이용해서 2x2 블록 픽셀의 채널 값 샘플링
    // Gather 는 한번에 인접 2x2 픽셀 데이터를 읽어올 때 캐시를 이용하기 때문에 Load 보다 효율적이다.
    // inputTexture 가 sRGB 포맷이라면 읽어올 때 자동으로 선형화된다.
    float4 rrrr = inputTexture.GatherRed(pointSampler, uv);
    float4 gggg = inputTexture.GatherGreen(pointSampler, uv);
    float4 bbbb = inputTexture.GatherBlue(pointSampler, uv);
    float4 aaaa = inputTexture.GatherAlpha(pointSampler, uv);

    float4 color0 = float4(rrrr.x, gggg.x, bbbb.x, aaaa.x);
    float4 color1 = float4(rrrr.y, gggg.y, bbbb.y, aaaa.y);
    float4 color2 = float4(rrrr.z, gggg.z, bbbb.z, aaaa.z);
    float4 color3 = float4(rrrr.w, gggg.w, bbbb.w, aaaa.w);
#else
    uint2 srcCoord = dstCoord.xy * 2;

    // Load 를 이용해서 2x2 블록 픽셀 샘플링
#ifdef MIP_TEXTURE_ARRAY
    float4 color0 = inputTexture.Load(int4(srcCoord, dstCoord.z, 0));
    float4 color1 = inputTexture.Load(int4(srcCoord + uint2(1, 0), dstCoord.z, 0));
    float4 color2 = inputTexture.Load(int4(srcCoord + uint2(0, 1), dstCoord.z, 0));
    float4 color3 = inputTexture.Load(int4(srcCoord + uint2(1, 1), dstCoord.z, 0));
#else
    float4 color0 = inputTexture.Load(int3(srcCoord, 0));
    float4 color1 = inputTexture.Load(int3(srcCoord + uint2(1, 0), 0));
    float4 color2 = inputTexture.Load(int3(srcCoord + uint2(0, 1), 0));
    float4 color3 = inputTexture.Load(int3(srcCoord + uint2(1, 1), 0));
#endif
#endif

    float4 color = 0;
    float alphaSum = color0.a + color1.a + color2.a + color3.a;

    if ((mipGenParams.flags & MipGenOptionBitPreserveCoverage) && alphaSum > 0) {
        color.rgb += color0.rgb * color0.a;
        color.rgb += color1.rgb * color1.a;
        color.rgb += color2.rgb * color2.a;
        color.rgb += color3.rgb * color3.a;
        color.rgb /= alphaSum;
        color.a = max(color0.a, max(color1.a, max(color2.a, color3.a)));
    } else {
        color = (color0 + color1 + color2 + color3) * 0.25;
    }

    // 텍스쳐가 sRGB 포맷이라면 sRGB 값으로 변환한다.
    if (mipGenParams.flags & MipGenOptionBitSRGB) {
        color.rgb = LinearToGammaApprox(color.rgb);
    }

    // 출력 텍스쳐에 저장
    outputTexture[dstCoord] = color;
#endif
}
