#ifndef __LIGHT_CULLING_COMMON_HLSLI__
#define __LIGHT_CULLING_COMMON_HLSLI__

#include "../ShaderInterop.h"
#include "../SurfaceDrawCommon.hlsli"

#ifndef SHADER_LIGHT_USE_WAVE_OPS
#define SHADER_LIGHT_USE_WAVE_OPS 0
#endif

struct ShaderLightIterator {
    uint tileWordBaseIndex;
    uint nextWordIndex;
    uint endWordIndex;
    uint currentWordFirstLightIndex;
    uint minLightIndex;
    uint maxLightIndex;
    uint lightMask;
};

float LinearizeLightCullingDepth(float ndcZ) {
    if ((cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0) {
        return LinearizeOrthogonalDepth(ndcZ, cameraConstants.projectionScaleBiasZ.x, cameraConstants.projectionScaleBiasZ.y);
    }
    return LinearizePerspectiveDepth(ndcZ, cameraConstants.projectionScaleBiasZ.x, cameraConstants.projectionScaleBiasZ.y);
}

uint LoadLightZBin(uint address, bool waveCoherent) {
#if SHADER_LIGHT_USE_WAVE_OPS
    // 모든 Wave 가 같은 ZBin 만 읽는다면 Wave 당 한번만 읽는다.
    if (waveCoherent && WaveActiveAllEqual(address)) {
        uint packedLightRange = 0;
        if (WaveIsFirstLane()) {
            packedLightRange = GetBufferUInt(frameConstants.shaderLightZBinTableIndex)[address];
        }
        return WaveReadLaneFirst(packedLightRange);
    }
#endif
    return GetBufferUInt(frameConstants.shaderLightZBinTableIndex)[address];
}

uint LoadLightTileWord(uint address, bool waveCoherent) {
#if SHADER_LIGHT_USE_WAVE_OPS
    // 모든 Wave 가 같은 mask word 만 읽는다면 Wave 당 한번만 읽는다.
    if (waveCoherent && WaveActiveAllEqual(address)) {
        uint lightMask = 0;
        if (WaveIsFirstLane()) {
            lightMask = GetBufferUInt(frameConstants.shaderLightTileWordTableIndex)[address];
        }
        return WaveReadLaneFirst(lightMask);
    }
#endif
    return GetBufferUInt(frameConstants.shaderLightTileWordTableIndex)[address];
}

ShaderLightIterator BeginShaderLightIteration(uint2 pixelCoord, float ndcZ, bool useZBin, bool waveCoherent) {
    ShaderLightIterator iterator;
    iterator.tileWordBaseIndex = ShaderLightInvalidOffset;
    iterator.nextWordIndex = 0;
    iterator.endWordIndex = 0;
    iterator.currentWordFirstLightIndex = 0;
    iterator.minLightIndex = 0;
    iterator.maxLightIndex = 0;
    iterator.lightMask = 0;

    if (cameraConstants.numLocalLights == 0 || cameraConstants.lightCullingLightRefTableOffset == ShaderLightInvalidOffset) {
        return iterator;
    }

    iterator.maxLightIndex = cameraConstants.numLocalLights - 1;

    // Z-bin 을 사용할 수 있다면, minLightIndex 와 maxLightIndex 를 현재 픽셀의 linear view Z 와 겹치는 라이트 범위로 제한한다.
    if (useZBin && cameraConstants.lightCullingZBinTableOffset != ShaderLightInvalidOffset) {
        const float viewZ = LinearizeLightCullingDepth(ndcZ);
        const float zBin = viewZ * cameraConstants.lightCullingZBinScale + cameraConstants.lightCullingZBinBias;
        const uint zBinIndex = (uint)clamp(zBin, 0.0f, (float)(ShaderLightZBinCount - 1));
        const uint zBinAddress = cameraConstants.lightCullingZBinTableOffset + zBinIndex;
        const uint packedLightRange = LoadLightZBin(zBinAddress, waveCoherent);
        const uint minLightIndex = packedLightRange & ShaderLightZBinIndexMask;
        const uint maxLightIndex = packedLightRange >> 16;

        // Z-bin 이 비어있는 경우
        if (minLightIndex > maxLightIndex || minLightIndex >= cameraConstants.numLocalLights) {
            return iterator;
        }

        iterator.minLightIndex = minLightIndex;
        iterator.maxLightIndex = min(maxLightIndex, cameraConstants.numLocalLights - 1);
    }

    // 나중에 테스트할 mask word index 의 범위를 기록한다.
    iterator.nextWordIndex = iterator.minLightIndex >> ShaderLightWordShift;
    iterator.endWordIndex = (iterator.maxLightIndex >> ShaderLightWordShift) + 1;

    if (cameraConstants.lightCullingTileWordTableOffset != ShaderLightInvalidOffset) {
        const uint2 tileCoord = pixelCoord / ShaderLightTileSize;
        const uint tileIndex = tileCoord.y * cameraConstants.lightCullingTileCountX + tileCoord.x;

        // 현재 픽셀이 속한 타일의 mask word base index 를 기록한다.
        iterator.tileWordBaseIndex = cameraConstants.lightCullingTileWordTableOffset + tileIndex * cameraConstants.lightCullingWordsPerTile;
    }

    return iterator;
}

uint LoadShaderLightMask(ShaderLightIterator iterator, uint wordIndex, bool waveCoherent) {
    const uint firstWordIndex = iterator.minLightIndex >> ShaderLightWordShift;
    if (wordIndex < firstWordIndex || wordIndex >= iterator.endWordIndex) {
        return 0;
    }

    const uint currentWordFirstLightIndex = wordIndex << ShaderLightWordShift;
    uint lightMask = 0xFFFFFFFFu;
    if (iterator.tileWordBaseIndex != ShaderLightInvalidOffset) {
        lightMask = LoadLightTileWord(iterator.tileWordBaseIndex + wordIndex, waveCoherent);
    }

    // 첫번째 mask word 에서 Z-bin 범위 밖의 비트를 제거한다.
    if (currentWordFirstLightIndex < iterator.minLightIndex) {
        lightMask &= 0xFFFFFFFFu << (iterator.minLightIndex - currentWordFirstLightIndex);
    }

    // 마지막 mask word 에서 Z-bin 범위 밖의 비트를 제거한다.
    const uint currentWordLastLightIndex = currentWordFirstLightIndex + ShaderLightWordBits - 1;
    if (currentWordLastLightIndex > iterator.maxLightIndex) {
        const uint validBitCount = iterator.maxLightIndex - currentWordFirstLightIndex + 1;
        lightMask &= (1u << validBitCount) - 1u;
    }

    return lightMask;
}

bool LoadNextShaderLightMask(inout ShaderLightIterator iterator) {
    while (iterator.nextWordIndex < iterator.endWordIndex) {
        const uint wordIndex = iterator.nextWordIndex++;
        iterator.currentWordFirstLightIndex = wordIndex << ShaderLightWordShift;

        iterator.lightMask = LoadShaderLightMask(iterator, wordIndex, false);
        if (iterator.lightMask != 0) {
            return true;
        }
    }

    return false;
}

bool NextShaderLightIndex(inout ShaderLightIterator iterator, out uint lightIndex) {
    lightIndex = 0;

    if (iterator.lightMask == 0 && !LoadNextShaderLightMask(iterator)) {
        return false;
    }

    // lightMask 의 가장 낮은 비트 제거 및 lightIndex 가져오기
    const uint bitIndex = firstbitlow(iterator.lightMask);
    iterator.lightMask &= iterator.lightMask - 1;
    lightIndex = iterator.currentWordFirstLightIndex + bitIndex;
    return true;
}

bool NextShaderLight(inout ShaderLightIterator iterator, out ShaderLightRef shaderLightRef) {
    uint lightIndex;
    if (!NextShaderLightIndex(iterator, lightIndex)) {
        shaderLightRef.packedLightRef = 0;
        shaderLightRef.packedTileMin = 0;
        shaderLightRef.packedTileMax = 0;
        return false;
    }

    shaderLightRef = GetShaderLightRefTable(frameConstants.shaderLightRefTableIndex)[cameraConstants.lightCullingLightRefTableOffset + lightIndex];
    return true;
}

#endif // __LIGHT_CULLING_COMMON_HLSLI__
