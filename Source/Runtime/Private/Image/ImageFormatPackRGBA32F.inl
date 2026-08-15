#include "Precompiled.h"
#include "Math/MathCommon.h"
#include "Math/FloatConverter.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
//
// RGBA32FToXXX (packing function from rgba32f to custom format)
//
//--------------------------------------------------------------------------------------------------

static void RGBA32FToL8(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Math::Ftob(255.0f * (0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]));
    }
}

static void RGBA32FToA8(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[3]);
    }
}

static void RGBA32FToLA88(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftob(255.0f * (0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]));
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[3]);
    }
}

static void RGBA32FToL16(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Clamp<int>((0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]) * 65535.0f, 0, 65535);
    }
}

static void RGBA32FToL24X8(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Clamp<int>((0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]) * 0x00FFFFFF, 0, 0x00FFFFFF);
    }
}

static void RGBA32FToR8(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = reinterpret_cast<byte *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~31) * 4;
    const simd8f scale8 = set1_256ps(255.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);

    for (; srcPtr < simd8End; srcPtr += 128, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 4; group++) {
            const float *groupSrc = srcPtr + group * 32;
            const simd8f rgba01 = loadu_256ps(groupSrc + 0);
            const simd8f rgba23 = loadu_256ps(groupSrc + 8);
            const simd8f rgba45 = loadu_256ps(groupSrc + 16);
            const simd8f rgba67 = loadu_256ps(groupSrc + 24);
            const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
            const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
            const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
            const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

            simd8f value;
            simd8f g;
            simd8f b;
            simd8f a;
            transpose2x4x4(rgba04, rgba15, rgba26, rgba37, value, g, b, a);

            value *= scale8;
            value = select_256ps(zero8, value, value == value);
            value = min_256ps(max_256ps(value, zero8), maxValue8);
            values[group] = trunc_256ps_to_epi32(value);
        }

        const simd8i packed8 = packus_256epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~15) * 4;
    const simd4f scale4 = set1_ps(255.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);

    for (; srcPtr < simd4End; srcPtr += 64, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 4; group++) {
            const float *groupSrc = srcPtr + group * 16;
            simd4f rgba0 = loadu_ps(groupSrc + 0);
            simd4f rgba1 = loadu_ps(groupSrc + 4);
            simd4f rgba2 = loadu_ps(groupSrc + 8);
            simd4f rgba3 = loadu_ps(groupSrc + 12);
            transpose4x4(rgba0, rgba1, rgba2, rgba3);

            simd4f value = rgba0;
            value *= scale4;
            value = select_ps(zero4, value, value == value);
            value = min_ps(max_ps(value, zero4), maxValue4);
            values[group] = trunc_ps_to_epi32(value);
        }

        const simd4i packed8 = packus_epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
    }
#endif
}

static void RGBA32FToRG88(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = reinterpret_cast<byte *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8f scale8 = set1_256ps(255.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 2; group++) {
            const float *groupSrc = srcPtr + group * 32;
            const simd8f rgba01 = loadu_256ps(groupSrc + 0);
            const simd8f rgba23 = loadu_256ps(groupSrc + 8);
            const simd8f rgba45 = loadu_256ps(groupSrc + 16);
            const simd8f rgba67 = loadu_256ps(groupSrc + 24);
            const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
            const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
            const simd8f rgba46 = shuffle_256ps<0, 2>(rgba45, rgba67);
            const simd8f rgba57 = shuffle_256ps<1, 3>(rgba45, rgba67);

            simd8f rg0 = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
            simd8f rg1 = shuffle_256ps<0, 1, 0, 1>(rgba46, rgba57);
            rg0 *= scale8;
            rg1 *= scale8;
            rg0 = select_256ps(zero8, rg0, rg0 == rg0);
            rg1 = select_256ps(zero8, rg1, rg1 == rg1);
            rg0 = min_256ps(max_256ps(rg0, zero8), maxValue8);
            rg1 = min_256ps(max_256ps(rg1, zero8), maxValue8);
            values[group * 2 + 0] = trunc_256ps_to_epi32(rg0);
            values[group * 2 + 1] = trunc_256ps_to_epi32(rg1);
        }

        const simd8i packed8 = packus_256epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4f scale4 = set1_ps(255.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 2; group++) {
            const float *groupSrc = srcPtr + group * 16;
            const simd4f rgba0 = loadu_ps(groupSrc + 0);
            const simd4f rgba1 = loadu_ps(groupSrc + 4);
            const simd4f rgba2 = loadu_ps(groupSrc + 8);
            const simd4f rgba3 = loadu_ps(groupSrc + 12);

            simd4f rg0 = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
            simd4f rg1 = shuffle_ps<0, 1, 0, 1>(rgba2, rgba3);
            rg0 *= scale4;
            rg1 *= scale4;
            rg0 = select_ps(zero4, rg0, rg0 == rg0);
            rg1 = select_ps(zero4, rg1, rg1 == rg1);
            rg0 = min_ps(max_ps(rg0, zero4), maxValue4);
            rg1 = min_ps(max_ps(rg1, zero4), maxValue4);
            values[group * 2 + 0] = trunc_ps_to_epi32(rg0);
            values[group * 2 + 1] = trunc_ps_to_epi32(rg1);
        }

        const simd4i packed8 = packus_epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
    }
#endif
}

static void RGBA32FToRGB888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[2]);
    }
}

static void RGBA32FToBGR888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[0]);
    }
}

static void RGBA32FToRGBX8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(255.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);
    const simd8i rgbMask8 = set1_256epi32(0x00FFFFFF);
    const simd8i alphaMask8 = set1_256epi32(static_cast<int32_t>(0xFF000000u));

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
        const simd8i rgbx8 = (rgba8 & rgbMask8) | alphaMask8;

        storeu_si256(rgbx8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(255.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);
    const simd4i rgbMask4 = set1_epi32(0x00FFFFFF);
    const simd4i alphaMask4 = set1_epi32(static_cast<int32_t>(0xFF000000u));

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
        const simd4i rgbx8 = (rgba8 & rgbMask4) | alphaMask4;

        storeu_si128(rgbx8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[3] = 255;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[3] = 255;
    }
#endif
}

static void RGBA32FToBGRX8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(255.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);
    const simd8i greenMask8 = set1_256epi32(0x0000FF00);
    const simd8i highRBMask8 = set1_256epi32(0x00FF0000);
    const simd8i lowRBMask8 = set1_256epi32(0x000000FF);
    const simd8i alphaMask8 = set1_256epi32(static_cast<int32_t>(0xFF000000u));

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
        const simd8i bgrx8 = (rgba8 & greenMask8) |
            srl_256epi32(rgba8 & highRBMask8, 16) |
            ((rgba8 & lowRBMask8) << 16) |
            alphaMask8;

        storeu_si256(bgrx8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(255.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);
    const simd4i greenMask4 = set1_epi32(0x0000FF00);
    const simd4i highRBMask4 = set1_epi32(0x00FF0000);
    const simd4i lowRBMask4 = set1_epi32(0x000000FF);
    const simd4i alphaMask4 = set1_epi32(static_cast<int32_t>(0xFF000000u));

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
        const simd4i bgrx8 = (rgba8 & greenMask4) |
            srl_epi32(rgba8 & highRBMask4, 16) |
            ((rgba8 & lowRBMask4) << 16) |
            alphaMask4;

        storeu_si128(bgrx8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[3] = 255;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[3] = 255;
    }
#endif
}

static void RGBA32FToBGRA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(255.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);
    const simd8i keepMask8 = set1_256epi32(static_cast<int32_t>(0xFF00FF00u));
    const simd8i highRBMask8 = set1_256epi32(0x00FF0000);
    const simd8i lowRBMask8 = set1_256epi32(0x000000FF);

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
        const simd8i bgra8 = (rgba8 & keepMask8) |
            srl_256epi32(rgba8 & highRBMask8, 16) |
            ((rgba8 & lowRBMask8) << 16);

        storeu_si256(bgra8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(255.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);
    const simd4i keepMask4 = set1_epi32(static_cast<int32_t>(0xFF00FF00u));
    const simd4i highRBMask4 = set1_epi32(0x00FF0000);
    const simd4i lowRBMask4 = set1_epi32(0x000000FF);

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
        const simd4i bgra8 = (rgba8 & keepMask4) |
            srl_epi32(rgba8 & highRBMask4, 16) |
            ((rgba8 & lowRBMask4) << 16);

        storeu_si128(bgra8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[3]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[3]);
    }
#endif
}

static void RGBA32FToABGR8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(255.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);
    const simd8i byteMask8 = set1_256epi32(0x00FF00FF);

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
        const simd8i swapped8 = ((rgba8 & byteMask8) << 8) |
            (srl_256epi32(rgba8, 8) & byteMask8);
        const simd8i abgr8 = (swapped8 << 16) | srl_256epi32(swapped8, 16);

        storeu_si256(abgr8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(255.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);
    const simd4i byteMask4 = set1_epi32(0x00FF00FF);

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
        const simd4i swapped8 = ((rgba8 & byteMask4) << 8) |
            (srl_epi32(rgba8, 8) & byteMask4);
        const simd4i abgr8 = (swapped8 << 16) | srl_epi32(swapped8, 16);

        storeu_si128(abgr8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[3]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[0]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[3]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[0]);
    }
#endif
}

static void RGBA32FToARGB8888(const byte *src, byte *dst, int numPixels) {
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
        const simd8i argb8 = (rgba8 << 8) | srl_256epi32(rgba8, 24);

        storeu_si256(argb8, reinterpret_cast<int32_t *>(dstPtr));
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
        const simd4i argb8 = (rgba8 << 8) | srl_epi32(rgba8, 24);

        storeu_si128(argb8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[3]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[2]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[3]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[2]);
    }
#endif
}

