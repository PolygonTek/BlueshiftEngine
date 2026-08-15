#include "Precompiled.h"
#include "Math/MathCommon.h"
#include "Math/FloatConverter.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
//
// XXXToRGBA32F (unpacking function from custom format to rgba32f)
//
//--------------------------------------------------------------------------------------------------

static void L8ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void A8ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = 1.0f;
        dstPtr[3] = srcPtr[0] * invNorm;
    }
}

static void LA88ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = srcPtr[1] * invNorm;
    }
}

static void L16ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT16_MAX;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);
    float l;

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        l = srcPtr[0] * invNorm;
        dstPtr[0] = l;
        dstPtr[1] = l;
        dstPtr[2] = l;
        dstPtr[3] = 1.0f;
    }
}

static void L24X8ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 0x00FFFFFF;
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);
    float l;

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        l = (srcPtr[0] & 0x00FFFFFF) * invNorm;
        dstPtr[0] = l;
        dstPtr[1] = l;
        dstPtr[2] = l;
        dstPtr[3] = 1.0f;
    }
}

static void R8ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = reinterpret_cast<const byte *>(src);
    const byte *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~31);
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 128) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epu8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8f r = epi32_to_256ps(values[group]) * scale8;
            simd8f rgba04;
            simd8f rgba15;
            simd8f rgba26;
            simd8f rgba37;
            transpose2x4x4(r, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

            float *groupDst = dstPtr + group * 32;
            storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), groupDst + 0);
            storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), groupDst + 8);
            storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), groupDst + 16);
            storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), groupDst + 24);
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const byte *simd4End = srcPtr + (remainingPixels & ~15);
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 64) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epu8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            simd4f rgba0 = epi32_to_ps(values[group]) * scale4;
            simd4f rgba1 = zero4;
            simd4f rgba2 = zero4;
            simd4f rgba3 = one4;
            transpose4x4(rgba0, rgba1, rgba2, rgba3);

            float *groupDst = dstPtr + group * 16;
            storeu_ps(rgba0, groupDst + 0);
            storeu_ps(rgba1, groupDst + 4);
            storeu_ps(rgba2, groupDst + 8);
            storeu_ps(rgba3, groupDst + 12);
        }
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void R8SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
#if defined(ENABLE_SIMD8_INTRIN) || defined(ENABLE_SIMD4_INTRIN)
    const float invNorm = 1.0f / INT8_MAX;
#endif
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int8_t *simd8End = srcPtr + (numPixels & ~31);
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 128) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epi8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8f r = max_256ps(epi32_to_256ps(values[group]) * scale8, minValue8);
            simd8f rgba04;
            simd8f rgba15;
            simd8f rgba26;
            simd8f rgba37;
            transpose2x4x4(r, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

            float *groupDst = dstPtr + group * 32;
            storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), groupDst + 0);
            storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), groupDst + 8);
            storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), groupDst + 16);
            storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), groupDst + 24);
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const int8_t *simd4End = srcPtr + (remainingPixels & ~15);
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 64) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epi8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            simd4f rgba0 = max_ps(epi32_to_ps(values[group]) * scale4, minValue4);
            simd4f rgba1 = zero4;
            simd4f rgba2 = zero4;
            simd4f rgba3 = one4;
            transpose4x4(rgba0, rgba1, rgba2, rgba3);

            float *groupDst = dstPtr + group * 16;
            storeu_ps(rgba0, groupDst + 0);
            storeu_ps(rgba1, groupDst + 4);
            storeu_ps(rgba2, groupDst + 8);
            storeu_ps(rgba3, groupDst + 12);
        }
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = Math::SNorm8ToFloat(srcPtr[0]);
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = Math::SNorm8ToFloat(srcPtr[0]);
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void R8SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int8_t *simd8End = srcPtr + (numPixels & ~31);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 128) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epi8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8f r = epi32_to_256ps(values[group]);
            simd8f rgba04;
            simd8f rgba15;
            simd8f rgba26;
            simd8f rgba37;
            transpose2x4x4(r, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

            float *groupDst = dstPtr + group * 32;
            storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), groupDst + 0);
            storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), groupDst + 8);
            storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), groupDst + 16);
            storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), groupDst + 24);
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const int8_t *simd4End = srcPtr + (remainingPixels & ~15);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 64) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epi8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            simd4f rgba0 = epi32_to_ps(values[group]);
            simd4f rgba1 = zero4;
            simd4f rgba2 = zero4;
            simd4f rgba3 = one4;
            transpose4x4(rgba0, rgba1, rgba2, rgba3);

            float *groupDst = dstPtr + group * 16;
            storeu_ps(rgba0, groupDst + 0);
            storeu_ps(rgba1, groupDst + 4);
            storeu_ps(rgba2, groupDst + 8);
            storeu_ps(rgba3, groupDst + 12);
        }
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void R8UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint8_t *simd8End = srcPtr + (numPixels & ~31);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 128) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epu8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8f r = epi32_to_256ps(values[group]);
            simd8f rgba04;
            simd8f rgba15;
            simd8f rgba26;
            simd8f rgba37;
            transpose2x4x4(r, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

            float *groupDst = dstPtr + group * 32;
            storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), groupDst + 0);
            storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), groupDst + 8);
            storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), groupDst + 16);
            storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), groupDst + 24);
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint8_t *simd4End = srcPtr + (remainingPixels & ~15);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 64) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epu8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            simd4f rgba0 = epi32_to_ps(values[group]);
            simd4f rgba1 = zero4;
            simd4f rgba2 = zero4;
            simd4f rgba3 = one4;
            transpose4x4(rgba0, rgba1, rgba2, rgba3);

            float *groupDst = dstPtr + group * 16;
            storeu_ps(rgba0, groupDst + 0);
            storeu_ps(rgba1, groupDst + 4);
            storeu_ps(rgba2, groupDst + 8);
            storeu_ps(rgba3, groupDst + 12);
        }
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG88ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = reinterpret_cast<const byte *>(src);
    const byte *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~15) * 2;
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 64) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epu8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8f rg = epi32_to_256ps(values[group]) * scale8;
            const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg, zeroOne8);
            const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg, zeroOne8);

            float *groupDst = dstPtr + group * 16;
            storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), groupDst + 0);
            storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), groupDst + 8);
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const byte *simd4End = srcPtr + (remainingPixels & ~7) * 2;
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 32) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epu8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd4f rg = epi32_to_ps(values[group]) * scale4;
            float *groupDst = dstPtr + group * 8;
            storeu_ps(shuffle_ps<0, 1, 0, 1>(rg, zeroOne4), groupDst + 0);
            storeu_ps(shuffle_ps<2, 3, 0, 1>(rg, zeroOne4), groupDst + 4);
        }
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG88SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
#if defined(ENABLE_SIMD8_INTRIN) || defined(ENABLE_SIMD4_INTRIN)
    const float invNorm = 1.0f / INT8_MAX;
