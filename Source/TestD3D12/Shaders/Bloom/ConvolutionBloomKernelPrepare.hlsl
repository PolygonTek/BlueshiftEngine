#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

// 커널 소스 이미지
Texture2D<float4> kernelSourceTexture : register(t0);

// 커널 스펙트럼 버퍼
RWStructuredBuffer<float2> kernelSpectrumBuffer : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 outputCoord = dispatchThreadId.xy;
    if (outputCoord.x >= bloomParams.outputSize.x || outputCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    uint outputPixelCount = bloomParams.outputSize.x * bloomParams.outputSize.y;
    uint outputIndex = outputCoord.y * bloomParams.outputSize.x + outputCoord.x;

    // 출력 버퍼 시작이 커널 center 가 되도록 circular shift 된 좌표
    uint2 shiftedCoord = (outputCoord + bloomParams.outputSize / 2) % bloomParams.outputSize;

    // 회전된 kernel 을 만들기 위해 sourceCoord 를 kernel center 기준으로 역방향 회전한다.
    float2 sourceCoord = float2(shiftedCoord) + 0.5;
    float2 halfSize = bloomParams.outputSize * 0.5;
 
    // content padding 으로 줄어든 만큼 kernel 도 줄여 resolve 후 screen-space radius 가 커지지 않도록 한다.
    float2 kernelSpatialScale = max(BP_BLOOM_KERNEL_SPATIAL_SCALE, float2(0.0001, 0.0001));
    float2 centeredCoord = (sourceCoord - halfSize) / kernelSpatialScale;
    float2 rotatedCoord = float2(
        centeredCoord.x * BP_BLOOM_KERNEL_ROTATION_COS + centeredCoord.y * BP_BLOOM_KERNEL_ROTATION_SIN,
        centeredCoord.y * BP_BLOOM_KERNEL_ROTATION_COS - centeredCoord.x * BP_BLOOM_KERNEL_ROTATION_SIN) + halfSize;

    // 회전된 좌표가 outputSize 안에 들어왔을 때만 샘플링하고, 그게 아니면 0 으로 채운다.
    float3 color = float3(0.0, 0.0, 0.0);
    if (all(rotatedCoord >= 0.0) && all(rotatedCoord < bloomParams.outputSize)) {
        // 출력 버퍼 기준 좌표를 소스 텍스쳐 좌표로 변환
        float2 uv = rotatedCoord * bloomParams.outputTexelSize;
        color = max(kernelSourceTexture.SampleLevel(linearClampSampler, uv, 0).rgb, float3(0.0, 0.0, 0.0));
    }

    // buffer 의 앞쪽에는 rg 배열만 담고, 이후에 b0 배열을 담는다. (패킹된 FFTDualReal 셰이더를 사용하기에 유리)
    kernelSpectrumBuffer[outputIndex] = color.rg;
    kernelSpectrumBuffer[outputPixelCount + outputIndex] = float2(color.b, 0.0);
}