static void RGBA32FToRGBA1010102(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);
    const simd8f rgbScale8 = set1_256ps(1023.0f);
    const simd8f alphaScale8 = set1_256ps(3.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 8) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r, g, b, a);

        r = select_256ps(zero8, r, r == r);
        g = select_256ps(zero8, g, g == g);
        b = select_256ps(zero8, b, b == b);
        a = select_256ps(zero8, a, a == a);

        r = min_256ps(max_256ps(r, zero8), one8);
        g = min_256ps(max_256ps(g, zero8), one8);
        b = min_256ps(max_256ps(b, zero8), one8);
        a = min_256ps(max_256ps(a, zero8), one8);

        const simd8i r32 = trunc_256ps_to_epi32(r * rgbScale8);
        const simd8i g32 = trunc_256ps_to_epi32(g * rgbScale8);
        const simd8i b32 = trunc_256ps_to_epi32(b * rgbScale8);
        const simd8i a32 = trunc_256ps_to_epi32(a * alphaScale8);
        const simd8i packed = r32 | (g32 << 10) | (b32 << 20) | (a32 << 30);

        storeu_si256(packed, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);
    const simd4f rgbScale4 = set1_ps(1023.0f);
    const simd4f alphaScale4 = set1_ps(3.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 4) {
        simd4f r = loadu_ps(srcPtr + 0);
        simd4f g = loadu_ps(srcPtr + 4);
        simd4f b = loadu_ps(srcPtr + 8);
        simd4f a = loadu_ps(srcPtr + 12);

        transpose4x4(r, g, b, a);

        r = select_ps(zero4, r, r == r);
        g = select_ps(zero4, g, g == g);
        b = select_ps(zero4, b, b == b);
        a = select_ps(zero4, a, a == a);

        r = min_ps(max_ps(r, zero4), one4);
        g = min_ps(max_ps(g, zero4), one4);
        b = min_ps(max_ps(b, zero4), one4);
        a = min_ps(max_ps(a, zero4), one4);

        const simd4i r32 = trunc_ps_to_epi32(r * rgbScale4);
        const simd4i g32 = trunc_ps_to_epi32(g * rgbScale4);
        const simd4i b32 = trunc_ps_to_epi32(b * rgbScale4);
        const simd4i a32 = trunc_ps_to_epi32(a * alphaScale4);
        const simd4i packed = r32 | (g32 << 10) | (b32 << 20) | (a32 << 30);

        storeu_si128(packed, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        const uint32_t r = static_cast<uint32_t>(Math::Ftoi(1023.0f * Clamp01(srcPtr[0]))) & 0x3FF;
        const uint32_t g = static_cast<uint32_t>(Math::Ftoi(1023.0f * Clamp01(srcPtr[1]))) & 0x3FF;
        const uint32_t b = static_cast<uint32_t>(Math::Ftoi(1023.0f * Clamp01(srcPtr[2]))) & 0x3FF;
        const uint32_t a = static_cast<uint32_t>(Math::Ftoi(3.0f * Clamp01(srcPtr[3]))) & 0x3;

        *dstPtr = r | (g << 10) | (b << 20) | (a << 30);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        const uint32_t r = static_cast<uint32_t>(Math::Ftoi(1023.0f * Clamp01(srcPtr[0]))) & 0x3FF;
        const uint32_t g = static_cast<uint32_t>(Math::Ftoi(1023.0f * Clamp01(srcPtr[1]))) & 0x3FF;
        const uint32_t b = static_cast<uint32_t>(Math::Ftoi(1023.0f * Clamp01(srcPtr[2]))) & 0x3FF;
        const uint32_t a = static_cast<uint32_t>(Math::Ftoi(3.0f * Clamp01(srcPtr[3]))) & 0x3;

        *dstPtr = r | (g << 10) | (b << 20) | (a << 30);
    }
#endif
}

static void RGBA32FToRGBA1010102UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f zero8 = setzero_256ps();
    const simd8f rgbMax8 = set1_256ps(1023.0f);
    const simd8f alphaMax8 = set1_256ps(3.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 8) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r, g, b, a);

        r = select_256ps(zero8, r, r == r);
        g = select_256ps(zero8, g, g == g);
        b = select_256ps(zero8, b, b == b);
        a = select_256ps(zero8, a, a == a);

        r = min_256ps(max_256ps(r, zero8), rgbMax8);
        g = min_256ps(max_256ps(g, zero8), rgbMax8);
        b = min_256ps(max_256ps(b, zero8), rgbMax8);
        a = min_256ps(max_256ps(a, zero8), alphaMax8);

        const simd8i r32 = trunc_256ps_to_epi32(r);
        const simd8i g32 = trunc_256ps_to_epi32(g);
        const simd8i b32 = trunc_256ps_to_epi32(b);
        const simd8i a32 = trunc_256ps_to_epi32(a);
        const simd8i packed = r32 | (g32 << 10) | (b32 << 20) | (a32 << 30);

        storeu_si256(packed, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f zero4 = setzero_ps();
    const simd4f rgbMax4 = set1_ps(1023.0f);
    const simd4f alphaMax4 = set1_ps(3.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 4) {
        simd4f r = loadu_ps(srcPtr + 0);
        simd4f g = loadu_ps(srcPtr + 4);
        simd4f b = loadu_ps(srcPtr + 8);
        simd4f a = loadu_ps(srcPtr + 12);

        transpose4x4(r, g, b, a);

        r = select_ps(zero4, r, r == r);
        g = select_ps(zero4, g, g == g);
        b = select_ps(zero4, b, b == b);
        a = select_ps(zero4, a, a == a);

        r = min_ps(max_ps(r, zero4), rgbMax4);
        g = min_ps(max_ps(g, zero4), rgbMax4);
        b = min_ps(max_ps(b, zero4), rgbMax4);
        a = min_ps(max_ps(a, zero4), alphaMax4);

        const simd4i r32 = trunc_ps_to_epi32(r);
        const simd4i g32 = trunc_ps_to_epi32(g);
        const simd4i b32 = trunc_ps_to_epi32(b);
        const simd4i a32 = trunc_ps_to_epi32(a);
        const simd4i packed = r32 | (g32 << 10) | (b32 << 20) | (a32 << 30);

        storeu_si128(packed, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        const uint32_t r = static_cast<uint32_t>(Math::Ftoi(Clamp<float>(srcPtr[0], 0, 1023))) & 0x3FF;
        const uint32_t g = static_cast<uint32_t>(Math::Ftoi(Clamp<float>(srcPtr[1], 0, 1023))) & 0x3FF;
        const uint32_t b = static_cast<uint32_t>(Math::Ftoi(Clamp<float>(srcPtr[2], 0, 1023))) & 0x3FF;
        const uint32_t a = static_cast<uint32_t>(Math::Ftoi(Clamp<float>(srcPtr[3], 0, 3))) & 0x3;

        *dstPtr = r | (g << 10) | (b << 20) | (a << 30);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        const uint32_t r = static_cast<uint32_t>(Math::Ftoi(Clamp<float>(srcPtr[0], 0, 1023))) & 0x3FF;
        const uint32_t g = static_cast<uint32_t>(Math::Ftoi(Clamp<float>(srcPtr[1], 0, 1023))) & 0x3FF;
        const uint32_t b = static_cast<uint32_t>(Math::Ftoi(Clamp<float>(srcPtr[2], 0, 1023))) & 0x3FF;
        const uint32_t a = static_cast<uint32_t>(Math::Ftoi(Clamp<float>(srcPtr[3], 0, 3))) & 0x3;

        *dstPtr = r | (g << 10) | (b << 20) | (a << 30);
    }
#endif
}

static void RGBA32FToR8SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~31) * 4;
    const simd8f scale8 = set1_256ps(255.0f / 2.0f);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f maxValue8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 128, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 4; group++) {
            const float *groupSrc = srcPtr + group * 32;
            const simd8f rgba01 = loadu_256ps(groupSrc + 0);
            const simd8f rgba23 = loadu_256ps(groupSrc + 8);
            const simd8f rgba45 = loadu_256ps(groupSrc + 16);
            const simd8f rgba67 = loadu_256ps(groupSrc + 24);
            const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
            const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
            const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
            const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

            simd8f value;
            simd8f g;
            simd8f b;
            simd8f a;
            transpose2x4x4(rgba04, rgba15, rgba26, rgba37, value, g, b, a);

            value = select_256ps(minValue8, value, value == value);
            value = min_256ps(max_256ps(value, minValue8), maxValue8);
            values[group] = floor_256ps_to_epi32(value * scale8);
        }

        const simd8i packed8 = packs_256epi32_to_epi8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~15) * 4;
    const simd4f scale4 = set1_ps(255.0f / 2.0f);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f maxValue4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 64, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 4; group++) {
            const float *groupSrc = srcPtr + group * 16;
            simd4f rgba0 = loadu_ps(groupSrc + 0);
            simd4f rgba1 = loadu_ps(groupSrc + 4);
            simd4f rgba2 = loadu_ps(groupSrc + 8);
            simd4f rgba3 = loadu_ps(groupSrc + 12);
            transpose4x4(rgba0, rgba1, rgba2, rgba3);

            simd4f value = rgba0;
            value = select_ps(minValue4, value, value == value);
            value = min_ps(max_ps(value, minValue4), maxValue4);
            values[group] = floor_ps_to_epi32(value * scale4);
        }

        const simd4i packed8 = packs_epi32_to_epi8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
    }
#endif
}

static void RGBA32FToR8SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~31) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f minValue8 = set1_256ps(-128.0f);
    const simd8f maxValue8 = set1_256ps(127.0f);

    for (; srcPtr < simd8End; srcPtr += 128, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 4; group++) {
            const float *groupSrc = srcPtr + group * 32;
            const simd8f rgba01 = loadu_256ps(groupSrc + 0);
            const simd8f rgba23 = loadu_256ps(groupSrc + 8);
            const simd8f rgba45 = loadu_256ps(groupSrc + 16);
            const simd8f rgba67 = loadu_256ps(groupSrc + 24);
            const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
            const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
            const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
            const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

            simd8f value;
            simd8f g;
            simd8f b;
            simd8f a;
            transpose2x4x4(rgba04, rgba15, rgba26, rgba37, value, g, b, a);

            value = select_256ps(minValue8, value, value == value);
            value = min_256ps(max_256ps(value, minValue8), maxValue8);
            values[group] = floor_256ps_to_epi32(value + half8);
        }

        const simd8i packed8 = packs_256epi32_to_epi8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~15) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f minValue4 = set1_ps(-128.0f);
    const simd4f maxValue4 = set1_ps(127.0f);

    for (; srcPtr < simd4End; srcPtr += 64, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 4; group++) {
            const float *groupSrc = srcPtr + group * 16;
            simd4f rgba0 = loadu_ps(groupSrc + 0);
            simd4f rgba1 = loadu_ps(groupSrc + 4);
            simd4f rgba2 = loadu_ps(groupSrc + 8);
            simd4f rgba3 = loadu_ps(groupSrc + 12);
            transpose4x4(rgba0, rgba1, rgba2, rgba3);

            simd4f value = rgba0;
            value = select_ps(minValue4, value, value == value);
            value = min_ps(max_ps(value, minValue4), maxValue4);
            values[group] = floor_ps_to_epi32(value + half4);
        }

        const simd4i packed8 = packs_epi32_to_epi8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
    }
#endif
}

static void RGBA32FToR8UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~31) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);

    for (; srcPtr < simd8End; srcPtr += 128, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 4; group++) {
            const float *groupSrc = srcPtr + group * 32;
            const simd8f rgba01 = loadu_256ps(groupSrc + 0);
            const simd8f rgba23 = loadu_256ps(groupSrc + 8);
            const simd8f rgba45 = loadu_256ps(groupSrc + 16);
            const simd8f rgba67 = loadu_256ps(groupSrc + 24);
            const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
            const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
            const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
            const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

            simd8f value;
            simd8f g;
            simd8f b;
            simd8f a;
            transpose2x4x4(rgba04, rgba15, rgba26, rgba37, value, g, b, a);

            value = select_256ps(zero8, value, value == value);
            value = min_256ps(max_256ps(value, zero8), maxValue8);
            values[group] = trunc_256ps_to_epi32(value + half8);
        }

        const simd8i packed8 = packus_256epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~15) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);

    for (; srcPtr < simd4End; srcPtr += 64, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 4; group++) {
            const float *groupSrc = srcPtr + group * 16;
            simd4f rgba0 = loadu_ps(groupSrc + 0);
            simd4f rgba1 = loadu_ps(groupSrc + 4);
            simd4f rgba2 = loadu_ps(groupSrc + 8);
            simd4f rgba3 = loadu_ps(groupSrc + 12);
            transpose4x4(rgba0, rgba1, rgba2, rgba3);

            simd4f value = rgba0;
            value = select_ps(zero4, value, value == value);
            value = min_ps(max_ps(value, zero4), maxValue4);
            values[group] = trunc_ps_to_epi32(value + half4);
        }

        const simd4i packed8 = packus_epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT8_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT8_MAX));
    }