#endif
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int8_t *simd8End = srcPtr + (numPixels & ~15) * 2;
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 64) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epi8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8f rg = max_256ps(epi32_to_256ps(values[group]) * scale8, minValue8);
            const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg, zeroOne8);
            const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg, zeroOne8);

            float *groupDst = dstPtr + group * 16;
            storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), groupDst + 0);
            storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), groupDst + 8);
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const int8_t *simd4End = srcPtr + (remainingPixels & ~7) * 2;
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 32) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epi8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd4f rg = max_ps(epi32_to_ps(values[group]) * scale4, minValue4);
            float *groupDst = dstPtr + group * 8;
            storeu_ps(shuffle_ps<0, 1, 0, 1>(rg, zeroOne4), groupDst + 0);
            storeu_ps(shuffle_ps<2, 3, 0, 1>(rg, zeroOne4), groupDst + 4);
        }
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = Math::SNorm8ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm8ToFloat(srcPtr[1]);
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = Math::SNorm8ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm8ToFloat(srcPtr[1]);
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG88SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int8_t *simd8End = srcPtr + (numPixels & ~15) * 2;

    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 64) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epi8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8f rg = epi32_to_256ps(values[group]);
            const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg, zeroOne8);
            const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg, zeroOne8);

            float *groupDst = dstPtr + group * 16;
            storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), groupDst + 0);
            storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), groupDst + 8);
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const int8_t *simd4End = srcPtr + (remainingPixels & ~7) * 2;

    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 32) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epi8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd4f rg = epi32_to_ps(values[group]);
            float *groupDst = dstPtr + group * 8;
            storeu_ps(shuffle_ps<0, 1, 0, 1>(rg, zeroOne4), groupDst + 0);
            storeu_ps(shuffle_ps<2, 3, 0, 1>(rg, zeroOne4), groupDst + 4);
        }
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG88UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint8_t *simd8End = srcPtr + (numPixels & ~15) * 2;

    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 64) {
        const simd8i packed8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        simd8i values[4];
        unpack_epu8_to_256epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd8f rg = epi32_to_256ps(values[group]);
            const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg, zeroOne8);
            const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg, zeroOne8);

            float *groupDst = dstPtr + group * 16;
            storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), groupDst + 0);
            storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), groupDst + 8);
        }
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const uint8_t *simd4End = srcPtr + (remainingPixels & ~7) * 2;

    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 32) {
        const simd4i packed8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        simd4i values[4];
        unpack_epu8_to_epi32(packed8, values[0], values[1], values[2], values[3]);

        for (int group = 0; group < 4; group++) {
            const simd4f rg = epi32_to_ps(values[group]);
            float *groupDst = dstPtr + group * 8;
            storeu_ps(shuffle_ps<0, 1, 0, 1>(rg, zeroOne4), groupDst + 0);
            storeu_ps(shuffle_ps<2, 3, 0, 1>(rg, zeroOne4), groupDst + 4);
        }
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RGB888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[2] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void RGB888SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = Math::SNorm8ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm8ToFloat(srcPtr[1]);
        dstPtr[2] = Math::SNorm8ToFloat(srcPtr[2]);
        dstPtr[3] = 1.0f;
    }
}

static void RGB888SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGB888UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void BGR888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[2] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void BGRX8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(invNorm);
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
        dstPtr[0] = srcPtr[2] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void BGRA8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8i keepMask8 = set1_256epi32(static_cast<int32_t>(0xFF00FF00u));
    const simd8i highRBMask8 = set1_256epi32(0x00FF0000);
    const simd8i lowRBMask8 = set1_256epi32(0x000000FF);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i bgra8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i rgba8 = (bgra8 & keepMask8) |
            srl_256epi32(bgra8 & highRBMask8, 16) |
            ((bgra8 & lowRBMask8) << 16);

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
    const simd4i keepMask4 = set1_epi32(static_cast<int32_t>(0xFF00FF00u));
    const simd4i highRBMask4 = set1_epi32(0x00FF0000);
    const simd4i lowRBMask4 = set1_epi32(0x000000FF);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i bgra8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i rgba8 = (bgra8 & keepMask4) |
            srl_epi32(bgra8 & highRBMask4, 16) |
            ((bgra8 & lowRBMask4) << 16);

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
        dstPtr[0] = srcPtr[2] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = srcPtr[3] * invNorm;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = srcPtr[3] * invNorm;
    }
#endif
}

static void ABGR8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8i byteMask8 = set1_256epi32(0x00FF00FF);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i abgr8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i swapped8 = ((abgr8 & byteMask8) << 8) |
            (srl_256epi32(abgr8, 8) & byteMask8);
        const simd8i rgba8 = (swapped8 << 16) | srl_256epi32(swapped8, 16);

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
    const simd4i byteMask4 = set1_epi32(0x00FF00FF);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i abgr8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i swapped8 = ((abgr8 & byteMask4) << 8) |
            (srl_epi32(abgr8, 8) & byteMask4);
        const simd4i rgba8 = (swapped8 << 16) | srl_epi32(swapped8, 16);

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
        dstPtr[0] = srcPtr[3] * invNorm;
        dstPtr[1] = srcPtr[2] * invNorm;
        dstPtr[2] = srcPtr[1] * invNorm;
        dstPtr[3] = srcPtr[0] * invNorm;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[3] * invNorm;
        dstPtr[1] = srcPtr[2] * invNorm;
        dstPtr[2] = srcPtr[1] * invNorm;
        dstPtr[3] = srcPtr[0] * invNorm;
    }
#endif
}

static void ARGB8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(invNorm);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i argb8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i rgba8 = srl_256epi32(argb8, 8) | (argb8 << 24);

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
        const simd4i argb8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i rgba8 = srl_epi32(argb8, 8) | (argb8 << 24);

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
        dstPtr[0] = srcPtr[1] * invNorm;
        dstPtr[1] = srcPtr[2] * invNorm;
        dstPtr[2] = srcPtr[3] * invNorm;
        dstPtr[3] = srcPtr[0] * invNorm;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[1] * invNorm;
        dstPtr[1] = srcPtr[2] * invNorm;
        dstPtr[2] = srcPtr[3] * invNorm;
        dstPtr[3] = srcPtr[0] * invNorm;
    }
#endif
}

