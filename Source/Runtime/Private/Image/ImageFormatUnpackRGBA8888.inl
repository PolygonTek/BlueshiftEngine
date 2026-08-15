#include "Precompiled.h"
#include "Math/MathCommon.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
//
// XXXToRGBA8888 (unpacking function from custom format to rgba8888)
//
//--------------------------------------------------------------------------------------------------

static void L8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~31);
    const simd8i alphaMask8 = set1_256epi32(static_cast<int32_t>(0xFF000000u));
    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 128) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epu8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8i l = values[group];
            const simd8i rgba8 = l | (l << 8) | (l << 16) | alphaMask8;
            storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 32));
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const byte *simd4End = srcPtr + (remainingPixels & ~15);
    const simd4i alphaMask4 = set1_epi32(static_cast<int32_t>(0xFF000000u));
    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 64) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epu8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd4i l = values[group];
            const simd4i rgba8 = l | (l << 8) | (l << 16) | alphaMask4;
            storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 16));
        }
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[0];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 255;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[0];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 255;
    }
#endif
}

static void A8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~31);
    const simd8i rgbMask8 = set1_256epi32(0x00FFFFFF);
    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 128) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epu8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8i rgba8 = rgbMask8 | (values[group] << 24);
            storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 32));
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const byte *simd4End = srcPtr + (remainingPixels & ~15);
    const simd4i rgbMask4 = set1_epi32(0x00FFFFFF);
    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 64) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epu8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd4i rgba8 = rgbMask4 | (values[group] << 24);
            storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 16));
        }
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = 255;
        dstPtr[1] = 255;
        dstPtr[2] = 255;
        dstPtr[3] = srcPtr[0];
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = 255;
        dstPtr[1] = 255;
        dstPtr[2] = 255;
        dstPtr[3] = srcPtr[0];
    }
#endif
}

static void LA88ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 2;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~15) * 2;
    const simd8i byteMask8 = set1_256epi32(0xFF);
    const simd8i alphaMask8 = set1_256epi32(0xFF00);
    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 64) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[2];
        unpack_epu16_to_256epi32(packed8, values[0], values[1]);

        for (int group = 0; group < 2; group++) {
            const simd8i l = values[group] & byteMask8;
            const simd8i rgba8 = l | (l << 8) | (l << 16) | ((values[group] & alphaMask8) << 16);
            storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 32));
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const byte *simd4End = srcPtr + (remainingPixels & ~7) * 2;
    const simd4i byteMask4 = set1_epi32(0xFF);
    const simd4i alphaMask4 = set1_epi32(0xFF00);
    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 32) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[2];
        unpack_epu16_to_epi32(packed8, values[0], values[1]);

        for (int group = 0; group < 2; group++) {
            const simd4i l = values[group] & byteMask4;
            const simd4i rgba8 = l | (l << 8) | (l << 16) | ((values[group] & alphaMask4) << 16);
            storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 16));
        }
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[0];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[1];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[0];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[1];
    }
#endif
}

static void R8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~31);
    const simd8i alphaMask8 = set1_256epi32(static_cast<int32_t>(0xFF000000u));
    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 128) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epu8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8i rgba8 = values[group] | alphaMask8;
            storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 32));
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const byte *simd4End = srcPtr + (remainingPixels & ~15);
    const simd4i alphaMask4 = set1_epi32(static_cast<int32_t>(0xFF000000u));
    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 64) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epu8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd4i rgba8 = values[group] | alphaMask4;
            storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 16));
        }
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 255;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 255;
    }
#endif
}

static void RG88ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 2;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~15) * 2;
    const simd8i alphaMask8 = set1_256epi32(static_cast<int32_t>(0xFF000000u));
    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 64) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[2];
        unpack_epu16_to_256epi32(packed8, values[0], values[1]);

        for (int group = 0; group < 2; group++) {
            const simd8i rgba8 = values[group] | alphaMask8;
            storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 32));
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const byte *simd4End = srcPtr + (remainingPixels & ~7) * 2;
    const simd4i alphaMask4 = set1_epi32(static_cast<int32_t>(0xFF000000u));
    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 32) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[2];
        unpack_epu16_to_epi32(packed8, values[0], values[1]);

        for (int group = 0; group < 2; group++) {
            const simd4i rgba8 = values[group] | alphaMask4;
            storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr + group * 16));
        }
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 255;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 255;
    }