#endif
}

static void RGBA32FToRG88SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8f scale8 = set1_256ps(255.0f / 2.0f);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f maxValue8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 2; group++) {
            const float *groupSrc = srcPtr + group * 32;
            const simd8f rgba01 = loadu_256ps(groupSrc + 0);
            const simd8f rgba23 = loadu_256ps(groupSrc + 8);
            const simd8f rgba45 = loadu_256ps(groupSrc + 16);
            const simd8f rgba67 = loadu_256ps(groupSrc + 24);
            const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
            const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
            const simd8f rgba46 = shuffle_256ps<0, 2>(rgba45, rgba67);
            const simd8f rgba57 = shuffle_256ps<1, 3>(rgba45, rgba67);

            simd8f rg0 = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
            simd8f rg1 = shuffle_256ps<0, 1, 0, 1>(rgba46, rgba57);
            rg0 = select_256ps(minValue8, rg0, rg0 == rg0);
            rg1 = select_256ps(minValue8, rg1, rg1 == rg1);
            rg0 = min_256ps(max_256ps(rg0, minValue8), maxValue8);
            rg1 = min_256ps(max_256ps(rg1, minValue8), maxValue8);
            values[group * 2 + 0] = floor_256ps_to_epi32(rg0 * scale8);
            values[group * 2 + 1] = floor_256ps_to_epi32(rg1 * scale8);
        }

        const simd8i packed8 = packs_256epi32_to_epi8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4f scale4 = set1_ps(255.0f / 2.0f);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f maxValue4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 2; group++) {
            const float *groupSrc = srcPtr + group * 16;
            const simd4f rgba0 = loadu_ps(groupSrc + 0);
            const simd4f rgba1 = loadu_ps(groupSrc + 4);
            const simd4f rgba2 = loadu_ps(groupSrc + 8);
            const simd4f rgba3 = loadu_ps(groupSrc + 12);

            simd4f rg0 = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
            simd4f rg1 = shuffle_ps<0, 1, 0, 1>(rgba2, rgba3);
            rg0 = select_ps(minValue4, rg0, rg0 == rg0);
            rg1 = select_ps(minValue4, rg1, rg1 == rg1);
            rg0 = min_ps(max_ps(rg0, minValue4), maxValue4);
            rg1 = min_ps(max_ps(rg1, minValue4), maxValue4);
            values[group * 2 + 0] = floor_ps_to_epi32(rg0 * scale4);
            values[group * 2 + 1] = floor_ps_to_epi32(rg1 * scale4);
        }

        const simd4i packed8 = packs_epi32_to_epi8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
    }
#endif
}

static void RGBA32FToRG88SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f minValue8 = set1_256ps(-128.0f);
    const simd8f maxValue8 = set1_256ps(127.0f);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 2; group++) {
            const float *groupSrc = srcPtr + group * 32;
            const simd8f rgba01 = loadu_256ps(groupSrc + 0);
            const simd8f rgba23 = loadu_256ps(groupSrc + 8);
            const simd8f rgba45 = loadu_256ps(groupSrc + 16);
            const simd8f rgba67 = loadu_256ps(groupSrc + 24);
            const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
            const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
            const simd8f rgba46 = shuffle_256ps<0, 2>(rgba45, rgba67);
            const simd8f rgba57 = shuffle_256ps<1, 3>(rgba45, rgba67);

            simd8f rg0 = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
            simd8f rg1 = shuffle_256ps<0, 1, 0, 1>(rgba46, rgba57);
            rg0 = select_256ps(minValue8, rg0, rg0 == rg0);
            rg1 = select_256ps(minValue8, rg1, rg1 == rg1);
            rg0 = min_256ps(max_256ps(rg0, minValue8), maxValue8);
            rg1 = min_256ps(max_256ps(rg1, minValue8), maxValue8);
            values[group * 2 + 0] = floor_256ps_to_epi32(rg0 + half8);
            values[group * 2 + 1] = floor_256ps_to_epi32(rg1 + half8);
        }

        const simd8i packed8 = packs_256epi32_to_epi8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f minValue4 = set1_ps(-128.0f);
    const simd4f maxValue4 = set1_ps(127.0f);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 2; group++) {
            const float *groupSrc = srcPtr + group * 16;
            const simd4f rgba0 = loadu_ps(groupSrc + 0);
            const simd4f rgba1 = loadu_ps(groupSrc + 4);
            const simd4f rgba2 = loadu_ps(groupSrc + 8);
            const simd4f rgba3 = loadu_ps(groupSrc + 12);

            simd4f rg0 = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
            simd4f rg1 = shuffle_ps<0, 1, 0, 1>(rgba2, rgba3);
            rg0 = select_ps(minValue4, rg0, rg0 == rg0);
            rg1 = select_ps(minValue4, rg1, rg1 == rg1);
            rg0 = min_ps(max_ps(rg0, minValue4), maxValue4);
            rg1 = min_ps(max_ps(rg1, minValue4), maxValue4);
            values[group * 2 + 0] = floor_ps_to_epi32(rg0 + half4);
            values[group * 2 + 1] = floor_ps_to_epi32(rg1 + half4);
        }

        const simd4i packed8 = packs_epi32_to_epi8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
        dstPtr[1] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[1]), INT8_MIN, INT8_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
        dstPtr[1] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[1]), INT8_MIN, INT8_MAX));
    }
#endif
}

static void RGBA32FToRG88UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 32) {
        simd8i values[4];

        for (int group = 0; group < 2; group++) {
            const float *groupSrc = srcPtr + group * 32;
            const simd8f rgba01 = loadu_256ps(groupSrc + 0);
            const simd8f rgba23 = loadu_256ps(groupSrc + 8);
            const simd8f rgba45 = loadu_256ps(groupSrc + 16);
            const simd8f rgba67 = loadu_256ps(groupSrc + 24);
            const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
            const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
            const simd8f rgba46 = shuffle_256ps<0, 2>(rgba45, rgba67);
            const simd8f rgba57 = shuffle_256ps<1, 3>(rgba45, rgba67);

            simd8f rg0 = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
            simd8f rg1 = shuffle_256ps<0, 1, 0, 1>(rgba46, rgba57);
            rg0 = select_256ps(zero8, rg0, rg0 == rg0);
            rg1 = select_256ps(zero8, rg1, rg1 == rg1);
            rg0 = min_256ps(max_256ps(rg0, zero8), maxValue8);
            rg1 = min_256ps(max_256ps(rg1, zero8), maxValue8);
            values[group * 2 + 0] = trunc_256ps_to_epi32(rg0 + half8);
            values[group * 2 + 1] = trunc_256ps_to_epi32(rg1 + half8);
        }

        const simd8i packed8 = packus_256epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si256(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 16) {
        simd4i values[4];

        for (int group = 0; group < 2; group++) {
            const float *groupSrc = srcPtr + group * 16;
            const simd4f rgba0 = loadu_ps(groupSrc + 0);
            const simd4f rgba1 = loadu_ps(groupSrc + 4);
            const simd4f rgba2 = loadu_ps(groupSrc + 8);
            const simd4f rgba3 = loadu_ps(groupSrc + 12);

            simd4f rg0 = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
            simd4f rg1 = shuffle_ps<0, 1, 0, 1>(rgba2, rgba3);
            rg0 = select_ps(zero4, rg0, rg0 == rg0);
            rg1 = select_ps(zero4, rg1, rg1 == rg1);
            rg0 = min_ps(max_ps(rg0, zero4), maxValue4);
            rg1 = min_ps(max_ps(rg1, zero4), maxValue4);
            values[group * 2 + 0] = trunc_ps_to_epi32(rg0 + half4);
            values[group * 2 + 1] = trunc_ps_to_epi32(rg1 + half4);
        }

        const simd4i packed8 = packus_epi32_to_epu8(values[0], values[1], values[2], values[3]);
        storeu_si128(packed8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0.0f, UINT8_MAX));
        dstPtr[1] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[1]), 0.0f, UINT8_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0.0f, UINT8_MAX));
        dstPtr[1] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[1]), 0.0f, UINT8_MAX));
    }
#endif
}

static void RGBA32FToRGB888SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
        dstPtr[2] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[2], -1.0f, 1.0f)));
    }
}

static void RGBA32FToRGB888SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
        dstPtr[1] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[1]), INT8_MIN, INT8_MAX));
        dstPtr[2] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[2]), INT8_MIN, INT8_MAX));
    }
}

static void RGBA32FToRGB888UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0.0f, UINT8_MAX));
        dstPtr[1] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[1]), 0.0f, UINT8_MAX));
        dstPtr[2] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[2]), 0.0f, UINT8_MAX));
    }
}

static void RGBA32FToRGBA8888SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(255.0f / 2.0f);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f maxValue8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        simd8f rgba0 = loadu_256ps(srcPtr + 0);
        simd8f rgba1 = loadu_256ps(srcPtr + 8);
        simd8f rgba2 = loadu_256ps(srcPtr + 16);
        simd8f rgba3 = loadu_256ps(srcPtr + 24);

        // Match Clamp, Math::Floor and Math::Ftoi8:
        // map NaN to -1, clamp to [-1, 1], scale by 127.5
        // and round toward negative infinity.
        rgba0 = select_256ps(minValue8, rgba0, rgba0 == rgba0);
        rgba1 = select_256ps(minValue8, rgba1, rgba1 == rgba1);
        rgba2 = select_256ps(minValue8, rgba2, rgba2 == rgba2);
        rgba3 = select_256ps(minValue8, rgba3, rgba3 == rgba3);

        rgba0 = min_256ps(max_256ps(rgba0, minValue8), maxValue8);
        rgba1 = min_256ps(max_256ps(rgba1, minValue8), maxValue8);
        rgba2 = min_256ps(max_256ps(rgba2, minValue8), maxValue8);
        rgba3 = min_256ps(max_256ps(rgba3, minValue8), maxValue8);

        const simd8i rgba32_0 = floor_256ps_to_epi32(rgba0 * scale8);
        const simd8i rgba32_1 = floor_256ps_to_epi32(rgba1 * scale8);
        const simd8i rgba32_2 = floor_256ps_to_epi32(rgba2 * scale8);
        const simd8i rgba32_3 = floor_256ps_to_epi32(rgba3 * scale8);
        const simd8i rgba8 = packs_256epi32_to_epi8(rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(255.0f / 2.0f);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f maxValue4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        // Match Clamp, Math::Floor and Math::Ftoi8:
        // map NaN to -1, clamp to [-1, 1], scale by 127.5
        // and round toward negative infinity.
        rgba0 = select_ps(minValue4, rgba0, rgba0 == rgba0);
        rgba1 = select_ps(minValue4, rgba1, rgba1 == rgba1);
        rgba2 = select_ps(minValue4, rgba2, rgba2 == rgba2);
        rgba3 = select_ps(minValue4, rgba3, rgba3 == rgba3);

        rgba0 = min_ps(max_ps(rgba0, minValue4), maxValue4);
        rgba1 = min_ps(max_ps(rgba1, minValue4), maxValue4);
        rgba2 = min_ps(max_ps(rgba2, minValue4), maxValue4);
        rgba3 = min_ps(max_ps(rgba3, minValue4), maxValue4);

        const simd4i rgba32_0 = floor_ps_to_epi32(rgba0 * scale4);
        const simd4i rgba32_1 = floor_ps_to_epi32(rgba1 * scale4);
        const simd4i rgba32_2 = floor_ps_to_epi32(rgba2 * scale4);
        const simd4i rgba32_3 = floor_ps_to_epi32(rgba3 * scale4);
        const simd4i rgba8 = packs_epi32_to_epi8(rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
        dstPtr[2] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[2], -1.0f, 1.0f)));
        dstPtr[3] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[3], -1.0f, 1.0f)));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
        dstPtr[2] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[2], -1.0f, 1.0f)));
        dstPtr[3] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[3], -1.0f, 1.0f)));
    }