static void RGBA1010102ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float inv1023 = 1.0f / 1023.0f;
    const float inv3 = 1.0f / 3.0f;
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint32_t *simd8End = srcPtr + (numPixels & ~7);
    const simd8i rgbMask8 = set1_256epi32(0x3FF);
    const simd8i alphaMask8 = set1_256epi32(0x3);
    const simd8f inv1023_8 = set1_256ps(inv1023);
    const simd8f inv3_8 = set1_256ps(inv3);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 32) {
        const simd8i packed = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8f r = epi32_to_256ps(packed & rgbMask8) * inv1023_8;
        simd8f g = epi32_to_256ps(srl_256epi32(packed, 10) & rgbMask8) * inv1023_8;
        simd8f b = epi32_to_256ps(srl_256epi32(packed, 20) & rgbMask8) * inv1023_8;
        simd8f a = epi32_to_256ps(srl_256epi32(packed, 30) & alphaMask8) * inv3_8;

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r, g, b, a, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint32_t *simd4End = srcPtr + (remainingPixels & ~3);
    const simd4i rgbMask4 = set1_epi32(0x3FF);
    const simd4i alphaMask4 = set1_epi32(0x3);
    const simd4f inv1023_4 = set1_ps(inv1023);
    const simd4f inv3_4 = set1_ps(inv3);

    for (; srcPtr < simd4End; srcPtr += 4, dstPtr += 16) {
        const simd4i packed = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4f rgba0 = epi32_to_ps(packed & rgbMask4) * inv1023_4;
        simd4f rgba1 = epi32_to_ps(srl_epi32(packed, 10) & rgbMask4) * inv1023_4;
        simd4f rgba2 = epi32_to_ps(srl_epi32(packed, 20) & rgbMask4) * inv1023_4;
        simd4f rgba3 = epi32_to_ps(srl_epi32(packed, 30) & alphaMask4) * inv3_4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = (*srcPtr & 0x3FF) * inv1023;
        dstPtr[1] = ((*srcPtr >> 10) & 0x3FF) * inv1023;
        dstPtr[2] = ((*srcPtr >> 20) & 0x3FF) * inv1023;
        dstPtr[3] = ((*srcPtr >> 30) & 0x3) * inv3;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = (*srcPtr & 0x3FF) * inv1023;
        dstPtr[1] = ((*srcPtr >> 10) & 0x3FF) * inv1023;
        dstPtr[2] = ((*srcPtr >> 20) & 0x3FF) * inv1023;
        dstPtr[3] = ((*srcPtr >> 30) & 0x3) * inv3;
    }
#endif
}

static void RGBA1010102UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint32_t *simd8End = srcPtr + (numPixels & ~7);
    const simd8i rgbMask8 = set1_256epi32(0x3FF);
    const simd8i alphaMask8 = set1_256epi32(0x3);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 32) {
        const simd8i packed = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8f r = epi32_to_256ps(packed & rgbMask8);
        simd8f g = epi32_to_256ps(srl_256epi32(packed, 10) & rgbMask8);
        simd8f b = epi32_to_256ps(srl_256epi32(packed, 20) & rgbMask8);
        simd8f a = epi32_to_256ps(srl_256epi32(packed, 30) & alphaMask8);

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r, g, b, a, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint32_t *simd4End = srcPtr + (remainingPixels & ~3);
    const simd4i rgbMask4 = set1_epi32(0x3FF);
    const simd4i alphaMask4 = set1_epi32(0x3);

    for (; srcPtr < simd4End; srcPtr += 4, dstPtr += 16) {
        const simd4i packed = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4f rgba0 = epi32_to_ps(packed & rgbMask4);
        simd4f rgba1 = epi32_to_ps(srl_epi32(packed, 10) & rgbMask4);
        simd4f rgba2 = epi32_to_ps(srl_epi32(packed, 20) & rgbMask4);
        simd4f rgba3 = epi32_to_ps(srl_epi32(packed, 30) & alphaMask4);

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = (*srcPtr & 0x3FF);
        dstPtr[1] = ((*srcPtr >> 10) & 0x3FF);
        dstPtr[2] = ((*srcPtr >> 20) & 0x3FF);
        dstPtr[3] = ((*srcPtr >> 30) & 0x3);
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = (*srcPtr & 0x3FF);
        dstPtr[1] = ((*srcPtr >> 10) & 0x3FF);
        dstPtr[2] = ((*srcPtr >> 20) & 0x3FF);
        dstPtr[3] = ((*srcPtr >> 30) & 0x3);
    }
#endif
}

static void RGBA8888SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
#if defined(ENABLE_SIMD8_INTRIN) || defined(ENABLE_SIMD4_INTRIN)
    const float invNorm = 1.0f / INT8_MAX;
#endif
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int8_t *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f invNorm8 = set1_256ps(invNorm);
    const simd8f minValue8 = set1_256ps(-1.0f);

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rgba32_0;
        simd8i rgba32_1;
        simd8i rgba32_2;
        simd8i rgba32_3;
        unpack_epi8_to_256epi32(rgba8, rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_256ps(max_256ps(epi32_to_256ps(rgba32_0) * invNorm8, minValue8), dstPtr + 0);
        storeu_256ps(max_256ps(epi32_to_256ps(rgba32_1) * invNorm8, minValue8), dstPtr + 8);
        storeu_256ps(max_256ps(epi32_to_256ps(rgba32_2) * invNorm8, minValue8), dstPtr + 16);
        storeu_256ps(max_256ps(epi32_to_256ps(rgba32_3) * invNorm8, minValue8), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const int8_t *simd4End = srcPtr + (remainingPixels & ~3) * 4;
    const simd4f invNorm4 = set1_ps(invNorm);
    const simd4f minValue4 = set1_ps(-1.0f);

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rgba32_0;
        simd4i rgba32_1;
        simd4i rgba32_2;
        simd4i rgba32_3;
        unpack_epi8_to_epi32(rgba8, rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_ps(max_ps(epi32_to_ps(rgba32_0) * invNorm4, minValue4), dstPtr + 0);
        storeu_ps(max_ps(epi32_to_ps(rgba32_1) * invNorm4, minValue4), dstPtr + 4);
        storeu_ps(max_ps(epi32_to_ps(rgba32_2) * invNorm4, minValue4), dstPtr + 8);
        storeu_ps(max_ps(epi32_to_ps(rgba32_3) * invNorm4, minValue4), dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::SNorm8ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm8ToFloat(srcPtr[1]);
        dstPtr[2] = Math::SNorm8ToFloat(srcPtr[2]);
        dstPtr[3] = Math::SNorm8ToFloat(srcPtr[3]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::SNorm8ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm8ToFloat(srcPtr[1]);
        dstPtr[2] = Math::SNorm8ToFloat(srcPtr[2]);
        dstPtr[3] = Math::SNorm8ToFloat(srcPtr[3]);
    }
#endif
}

static void RGBA8888SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int8_t *simd8End = srcPtr + (numPixels & ~7) * 4;

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rgba32_0;
        simd8i rgba32_1;
        simd8i rgba32_2;
        simd8i rgba32_3;
        unpack_epi8_to_256epi32(rgba8, rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_256ps(epi32_to_256ps(rgba32_0), dstPtr + 0);
        storeu_256ps(epi32_to_256ps(rgba32_1), dstPtr + 8);
        storeu_256ps(epi32_to_256ps(rgba32_2), dstPtr + 16);
        storeu_256ps(epi32_to_256ps(rgba32_3), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const int8_t *simd4End = srcPtr + (remainingPixels & ~3) * 4;

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rgba32_0;
        simd4i rgba32_1;
        simd4i rgba32_2;
        simd4i rgba32_3;
        unpack_epi8_to_epi32(rgba8, rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_ps(epi32_to_ps(rgba32_0), dstPtr + 0);
        storeu_ps(epi32_to_ps(rgba32_1), dstPtr + 4);
        storeu_ps(epi32_to_ps(rgba32_2), dstPtr + 8);
        storeu_ps(epi32_to_ps(rgba32_3), dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#endif
}

static void RGBA8888UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint8_t *simd8End = srcPtr + (numPixels & ~7) * 4;

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgba8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rgba32_0;
        simd8i rgba32_1;
        simd8i rgba32_2;
        simd8i rgba32_3;
        unpack_epu8_to_256epi32(rgba8, rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_256ps(epi32_to_256ps(rgba32_0), dstPtr + 0);
        storeu_256ps(epi32_to_256ps(rgba32_1), dstPtr + 8);
        storeu_256ps(epi32_to_256ps(rgba32_2), dstPtr + 16);
        storeu_256ps(epi32_to_256ps(rgba32_3), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const uint8_t *simd4End = srcPtr + (remainingPixels & ~3) * 4;

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgba8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rgba32_0;
        simd4i rgba32_1;
        simd4i rgba32_2;
        simd4i rgba32_3;
        unpack_epu8_to_epi32(rgba8, rgba32_0, rgba32_1, rgba32_2, rgba32_3);

        storeu_ps(epi32_to_ps(rgba32_0), dstPtr + 0);
        storeu_ps(epi32_to_ps(rgba32_1), dstPtr + 4);
        storeu_ps(epi32_to_ps(rgba32_2), dstPtr + 8);
        storeu_ps(epi32_to_ps(rgba32_3), dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#endif
}

static void RGBX8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT8_MAX;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const byte *simd8End = srcPtr + (numPixels & ~7) * 4;
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8i rgbMask8 = set1_256epi32(0x00FFFFFF);
    const simd8i alphaMask8 = set1_256epi32(static_cast<int32_t>(0xFF000000u));

    for (; srcPtr < simd8End; srcPtr += 32, dstPtr += 32) {
        const simd8i rgbx8 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8i rgba8 = (rgbx8 & rgbMask8) | alphaMask8;

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
    const simd4i rgbMask4 = set1_epi32(0x00FFFFFF);
    const simd4i alphaMask4 = set1_epi32(static_cast<int32_t>(0xFF000000u));

    for (; srcPtr < simd4End; srcPtr += 16, dstPtr += 16) {
        const simd4i rgbx8 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4i rgba8 = (rgbx8 & rgbMask4) | alphaMask4;

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
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[2] * invNorm;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RGBE9995ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);
    float m;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        m = Math::Ldexp(1.0f, static_cast<int>((*srcPtr >> 27) & 0x1Fu) - 24);
        dstPtr[0] = (*srcPtr & 0x1FF) * m;
        dstPtr[1] = ((*srcPtr >> 9) & 0x1FF) * m;
        dstPtr[2] = ((*srcPtr >> 18) & 0x1FF) * m;
        dstPtr[3] = 1.0f;
    }
}

static void RGBX4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void BGRX4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 0) & 0xF) * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void RGBA4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[3] = ((*srcPtr >> 12) & 0xF) * invNorm;
    }
}

static void BGRA4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 0) & 0xF) * invNorm;
        dstPtr[3] = ((*srcPtr >> 12) & 0xF) * invNorm;
    }
}

