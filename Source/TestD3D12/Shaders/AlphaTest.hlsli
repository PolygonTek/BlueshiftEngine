#ifndef __ALPHA_TEST_HLSLI__
#define __ALPHA_TEST_HLSLI__

float GetBayer8x8Threshold(uint2 pixelCoord, uint patternSeed) {
    const uint2 p = (pixelCoord + uint2(patternSeed, patternSeed >> 3u)) & 7u;
    const uint x = p.x ^ p.y;
    const uint rank = ((x & 1u) << 5u) | ((p.y & 1u) << 4u) | ((x & 2u) << 2u) | ((p.y & 2u) << 1u) | ((x & 4u) >> 1u) | ((p.y & 4u) >> 2u);
    return (float(rank) + 0.5f) * (1.0f / 64.0f);
}

uint GetSurfaceDitherPatternSeed(uint surfaceFlags) {
    return (surfaceFlags >> ShaderSurfaceDitherPatternShift) & ShaderSurfaceDitherPatternMask;
}

float GetSurfaceFadeOpacity(uint surfaceFlags) {
    return float(surfaceFlags >> ShaderSurfaceFadeOpacityShift) * (1.0f / 65535.0f);
}

void ApplyDitherFade(float fadeOpacity, uint2 pixelCoord, uint patternSeed) {
    clip(fadeOpacity - GetBayer8x8Threshold(pixelCoord, patternSeed));
}

// patternSeed 는 호출부에서 재질 디더링 또는 오브젝트 페이드에 맞게 선택한다.
void ApplyAlphaTest(float alpha, float alphaCutoff, float fadeOpacity, uint2 pixelCoord, uint patternSeed) {
#ifdef PERFORATED
    clip(alpha - alphaCutoff);
#endif

#if defined(DITHERED) || defined(DITHER_FADE)
    float coverage = 1.0f;
    #ifdef DITHERED
        coverage = saturate(alpha);
    #endif

    #ifdef DITHER_FADE
        coverage *= fadeOpacity;
    #endif

    clip(coverage - GetBayer8x8Threshold(pixelCoord, patternSeed));
#endif
}

#endif // __ALPHA_TEST_HLSLI__