#endif
}

static void RGBA32FToRGBA8888SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f minValue8 = set1_256ps(-128.0f);
    const simd8f maxValue8 = set1_256ps(127.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        simd8f rgba0 = loadu_256ps(srcPtr + 0);
        simd8f rgba1 = loadu_256ps(srcPtr + 8);
        simd8f rgba2 = loadu_256ps(srcPtr + 16);
        simd8f rgba3 = loadu_256ps(srcPtr + 24);

        // Match Math::Round followed by Clamp and Math::Ftoi8:
        // map NaN to INT8_MIN, clamp to the signed 8-bit range,
        // add 0.5 and round toward negative infinity.
        rgba0 = select_256ps(minValue8, rgba0, rgba0 == rgba0);
        rgba1 = select_256ps(minValue8, rgba1, rgba1 == rgba1);
        rgba2 = select_256ps(minValue8, rgba2, rgba2 == rgba2);
        rgba3 = select_256ps(minValue8, rgba3, rgba3 == rgba3);

        rgba0 = min_256ps(max_256ps(rgba0, minValue8), maxValue8);
        rgba1 = min_256ps(max_256ps(rgba1, minValue8), maxValue8);
        rgba2 = min_256ps(max_256ps(rgba2, minValue8), maxValue8);
        rgba3 = min_256ps(max_256ps(rgba3, minValue8), maxValue8);

        const simd8i rgba32_0 = floor_256ps_to_epi32(rgba0 + half8);
        const simd8i rgba32_1 = floor_256ps_to_epi32(rgba1 + half8);
        const simd8i rgba32_2 = floor_256ps_to_epi32(rgba2 + half8);
        const simd8i rgba32_3 = floor_256ps_to_epi32(rgba3 + half8);
        const simd8i rgba8 = packs_256epi32_to_epi8(rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f minValue4 = set1_ps(-128.0f);
    const simd4f maxValue4 = set1_ps(127.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        // Match Math::Round followed by Clamp and Math::Ftoi8:
        // map NaN to INT8_MIN, clamp to the signed 8-bit range,
        // add 0.5 and round toward negative infinity.
        rgba0 = select_ps(minValue4, rgba0, rgba0 == rgba0);
        rgba1 = select_ps(minValue4, rgba1, rgba1 == rgba1);
        rgba2 = select_ps(minValue4, rgba2, rgba2 == rgba2);
        rgba3 = select_ps(minValue4, rgba3, rgba3 == rgba3);

        rgba0 = min_ps(max_ps(rgba0, minValue4), maxValue4);
        rgba1 = min_ps(max_ps(rgba1, minValue4), maxValue4);
        rgba2 = min_ps(max_ps(rgba2, minValue4), maxValue4);
        rgba3 = min_ps(max_ps(rgba3, minValue4), maxValue4);

        const simd4i rgba32_0 = floor_ps_to_epi32(rgba0 + half4);
        const simd4i rgba32_1 = floor_ps_to_epi32(rgba1 + half4);
        const simd4i rgba32_2 = floor_ps_to_epi32(rgba2 + half4);
        const simd4i rgba32_3 = floor_ps_to_epi32(rgba3 + half4);
        const simd4i rgba8 = packs_epi32_to_epi8(rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
        dstPtr[1] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[1]), INT8_MIN, INT8_MAX));
        dstPtr[2] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[2]), INT8_MIN, INT8_MAX));
        dstPtr[3] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[3]), INT8_MIN, INT8_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
        dstPtr[1] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[1]), INT8_MIN, INT8_MAX));
        dstPtr[2] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[2]), INT8_MIN, INT8_MAX));
        dstPtr[3] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[3]), INT8_MIN, INT8_MAX));
    }
#endif
}

static void RGBA32FToRGBA8888UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(255.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        simd8f rgba0 = loadu_256ps(srcPtr + 0);
        simd8f rgba1 = loadu_256ps(srcPtr + 8);
        simd8f rgba2 = loadu_256ps(srcPtr + 16);
        simd8f rgba3 = loadu_256ps(srcPtr + 24);

        // Match Math::Round followed by Clamp and Math::Ftob:
        // map NaN to zero, clamp to [0, 255], add 0.5 and
        // truncate toward zero.
        rgba0 = select_256ps(zero8, rgba0, rgba0 == rgba0);
        rgba1 = select_256ps(zero8, rgba1, rgba1 == rgba1);
        rgba2 = select_256ps(zero8, rgba2, rgba2 == rgba2);
        rgba3 = select_256ps(zero8, rgba3, rgba3 == rgba3);

        rgba0 = min_256ps(max_256ps(rgba0, zero8), maxValue8);
        rgba1 = min_256ps(max_256ps(rgba1, zero8), maxValue8);
        rgba2 = min_256ps(max_256ps(rgba2, zero8), maxValue8);
        rgba3 = min_256ps(max_256ps(rgba3, zero8), maxValue8);

        const simd8i rgba32_0 = trunc_256ps_to_epi32(rgba0 + half8);
        const simd8i rgba32_1 = trunc_256ps_to_epi32(rgba1 + half8);
        const simd8i rgba32_2 = trunc_256ps_to_epi32(rgba2 + half8);
        const simd8i rgba32_3 = trunc_256ps_to_epi32(rgba3 + half8);
        const simd8i rgba8 = packus_256epi32_to_epu8(rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_si256(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(255.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        // Match Math::Round followed by Clamp and Math::Ftob:
        // map NaN to zero, clamp to [0, 255], add 0.5 and
        // truncate toward zero.
        rgba0 = select_ps(zero4, rgba0, rgba0 == rgba0);
        rgba1 = select_ps(zero4, rgba1, rgba1 == rgba1);
        rgba2 = select_ps(zero4, rgba2, rgba2 == rgba2);
        rgba3 = select_ps(zero4, rgba3, rgba3 == rgba3);

        rgba0 = min_ps(max_ps(rgba0, zero4), maxValue4);
        rgba1 = min_ps(max_ps(rgba1, zero4), maxValue4);
        rgba2 = min_ps(max_ps(rgba2, zero4), maxValue4);
        rgba3 = min_ps(max_ps(rgba3, zero4), maxValue4);

        const simd4i rgba32_0 = trunc_ps_to_epi32(rgba0 + half4);
        const simd4i rgba32_1 = trunc_ps_to_epi32(rgba1 + half4);
        const simd4i rgba32_2 = trunc_ps_to_epi32(rgba2 + half4);
        const simd4i rgba32_3 = trunc_ps_to_epi32(rgba3 + half4);
        const simd4i rgba8 = packus_epi32_to_epu8(rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_si128(rgba8, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0.0f, UINT8_MAX));
        dstPtr[1] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[1]), 0.0f, UINT8_MAX));
        dstPtr[2] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[2]), 0.0f, UINT8_MAX));
        dstPtr[3] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[3]), 0.0f, UINT8_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0.0f, UINT8_MAX));
        dstPtr[1] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[1]), 0.0f, UINT8_MAX));
        dstPtr[2] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[2]), 0.0f, UINT8_MAX));
        dstPtr[3] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[3]), 0.0f, UINT8_MAX));
    }
#endif
}

static void RGBA32FToRGBE9995(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = RGBE9995::FromColor3(srcPtr[0], srcPtr[1], srcPtr[2]);
    }
}

static void RGBA32FToRGBX4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 8);
    }
}

static void RGBA32FToBGRX4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 0);
    }
}

static void RGBA32FToRGBA4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[3])) << 12);
    }
}

static void RGBA32FToBGRA4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 0) | (Math::Ftoi(15.0f * Clamp01(srcPtr[3])) << 12);
    }
}

static void RGBA32FToABGR4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 12) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[3])) << 0);
    }
}

static void RGBA32FToARGB4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 12) | (Math::Ftoi(15.0f * Clamp01(srcPtr[3])) << 0);
    }
}

static void RGBA32FToRGBX5551(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 10);
    }
}

static void RGBA32FToBGRX5551(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 10) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 0);
    }
}

static void RGBA32FToRGBA5551(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 10) | (srcPtr[3] >= 0.5f ? (1<<15) : 0);
    }
}

static void RGBA32FToBGRA5551(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 10) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 0) | (srcPtr[3] >= 0.5f ? (1 << 15) : 0);
    }
}

static void RGBA32FToABGR1555(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 11) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 6) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 1) | (srcPtr[3] >= 0.5f ? 1 : 0);
    }
}

static void RGBA32FToARGB1555(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 1) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 6) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 11) | (srcPtr[3] >= 0.5f ? 1 : 0);
    }
}

static void RGBA32FToRGB565(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(63.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 11);
    }
}

static void RGBA32FToBGR565(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 11) | (Math::Ftoi(63.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 0);
    }
}

static void RGBA32FToL16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = F16Converter::FromF32(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]);
    }
}

static void RGBA32FToA16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[3]);
    }
}

static void RGBA32FToLA16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = F16Converter::FromF32(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]);
        dstPtr[1] = F16Converter::FromF32(srcPtr[3]);
    }
}

static void RGBA32FToR16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[0]);
    }
}