#endif
}

static void RGB888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 3;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 255;
    }
}

static void BGR888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 3;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 255;
    }
}

static void RGBX8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8i rgbMask8 = set1_256epi32(0x00FFFFFF);
    const simd8i alphaMask8 = set1_256epi32(static_cast<int32_t>(0xFF000000u));

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgbx8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i rgba8 = (rgbx8 & rgbMask8) | alphaMask8;

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4i rgbMask4 = set1_epi32(0x00FFFFFF);
    const simd4i alphaMask4 = set1_epi32(static_cast<int32_t>(0xFF000000u));

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgbx8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i rgba8 = (rgbx8 & rgbMask4) | alphaMask4;

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 255;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 255;
    }
#endif
}

static void BGRX8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8i greenMask8 = set1_256epi32(0x0000FF00);
    const simd8i highRBMask8 = set1_256epi32(0x00FF0000);
    const simd8i lowRBMask8 = set1_256epi32(0x000000FF);
    const simd8i alphaMask8 = set1_256epi32(static_cast<int32_t>(0xFF000000u));

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i bgrx8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i rgba8 = (bgrx8 & greenMask8) |
            srl_256epi32(bgrx8 & highRBMask8, 16) |
            ((bgrx8 & lowRBMask8) << 16) |
            alphaMask8;

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4i greenMask4 = set1_epi32(0x0000FF00);
    const simd4i highRBMask4 = set1_epi32(0x00FF0000);
    const simd4i lowRBMask4 = set1_epi32(0x000000FF);
    const simd4i alphaMask4 = set1_epi32(static_cast<int32_t>(0xFF000000u));

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i bgrx8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i rgba8 = (bgrx8 & greenMask4) |
            srl_epi32(bgrx8 & highRBMask4, 16) |
            ((bgrx8 & lowRBMask4) << 16) |
            alphaMask4;

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 255;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 255;
    }
#endif
}

static void RGBA8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    memcpy(dst, src, sizeof(byte) * 4 * numPixels);
}

static void BGRA8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8i keepMask8 = set1_256epi32(static_cast<int32_t>(0xFF00FF00u));
    const simd8i highRBMask8 = set1_256epi32(0x00FF0000);
    const simd8i lowRBMask8 = set1_256epi32(0x000000FF);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i bgra8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i rgba8 = (bgra8 & keepMask8) |
            srl_256epi32(bgra8 & highRBMask8, 16) |
            ((bgra8 & lowRBMask8) << 16);

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4i keepMask4 = set1_epi32(static_cast<int32_t>(0xFF00FF00u));
    const simd4i highRBMask4 = set1_epi32(0x00FF0000);
    const simd4i lowRBMask4 = set1_epi32(0x000000FF);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i bgra8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i rgba8 = (bgra8 & keepMask4) |
            srl_epi32(bgra8 & highRBMask4, 16) |
            ((bgra8 & lowRBMask4) << 16);

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
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

static void ABGR8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8i byteMask8 = set1_256epi32(0x00FF00FF);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i abgr8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i swapped8 = ((abgr8 & byteMask8) << 8) |
            (srl_256epi32(abgr8, 8) & byteMask8);
        const simd8i rgba8 = (swapped8 << 16) | srl_256epi32(swapped8, 16);

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4i byteMask4 = set1_epi32(0x00FF00FF);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i abgr8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i swapped8 = ((abgr8 & byteMask4) << 8) |
            (srl_epi32(abgr8, 8) & byteMask4);
        const simd4i rgba8 = (swapped8 << 16) | srl_epi32(swapped8, 16);

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
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

static void ARGB8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i argb8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i rgba8 = srl_256epi32(argb8, 8) | (argb8 << 24);

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const byte *simd4End = srcPtr + (remainingPixels & ~3) * 4;

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i argb8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i rgba8 = srl_epi32(argb8, 8) | (argb8 << 24);

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[1];
        dstPtr[1] = srcPtr[2];
        dstPtr[2] = srcPtr[3];
        dstPtr[3] = srcPtr[0];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[1];
        dstPtr[1] = srcPtr[2];
        dstPtr[2] = srcPtr[3];
        dstPtr[3] = srcPtr[0];
    }