static void ABGR4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 12) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[3] = ((*srcPtr >> 0) & 0xF) * invNorm;
    }
}

static void ARGB4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 12) & 0xF) * invNorm;
        dstPtr[3] = ((*srcPtr >> 0) & 0xF) * invNorm;
    }
}

static void RGBX5551ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 5) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 10) & 0x1F) * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void BGRX5551ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 10) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 5) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 0) & 0x1F) * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void RGBA5551ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 5) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 10) & 0x1F) * invNorm;
        dstPtr[3] = ((*srcPtr >> 15));
    }
}

static void BGRA5551ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 10) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 5) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 0) & 0x1F) * invNorm;
        dstPtr[3] = ((*srcPtr >> 15));
    }
}

static void ABGR1555ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 11) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 6) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 1) & 0x1F) * invNorm;
        dstPtr[3] = ((*srcPtr >> 0) & 0x1);
    }
}

static void ARGB1555ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 1) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 6) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 11) & 0x1F) * invNorm;
        dstPtr[3] = ((*srcPtr >> 0) & 0x1);
    }
}

static void RGB565ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm5 = 1.0f / 31.0f;
    const float invNorm6 = 1.0f / 63.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0x1F) * invNorm5;
        dstPtr[1] = ((*srcPtr >> 5) & 0x3F) * invNorm6;
        dstPtr[2] = ((*srcPtr >> 11) & 0x1F) * invNorm5;
        dstPtr[3] = 1.0f;
    }
}

static void BGR565ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm5 = 1.0f / 31.0f;
    const float invNorm6 = 1.0f / 63.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 11) & 0x1F) * invNorm5;
        dstPtr[1] = ((*srcPtr >> 5) & 0x3F) * invNorm6;
        dstPtr[2] = ((*srcPtr >> 0) & 0x1F) * invNorm5;
        dstPtr[3] = 1.0f;
    }
}

static void L16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[3] = 1.0f;
    }
}

static void A16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = 1.0f;
        dstPtr[1] = 1.0f;
        dstPtr[2] = 1.0f;
        dstPtr[3] = F16Converter::ToF32(srcPtr[0]);
    }
}

static void LA16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[3] = F16Converter::ToF32(srcPtr[1]);
    }
}

static void R16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void R16ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT16_MAX;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~15);
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 64) {
        const simd8i r16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i r32_0;
        simd8i r32_1;
        unpack_epu16_to_256epi32(r16, r32_0, r32_1);

        const simd8f r0 = epi32_to_256ps(r32_0) * scale8;
        const simd8f r1 = epi32_to_256ps(r32_1) * scale8;

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r0, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);

        transpose2x4x4(r1, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 32);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 40);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 48);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 56);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~7);
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 32) {
        const simd4i r16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i r32_0;
        simd4i r32_1;
        unpack_epu16_to_epi32(r16, r32_0, r32_1);

        simd4f rgba0 = epi32_to_ps(r32_0) * scale4;
        simd4f rgba1 = zero4;
        simd4f rgba2 = zero4;
        simd4f rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);

        rgba0 = epi32_to_ps(r32_1) * scale4;
        rgba1 = zero4;
        rgba2 = zero4;
        rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 16);
        storeu_ps(rgba1, dstPtr + 20);
        storeu_ps(rgba2, dstPtr + 24);
        storeu_ps(rgba3, dstPtr + 28);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void R16SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
#if defined(ENABLE_SIMD8_INTRIN) || defined(ENABLE_SIMD4_INTRIN)
    const float invNorm = 1.0f / INT16_MAX;
