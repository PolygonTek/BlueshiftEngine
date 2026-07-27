#ifndef __PARALLAX_MAPPING_HLSLI__
#define __PARALLAX_MAPPING_HLSLI__

// Traces the height field in tangent space and returns the displaced texture coordinates.
float2 ParallaxMapping(Texture2D heightMap, SamplerState heightSampler, float2 texCoord, float heightScale, float3 viewTS, float2 texCoordDx, float2 texCoordDy) {
    if (heightScale <= 0.0f) {
        return texCoord;
    }

    static const float MinLayerCount = 8.0f;
    static const float MaxLayerCount = 20.0f;

    const float layerCount = lerp(MaxLayerCount, MinLayerCount, saturate(abs(viewTS.z)));
    const float layerDepth = rcp(layerCount);

    const float safeViewZ = abs(viewTS.z) > 1e-4f ? viewTS.z : (viewTS.z >= 0.0f ? 1e-4f : -1e-4f);
    const float2 parallaxOffset = heightScale * viewTS.xy / safeViewZ;
    const float2 deltaTexCoord = parallaxOffset / layerCount;

    float2 currentTexCoord = texCoord;
    float currentHeight = heightMap.SampleGrad(heightSampler, currentTexCoord, texCoordDx, texCoordDy).x * 2.0f - 1.0f;
    float currentLayerDepth = -1.0f;

    [loop]
    while (currentHeight >= currentLayerDepth) {
        currentTexCoord += deltaTexCoord * currentHeight;
        currentHeight = heightMap.SampleGrad(heightSampler, currentTexCoord, texCoordDx, texCoordDy).x * 2.0f - 1.0f;
        currentLayerDepth += layerDepth;
    }

    return currentTexCoord;
}

#endif // __PARALLAX_MAPPING_HLSLI__
