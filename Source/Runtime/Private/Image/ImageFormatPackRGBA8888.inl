#include "Precompiled.h"
#include "Math/MathCommon.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
//
// RGBA8888ToXXX (packing function from rgba8888 to custom format)
//
//--------------------------------------------------------------------------------------------------

static void RGBA8888ToL8(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = (byte)(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]);
    }
}

static void RGBA8888ToA8(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~31) * 4;

    for (; srcPtr < simd8End; srcPtr += 128, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 4; group++) {
            const simd8i rgba = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + group * 32));
            values[group] = srl_256epi32(rgba, 24);
        }

        const simd8i packed8 = packus_256epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~15) * 4;

    for (; srcPtr < simd4End; srcPtr += 64, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 4; group++) {
            const simd4i rgba = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + group * 16));
            values[group] = srl_epi32(rgba, 24);
        }

        const simd4i packed8 = packus_epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = srcPtr[3];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = srcPtr[3];
    }
#endif
}

static void RGBA8888ToLA88(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = (byte)(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]);
        dstPtr[1] = srcPtr[3];
    }
}

static void RGBA8888ToR8(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~31) * 4;
    const simd8i byteMask8 = set1_256epi32(0xFF);

    for (; srcPtr < simd8End; srcPtr += 128, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 4; group++) {
            const simd8i rgba = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + group * 32));
            values[group] = rgba & byteMask8;
        }

        const simd8i packed8 = packus_256epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~15) * 4;
    const simd4i byteMask4 = set1_epi32(0xFF);

    for (; srcPtr < simd4End; srcPtr += 64, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 4; group++) {
            const simd4i rgba = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + group * 16));
            values[group] = rgba & byteMask4;
        }

        const simd4i packed8 = packus_epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = srcPtr[0];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = srcPtr[0];
    }
#endif
}

static void RGBA8888ToRG88(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8i rgMask8 = set1_256epi32(0xFFFF);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 32) {
        const simd8i rgba0 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + 0));
        const simd8i rgba1 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + 32));
        const simd8i rg16 = packus_256epi32_to_epu16(rgba0 & rgMask8, rgba1 & rgMask8);

        storeu_si256(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4i rgMask4 = set1_epi32(0xFFFF);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 16) {
        const simd4i rgba0 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + 0));
        const simd4i rgba1 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + 16));
        const simd4i rg16 = packus_epi32_to_epu16(rgba0 & rgMask4, rgba1 & rgMask4);

        storeu_si128(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
    }
#endif
}

static void RGBA8888ToRGB888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
    }
}

static void RGBA8888ToBGR888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
    }
}

static void RGBA8888ToRGBX8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8i rgbMask8 = set1_256epi32(0x00FFFFFF);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i rgbx8 = rgba8 & rgbMask8;

        storeu_si256(rgbx8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4i rgbMask4 = set1_epi32(0x00FFFFFF);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i rgbx8 = rgba8 & rgbMask4;

        storeu_si128(rgbx8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 0;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 0;
    }
#endif
}

static void RGBA8888ToBGRX8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8i greenMask8 = set1_256epi32(0x0000FF00);
    const simd8i highRBMask8 = set1_256epi32(0x00FF0000);
    const simd8i lowRBMask8 = set1_256epi32(0x000000FF);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i bgrx8 = (rgba8 & greenMask8) |
            srl_256epi32(rgba8 & highRBMask8, 16) |
            ((rgba8 & lowRBMask8) << 16);

        storeu_si256(bgrx8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4i greenMask4 = set1_epi32(0x0000FF00);
    const simd4i highRBMask4 = set1_epi32(0x00FF0000);
    const simd4i lowRBMask4 = set1_epi32(0x000000FF);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i bgrx8 = (rgba8 & greenMask4) |
            srl_epi32(rgba8 & highRBMask4, 16) |
            ((rgba8 & lowRBMask4) << 16);

        storeu_si128(bgrx8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 0;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 0;
    }
#endif
}

static void RGBA8888ToBGRA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8i keepMask8 = set1_256epi32(static_cast<int32_t>(0xFF00FF00u));
    const simd8i highRBMask8 = set1_256epi32(0x00FF0000);
    const simd8i lowRBMask8 = set1_256epi32(0x000000FF);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i bgra8 = (rgba8 & keepMask8) |
            srl_256epi32(rgba8 & highRBMask8, 16) |
            ((rgba8 & lowRBMask8) << 16);

        storeu_si256(bgra8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4i keepMask4 = set1_epi32(static_cast<int32_t>(0xFF00FF00u));
    const simd4i highRBMask4 = set1_epi32(0x00FF0000);
    const simd4i lowRBMask4 = set1_epi32(0x000000FF);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i bgra8 = (rgba8 & keepMask4) |
            srl_epi32(rgba8 & highRBMask4, 16) |
            ((rgba8 & lowRBMask4) << 16);

        storeu_si128(bgra8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[3];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[3];
    }
#endif
}