#endif
    const int16_t *srcPtr = reinterpret_cast<const int16_t *>(src);
    const int16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int16_t *simd8End = srcPtr + (numPixels & ~15);
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 64) {
        const simd8i r16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i r32_0;
        simd8i r32_1;
        unpack_epi16_to_256epi32(r16, r32_0, r32_1);

        const simd8f r0 = max_256ps(epi32_to_256ps(r32_0) * scale8, minValue8);
        const simd8f r1 = max_256ps(epi32_to_256ps(r32_1) * scale8, minValue8);

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r0, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);

        transpose2x4x4(r1, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 32);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 40);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 48);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 56);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const int16_t *simd4End = srcPtr + (remainingPixels & ~7);
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 32) {
        const simd4i r16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i r32_0;
        simd4i r32_1;
        unpack_epi16_to_epi32(r16, r32_0, r32_1);

        simd4f rgba0 = max_ps(epi32_to_ps(r32_0) * scale4, minValue4);
        simd4f rgba1 = zero4;
        simd4f rgba2 = zero4;
        simd4f rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);

        rgba0 = max_ps(epi32_to_ps(r32_1) * scale4, minValue4);
        rgba1 = zero4;
        rgba2 = zero4;
        rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 16);
        storeu_ps(rgba1, dstPtr + 20);
        storeu_ps(rgba2, dstPtr + 24);
        storeu_ps(rgba3, dstPtr + 28);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = Math::SNorm16ToFloat(srcPtr[0]);
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = Math::SNorm16ToFloat(srcPtr[0]);
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void R16SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int16_t *srcPtr = reinterpret_cast<const int16_t *>(src);
    const int16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int16_t *simd8End = srcPtr + (numPixels & ~15);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 64) {
        const simd8i r16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i r32_0;
        simd8i r32_1;
        unpack_epi16_to_256epi32(r16, r32_0, r32_1);

        const simd8f r0 = epi32_to_256ps(r32_0);
        const simd8f r1 = epi32_to_256ps(r32_1);

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r0, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);

        transpose2x4x4(r1, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 32);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 40);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 48);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 56);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const int16_t *simd4End = srcPtr + (remainingPixels & ~7);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 32) {
        const simd4i r16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i r32_0;
        simd4i r32_1;
        unpack_epi16_to_epi32(r16, r32_0, r32_1);

        simd4f rgba0 = epi32_to_ps(r32_0);
        simd4f rgba1 = zero4;
        simd4f rgba2 = zero4;
        simd4f rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);

        rgba0 = epi32_to_ps(r32_1);
        rgba1 = zero4;
        rgba2 = zero4;
        rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 16);
        storeu_ps(rgba1, dstPtr + 20);
        storeu_ps(rgba2, dstPtr + 24);
        storeu_ps(rgba3, dstPtr + 28);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void R16UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~15);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 64) {
        const simd8i r16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i r32_0;
        simd8i r32_1;
        unpack_epu16_to_256epi32(r16, r32_0, r32_1);

        const simd8f r0 = epi32_to_256ps(r32_0);
        const simd8f r1 = epi32_to_256ps(r32_1);

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r0, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);

        transpose2x4x4(r1, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 32);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 40);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 48);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 56);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~7);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 32) {
        const simd4i r16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i r32_0;
        simd4i r32_1;
        unpack_epu16_to_epi32(r16, r32_0, r32_1);

        simd4f rgba0 = epi32_to_ps(r32_0);
        simd4f rgba1 = zero4;
        simd4f rgba2 = zero4;
        simd4f rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);

        rgba0 = epi32_to_ps(r32_1);
        rgba1 = zero4;
        rgba2 = zero4;
        rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 16);
        storeu_ps(rgba1, dstPtr + 20);
        storeu_ps(rgba2, dstPtr + 24);
        storeu_ps(rgba3, dstPtr + 28);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[1] = F16Converter::ToF32(srcPtr[1]);
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG16ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT16_MAX;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~7) * 2;
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 32) {
        const simd8i rg16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rg32_0;
        simd8i rg32_1;
        unpack_epu16_to_256epi32(rg16, rg32_0, rg32_1);

        const simd8f rg0 = epi32_to_256ps(rg32_0) * scale8;
        const simd8f rg1 = epi32_to_256ps(rg32_1) * scale8;

        const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg0, zeroOne8);
        const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg0, zeroOne8);
        const simd8f rgba46 = shuffle_256ps<0, 1, 0, 1>(rg1, zeroOne8);
        const simd8f rgba57 = shuffle_256ps<2, 3, 0, 1>(rg1, zeroOne8);

        storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), dstPtr + 0);
        storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), dstPtr + 8);
        storeu_256ps(shuffle_256ps<0, 2>(rgba46, rgba57), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba46, rgba57), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~3) * 2;
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 16) {
        const simd4i rg16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rg32_0;
        simd4i rg32_1;
        unpack_epu16_to_epi32(rg16, rg32_0, rg32_1);

        const simd4f rg0 = epi32_to_ps(rg32_0) * scale4;
        const simd4f rg1 = epi32_to_ps(rg32_1) * scale4;

        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg0, zeroOne4), dstPtr + 0);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg0, zeroOne4), dstPtr + 4);
        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg1, zeroOne4), dstPtr + 8);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg1, zeroOne4), dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG16SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
#if defined(ENABLE_SIMD8_INTRIN) || defined(ENABLE_SIMD4_INTRIN)
    const float invNorm = 1.0f / INT16_MAX;
#endif
    const int16_t *srcPtr = reinterpret_cast<const int16_t *>(src);
    const int16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int16_t *simd8End = srcPtr + (numPixels & ~7) * 2;
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f minValue8 = set1_256ps(-1.0f);
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 32) {
        const simd8i rg16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rg32_0;
        simd8i rg32_1;
        unpack_epi16_to_256epi32(rg16, rg32_0, rg32_1);

        const simd8f rg0 = max_256ps(epi32_to_256ps(rg32_0) * scale8, minValue8);
        const simd8f rg1 = max_256ps(epi32_to_256ps(rg32_1) * scale8, minValue8);

        const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg0, zeroOne8);
        const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg0, zeroOne8);
        const simd8f rgba46 = shuffle_256ps<0, 1, 0, 1>(rg1, zeroOne8);
        const simd8f rgba57 = shuffle_256ps<2, 3, 0, 1>(rg1, zeroOne8);

        storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), dstPtr + 0);
        storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), dstPtr + 8);
        storeu_256ps(shuffle_256ps<0, 2>(rgba46, rgba57), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba46, rgba57), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const int16_t *simd4End = srcPtr + (remainingPixels & ~3) * 2;
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f minValue4 = set1_ps(-1.0f);
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 16) {
        const simd4i rg16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rg32_0;
        simd4i rg32_1;
        unpack_epi16_to_epi32(rg16, rg32_0, rg32_1);

        const simd4f rg0 = max_ps(epi32_to_ps(rg32_0) * scale4, minValue4);
        const simd4f rg1 = max_ps(epi32_to_ps(rg32_1) * scale4, minValue4);

        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg0, zeroOne4), dstPtr + 0);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg0, zeroOne4), dstPtr + 4);
        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg1, zeroOne4), dstPtr + 8);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg1, zeroOne4), dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = Math::SNorm16ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm16ToFloat(srcPtr[1]);
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = Math::SNorm16ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm16ToFloat(srcPtr[1]);
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG16SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int16_t *srcPtr = reinterpret_cast<const int16_t *>(src);
    const int16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int16_t *simd8End = srcPtr + (numPixels & ~7) * 2;
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 32) {
        const simd8i rg16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rg32_0;
        simd8i rg32_1;
        unpack_epi16_to_256epi32(rg16, rg32_0, rg32_1);

        const simd8f rg0 = epi32_to_256ps(rg32_0);
        const simd8f rg1 = epi32_to_256ps(rg32_1);

        const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg0, zeroOne8);
        const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg0, zeroOne8);
        const simd8f rgba46 = shuffle_256ps<0, 1, 0, 1>(rg1, zeroOne8);
        const simd8f rgba57 = shuffle_256ps<2, 3, 0, 1>(rg1, zeroOne8);

        storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), dstPtr + 0);
        storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), dstPtr + 8);
        storeu_256ps(shuffle_256ps<0, 2>(rgba46, rgba57), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba46, rgba57), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const int16_t *simd4End = srcPtr + (remainingPixels & ~3) * 2;
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 16) {
        const simd4i rg16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rg32_0;
        simd4i rg32_1;
        unpack_epi16_to_epi32(rg16, rg32_0, rg32_1);

        const simd4f rg0 = epi32_to_ps(rg32_0);
        const simd4f rg1 = epi32_to_ps(rg32_1);

        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg0, zeroOne4), dstPtr + 0);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg0, zeroOne4), dstPtr + 4);
        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg1, zeroOne4), dstPtr + 8);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg1, zeroOne4), dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG16UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~7) * 2;
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 32) {
        const simd8i rg16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rg32_0;
        simd8i rg32_1;
        unpack_epu16_to_256epi32(rg16, rg32_0, rg32_1);

        const simd8f rg0 = epi32_to_256ps(rg32_0);
        const simd8f rg1 = epi32_to_256ps(rg32_1);

        const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg0, zeroOne8);
        const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg0, zeroOne8);
        const simd8f rgba46 = shuffle_256ps<0, 1, 0, 1>(rg1, zeroOne8);
        const simd8f rgba57 = shuffle_256ps<2, 3, 0, 1>(rg1, zeroOne8);

        storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), dstPtr + 0);
        storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), dstPtr + 8);
        storeu_256ps(shuffle_256ps<0, 2>(rgba46, rgba57), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba46, rgba57), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~3) * 2;
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 16) {
        const simd4i rg16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rg32_0;
        simd4i rg32_1;
        unpack_epu16_to_epi32(rg16, rg32_0, rg32_1);

        const simd4f rg0 = epi32_to_ps(rg32_0);
        const simd4f rg1 = epi32_to_ps(rg32_1);

        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg0, zeroOne4), dstPtr + 0);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg0, zeroOne4), dstPtr + 4);
        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg1, zeroOne4), dstPtr + 8);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg1, zeroOne4), dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RGB16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[1] = F16Converter::ToF32(srcPtr[1]);
        dstPtr[2] = F16Converter::ToF32(srcPtr[2]);
        dstPtr[3] = 1.0f;
    }
}