static void RGBA32FToR16(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8f scale8 = set1_256ps(65535.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(65535.0f);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 16) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r0;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r0, g, b, a);

        const simd8f rgba89 = loadu_256ps(srcPtr + 32);
        const simd8f rgba1011 = loadu_256ps(srcPtr + 40);
        const simd8f rgba1213 = loadu_256ps(srcPtr + 48);
        const simd8f rgba1415 = loadu_256ps(srcPtr + 56);

        const simd8f rgba812 = shuffle_256ps<0, 2>(rgba89, rgba1213);
        const simd8f rgba913 = shuffle_256ps<1, 3>(rgba89, rgba1213);
        const simd8f rgba1014 = shuffle_256ps<0, 2>(rgba1011, rgba1415);
        const simd8f rgba1115 = shuffle_256ps<1, 3>(rgba1011, rgba1415);

        simd8f r1;
        transpose2x4x4(rgba812, rgba913, rgba1014, rgba1115, r1, g, b, a);

        r0 *= scale8;
        r1 *= scale8;

        // Match Math::Ftoui16: map NaN to zero, clamp to [0, 65535]
        // and truncate toward zero.
        r0 = select_256ps(zero8, r0, r0 == r0);
        r1 = select_256ps(zero8, r1, r1 == r1);

        r0 = min_256ps(max_256ps(r0, zero8), maxValue8);
        r1 = min_256ps(max_256ps(r1, zero8), maxValue8);

        const simd8i r32_0 = trunc_256ps_to_epi32(r0);
        const simd8i r32_1 = trunc_256ps_to_epi32(r1);
        const simd8i r16 =
            packus_256epi32_to_epu16(r32_0, r32_1);

        storeu_si256(r16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4f scale4 = set1_ps(65535.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(65535.0f);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 8) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);
        simd4f r0 = rgba0 * scale4;

        rgba0 = loadu_ps(srcPtr + 16);
        rgba1 = loadu_ps(srcPtr + 20);
        rgba2 = loadu_ps(srcPtr + 24);
        rgba3 = loadu_ps(srcPtr + 28);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);
        simd4f r1 = rgba0 * scale4;

        // Match Math::Ftoui16: map NaN to zero, clamp to [0, 65535]
        // and truncate toward zero.
        r0 = select_ps(zero4, r0, r0 == r0);
        r1 = select_ps(zero4, r1, r1 == r1);

        r0 = min_ps(max_ps(r0, zero4), maxValue4);
        r1 = min_ps(max_ps(r1, zero4), maxValue4);

        const simd4i r32_0 = trunc_ps_to_epi32(r0);
        const simd4i r32_1 = trunc_ps_to_epi32(r1);
        const simd4i r16 = packus_epi32_to_epu16(r32_0, r32_1);

        storeu_si128(r16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[0]));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[0]));
    }
#endif
}

static void RGBA32FToR16SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8f scale8 = set1_256ps(UINT16_MAX / 2.0f);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f maxValue8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 16) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r0;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r0, g, b, a);

        const simd8f rgba89 = loadu_256ps(srcPtr + 32);
        const simd8f rgba1011 = loadu_256ps(srcPtr + 40);
        const simd8f rgba1213 = loadu_256ps(srcPtr + 48);
        const simd8f rgba1415 = loadu_256ps(srcPtr + 56);

        const simd8f rgba812 = shuffle_256ps<0, 2>(rgba89, rgba1213);
        const simd8f rgba913 = shuffle_256ps<1, 3>(rgba89, rgba1213);
        const simd8f rgba1014 = shuffle_256ps<0, 2>(rgba1011, rgba1415);
        const simd8f rgba1115 = shuffle_256ps<1, 3>(rgba1011, rgba1415);

        simd8f r1;
        transpose2x4x4(rgba812, rgba913, rgba1014, rgba1115, r1, g, b, a);

        // Match Clamp, Math::Floor and Math::Ftoi16:
        // map NaN to -1, clamp to [-1, 1], scale by 32767.5
        // and round toward negative infinity.
        r0 = select_256ps(minValue8, r0, r0 == r0);
        r1 = select_256ps(minValue8, r1, r1 == r1);

        r0 = min_256ps(max_256ps(r0, minValue8), maxValue8);
        r1 = min_256ps(max_256ps(r1, minValue8), maxValue8);

        const simd8i r32_0 = floor_256ps_to_epi32(r0 * scale8);
        const simd8i r32_1 = floor_256ps_to_epi32(r1 * scale8);
        const simd8i r16 = packs_256epi32_to_epi16(r32_0, r32_1);

        storeu_si256(r16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4f scale4 = set1_ps(UINT16_MAX / 2.0f);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f maxValue4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 8) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);
        simd4f r0 = rgba0;

        rgba0 = loadu_ps(srcPtr + 16);
        rgba1 = loadu_ps(srcPtr + 20);
        rgba2 = loadu_ps(srcPtr + 24);
        rgba3 = loadu_ps(srcPtr + 28);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);
        simd4f r1 = rgba0;

        // Match Clamp, Math::Floor and Math::Ftoi16:
        // map NaN to -1, clamp to [-1, 1], scale by 32767.5
        // and round toward negative infinity.
        r0 = select_ps(minValue4, r0, r0 == r0);
        r1 = select_ps(minValue4, r1, r1 == r1);

        r0 = min_ps(max_ps(r0, minValue4), maxValue4);
        r1 = min_ps(max_ps(r1, minValue4), maxValue4);

        const simd4i r32_0 = floor_ps_to_epi32(r0 * scale4);
        const simd4i r32_1 = floor_ps_to_epi32(r1 * scale4);
        const simd4i r16 = packs_epi32_to_epi16(r32_0, r32_1);

        storeu_si128(r16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
    }
#endif
}

static void RGBA32FToR16SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f minValue8 = set1_256ps(-32768.0f);
    const simd8f maxValue8 = set1_256ps(32767.0f);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 16) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r0;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r0, g, b, a);

        const simd8f rgba89 = loadu_256ps(srcPtr + 32);
        const simd8f rgba1011 = loadu_256ps(srcPtr + 40);
        const simd8f rgba1213 = loadu_256ps(srcPtr + 48);
        const simd8f rgba1415 = loadu_256ps(srcPtr + 56);

        const simd8f rgba812 = shuffle_256ps<0, 2>(rgba89, rgba1213);
        const simd8f rgba913 = shuffle_256ps<1, 3>(rgba89, rgba1213);
        const simd8f rgba1014 = shuffle_256ps<0, 2>(rgba1011, rgba1415);
        const simd8f rgba1115 = shuffle_256ps<1, 3>(rgba1011, rgba1415);

        simd8f r1;
        transpose2x4x4(rgba812, rgba913, rgba1014, rgba1115, r1, g, b, a);

        // Match Math::Round followed by Clamp and Math::Ftoi16:
        // map NaN to INT16_MIN, clamp to the signed 16-bit range,
        // add 0.5 and round toward negative infinity.
        r0 = select_256ps(minValue8, r0, r0 == r0);
        r1 = select_256ps(minValue8, r1, r1 == r1);

        r0 = min_256ps(max_256ps(r0, minValue8), maxValue8);
        r1 = min_256ps(max_256ps(r1, minValue8), maxValue8);

        const simd8i r32_0 = floor_256ps_to_epi32(r0 + half8);
        const simd8i r32_1 = floor_256ps_to_epi32(r1 + half8);
        const simd8i r16 = packs_256epi32_to_epi16(r32_0, r32_1);

        storeu_si256(r16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f minValue4 = set1_ps(-32768.0f);
    const simd4f maxValue4 = set1_ps(32767.0f);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 8) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);
        simd4f r0 = rgba0;

        rgba0 = loadu_ps(srcPtr + 16);
        rgba1 = loadu_ps(srcPtr + 20);
        rgba2 = loadu_ps(srcPtr + 24);
        rgba3 = loadu_ps(srcPtr + 28);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);
        simd4f r1 = rgba0;

        // Match Math::Round followed by Clamp and Math::Ftoi16:
        // map NaN to INT16_MIN, clamp to the signed 16-bit range,
        // add 0.5 and round toward negative infinity.
        r0 = select_ps(minValue4, r0, r0 == r0);
        r1 = select_ps(minValue4, r1, r1 == r1);

        r0 = min_ps(max_ps(r0, minValue4), maxValue4);
        r1 = min_ps(max_ps(r1, minValue4), maxValue4);

        const simd4i r32_0 = floor_ps_to_epi32(r0 + half4);
        const simd4i r32_1 = floor_ps_to_epi32(r1 + half4);
        const simd4i r16 = packs_epi32_to_epi16(r32_0, r32_1);

        storeu_si128(r16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[0]), INT16_MIN, INT16_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[0]), INT16_MIN, INT16_MAX));
    }
#endif
}

static void RGBA32FToR16UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~15) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(65535.0f);

    for (; srcPtr < simd8End; srcPtr += 64, dstPtr += 16) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r0;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r0, g, b, a);

        const simd8f rgba89 = loadu_256ps(srcPtr + 32);
        const simd8f rgba1011 = loadu_256ps(srcPtr + 40);
        const simd8f rgba1213 = loadu_256ps(srcPtr + 48);
        const simd8f rgba1415 = loadu_256ps(srcPtr + 56);

        const simd8f rgba812 = shuffle_256ps<0, 2>(rgba89, rgba1213);
        const simd8f rgba913 = shuffle_256ps<1, 3>(rgba89, rgba1213);
        const simd8f rgba1014 = shuffle_256ps<0, 2>(rgba1011, rgba1415);
        const simd8f rgba1115 = shuffle_256ps<1, 3>(rgba1011, rgba1415);

        simd8f r1;
        transpose2x4x4(rgba812, rgba913, rgba1014, rgba1115, r1, g, b, a);

        // Match Math::Round followed by Clamp and Math::Ftoui16:
        // map NaN to zero, clamp to [0, 65535], add 0.5 and
        // truncate toward zero.
        r0 = select_256ps(zero8, r0, r0 == r0);
        r1 = select_256ps(zero8, r1, r1 == r1);

        r0 = min_256ps(max_256ps(r0, zero8), maxValue8);
        r1 = min_256ps(max_256ps(r1, zero8), maxValue8);

        const simd8i r32_0 = trunc_256ps_to_epi32(r0 + half8);
        const simd8i r32_1 = trunc_256ps_to_epi32(r1 + half8);
        const simd8i r16 =
            packus_256epi32_to_epu16(r32_0, r32_1);

        storeu_si256(r16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~7) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(65535.0f);

    for (; srcPtr < simd4End; srcPtr += 32, dstPtr += 8) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);
        simd4f r0 = rgba0;

        rgba0 = loadu_ps(srcPtr + 16);
        rgba1 = loadu_ps(srcPtr + 20);
        rgba2 = loadu_ps(srcPtr + 24);
        rgba3 = loadu_ps(srcPtr + 28);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);
        simd4f r1 = rgba0;

        // Match Math::Round followed by Clamp and Math::Ftoui16:
        // map NaN to zero, clamp to [0, 65535], add 0.5 and
        // truncate toward zero.
        r0 = select_ps(zero4, r0, r0 == r0);
        r1 = select_ps(zero4, r1, r1 == r1);

        r0 = min_ps(max_ps(r0, zero4), maxValue4);
        r1 = min_ps(max_ps(r1, zero4), maxValue4);

        const simd4i r32_0 = trunc_ps_to_epi32(r0 + half4);
        const simd4i r32_1 = trunc_ps_to_epi32(r1 + half4);
        const simd4i r16 = packus_epi32_to_epu16(r32_0, r32_1);

        storeu_si128(r16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT16_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT16_MAX));
    }
