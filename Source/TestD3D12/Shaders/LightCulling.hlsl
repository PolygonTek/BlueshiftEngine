#include "Common.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer ShaderLightCullingCB : register(b0) {
    ShaderLightCullingParams lightCullingParams;
}

StructuredBuffer<ShaderLightRef> lightRefs : register(t0);

// 타일 좌표와 light word 인덱스에 대응하는 word-local bit mask 배열
RWBuffer<uint> lightTileMasks : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(ShaderLightCullingThreadGroupSizeX, ShaderLightCullingThreadGroupSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 tileCoord = dispatchThreadId.xy;
    const uint maskWordIndex = dispatchThreadId.z;

    if (tileCoord.x >= lightCullingParams.lightTileCountX || tileCoord.y >= lightCullingParams.lightTileCountY) {
        return;
    }

    // 비트 마스크 워드 별 첫번째 라이트 인덱스
    const uint firstLightIndex = maskWordIndex << ShaderLightMaskWordShift;

    uint lightMask = 0;

    [unroll]
    for (uint bitIndex = 0; bitIndex < ShaderLightMaskWordBits; ++bitIndex) {
        const uint lightIndex = firstLightIndex + bitIndex;
        if (lightIndex >= lightCullingParams.numLights) {
            break;
        }

        // 현재 라이트 참조
        const ShaderLightRef lightRef = lightRefs[lightCullingParams.lightRefOffset + lightIndex];
        const uint2 tileMin = uint2(lightRef.packedTileMin & ShaderLightTileCoordMask, lightRef.packedTileMin >> ShaderLightTileCoordBitCount);
        const uint2 tileMax = uint2(lightRef.packedTileMax & ShaderLightTileCoordMask, lightRef.packedTileMax >> ShaderLightTileCoordBitCount);

        // 현재 2D 타일 안에 라이트가 들어와 있다면, 라이트 비트 마스크에 비트를 추가한다.
        if (tileCoord.x >= tileMin.x && tileCoord.y >= tileMin.y && tileCoord.x <= tileMax.x && tileCoord.y <= tileMax.y) {
            lightMask |= 1u << bitIndex;
        }
    }

    // (tileCoord, maskWordIndex) -> tileMaskIndex
    const uint tileIndex = lightCullingParams.lightTileCountX * tileCoord.y + tileCoord.x;
    const uint tileMaskIndex = lightCullingParams.lightTileMaskOffset + tileIndex * lightCullingParams.lightMaskWordCount + maskWordIndex;
    lightTileMasks[tileMaskIndex] = lightMask;
}