static void RGB16ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT16_MAX;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[2] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void RGB16SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int16_t *srcPtr = reinterpret_cast<const int16_t *>(src);
    const int16_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = Math::SNorm16ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm16ToFloat(srcPtr[1]);
        dstPtr[2] = Math::SNorm16ToFloat(srcPtr[2]);
        dstPtr[3] = 1.0f;
    }
}

static void RGB16SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int16_t *srcPtr = reinterpret_cast<const int16_t *>(src);
    const int16_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGB16UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGBA16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[1] = F16Converter::ToF32(srcPtr[1]);
        dstPtr[2] = F16Converter::ToF32(srcPtr[2]);
        dstPtr[3] = F16Converter::ToF32(srcPtr[3]);
    }
}

static void RGBA16ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / UINT16_MAX;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~3) * 4;
    const simd8f scale8 = set1_256ps(invNorm);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 16) {
        const simd8i rgba16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rgba32_0;
        simd8i rgba32_1;
        unpack_epu16_to_256epi32(rgba16, rgba32_0, rgba32_1);

        storeu_256ps(epi32_to_256ps(rgba32_0) * scale8, dstPtr + 0);
        storeu_256ps(epi32_to_256ps(rgba32_1) * scale8, dstPtr + 8);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~1) * 4;
    const simd4f scale4 = set1_ps(invNorm);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 8) {
        const simd4i rgba16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rgba32_0;
        simd4i rgba32_1;
        unpack_epu16_to_epi32(rgba16, rgba32_0, rgba32_1);

        storeu_ps(epi32_to_ps(rgba32_0) * scale4, dstPtr + 0);
        storeu_ps(epi32_to_ps(rgba32_1) * scale4, dstPtr + 4);
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

static void RGBA16SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
#if defined(ENABLE_SIMD8_INTRIN) || defined(ENABLE_SIMD4_INTRIN)
    const float invNorm = 1.0f / INT16_MAX;
#endif
    const int16_t *srcPtr = reinterpret_cast<const int16_t *>(src);
    const int16_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int16_t *simd8End = srcPtr + (numPixels & ~3) * 4;
    const simd8f scale8 = set1_256ps(invNorm);
    const simd8f minValue8 = set1_256ps(-1.0f);

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 16) {
        const simd8i rgba16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rgba32_0;
        simd8i rgba32_1;
        unpack_epi16_to_256epi32(rgba16, rgba32_0, rgba32_1);

        const simd8f rgba0 = max_256ps(epi32_to_256ps(rgba32_0) * scale8, minValue8);
        const simd8f rgba1 = max_256ps(epi32_to_256ps(rgba32_1) * scale8, minValue8);
        storeu_256ps(rgba0, dstPtr + 0);
        storeu_256ps(rgba1, dstPtr + 8);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const int16_t *simd4End = srcPtr + (remainingPixels & ~1) * 4;
    const simd4f scale4 = set1_ps(invNorm);
    const simd4f minValue4 = set1_ps(-1.0f);

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 8) {
        const simd4i rgba16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rgba32_0;
        simd4i rgba32_1;
        unpack_epi16_to_epi32(rgba16, rgba32_0, rgba32_1);

        const simd4f rgba0 = max_ps(epi32_to_ps(rgba32_0) * scale4, minValue4);
        const simd4f rgba1 = max_ps(epi32_to_ps(rgba32_1) * scale4, minValue4);
        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::SNorm16ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm16ToFloat(srcPtr[1]);
        dstPtr[2] = Math::SNorm16ToFloat(srcPtr[2]);
        dstPtr[3] = Math::SNorm16ToFloat(srcPtr[3]);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::SNorm16ToFloat(srcPtr[0]);
        dstPtr[1] = Math::SNorm16ToFloat(srcPtr[1]);
        dstPtr[2] = Math::SNorm16ToFloat(srcPtr[2]);
        dstPtr[3] = Math::SNorm16ToFloat(srcPtr[3]);
    }
#endif
}

static void RGBA16SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int16_t *srcPtr = reinterpret_cast<const int16_t *>(src);
    const int16_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int16_t *simd8End = srcPtr + (numPixels & ~3) * 4;

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 16) {
        const simd8i rgba16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rgba32_0;
        simd8i rgba32_1;
        unpack_epi16_to_256epi32(rgba16, rgba32_0, rgba32_1);

        storeu_256ps(epi32_to_256ps(rgba32_0), dstPtr + 0);
        storeu_256ps(epi32_to_256ps(rgba32_1), dstPtr + 8);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const int16_t *simd4End = srcPtr + (remainingPixels & ~1) * 4;

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 8) {
        const simd4i rgba16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rgba32_0;
        simd4i rgba32_1;
        unpack_epi16_to_epi32(rgba16, rgba32_0, rgba32_1);

        storeu_ps(epi32_to_ps(rgba32_0), dstPtr + 0);
        storeu_ps(epi32_to_ps(rgba32_1), dstPtr + 4);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#endif
}