#endif
}

static void RGBA32FToRG16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[0]);
        dstPtr[1] = F16Converter::FromF32(srcPtr[1]);
    }
}

static void RGBA32FToRG16(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(65535.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(65535.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 16) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
        const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
        const simd8f rgba46 = shuffle_256ps<0, 2>(rgba45, rgba67);
        const simd8f rgba57 = shuffle_256ps<1, 3>(rgba45, rgba67);

        simd8f rg0 = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13) * scale8;
        simd8f rg1 = shuffle_256ps<0, 1, 0, 1>(rgba46, rgba57) * scale8;

        // Match Math::Ftoui16: map NaN to zero, clamp to [0, 65535]
        // and truncate toward zero.
        rg0 = select_256ps(zero8, rg0, rg0 == rg0);
        rg1 = select_256ps(zero8, rg1, rg1 == rg1);

        rg0 = min_256ps(max_256ps(rg0, zero8), maxValue8);
        rg1 = min_256ps(max_256ps(rg1, zero8), maxValue8);

        const simd8i rg32_0 = trunc_256ps_to_epi32(rg0);
        const simd8i rg32_1 = trunc_256ps_to_epi32(rg1);
        const simd8i rg16 =
            packus_256epi32_to_epu16(rg32_0, rg32_1);

        storeu_si256(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(65535.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(65535.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 8) {
        const simd4f rgba0 = loadu_ps(srcPtr + 0);
        const simd4f rgba1 = loadu_ps(srcPtr + 4);
        const simd4f rgba2 = loadu_ps(srcPtr + 8);
        const simd4f rgba3 = loadu_ps(srcPtr + 12);

        simd4f rg0 = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1) * scale4;
        simd4f rg1 = shuffle_ps<0, 1, 0, 1>(rgba2, rgba3) * scale4;

        // Match Math::Ftoui16: map NaN to zero, clamp to [0, 65535]
        // and truncate toward zero.
        rg0 = select_ps(zero4, rg0, rg0 == rg0);
        rg1 = select_ps(zero4, rg1, rg1 == rg1);

        rg0 = min_ps(max_ps(rg0, zero4), maxValue4);
        rg1 = min_ps(max_ps(rg1, zero4), maxValue4);

        const simd4i rg32_0 = trunc_ps_to_epi32(rg0);
        const simd4i rg32_1 = trunc_ps_to_epi32(rg1);
        const simd4i rg16 = packus_epi32_to_epu16(rg32_0, rg32_1);

        storeu_si128(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[0]));
        dstPtr[1] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[1]));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[0]));
        dstPtr[1] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[1]));
    }
#endif
}

static void RGBA32FToRG16SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(UINT16_MAX / 2.0f);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f maxValue8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 16) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
        const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
        const simd8f rgba46 = shuffle_256ps<0, 2>(rgba45, rgba67);
        const simd8f rgba57 = shuffle_256ps<1, 3>(rgba45, rgba67);

        simd8f rg0 = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
        simd8f rg1 = shuffle_256ps<0, 1, 0, 1>(rgba46, rgba57);

        // Match Clamp, Math::Floor and Math::Ftoi16:
        // map NaN to -1, clamp to [-1, 1], scale by 32767.5
        // and round toward negative infinity.
        rg0 = select_256ps(minValue8, rg0, rg0 == rg0);
        rg1 = select_256ps(minValue8, rg1, rg1 == rg1);

        rg0 = min_256ps(max_256ps(rg0, minValue8), maxValue8);
        rg1 = min_256ps(max_256ps(rg1, minValue8), maxValue8);

        const simd8i rg32_0 = floor_256ps_to_epi32(rg0 * scale8);
        const simd8i rg32_1 = floor_256ps_to_epi32(rg1 * scale8);
        const simd8i rg16 = packs_256epi32_to_epi16(rg32_0, rg32_1);

        storeu_si256(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f scale4 = set1_ps(UINT16_MAX / 2.0f);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f maxValue4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 8) {
        const simd4f rgba0 = loadu_ps(srcPtr + 0);
        const simd4f rgba1 = loadu_ps(srcPtr + 4);
        const simd4f rgba2 = loadu_ps(srcPtr + 8);
        const simd4f rgba3 = loadu_ps(srcPtr + 12);

        simd4f rg0 = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
        simd4f rg1 = shuffle_ps<0, 1, 0, 1>(rgba2, rgba3);

        // Match Clamp, Math::Floor and Math::Ftoi16:
        // map NaN to -1, clamp to [-1, 1], scale by 32767.5
        // and round toward negative infinity.
        rg0 = select_ps(minValue4, rg0, rg0 == rg0);
        rg1 = select_ps(minValue4, rg1, rg1 == rg1);

        rg0 = min_ps(max_ps(rg0, minValue4), maxValue4);
        rg1 = min_ps(max_ps(rg1, minValue4), maxValue4);

        const simd4i rg32_0 = floor_ps_to_epi32(rg0 * scale4);
        const simd4i rg32_1 = floor_ps_to_epi32(rg1 * scale4);
        const simd4i rg16 = packs_epi32_to_epi16(rg32_0, rg32_1);

        storeu_si128(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
    }
#endif
}

static void RGBA32FToRG16SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f minValue8 = set1_256ps(-32768.0f);
    const simd8f maxValue8 = set1_256ps(32767.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 16) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
        const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
        const simd8f rgba46 = shuffle_256ps<0, 2>(rgba45, rgba67);
        const simd8f rgba57 = shuffle_256ps<1, 3>(rgba45, rgba67);

        simd8f rg0 = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
        simd8f rg1 = shuffle_256ps<0, 1, 0, 1>(rgba46, rgba57);

        // Match Math::Round followed by Clamp and Math::Ftoi16:
        // map NaN to INT16_MIN, clamp to the signed 16-bit range,
        // add 0.5 and round toward negative infinity.
        rg0 = select_256ps(minValue8, rg0, rg0 == rg0);
        rg1 = select_256ps(minValue8, rg1, rg1 == rg1);

        rg0 = min_256ps(max_256ps(rg0, minValue8), maxValue8);
        rg1 = min_256ps(max_256ps(rg1, minValue8), maxValue8);

        const simd8i rg32_0 = floor_256ps_to_epi32(rg0 + half8);
        const simd8i rg32_1 = floor_256ps_to_epi32(rg1 + half8);
        const simd8i rg16 = packs_256epi32_to_epi16(rg32_0, rg32_1);

        storeu_si256(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f minValue4 = set1_ps(-32768.0f);
    const simd4f maxValue4 = set1_ps(32767.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 8) {
        const simd4f rgba0 = loadu_ps(srcPtr + 0);
        const simd4f rgba1 = loadu_ps(srcPtr + 4);
        const simd4f rgba2 = loadu_ps(srcPtr + 8);
        const simd4f rgba3 = loadu_ps(srcPtr + 12);

        simd4f rg0 = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
        simd4f rg1 = shuffle_ps<0, 1, 0, 1>(rgba2, rgba3);

        // Match Math::Round followed by Clamp and Math::Ftoi16:
        // map NaN to INT16_MIN, clamp to the signed 16-bit range,
        // add 0.5 and round toward negative infinity.
        rg0 = select_ps(minValue4, rg0, rg0 == rg0);
        rg1 = select_ps(minValue4, rg1, rg1 == rg1);

        rg0 = min_ps(max_ps(rg0, minValue4), maxValue4);
        rg1 = min_ps(max_ps(rg1, minValue4), maxValue4);

        const simd4i rg32_0 = floor_ps_to_epi32(rg0 + half4);
        const simd4i rg32_1 = floor_ps_to_epi32(rg1 + half4);
        const simd4i rg16 = packs_epi32_to_epi16(rg32_0, rg32_1);

        storeu_si128(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[0]), INT16_MIN, INT16_MAX));
        dstPtr[1] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[1]), INT16_MIN, INT16_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[0]), INT16_MIN, INT16_MAX));
        dstPtr[1] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[1]), INT16_MIN, INT16_MAX));
    }
#endif
}

static void RGBA32FToRG16UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(65535.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 16) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
        const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
        const simd8f rgba46 = shuffle_256ps<0, 2>(rgba45, rgba67);
        const simd8f rgba57 = shuffle_256ps<1, 3>(rgba45, rgba67);

        simd8f rg0 = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
        simd8f rg1 = shuffle_256ps<0, 1, 0, 1>(rgba46, rgba57);

        // Match Math::Round followed by Clamp and Math::Ftoui16:
        // map NaN to zero, clamp to [0, 65535], add 0.5 and
        // truncate toward zero.
        rg0 = select_256ps(zero8, rg0, rg0 == rg0);
        rg1 = select_256ps(zero8, rg1, rg1 == rg1);

        rg0 = min_256ps(max_256ps(rg0, zero8), maxValue8);
        rg1 = min_256ps(max_256ps(rg1, zero8), maxValue8);

        const simd8i rg32_0 = trunc_256ps_to_epi32(rg0 + half8);
        const simd8i rg32_1 = trunc_256ps_to_epi32(rg1 + half8);
        const simd8i rg16 =
            packus_256epi32_to_epu16(rg32_0, rg32_1);

        storeu_si256(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(65535.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 8) {
        const simd4f rgba0 = loadu_ps(srcPtr + 0);
        const simd4f rgba1 = loadu_ps(srcPtr + 4);
        const simd4f rgba2 = loadu_ps(srcPtr + 8);
        const simd4f rgba3 = loadu_ps(srcPtr + 12);

        simd4f rg0 = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
        simd4f rg1 = shuffle_ps<0, 1, 0, 1>(rgba2, rgba3);

        // Match Math::Round followed by Clamp and Math::Ftoui16:
        // map NaN to zero, clamp to [0, 65535], add 0.5 and
        // truncate toward zero.
        rg0 = select_ps(zero4, rg0, rg0 == rg0);
        rg1 = select_ps(zero4, rg1, rg1 == rg1);

        rg0 = min_ps(max_ps(rg0, zero4), maxValue4);
        rg1 = min_ps(max_ps(rg1, zero4), maxValue4);

        const simd4i rg32_0 = trunc_ps_to_epi32(rg0 + half4);
        const simd4i rg32_1 = trunc_ps_to_epi32(rg1 + half4);
        const simd4i rg16 = packus_epi32_to_epu16(rg32_0, rg32_1);

        storeu_si128(rg16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT16_MAX));
        dstPtr[1] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[1]), 0, UINT16_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT16_MAX));
        dstPtr[1] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[1]), 0, UINT16_MAX));
    }
#endif
}

static void RGBA32FToRGB16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[0]);
        dstPtr[1] = F16Converter::FromF32(srcPtr[1]);
        dstPtr[2] = F16Converter::FromF32(srcPtr[2]);
    }
}

static void RGBA32FToRGB16(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[0]));
        dstPtr[1] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[1]));
        dstPtr[2] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[2]));
    }
}