#endif
}

static void RGBX4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[3] = 255;
    }
}

static void BGRX4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[3] = 255;
    }
}

static void RGBA4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~15);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 64) {
        const simd8i packed16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i packed32_0;
        simd8i packed32_1;
        unpack_epu16_to_256epi32(packed16, packed32_0, packed32_1);

        simd8i rgba4_0 =
            (packed32_0 & 0x000F) |
            ((packed32_0 & 0x00F0) << 4) |
            ((packed32_0 & 0x0F00) << 8) |
            ((packed32_0 & 0xF000) << 12);
        simd8i rgba4_1 =
            (packed32_1 & 0x000F) |
            ((packed32_1 & 0x00F0) << 4) |
            ((packed32_1 & 0x0F00) << 8) |
            ((packed32_1 & 0xF000) << 12);

        rgba4_0 |= rgba4_0 << 4;
        rgba4_1 |= rgba4_1 << 4;

        storeu_si256(rgba4_0, reinterpret_cast<int32_t *>(dstPtr + 0));
        storeu_si256(rgba4_1, reinterpret_cast<int32_t *>(dstPtr + 32));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~7);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 32) {
        const simd4i packed16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i packed32_0;
        simd4i packed32_1;
        unpack_epu16_to_epi32(packed16, packed32_0, packed32_1);

        simd4i rgba4_0 =
            (packed32_0 & 0x000F) |
            ((packed32_0 & 0x00F0) << 4) |
            ((packed32_0 & 0x0F00) << 8) |
            ((packed32_0 & 0xF000) << 12);
        simd4i rgba4_1 =
            (packed32_1 & 0x000F) |
            ((packed32_1 & 0x00F0) << 4) |
            ((packed32_1 & 0x0F00) << 8) |
            ((packed32_1 & 0xF000) << 12);

        rgba4_0 |= rgba4_0 << 4;
        rgba4_1 |= rgba4_1 << 4;

        storeu_si128(rgba4_0, reinterpret_cast<int32_t *>(dstPtr + 0));
        storeu_si128(rgba4_1, reinterpret_cast<int32_t *>(dstPtr + 16));
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[3] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[3] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
    }
#endif
}

static void BGRA4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[3] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
    }
}

static void ABGR4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
        dstPtr[1] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[2] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[3] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
    }
}

static void ARGB4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[1] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[2] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
        dstPtr[3] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
    }
}

static void RGBX5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[3] = 255;
    }
}

static void BGRX5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[3] = 255;
    }
}

