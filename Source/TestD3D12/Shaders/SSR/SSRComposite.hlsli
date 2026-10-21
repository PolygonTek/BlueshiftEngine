#ifndef __SSR_COMPOSITE_HLSLI__
#define __SSR_COMPOSITE_HLSLI__

#include "SSRRadiance.hlsli"

float3 CompositeSSRRadiance(uint2 pixelCoord, float3 environmentRadiance, uint reflectionTextureIndex) {
    const Texture2D<float4> reflectionTexture = GetTexture2D(reflectionTextureIndex);
    const float4 reflection = reflectionTexture.Load(int3(pixelCoord, 0));
    const float3 premulRadiance = DecompressSSRRadiance(reflection.rgb);
    const float confidence = saturate(reflection.a);
    return environmentRadiance * (1.0f - confidence) + premulRadiance;
}

// 별도 prepass 에 기록된 가장 가까운 transparent surface 만 자신의 SSR 결과를 사용한다.
// 뒤쪽 transparent layer 는 환경 반사를 유지해 서로 다른 표면의 radiance 가 섞이지 않게 한다.
float3 CompositeTransparentSSRRadiance(uint2 pixelCoord, float depth, float3 environmentRadiance, uint reflectionTextureIndex, uint depthTextureIndex) {
    const Texture2D<float4> depthTexture = GetTexture2D(depthTextureIndex);
    const float receiverDepth = depthTexture.Load(int3(pixelCoord, 0)).r;
    if (abs(receiverDepth - depth) > 1.0e-5f) {
        return environmentRadiance;
    }
    return CompositeSSRRadiance(pixelCoord, environmentRadiance, reflectionTextureIndex);
}

#endif // __SSR_COMPOSITE_HLSLI__
