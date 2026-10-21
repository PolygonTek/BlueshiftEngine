#ifndef SSR_RESOLUTION_HLSLI
#define SSR_RESOLUTION_HLSLI

#ifndef SSR_RESOLUTION_DIVISOR
#define SSR_RESOLUTION_DIVISOR 2
#endif

#if SSR_RESOLUTION_DIVISOR != 1 && SSR_RESOLUTION_DIVISOR != 2 && SSR_RESOLUTION_DIVISOR != 4
#error SSR_RESOLUTION_DIVISOR must be 1, 2, or 4
#endif

#define SSR_BLOCK_PIXEL_COUNT (SSR_RESOLUTION_DIVISOR * SSR_RESOLUTION_DIVISOR)

float EncodeSSRBlockPixelIndex(uint blockPixelIndex) {
#if SSR_RESOLUTION_DIVISOR == 1
    return 0.0f;
#else
    return float(blockPixelIndex) * (1.0f / float(SSR_BLOCK_PIXEL_COUNT - 1));
#endif
}

uint2 DecodeSSRBlockPixelOffset(float encodedBlockPixelIndex) {
#if SSR_RESOLUTION_DIVISOR == 1
    return uint2(0, 0);
#else
    // 복원 오차로 인덱스가 작아지지 않도록 정수 변환 전에 0.5f 를 더해 반올림한다.
    const uint blockPixelIndex = min(uint(encodedBlockPixelIndex * float(SSR_BLOCK_PIXEL_COUNT - 1) + 0.5f), uint(SSR_BLOCK_PIXEL_COUNT - 1));
    return uint2(blockPixelIndex % SSR_RESOLUTION_DIVISOR, blockPixelIndex / SSR_RESOLUTION_DIVISOR);
#endif
}

#endif