static void RGBA5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~15);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 64) {
        const simd8i packed16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i packed32_0;
        simd8i packed32_1;
        unpack_epu16_to_256epi32(packed16, packed32_0, packed32_1);

        const simd8i rgba8_0 =
            ((packed32_0 << 3) & 0x000000F8) |
            ((packed32_0 >> 2) & 0x00000007) |
            ((packed32_0 << 6) & 0x0000F800) |
            ((packed32_0 << 1) & 0x00000700) |
            ((packed32_0 << 9) & 0x00F80000) |
            ((packed32_0 << 4) & 0x00070000) |
            ((packed32_0 >= 0x8000) & -16777216);
        const simd8i rgba8_1 =
            ((packed32_1 << 3) & 0x000000F8) |
            ((packed32_1 >> 2) & 0x00000007) |
            ((packed32_1 << 6) & 0x0000F800) |
            ((packed32_1 << 1) & 0x00000700) |
            ((packed32_1 << 9) & 0x00F80000) |
            ((packed32_1 << 4) & 0x00070000) |
            ((packed32_1 >= 0x8000) & -16777216);

        storeu_si256(rgba8_0, reinterpret_cast<int32_t *>(dstPtr + 0));
        storeu_si256(rgba8_1, reinterpret_cast<int32_t *>(dstPtr + 32));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~7);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 32) {
        const simd4i packed16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i packed32_0;
        simd4i packed32_1;
        unpack_epu16_to_epi32(packed16, packed32_0, packed32_1);

        const simd4i rgba8_0 =
            ((packed32_0 << 3) & 0x000000F8) |
            ((packed32_0 >> 2) & 0x00000007) |
            ((packed32_0 << 6) & 0x0000F800) |
            ((packed32_0 << 1) & 0x00000700) |
            ((packed32_0 << 9) & 0x00F80000) |
            ((packed32_0 << 4) & 0x00070000) |
            ((packed32_0 >= 0x8000) & -16777216);
        const simd4i rgba8_1 =
            ((packed32_1 << 3) & 0x000000F8) |
            ((packed32_1 >> 2) & 0x00000007) |
            ((packed32_1 << 6) & 0x0000F800) |
            ((packed32_1 << 1) & 0x00000700) |
            ((packed32_1 << 9) & 0x00F80000) |
            ((packed32_1 << 4) & 0x00070000) |
            ((packed32_1 >= 0x8000) & -16777216);

        storeu_si128(rgba8_0, reinterpret_cast<int32_t *>(dstPtr + 0));
        storeu_si128(rgba8_1, reinterpret_cast<int32_t *>(dstPtr + 16));
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[3] = ((int16_t)*srcPtr >> 15) & 0xFF;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[3] = ((int16_t)*srcPtr >> 15) & 0xFF;
    }
#endif
}

static void BGRA5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[3] = ((int16_t)*srcPtr >> 15) & 0xFF;
    }
}

static void ABGR1555ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 13) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xF8) | ((*srcPtr >> 8) & 0x7);
        dstPtr[2] = ((*srcPtr << 2) & 0xF8) | ((*srcPtr >> 3) & 0x7);
        dstPtr[3] = (*srcPtr & 1) ? 0xFF : 0;
    }
}

static void ARGB1555ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 2) & 0xF8) | ((*srcPtr >> 3) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xF8) | ((*srcPtr >> 8) & 0x7);
        dstPtr[2] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 13) & 0x7);
        dstPtr[3] = (*srcPtr & 1) ? 0xFF : 0;
    }
}

static void RGB565ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~15);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 64) {
        const simd8i packed16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i packed32_0;
        simd8i packed32_1;
        unpack_epu16_to_256epi32(packed16, packed32_0, packed32_1);

        const simd8i rgba8_0 =
            ((packed32_0 << 3) & 0x000000F8) |
            ((packed32_0 << 2) & 0x00030000) |
            ((packed32_0 >> 2) & 0x00000007) |
            ((packed32_0 << 5) & 0x0000FC00) |
            ((packed32_0 >> 1) & 0x00000300) |
            ((packed32_0 << 8) & 0x00F80000) |
            -16777216;
        const simd8i rgba8_1 =
            ((packed32_1 << 3) & 0x000000F8) |
            ((packed32_1 << 2) & 0x00030000) |
            ((packed32_1 >> 2) & 0x00000007) |
            ((packed32_1 << 5) & 0x0000FC00) |
            ((packed32_1 >> 1) & 0x00000300) |
            ((packed32_1 << 8) & 0x00F80000) |
            -16777216;

        storeu_si256(rgba8_0, reinterpret_cast<int32_t *>(dstPtr + 0));
        storeu_si256(rgba8_1, reinterpret_cast<int32_t *>(dstPtr + 32));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~7);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 32) {
        const simd4i packed16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i packed32_0;
        simd4i packed32_1;
        unpack_epu16_to_epi32(packed16, packed32_0, packed32_1);

        const simd4i rgba8_0 =
            ((packed32_0 << 3) & 0x000000F8) |
            ((packed32_0 << 2) & 0x00030000) |
            ((packed32_0 >> 2) & 0x00000007) |
            ((packed32_0 << 5) & 0x0000FC00) |
            ((packed32_0 >> 1) & 0x00000300) |
            ((packed32_0 << 8) & 0x00F80000) |
            -16777216;
        const simd4i rgba8_1 =
            ((packed32_1 << 3) & 0x000000F8) |
            ((packed32_1 << 2) & 0x00030000) |
            ((packed32_1 >> 2) & 0x00000007) |
            ((packed32_1 << 5) & 0x0000FC00) |
            ((packed32_1 >> 1) & 0x00000300) |
            ((packed32_1 << 8) & 0x00F80000) |
            -16777216;

        storeu_si128(rgba8_0, reinterpret_cast<int32_t *>(dstPtr + 0));
        storeu_si128(rgba8_1, reinterpret_cast<int32_t *>(dstPtr + 16));
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xFC) | ((*srcPtr >> 9) & 0x3);
        dstPtr[2] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 14) & 0x7);
        dstPtr[3] = 255;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xFC) | ((*srcPtr >> 9) & 0x3);
        dstPtr[2] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 14) & 0x7);
        dstPtr[3] = 255;
    }
