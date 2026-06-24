#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

// IFFT 결과 버퍼는 RG plane, B0 plane 이 연속으로 배치되어 있다.
StructuredBuffer<float2> bloomSpatialBuffer : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

float3 LoadBloomSpatial(uint2 coord) {
    uint pixelCount = bloomParams.inputSize.x * bloomParams.inputSize.y;
    uint index = coord.y * bloomParams.inputSize.x + coord.x;

    float2 rg = bloomSpatialBuffer[index];
    float b = bloomSpatialBuffer[pixelCount + index].x;
    return float3(rg, b);
}

float3 SampleBloomSpatial(float2 coord) {
    float2 maxCoord = float2(bloomParams.inputSize - 1);
    float2 clampedCoord = clamp(coord, 0.0, maxCoord);
    uint2 coord0 = uint2(floor(clampedCoord));
    uint2 coord1 = min(coord0 + 1, bloomParams.inputSize - 1);
    float2 f = clampedCoord - float2(coord0);

    float3 c00 = LoadBloomSpatial(coord0);
    float3 c10 = LoadBloomSpatial(uint2(coord1.x, coord0.y));
    float3 c01 = LoadBloomSpatial(uint2(coord0.x, coord1.y));
    float3 c11 = LoadBloomSpatial(coord1);

    return lerp(lerp(c00, c10, f.x), lerp(c01, c11, f.x), f.y);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= bloomParams.outputSize.x || pixelCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    float2 screenUv = (float2(pixelCoord) + 0.5) * bloomParams.outputTexelSize;
    float2 spatialCoord = BP_BLOOM_CONTENT_OFFSET + screenUv * BP_BLOOM_CONTENT_SIZE - 0.5;
    float3 color = max(SampleBloomSpatial(spatialCoord), 0.0);

    outputTexture[pixelCoord] = float4(color, 1.0);
}
