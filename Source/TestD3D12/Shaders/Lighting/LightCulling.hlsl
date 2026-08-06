#include "../Common.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer ShaderLightCullingCB : register(b0) {
    ShaderLightCullingParams lightCullingParams;
}

// 라이트 index 로 2D 타일 범위를 나타내는 배열
StructuredBuffer<ShaderLightRef> lightRefs : register(t0);

RWBuffer<uint> lightTileWords : register(u0);

// 각 2D 타일 별로 영향을 받는 라이트들의 index 를 tile mask words 테이블로 추려낸다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(ShaderLightCullingThreadGroupSizeX, ShaderLightCullingThreadGroupSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 tileCoord = dispatchThreadId.xy;
    const uint wordIndex = dispatchThreadId.z;

    if (tileCoord.x >= lightCullingParams.tileCountX || tileCoord.y >= lightCullingParams.tileCountY) {
        return;
    }

    // 비트 마스크 워드 별 첫번째 라이트 인덱스
    const uint firstLightIndex = wordIndex << ShaderLightWordShift;

    uint word = 0;

    [unroll]
    for (uint bitIndex = 0; bitIndex < ShaderLightWordBits; ++bitIndex) {
        const uint lightIndex = firstLightIndex + bitIndex;
        if (lightIndex >= lightCullingParams.numLocalLights) {
            break;
        }

        // 현재 라이트의 타일 범위를 구한다.
        const ShaderLightRef lightRef = lightRefs[lightCullingParams.lightRefTableOffset + lightIndex];
        const uint2 tileMin = uint2(lightRef.packedTileMin & ShaderLightTileCoordMask, lightRef.packedTileMin >> ShaderLightTileCoordBitCount);
        const uint2 tileMax = uint2(lightRef.packedTileMax & ShaderLightTileCoordMask, lightRef.packedTileMax >> ShaderLightTileCoordBitCount);

        // 현재 타일 안에 라이트가 들어와 있다면, bitmask word 에 비트를 추가한다.
        if (tileCoord.x >= tileMin.x && tileCoord.y >= tileMin.y && tileCoord.x <= tileMax.x && tileCoord.y <= tileMax.y) {
            word |= 1u << bitIndex;
        }
    }

    // (tileCoord, wordIndex) -> tileWordIndex
    const uint tileIndex = lightCullingParams.tileCountX * tileCoord.y + tileCoord.x;
    const uint tileWordIndex = lightCullingParams.tileWordTableOffset + tileIndex * lightCullingParams.wordsPerTile + wordIndex;
    lightTileWords[tileWordIndex] = word;
}