static void RGBA16UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint16_t *simd8End = srcPtr + (numPixels & ~3) * 4;

    for (; srcPtr < simd8End; srcPtr += 16, dstPtr += 16) {
        const simd8i rgba16 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));

        simd8i rgba32_0;
        simd8i rgba32_1;
        unpack_epu16_to_256epi32(rgba16, rgba32_0, rgba32_1);

        storeu_256ps(epi32_to_256ps(rgba32_0), dstPtr + 0);
        storeu_256ps(epi32_to_256ps(rgba32_1), dstPtr + 8);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 4);
    const uint16_t *simd4End = srcPtr + (remainingPixels & ~1) * 4;

    for (; srcPtr < simd4End; srcPtr += 8, dstPtr += 8) {
        const simd4i rgba16 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4i rgba32_0;
        simd4i rgba32_1;
        unpack_epu16_to_epi32(rgba16, rgba32_0, rgba32_1);

        storeu_ps(epi32_to_ps(rgba32_0), dstPtr + 0);
        storeu_ps(epi32_to_ps(rgba32_1), dstPtr + 4);
    }

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#endif
}

static void L32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0];
        dstPtr[3] = 1.0f;
    }
}

static void L32FX32ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0];
        dstPtr[3] = 1.0f;
    }
}

static void A32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = 1.0f;
        dstPtr[1] = 1.0f;
        dstPtr[2] = 1.0f;
        dstPtr[3] = srcPtr[0];
    }
}

static void LA32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[1];
    }
}

static void R32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~7);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 32) {
        const simd8f r = loadu_256ps(srcPtr);

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const float *simd4End = srcPtr + (remainingPixels & ~3);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 4, dstPtr += 16) {
        simd4f rgba0 = loadu_ps(srcPtr);
        simd4f rgba1 = zero4;
        simd4f rgba2 = zero4;
        simd4f rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0.0f;
        dstPtr[2] = 0.0f;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0.0f;
        dstPtr[2] = 0.0f;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void R32SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int32_t *srcPtr = reinterpret_cast<const int32_t *>(src);
    const int32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int32_t *simd8End = srcPtr + (numPixels & ~7);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 32) {
        const simd8i r32 = loadu_si256(srcPtr);
        const simd8f r = epi32_to_256ps(r32);

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const int32_t *simd4End = srcPtr + (remainingPixels & ~3);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 4, dstPtr += 16) {
        simd4f rgba0 = epi32_to_ps(loadu_si128(srcPtr));
        simd4f rgba1 = zero4;
        simd4f rgba2 = zero4;
        simd4f rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void R32UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint32_t *simd8End = srcPtr + (numPixels & ~7);
    const simd8f zero8 = setzero_256ps();
    const simd8f one8 = set1_256ps(1.0f);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 32) {
        const simd8i r32 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8f r = epu32_to_256ps(r32);

        simd8f rgba04;
        simd8f rgba15;
        simd8f rgba26;
        simd8f rgba37;
        transpose2x4x4(r, zero8, zero8, one8, rgba04, rgba15, rgba26, rgba37);

        storeu_256ps(shuffle_256ps<0, 2>(rgba04, rgba15), dstPtr + 0);
        storeu_256ps(shuffle_256ps<0, 2>(rgba26, rgba37), dstPtr + 8);
        storeu_256ps(shuffle_256ps<1, 3>(rgba04, rgba15), dstPtr + 16);
        storeu_256ps(shuffle_256ps<1, 3>(rgba26, rgba37), dstPtr + 24);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>(srcEnd - srcPtr);
    const uint32_t *simd4End = srcPtr + (remainingPixels & ~3);
    const simd4f zero4 = setzero_ps();
    const simd4f one4 = set1_ps(1.0f);

    for (; srcPtr < simd4End; srcPtr += 4, dstPtr += 16) {
        const simd4i r32 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));

        simd4f rgba0 = epu32_to_ps(r32);
        simd4f rgba1 = zero4;
        simd4f rgba2 = zero4;
        simd4f rgba3 = one4;

        transpose4x4(rgba0, rgba1, rgba2, rgba3);

        storeu_ps(rgba0, dstPtr + 0);
        storeu_ps(rgba1, dstPtr + 4);
        storeu_ps(rgba2, dstPtr + 8);
        storeu_ps(rgba3, dstPtr + 12);
    }

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~3) * 2;
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 16) {
        const simd8f rg = loadu_256ps(srcPtr);
        const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg, zeroOne8);
        const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg, zeroOne8);

        storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), dstPtr + 0);
        storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), dstPtr + 8);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const float *simd4End = srcPtr + (remainingPixels & ~1) * 2;
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 4, dstPtr += 8) {
        const simd4f rg = loadu_ps(srcPtr);

        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg, zeroOne4), dstPtr + 0);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg, zeroOne4), dstPtr + 4);
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG32SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int32_t *srcPtr = reinterpret_cast<const int32_t *>(src);
    const int32_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int32_t *simd8End = srcPtr + (numPixels & ~3) * 2;
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 16) {
        const simd8f rg = epi32_to_256ps(loadu_si256(srcPtr));
        const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg, zeroOne8);
        const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg, zeroOne8);

        storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), dstPtr + 0);
        storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), dstPtr + 8);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const int32_t *simd4End = srcPtr + (remainingPixels & ~1) * 2;
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 4, dstPtr += 8) {
        const simd4f rg = epi32_to_ps(loadu_si128(srcPtr));

        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg, zeroOne4), dstPtr + 0);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg, zeroOne4), dstPtr + 4);
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RG32UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint32_t *simd8End = srcPtr + (numPixels & ~3) * 2;
    const simd8f zeroOne8 = set_256ps(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 16) {
        const simd8i rg32 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        const simd8f rg = epu32_to_256ps(rg32);
        const simd8f rgba02 = shuffle_256ps<0, 1, 0, 1>(rg, zeroOne8);
        const simd8f rgba13 = shuffle_256ps<2, 3, 0, 1>(rg, zeroOne8);

        storeu_256ps(shuffle_256ps<0, 2>(rgba02, rgba13), dstPtr + 0);
        storeu_256ps(shuffle_256ps<1, 3>(rgba02, rgba13), dstPtr + 8);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const int remainingPixels = static_cast<int>((srcEnd - srcPtr) / 2);
    const uint32_t *simd4End = srcPtr + (remainingPixels & ~1) * 2;
    const simd4f zeroOne4 = set_ps(0.0f, 1.0f, 0.0f, 1.0f);

    for (; srcPtr < simd4End; srcPtr += 4, dstPtr += 8) {
        const simd4i rg32 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        const simd4f rg = epu32_to_ps(rg32);

        storeu_ps(shuffle_ps<0, 1, 0, 1>(rg, zeroOne4), dstPtr + 0);
        storeu_ps(shuffle_ps<2, 3, 0, 1>(rg, zeroOne4), dstPtr + 4);
    }

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
#endif
}

static void RGB32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGB32SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int32_t *srcPtr = reinterpret_cast<const int32_t *>(src);
    const int32_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGB32UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGBA32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    memcpy(dst, src, sizeof(float) * 4 * numPixels);
}

