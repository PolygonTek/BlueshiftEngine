#ifndef __LIGHT_CULLING_DEBUG_HLSLI__
#define __LIGHT_CULLING_DEBUG_HLSLI__

#include "LightCullingCommon.hlsli"

uint CountShaderLights(uint2 pixelCoord, float ndcDepth, bool useZBin) {
    ShaderLightIterator lightIterator = BeginShaderLightIteration(pixelCoord, ndcDepth, useZBin, false);
    uint lightCount = 0;

    while (LoadNextShaderLightMask(lightIterator)) {
        lightCount += countbits(lightIterator.lightMask);
        lightIterator.lightMask = 0;
    }

    return lightCount;
}

float3 LightCountToHeatMap(uint lightCount) {
    if (lightCount == 0) {
        return 0.0f;
    }

    // 1 ~ 64 개를 blue ~ red 로 로그 스케일 표시하고, 64 개 이상은 red 로 포화시킨다.
    const float normalizedCount = saturate(log2((float)lightCount + 1.0f) / 6.0f);
    return saturate(1.5f - abs(4.0f * normalizedCount - float3(3.0f, 2.0f, 1.0f)));
}

float3 VisualizeLightCulling(uint2 pixelCoord, float ndcDepth, uint debugMode) {
    const bool useZBin = debugMode == ShaderLightCullingDebugModeTileMaskAndZBin;
    return LightCountToHeatMap(CountShaderLights(pixelCoord, ndcDepth, useZBin));
}

#endif // __LIGHT_CULLING_DEBUG_HLSLI__