#endif
}

static void BGR565ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 14) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xFC) | ((*srcPtr >> 9) & 0x3);
        dstPtr[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[3] = 255;
    }
}

static void RGBA32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(255.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        simd8f rgba0 = loadu_256ps(srcPtr + 0) * scale8;
        simd8f rgba1 = loadu_256ps(srcPtr + 8) * scale8;
        simd8f rgba2 = loadu_256ps(srcPtr + 16) * scale8;
        simd8f rgba3 = loadu_256ps(srcPtr + 24) * scale8;

        // Match Math::Ftob: map NaN to zero, clamp to [0, 255]
        // and truncate toward zero.
        rgba0 = select_256ps(zero8, rgba0, rgba0 == rgba0);
        rgba1 = select_256ps(zero8, rgba1, rgba1 == rgba1);
        rgba2 = select_256ps(zero8, rgba2, rgba2 == rgba2);
        rgba3 = select_256ps(zero8, rgba3, rgba3 == rgba3);

        rgba0 = min_256ps(max_256ps(rgba0, zero8), maxValue8);
        rgba1 = min_256ps(max_256ps(rgba1, zero8), maxValue8);
        rgba2 = min_256ps(max_256ps(rgba2, zero8), maxValue8);
        rgba3 = min_256ps(max_256ps(rgba3, zero8), maxValue8);

        const simd8i rgba32_0 = trunc_256ps_to_epi32(rgba0);
        const simd8i rgba32_1 = trunc_256ps_to_epi32(rgba1);
        const simd8i rgba32_2 = trunc_256ps_to_epi32(rgba2);
        const simd8i rgba32_3 = trunc_256ps_to_epi32(rgba3);
        const simd8i rgba8 = packus_256epi32_to_epu8(rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(255.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        simd4f rgba0 = loadu_ps(srcPtr + 0) * scale4;
        simd4f rgba1 = loadu_ps(srcPtr + 4) * scale4;
        simd4f rgba2 = loadu_ps(srcPtr + 8) * scale4;
        simd4f rgba3 = loadu_ps(srcPtr + 12) * scale4;

        // Match Math::Ftob: map NaN to zero, clamp to [0, 255]
        // and truncate toward zero.
        rgba0 = select_ps(zero4, rgba0, rgba0 == rgba0);
        rgba1 = select_ps(zero4, rgba1, rgba1 == rgba1);
        rgba2 = select_ps(zero4, rgba2, rgba2 == rgba2);
        rgba3 = select_ps(zero4, rgba3, rgba3 == rgba3);

        rgba0 = min_ps(max_ps(rgba0, zero4), maxValue4);
        rgba1 = min_ps(max_ps(rgba1, zero4), maxValue4);
        rgba2 = min_ps(max_ps(rgba2, zero4), maxValue4);
        rgba3 = min_ps(max_ps(rgba3, zero4), maxValue4);

        const simd4i rgba32_0 = trunc_ps_to_epi32(rgba0);
        const simd4i rgba32_1 = trunc_ps_to_epi32(rgba1);
        const simd4i rgba32_2 = trunc_ps_to_epi32(rgba2);
        const simd4i rgba32_3 = trunc_ps_to_epi32(rgba3);
        const simd4i rgba8 = packus_epi32_to_epu8(rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[3]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[3]);
    }
#endif
}

BE_NAMESPACE_END