static BE_INLINE int32_t RoundToSInt32(float value) {
    const float rounded = Math::Round(value);

    constexpr float minValue = -2147483648.0f;
    constexpr float maxExclusive = 2147483648.0f;

    if (std::isnan(rounded)) {
        return 0;
    }
    if (rounded <= minValue) {
        return INT32_MIN;
    }
    if (rounded >= maxExclusive) {
        return INT32_MAX;
    }

    return static_cast<int32_t>(rounded);
}

static BE_INLINE uint32_t RoundToUInt32(float value) {
    const float rounded = Math::Round(value);

    constexpr float maxExclusive = 4294967296.0f;

    if (std::isnan(rounded) || rounded <= 0.0f) {
        return 0;
    }
    if (rounded >= maxExclusive) {
        return UINT32_MAX;
    }

    return static_cast<uint32_t>(rounded);
}

static void RGBA32FToRGBA32SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int32_t *dstPtr = reinterpret_cast<int32_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~1) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f minValue8 = set1_256ps(-2147483648.0f);
    const simd8f maxConvertible8 = set1_256ps(2147483520.0f);
    const simd8f maxExclusive8 = set1_256ps(2147483648.0f);
    const simd8i maxInt8 = set1_256epi32(INT32_MAX);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 8) {
        simd8f rgba = loadu_256ps(srcPtr);
        const simd8b maxMask = rgba >= maxExclusive8;

        // Match RoundToSInt32:
        // map NaN to zero, clamp to the safely convertible range,
        // add 0.5 and round toward negative infinity.
        rgba = select_256ps(zero8, rgba, rgba == rgba);
        rgba = min_256ps(max_256ps(rgba, minValue8), maxConvertible8);

        simd8i rgba32 = floor_256ps_to_epi32(rgba + half8);
        rgba32 = select_256epi32(rgba32, maxInt8, maxMask);

        storeu_si256(rgba32, dstPtr);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f minValue4 = set1_ps(-2147483648.0f);
    const simd4f maxConvertible4 = set1_ps(2147483520.0f);
    const simd4f maxExclusive4 = set1_ps(2147483648.0f);
    const simd4i maxInt4 = set1_epi32(INT32_MAX);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        simd4f rgba = loadu_ps(srcPtr);
        const simd4b maxMask = rgba >= maxExclusive4;

        // Match RoundToSInt32:
        // map NaN to zero, clamp to the safely convertible range,
        // add 0.5 and round toward negative infinity.
        rgba = select_ps(zero4, rgba, rgba == rgba);
        rgba = min_ps(max_ps(rgba, minValue4), maxConvertible4);

        simd4i rgba32 = floor_ps_to_epi32(rgba + half4);
        rgba32 = select_epi32(rgba32, maxInt4, maxMask);

        storeu_si128(rgba32, dstPtr);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = RoundToSInt32(srcPtr[0]);
        dstPtr[1] = RoundToSInt32(srcPtr[1]);
        dstPtr[2] = RoundToSInt32(srcPtr[2]);
        dstPtr[3] = RoundToSInt32(srcPtr[3]);
    }
#endif
}

static void RGBA32FToRGBA32UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const float *simd8End = srcPtr + (numPixels & ~1) * 4;
    const simd8f half8 = set1_256ps(0.5f);
    const simd8f zero8 = setzero_256ps();
    const simd8f maxConvertible8 = set1_256ps(4294967040.0f);
    const simd8f maxExclusive8 = set1_256ps(4294967296.0f);
    const simd8i maxInt8 = set1_256epi32(-1);

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 8) {
        simd8f rgba = loadu_256ps(srcPtr);
        const simd8b maxMask = rgba >= maxExclusive8;

        // Match RoundToUInt32: map NaN and negative values to zero, clamp to the safely
        // convertible range, add 0.5 and round toward negative infinity.
        rgba = select_256ps(zero8, rgba, rgba == rgba);
        rgba = min_256ps(max_256ps(rgba, zero8), maxConvertible8);
        rgba = floor_256ps(rgba + half8);

        simd8i rgba32 = trunc_256ps_to_epu32(rgba);
        rgba32 = select_256epi32(rgba32, maxInt8, maxMask);

        storeu_si256(rgba32, reinterpret_cast<int32_t *>(dstPtr));
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f half4 = set1_ps(0.5f);
    const simd4f zero4 = setzero_ps();
    const simd4f maxConvertible4 = set1_ps(4294967040.0f);
    const simd4f maxExclusive4 = set1_ps(4294967296.0f);
    const simd4i maxInt4 = set1_epi32(-1);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        simd4f rgba = loadu_ps(srcPtr);
        const simd4b maxMask = rgba >= maxExclusive4;

        // Match RoundToUInt32: map NaN and negative values to zero, clamp to the safely
        // convertible range, add 0.5 and round toward negative infinity.
        rgba = select_ps(zero4, rgba, rgba == rgba);
        rgba = min_ps(max_ps(rgba, zero4), maxConvertible4);
        rgba = floor_ps(rgba + half4);

        simd4i rgba32 = trunc_ps_to_epu32(rgba);
        rgba32 = select_epi32(rgba32, maxInt4, maxMask);

        storeu_si128(rgba32, reinterpret_cast<int32_t *>(dstPtr));
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = RoundToUInt32(srcPtr[0]);
        dstPtr[1] = RoundToUInt32(srcPtr[1]);
        dstPtr[2] = RoundToUInt32(srcPtr[2]);
        dstPtr[3] = RoundToUInt32(srcPtr[3]);
    }
#endif
}

static void RGBA32SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int32_t *srcPtr = reinterpret_cast<const int32_t *>(src);
    const int32_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const int32_t *simd8End = srcPtr + (numPixels & ~1) * 4;

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 8) {
        const simd8i rgba32 = loadu_si256(srcPtr);
        storeu_256ps(epi32_to_256ps(rgba32), dstPtr);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        const simd4i rgba32 = loadu_si128(srcPtr);
        storeu_ps(epi32_to_ps(rgba32), dstPtr);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#endif
}

static void RGBA32UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

#if defined(ENABLE_SIMD8_INTRIN)
    const uint32_t *simd8End = srcPtr + (numPixels & ~1) * 4;

    for (; srcPtr < simd8End; srcPtr += 8, dstPtr += 8) {
        const simd8i rgba32 = loadu_si256(reinterpret_cast<const int32_t *>(srcPtr));
        storeu_256ps(epu32_to_256ps(rgba32), dstPtr);
    }
#endif

#if defined(ENABLE_SIMD4_INTRIN)
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        const simd4i rgba32 = loadu_si128(reinterpret_cast<const int32_t *>(srcPtr));
        storeu_ps(epu32_to_ps(rgba32), dstPtr);
    }
#else
    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
#endif
}

static void RGB11F11F10FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = F11Converter::ToF32(*srcPtr & 0x7FF);
        dstPtr[1] = F11Converter::ToF32((*srcPtr >> 11) & 0x7FF);
        dstPtr[2] = F10Converter::ToF32((*srcPtr >> 22) & 0x3FF);
        dstPtr[3] = 1.0f;
    }
}

BE_NAMESPACE_END