static void RGBA8888ToABGR8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8i byteMask8 = set1_256epi32(0x00FF00FF);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i swapped8 = ((rgba8 & byteMask8) << 8) | (srl_256epi32(rgba8, 8) & byteMask8);
        const simd8i abgr8 = (swapped8 << 16) | srl_256epi32(swapped8, 16);

        storeu_si256(abgr8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4i byteMask4 = set1_epi32(0x00FF00FF);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i swapped8 = ((rgba8 & byteMask4) << 8) | (srl_epi32(rgba8, 8) & byteMask4);
        const simd4i abgr8 = (swapped8 << 16) | srl_epi32(swapped8, 16);

        storeu_si128(abgr8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[3];
        dstPtr[1] = srcPtr[2];
        dstPtr[2] = srcPtr[1];
        dstPtr[3] = srcPtr[0];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[3];
        dstPtr[1] = srcPtr[2];
        dstPtr[2] = srcPtr[1];
        dstPtr[3] = srcPtr[0];
    }
#endif
}

static void RGBA8888ToARGB8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i argb8 = (rgba8 << 8) | srl_256epi32(rgba8, 24);

        storeu_si256(argb8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i argb8 = (rgba8 << 8) | srl_epi32(rgba8, 24);

        storeu_si128(argb8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[3];
        dstPtr[1] = srcPtr[0];
        dstPtr[2] = srcPtr[1];
        dstPtr[3] = srcPtr[2];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[3];
        dstPtr[1] = srcPtr[0];
        dstPtr[2] = srcPtr[1];
        dstPtr[3] = srcPtr[2];
    }
#endif
}

static void RGBA8888ToRGBX4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[2] >> 4) << 8);
    }
}

static void RGBA8888ToBGRX4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[0] >> 4) << 8);
    }
}

static void RGBA8888ToRGBA4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~15) * 4;

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 16) {
        const simd8i rgba8_0 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + 0));
        const simd8i rgba8_1 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + 32));

        const simd8i packed32_0 =
            ((rgba8_0 >> 4) & 0x000F) |
            ((rgba8_0 >> 8) & 0x00F0) |
            ((rgba8_0 >> 12) & 0x0F00) |
            ((rgba8_0 >> 16) & 0xF000);
        const simd8i packed32_1 =
            ((rgba8_1 >> 4) & 0x000F) |
            ((rgba8_1 >> 8) & 0x00F0) |
            ((rgba8_1 >> 12) & 0x0F00) |
            ((rgba8_1 >> 16) & 0xF000);

        const simd8i packed16 = packus_256epi32_to_epu16(packed32_0, packed32_1);
        storeu_si256(packed16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~7) * 4;

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 8) {
        const simd4i rgba8_0 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + 0));
        const simd4i rgba8_1 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + 16));

        const simd4i packed32_0 =
            ((rgba8_0 >> 4) & 0x000F) |
            ((rgba8_0 >> 8) & 0x00F0) |
            ((rgba8_0 >> 12) & 0x0F00) |
            ((rgba8_0 >> 16) & 0xF000);
        const simd4i packed32_1 =
            ((rgba8_1 >> 4) & 0x000F) |
            ((rgba8_1 >> 8) & 0x00F0) |
            ((rgba8_1 >> 12) & 0x0F00) |
            ((rgba8_1 >> 16) & 0xF000);

        const simd4i packed16 = packus_epi32_to_epu16(packed32_0, packed32_1);
        storeu_si128(packed16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[2] >> 4) << 8) | ((srcPtr[3] >> 4) << 12);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[2] >> 4) << 8) | ((srcPtr[3] >> 4) << 12);
    }
#endif
}

static void RGBA8888ToBGRA4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[0] >> 4) << 8) | ((srcPtr[3] >> 4) << 12);
    }
}

static void RGBA8888ToABGR4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[3] >> 4) | ((srcPtr[2] >> 4) << 4) | ((srcPtr[1] >> 4) << 8) | ((srcPtr[0] >> 4) << 12);
    }
}

static void RGBA8888ToARGB4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[3] >> 4) | ((srcPtr[0] >> 4) << 4) | ((srcPtr[1] >> 4) << 8) | ((srcPtr[2] >> 4) << 12);
    }
}

static void RGBA8888ToRGBX5551(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[2] >> 3) << 10);
    }
}

static void RGBA8888ToBGRX5551(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[0] >> 3) << 10);
    }
}