static void RGBA32FToRGB16SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
        dstPtr[2] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[2], -1.0f, 1.0f)));
    }
}

static void RGBA32FToRGB16SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[0]), INT16_MIN, INT16_MAX));
        dstPtr[1] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[1]), INT16_MIN, INT16_MAX));
        dstPtr[2] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[2]), INT16_MIN, INT16_MAX));
    }
}

static void RGBA32FToRGB16UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT16_MAX));
        dstPtr[1] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[1]), 0, UINT16_MAX));
        dstPtr[2] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[2]), 0, UINT16_MAX));
    }
}

static void RGBA32FToRGBA16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[0]);
        dstPtr[1] = F16Converter::FromF32(srcPtr[1]);
        dstPtr[2] = F16Converter::FromF32(srcPtr[2]);
        dstPtr[3] = F16Converter::FromF32(srcPtr[3]);
    }
}

static void RGBA32FToRGBA16(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~3) * 4;
    const simd8f scale8 = set1_256ps(65535.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(65535.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 16) {
        simd8f rgba0 = loadu_256ps(srcPtr + 0) * scale8;
        simd8f rgba1 = loadu_256ps(srcPtr + 8) * scale8;

        // Match Math::Ftoui16: map NaN to zero, clamp to [0, 65535]
        // and truncate toward zero.
        rgba0 = select_256ps(zero8, rgba0, rgba0 == rgba0);
        rgba1 = select_256ps(zero8, rgba1, rgba1 == rgba1);

        rgba0 = min_256ps(max_256ps(rgba0, zero8), maxValue8);
        rgba1 = min_256ps(max_256ps(rgba1, zero8), maxValue8);

        const simd8i rgba32_0 = trunc_256ps_to_epi32(rgba0);
        const simd8i rgba32_1 = trunc_256ps_to_epi32(rgba1);
        const simd8i rgba16 = packus_256epi32_to_epu16(rgba32_0, rgba32_1);

        storeu_si256(rgba16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~1) * 4;
    const simd4f scale4 = set1_ps(65535.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(65535.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 8) {
        simd4f rgba0 = loadu_ps(srcPtr + 0) * scale4;
        simd4f rgba1 = loadu_ps(srcPtr + 4) * scale4;

        // Match Math::Ftoui16: map NaN to zero, clamp to [0, 65535]
        // and truncate toward zero.
        rgba0 = select_ps(zero4, rgba0, rgba0 == rgba0);
        rgba1 = select_ps(zero4, rgba1, rgba1 == rgba1);

        rgba0 = min_ps(max_ps(rgba0, zero4), maxValue4);
        rgba1 = min_ps(max_ps(rgba1, zero4), maxValue4);

        const simd4i rgba32_0 = trunc_ps_to_epi32(rgba0);
        const simd4i rgba32_1 = trunc_ps_to_epi32(rgba1);
        const simd4i rgba16 = packus_epi32_to_epu16(rgba32_0, rgba32_1);

        storeu_si128(rgba16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[0]));
        dstPtr[1] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[1]));
        dstPtr[2] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[2]));
        dstPtr[3] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[3]));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[0]));
        dstPtr[1] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[1]));
        dstPtr[2] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[2]));
        dstPtr[3] = Math::Ftoui16(UINT16_MAX * Clamp01(srcPtr[3]));
    }
#endif
}

static void RGBA32FToRGBA16SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~3) * 4;
    const simd8f scale8 = set1_256ps(UINT16_MAX / 2.0f);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f maxValue8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 16) {
        simd8f rgba0 = loadu_256ps(srcPtr + 0);
        simd8f rgba1 = loadu_256ps(srcPtr + 8);

        // Match Clamp, Math::Floor and Math::Ftoi16:
        // map NaN to -1, clamp to [-1, 1], scale by 32767.5
        // and round toward negative infinity.
        rgba0 = select_256ps(minValue8, rgba0, rgba0 == rgba0);
        rgba1 = select_256ps(minValue8, rgba1, rgba1 == rgba1);

        rgba0 = min_256ps(max_256ps(rgba0, minValue8), maxValue8);
        rgba1 = min_256ps(max_256ps(rgba1, minValue8), maxValue8);

        const simd8i rgba32_0 = floor_256ps_to_epi32(rgba0 * scale8);
        const simd8i rgba32_1 = floor_256ps_to_epi32(rgba1 * scale8);
        const simd8i rgba16 = packs_256epi32_to_epi16(rgba32_0, rgba32_1);

        storeu_si256(rgba16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~1) * 4;
    const simd4f scale4 = set1_ps(UINT16_MAX / 2.0f);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f maxValue4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 8) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);

        // Match Clamp, Math::Floor and Math::Ftoi16:
        // map NaN to -1, clamp to [-1, 1], scale by 32767.5
        // and round toward negative infinity.
        rgba0 = select_ps(minValue4, rgba0, rgba0 == rgba0);
        rgba1 = select_ps(minValue4, rgba1, rgba1 == rgba1);

        rgba0 = min_ps(max_ps(rgba0, minValue4), maxValue4);
        rgba1 = min_ps(max_ps(rgba1, minValue4), maxValue4);

        const simd4i rgba32_0 = floor_ps_to_epi32(rgba0 * scale4);
        const simd4i rgba32_1 = floor_ps_to_epi32(rgba1 * scale4);
        const simd4i rgba16 = packs_epi32_to_epi16(rgba32_0, rgba32_1);

        storeu_si128(rgba16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
        dstPtr[2] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[2], -1.0f, 1.0f)));
        dstPtr[3] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[3], -1.0f, 1.0f)));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
        dstPtr[2] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[2], -1.0f, 1.0f)));
        dstPtr[3] = Math::Ftoi16(Math::Floor((UINT16_MAX / 2.0f) * Clamp(srcPtr[3], -1.0f, 1.0f)));
    }
#endif
}

static void RGBA32FToRGBA16SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~3) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f minValue8 = set1_256ps(-32768.0f);
    const simd8f maxValue8 = set1_256ps(32767.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 16) {
        simd8f rgba0 = loadu_256ps(srcPtr + 0);
        simd8f rgba1 = loadu_256ps(srcPtr + 8);

        // Match Math::Round followed by Clamp and Math::Ftoi16:
        // map NaN to INT16_MIN, clamp to the signed 16-bit range,
        // add 0.5 and round toward negative infinity.
        rgba0 = select_256ps(minValue8, rgba0, rgba0 == rgba0);
        rgba1 = select_256ps(minValue8, rgba1, rgba1 == rgba1);

        rgba0 = min_256ps(max_256ps(rgba0, minValue8), maxValue8);
        rgba1 = min_256ps(max_256ps(rgba1, minValue8), maxValue8);

        const simd8i rgba32_0 = floor_256ps_to_epi32(rgba0 + half8);
        const simd8i rgba32_1 = floor_256ps_to_epi32(rgba1 + half8);
        const simd8i rgba16 = packs_256epi32_to_epi16(rgba32_0, rgba32_1);

        storeu_si256(rgba16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~1) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f minValue4 = set1_ps(-32768.0f);
    const simd4f maxValue4 = set1_ps(32767.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 8) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);

        // Match Math::Round followed by Clamp and Math::Ftoi16:
        // map NaN to INT16_MIN, clamp to the signed 16-bit range,
        // add 0.5 and round toward negative infinity.
        rgba0 = select_ps(minValue4, rgba0, rgba0 == rgba0);
        rgba1 = select_ps(minValue4, rgba1, rgba1 == rgba1);

        rgba0 = min_ps(max_ps(rgba0, minValue4), maxValue4);
        rgba1 = min_ps(max_ps(rgba1, minValue4), maxValue4);

        const simd4i rgba32_0 = floor_ps_to_epi32(rgba0 + half4);
        const simd4i rgba32_1 = floor_ps_to_epi32(rgba1 + half4);
        const simd4i rgba16 = packs_epi32_to_epi16(rgba32_0, rgba32_1);

        storeu_si128(rgba16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[0]), INT16_MIN, INT16_MAX));
        dstPtr[1] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[1]), INT16_MIN, INT16_MAX));
        dstPtr[2] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[2]), INT16_MIN, INT16_MAX));
        dstPtr[3] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[3]), INT16_MIN, INT16_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[0]), INT16_MIN, INT16_MAX));
        dstPtr[1] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[1]), INT16_MIN, INT16_MAX));
        dstPtr[2] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[2]), INT16_MIN, INT16_MAX));
        dstPtr[3] = Math::Ftoi16(Clamp<float>(Math::Round(srcPtr[3]), INT16_MIN, INT16_MAX));
    }
#endif
}

static void RGBA32FToRGBA16UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~3) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxValue8 = set1_256ps(65535.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 16) {
        simd8f rgba0 = loadu_256ps(srcPtr + 0);
        simd8f rgba1 = loadu_256ps(srcPtr + 8);

        // Match Math::Round followed by Clamp and Math::Ftoui16:
        // map NaN to zero, clamp to [0, 65535], add 0.5 and
        // truncate toward zero.
        rgba0 = select_256ps(zero8, rgba0, rgba0 == rgba0);
        rgba1 = select_256ps(zero8, rgba1, rgba1 == rgba1);

        rgba0 = min_256ps(max_256ps(rgba0, zero8), maxValue8);
        rgba1 = min_256ps(max_256ps(rgba1, zero8), maxValue8);

        const simd8i rgba32_0 = trunc_256ps_to_epi32(rgba0 + half8);
        const simd8i rgba32_1 = trunc_256ps_to_epi32(rgba1 + half8);
        const simd8i rgba16 = packus_256epi32_to_epu16(rgba32_0, rgba32_1);

        storeu_si256(rgba16, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~1) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxValue4 = set1_ps(65535.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 8) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);

        // Match Math::Round followed by Clamp and Math::Ftoui16:
        // map NaN to zero, clamp to [0, 65535], add 0.5 and
        // truncate toward zero.
        rgba0 = select_ps(zero4, rgba0, rgba0 == rgba0);
        rgba1 = select_ps(zero4, rgba1, rgba1 == rgba1);

        rgba0 = min_ps(max_ps(rgba0, zero4), maxValue4);
        rgba1 = min_ps(max_ps(rgba1, zero4), maxValue4);

        const simd4i rgba32_0 = trunc_ps_to_epi32(rgba0 + half4);
        const simd4i rgba32_1 = trunc_ps_to_epi32(rgba1 + half4);
        const simd4i rgba16 = packus_epi32_to_epu16(rgba32_0, rgba32_1);

        storeu_si128(rgba16, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT16_MAX));
        dstPtr[1] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[1]), 0, UINT16_MAX));
        dstPtr[2] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[2]), 0, UINT16_MAX));
        dstPtr[3] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[3]), 0, UINT16_MAX));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[0]), 0, UINT16_MAX));
        dstPtr[1] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[1]), 0, UINT16_MAX));
        dstPtr[2] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[2]), 0, UINT16_MAX));
        dstPtr[3] = Math::Ftoui16(Clamp<float>(Math::Round(srcPtr[3]), 0, UINT16_MAX));
    }