static void RGBA8888ToRGBA5551(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~15) * 4;

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 16) {
        const simd8i rgba8_0 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + 0));
        const simd8i rgba8_1 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + 32));

        const simd8i packed32_0 =
            ((rgba8_0 >> 3) & 0x001F) |
            ((rgba8_0 >> 6) & 0x03E0) |
            ((rgba8_0 >> 9) & 0x7C00) |
            ((rgba8_0 >> 16) & 0x8000);
        const simd8i packed32_1 =
            ((rgba8_1 >> 3) & 0x001F) |
            ((rgba8_1 >> 6) & 0x03E0) |
            ((rgba8_1 >> 9) & 0x7C00) |
            ((rgba8_1 >> 16) & 0x8000);

        const simd8i packed16 = packus_256epi32_to_epu16(packed32_0, packed32_1);
        storeu_si256(packed16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~7) * 4;

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 8) {
        const simd4i rgba8_0 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + 0));
        const simd4i rgba8_1 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + 16));

        const simd4i packed32_0 =
            ((rgba8_0 >> 3) & 0x001F) |
            ((rgba8_0 >> 6) & 0x03E0) |
            ((rgba8_0 >> 9) & 0x7C00) |
            ((rgba8_0 >> 16) & 0x8000);
        const simd4i packed32_1 =
            ((rgba8_1 >> 3) & 0x001F) |
            ((rgba8_1 >> 6) & 0x03E0) |
            ((rgba8_1 >> 9) & 0x7C00) |
            ((rgba8_1 >> 16) & 0x8000);

        const simd4i packed16 = packus_epi32_to_epu16(packed32_0, packed32_1);
        storeu_si128(packed16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[2] >> 3) << 10) | ((srcPtr[3] >> 7) << 15);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[2] >> 3) << 10) | ((srcPtr[3] >> 7) << 15);
    }
#endif
}

static void RGBA8888ToBGRA5551(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[0] >> 3) << 10) | ((srcPtr[3] >> 7) << 15);
    }
}

static void RGBA8888ToABGR1555(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[3] >> 7) | ((srcPtr[2] >> 3) << 1) | ((srcPtr[1] >> 3) << 6) | ((srcPtr[0] >> 3) << 11);
    }
}

static void RGBA8888ToARGB1555(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[3] >> 7) | ((srcPtr[0] >> 3) << 1) | ((srcPtr[1] >> 3) << 6) | ((srcPtr[2] >> 3) << 11);
    }
}

static void RGBA8888ToRGB565(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~15) * 4;

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 16) {
        const simd8i rgba8_0 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + 0));
        const simd8i rgba8_1 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr + 32));

        const simd8i packed32_0 =
            ((rgba8_0 >> 3) & 0x001F) |
            ((rgba8_0 >> 5) & 0x07E0) |
            ((rgba8_0 >> 8) & 0xF800);
        const simd8i packed32_1 =
            ((rgba8_1 >> 3) & 0x001F) |
            ((rgba8_1 >> 5) & 0x07E0) |
            ((rgba8_1 >> 8) & 0xF800);

        const simd8i packed16 = packus_256epi32_to_epu16(packed32_0, packed32_1);
        storeu_si256(packed16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~7) * 4;

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 8) {
        const simd4i rgba8_0 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + 0));
        const simd4i rgba8_1 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr + 16));

        const simd4i packed32_0 =
            ((rgba8_0 >> 3) & 0x001F) |
            ((rgba8_0 >> 5) & 0x07E0) |
            ((rgba8_0 >> 8) & 0xF800);
        const simd4i packed32_1 =
            ((rgba8_1 >> 3) & 0x001F) |
            ((rgba8_1 >> 5) & 0x07E0) |
            ((rgba8_1 >> 8) & 0xF800);

        const simd4i packed16 = packus_epi32_to_epu16(packed32_0, packed32_1);
        storeu_si128(packed16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 3) | ((srcPtr[1] >> 2) << 5) | ((srcPtr[2] >> 3) << 11);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 3) | ((srcPtr[1] >> 2) << 5) | ((srcPtr[2] >> 3) << 11);
    }
#endif
}

static void RGBA8888ToBGR565(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 3) | ((srcPtr[1] >> 2) << 5) | ((srcPtr[0] >> 3) << 11);
    }
}

static void RGBA8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(invNorm);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rgba32_0;
        simd8i rgba32_1;
        simd8i rgba32_2;
        simd8i rgba32_3;
        unpack_epu8_to_256epi32(rgba8, rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_256ps(epi32_to_256ps(rgba32_0) * scale8, dstPtr + 0);
        storeu_256ps(epi32_to_256ps(rgba32_1) * scale8, dstPtr + 8);
        storeu_256ps(epi32_to_256ps(rgba32_2) * scale8, dstPtr + 16);
        storeu_256ps(epi32_to_256ps(rgba32_3) * scale8, dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(invNorm);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rgba32_0;
        simd4i rgba32_1;
        simd4i rgba32_2;
        simd4i rgba32_3;
        unpack_epu8_to_epi32(rgba8, rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_ps(epi32_to_ps(rgba32_0) * scale4, dstPtr + 0);
        storeu_ps(epi32_to_ps(rgba32_1) * scale4, dstPtr + 4);
        storeu_ps(epi32_to_ps(rgba32_2) * scale4, dstPtr + 8);
        storeu_ps(epi32_to_ps(rgba32_3) * scale4, dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[2] * invNorm;
        dstPtr[3] = srcPtr[3] * invNorm;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[2] * invNorm;
        dstPtr[3] = srcPtr[3] * invNorm;
    }
#endif
}

BE_NAMESPACE_END