#endif
}

static void RGBA32FToL32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = 0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2];
    }
}

static void RGBA32FToL32FX32(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = 0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2];
    }
}

static void RGBA32FToA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = srcPtr[3];
    }
}

static void RGBA32FToLA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = 0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2];
        dstPtr[1] = srcPtr[3];
    }
}

static void RGBA32FToR32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 8) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r, g, b, a);

        storeu_256ps(r, dstPtr);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 4) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr);
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

static void RGBA32FToR32SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int32_t *dstPtr = reinterpret_cast<int32_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f minValue8 = set1_256ps(-2147483648.0f);
    const simd8f maxConvertible8 = set1_256ps(2147483520.0f);
    const simd8f maxExclusive8 = set1_256ps(2147483648.0f);
    const simd8i maxInt8 = set1_256epi32(INT32_MAX);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 8) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r, g, b, a);

        const simd8b maxMask = r >= maxExclusive8;

        // Match RoundToSInt32:
        // map NaN to zero, clamp to the safely convertible range,
        // add 0.5 and round toward negative infinity.
        r = select_256ps(zero8, r, r == r);
        r = min_256ps(max_256ps(r, minValue8), maxConvertible8);

        simd8i r32 = floor_256ps_to_epi32(r + half8);
        r32 = select_256epi32(r32, maxInt8, maxMask);

        storeu_si256(r32, dstPtr);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f minValue4 = set1_ps(-2147483648.0f);
    const simd4f maxConvertible4 = set1_ps(2147483520.0f);
    const simd4f maxExclusive4 = set1_ps(2147483648.0f);
    const simd4i maxInt4 = set1_epi32(INT32_MAX);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 4) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        simd4f r = rgba0;
        const simd4b maxMask = r >= maxExclusive4;

        // Match RoundToSInt32:
        // map NaN to zero, clamp to the safely convertible range,
        // add 0.5 and round toward negative infinity.
        r = select_ps(zero4, r, r == r);
        r = min_ps(max_ps(r, minValue4), maxConvertible4);

        simd4i r32 = floor_ps_to_epi32(r + half4);
        r32 = select_epi32(r32, maxInt4, maxMask);

        storeu_si128(r32, dstPtr);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = RoundToSInt32(srcPtr[0]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = RoundToSInt32(srcPtr[0]);
    }
#endif
}

static void RGBA32FToR32UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxConvertible8 = set1_256ps(4294967040.0f);
    const simd8f maxExclusive8 = set1_256ps(4294967296.0f);
    const simd8i maxInt8 = set1_256epi32(-1);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 8) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba45 = loadu_256ps(srcPtr + 16);
        const simd8f rgba67 = loadu_256ps(srcPtr + 24);

        const simd8f rgba04 = shuffle_256ps<0, 2>(rgba01, rgba45);
        const simd8f rgba15 = shuffle_256ps<1, 3>(rgba01, rgba45);
        const simd8f rgba26 = shuffle_256ps<0, 2>(rgba23, rgba67);
        const simd8f rgba37 = shuffle_256ps<1, 3>(rgba23, rgba67);

        simd8f r;
        simd8f g;
        simd8f b;
        simd8f a;
        transpose2x4x4(rgba04, rgba15, rgba26, rgba37, r, g, b, a);

        const simd8b maxMask = r >= maxExclusive8;

        // Match RoundToUInt32:
        // map NaN and negative values to zero, clamp to the safely
        // convertible range, add 0.5 and round toward negative infinity.
        r = select_256ps(zero8, r, r == r);
        r = min_256ps(max_256ps(r, zero8), maxConvertible8);
        r = floor_256ps(r + half8);

        simd8i r32 = trunc_256ps_to_epu32(r);
        r32 = select_256epi32(r32, maxInt8, maxMask);

        storeu_si256(r32, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxConvertible4 = set1_ps(4294967040.0f);
    const simd4f maxExclusive4 = set1_ps(4294967296.0f);
    const simd4i maxInt4 = set1_epi32(-1);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 4) {
        simd4f rgba0 = loadu_ps(srcPtr + 0);
        simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rgba2 = loadu_ps(srcPtr + 8);
        simd4f rgba3 = loadu_ps(srcPtr + 12);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        simd4f r = rgba0;
        const simd4b maxMask = r >= maxExclusive4;

        // Match RoundToUInt32:
        // map NaN and negative values to zero, clamp to the safely
        // convertible range, add 0.5 and round toward negative infinity.
        r = select_ps(zero4, r, r == r);
        r = min_ps(max_ps(r, zero4), maxConvertible4);
        r = floor_ps(r + half4);

        simd4i r32 = trunc_ps_to_epu32(r);
        r32 = select_epi32(r32, maxInt4, maxMask);

        storeu_si128(r32, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = RoundToUInt32(srcPtr[0]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        dstPtr[0] = RoundToUInt32(srcPtr[0]);
    }
#endif
}

static void RGBA32FToRG32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~3) * 4;

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 8) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
        const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
        const simd8f rg =
            shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);

        storeu_256ps(rg, dstPtr);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~1) * 4;

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 4) {
        const simd4f rgba0 = loadu_ps(srcPtr + 0);
        const simd4f rgba1 = loadu_ps(srcPtr + 4);

        storeu_ps(shuffle_ps<0, 1, 0, 1>(rgba0, rgba1), dstPtr);
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

static void RGBA32FToRG32SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int32_t *dstPtr = reinterpret_cast<int32_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~3) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f minValue8 = set1_256ps(-2147483648.0f);
    const simd8f maxConvertible8 = set1_256ps(2147483520.0f);
    const simd8f maxExclusive8 = set1_256ps(2147483648.0f);
    const simd8i maxInt8 = set1_256epi32(INT32_MAX);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 8) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
        const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
        simd8f rg = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
        const simd8b maxMask = rg >= maxExclusive8;

        // Match RoundToSInt32:
        // map NaN to zero, clamp to the safely convertible range,
        // add 0.5 and round toward negative infinity.
        rg = select_256ps(zero8, rg, rg == rg);
        rg = min_256ps(max_256ps(rg, minValue8), maxConvertible8);

        simd8i rg32 = floor_256ps_to_epi32(rg + half8);
        rg32 = select_256epi32(rg32, maxInt8, maxMask);

        storeu_si256(rg32, dstPtr);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~1) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f minValue4 = set1_ps(-2147483648.0f);
    const simd4f maxConvertible4 = set1_ps(2147483520.0f);
    const simd4f maxExclusive4 = set1_ps(2147483648.0f);
    const simd4i maxInt4 = set1_epi32(INT32_MAX);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 4) {
        const simd4f rgba0 = loadu_ps(srcPtr + 0);
        const simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rg = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
        const simd4b maxMask = rg >= maxExclusive4;

        // Match RoundToSInt32:
        // map NaN to zero, clamp to the safely convertible range,
        // add 0.5 and round toward negative infinity.
        rg = select_ps(zero4, rg, rg == rg);
        rg = min_ps(max_ps(rg, minValue4), maxConvertible4);

        simd4i rg32 = floor_ps_to_epi32(rg + half4);
        rg32 = select_epi32(rg32, maxInt4, maxMask);

        storeu_si128(rg32, dstPtr);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = RoundToSInt32(srcPtr[0]);
        dstPtr[1] = RoundToSInt32(srcPtr[1]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = RoundToSInt32(srcPtr[0]);
        dstPtr[1] = RoundToSInt32(srcPtr[1]);
    }
#endif
}

static void RGBA32FToRG32UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~3) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxConvertible8 = set1_256ps(4294967040.0f);
    const simd8f maxExclusive8 = set1_256ps(4294967296.0f);
    const simd8i maxInt8 = set1_256epi32(-1);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 8) {
        const simd8f rgba01 = loadu_256ps(srcPtr + 0);
        const simd8f rgba23 = loadu_256ps(srcPtr + 8);
        const simd8f rgba02 = shuffle_256ps<0, 2>(rgba01, rgba23);
        const simd8f rgba13 = shuffle_256ps<1, 3>(rgba01, rgba23);
        simd8f rg = shuffle_256ps<0, 1, 0, 1>(rgba02, rgba13);
        const simd8b maxMask = rg >= maxExclusive8;

        // Match RoundToUInt32:
        // map NaN and negative values to zero, clamp to the safely
        // convertible range, add 0.5 and round toward negative infinity.
        rg = select_256ps(zero8, rg, rg == rg);
        rg = min_256ps(max_256ps(rg, zero8), maxConvertible8);
        rg = floor_256ps(rg + half8);

        simd8i rg32 = trunc_256ps_to_epu32(rg);
        rg32 = select_256epi32(rg32, maxInt8, maxMask);

        storeu_si256(rg32, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const float *simd4End = srcPtr + (remainingPixels & ~1) * 4;
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxConvertible4 = set1_ps(4294967040.0f);
    const simd4f maxExclusive4 = set1_ps(4294967296.0f);
    const simd4i maxInt4 = set1_epi32(-1);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 4) {
        const simd4f rgba0 = loadu_ps(srcPtr + 0);
        const simd4f rgba1 = loadu_ps(srcPtr + 4);
        simd4f rg = shuffle_ps<0, 1, 0, 1>(rgba0, rgba1);
        const simd4b maxMask = rg >= maxExclusive4;

        // Match RoundToUInt32:
        // map NaN and negative values to zero, clamp to the safely
        // convertible range, add 0.5 and round toward negative infinity.
        rg = select_ps(zero4, rg, rg == rg);
        rg = min_ps(max_ps(rg, zero4), maxConvertible4);
        rg = floor_ps(rg + half4);

        simd4i rg32 = trunc_ps_to_epu32(rg);
        rg32 = select_epi32(rg32, maxInt4, maxMask);

        storeu_si128(rg32, reinterpret_cast<int32_t *>(dstPtr));
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = RoundToUInt32(srcPtr[0]);
        dstPtr[1] = RoundToUInt32(srcPtr[1]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = RoundToUInt32(srcPtr[0]);
        dstPtr[1] = RoundToUInt32(srcPtr[1]);
    }
#endif
}

static void RGBA32FToRGB32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
    }
}

static void RGBA32FToRGB32SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int32_t *dstPtr = reinterpret_cast<int32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = RoundToSInt32(srcPtr[0]);
        dstPtr[1] = RoundToSInt32(srcPtr[1]);
        dstPtr[2] = RoundToSInt32(srcPtr[2]);
    }
}

static void RGBA32FToRGB32UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = RoundToUInt32(srcPtr[0]);
        dstPtr[1] = RoundToUInt32(srcPtr[1]);
        dstPtr[2] = RoundToUInt32(srcPtr[2]);
    }
}

static void RGBA32FToRGB11F11F10F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        uint32_t r = F11Converter::FromF32(srcPtr[0]);
        uint32_t g = F11Converter::FromF32(srcPtr[1]);
        uint32_t b = F10Converter::FromF32(srcPtr[2]);
        *dstPtr = r | (g << 11) | (b << 22);
    }
}

BE_NAMESPACE_END
