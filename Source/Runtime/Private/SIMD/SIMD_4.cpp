// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Math/Math.h"
#include "Core/JointPose.h"
#include "SIMD/SIMD.h"

#if defined(HAVE_X86_SSE_INTRIN) || defined(HAVE_ARM_NEON_INTRIN)

BE_NAMESPACE_BEGIN

const simd4f SIMD_4::F4_zero                = { 0.0f, 0.0f, 0.0f, 0.0f };
const simd4f SIMD_4::F4_one                 = { 1.0f, 1.0f, 1.0f, 1.0f };
const simd4f SIMD_4::F4_3zero_one           = { 0.0f, 0.0f, 0.0f, 1.0f };
const simd4f SIMD_4::F4_3one_zero           = { 1.0f, 1.0f, 1.0f, 0.0f };
const simd4f SIMD_4::F4_half                = { 0.5f, 0.5f, 0.5f, 0.5f };
const simd4f SIMD_4::F4_255                 = { 255.0f, 255.0f, 255.0f, 255.0f };
const simd4f SIMD_4::F4_min_char            = { -128.0f, -128.0f, -128.0f, -128.0f };
const simd4f SIMD_4::F4_max_char            = { 127.0f, 127.0f, 127.0f, 127.0f };
const simd4f SIMD_4::F4_min_short           = { -32768.0f, -32768.0f, -32768.0f, -32768.0f };
const simd4f SIMD_4::F4_max_short           = { 32767.0f, 32767.0f, 32767.0f, 32767.0f };
const simd4f SIMD_4::F4_tiny                = { 1e-4f, 1e-4f, 1e-4f, 1e-4f };
const simd4f SIMD_4::F4_smallestNonDenorm   = { 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f };
const simd4f SIMD_4::F4_sign_bit            = simd4i(0x80000000, 0x80000000, 0x80000000, 0x80000000);
const simd4f SIMD_4::F4_mask_xxx0           = simd4i(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000);
const simd4f SIMD_4::F4_mask_000x           = simd4i(0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF);

void BE_FASTCALL SIMD_4::Add(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dstPtr = dst;
    const float *srcPtr = src;

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = c + load_ps(srcPtr + 0);
            simd4f x1 = c + load_ps(srcPtr + 4);
            simd4f x2 = c + load_ps(srcPtr + 8);
            simd4f x3 = c + load_ps(srcPtr + 12);
        
            store_ps(x0, dstPtr + 0);
            store_ps(x1, dstPtr + 4);
            store_ps(x2, dstPtr + 8);
            store_ps(x3, dstPtr + 12);

            srcPtr += 16;
            dstPtr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dstPtr++ = constant + *srcPtr++;
        count--;
    }
}

void BE_FASTCALL SIMD_4::Add(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dstPtr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = load_ps(src0_ptr + 0);
            simd4f x1 = load_ps(src0_ptr + 4);
            simd4f x2 = load_ps(src0_ptr + 8);
            simd4f x3 = load_ps(src0_ptr + 12);

            x0 = x0 + load_ps(src1_ptr + 0);
            x1 = x1 + load_ps(src1_ptr + 4);
            x2 = x2 + load_ps(src1_ptr + 8);
            x3 = x3 + load_ps(src1_ptr + 12);
        
            store_ps(x0, dstPtr + 0);
            store_ps(x1, dstPtr + 4);
            store_ps(x2, dstPtr + 8);
            store_ps(x3, dstPtr + 12);

            src0_ptr += 16;
            src1_ptr += 16;
            dstPtr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dstPtr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_4::Sub(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dstPtr = dst;
    const float *srcPtr = src;

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = c - load_ps(srcPtr + 0);
            simd4f x1 = c - load_ps(srcPtr + 4);
            simd4f x2 = c - load_ps(srcPtr + 8);
            simd4f x3 = c - load_ps(srcPtr + 12);
        
            store_ps(x0, dstPtr + 0);
            store_ps(x1, dstPtr + 4);
            store_ps(x2, dstPtr + 8);
            store_ps(x3, dstPtr + 12);

            srcPtr += 16;
            dstPtr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dstPtr++ = constant - *srcPtr++;
        count--;
    }
}

void BE_FASTCALL SIMD_4::Sub(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dstPtr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = load_ps(src0_ptr + 0);
            simd4f x1 = load_ps(src0_ptr + 4);
            simd4f x2 = load_ps(src0_ptr + 8);
            simd4f x3 = load_ps(src0_ptr + 12);

            x0 = x0 - load_ps(src1_ptr + 0);
            x1 = x1 - load_ps(src1_ptr + 4);
            x2 = x2 - load_ps(src1_ptr + 8);
            x3 = x3 - load_ps(src1_ptr + 12);
        
            store_ps(x0, dstPtr + 0);
            store_ps(x1, dstPtr + 4);
            store_ps(x2, dstPtr + 8);
            store_ps(x3, dstPtr + 12);

            src0_ptr += 16;
            src1_ptr += 16;
            dstPtr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dstPtr++ = *src0_ptr++ - *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_4::Mul(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dstPtr = dst;
    const float *srcPtr = src;

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = c * load_ps(srcPtr + 0);
            simd4f x1 = c * load_ps(srcPtr + 4);
            simd4f x2 = c * load_ps(srcPtr + 8);
            simd4f x3 = c * load_ps(srcPtr + 12);
        
            store_ps(x0, dstPtr + 0);
            store_ps(x1, dstPtr + 4);
            store_ps(x2, dstPtr + 8);
            store_ps(x3, dstPtr + 12);

            srcPtr += 16;
            dstPtr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dstPtr++ = constant * *srcPtr++;
        count--;
    }
}

void BE_FASTCALL SIMD_4::Mul(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dstPtr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = load_ps(src0_ptr + 0);
            simd4f x1 = load_ps(src0_ptr + 4);
            simd4f x2 = load_ps(src0_ptr + 8);
            simd4f x3 = load_ps(src0_ptr + 12);

            x0 = x0 * load_ps(src1_ptr + 0);
            x1 = x1 * load_ps(src1_ptr + 4);
            x2 = x2 * load_ps(src1_ptr + 8);
            x3 = x3 * load_ps(src1_ptr + 12);
        
            store_ps(x0, dstPtr + 0);
            store_ps(x1, dstPtr + 4);
            store_ps(x2, dstPtr + 8);
            store_ps(x3, dstPtr + 12);

            src0_ptr += 16;
            src1_ptr += 16;
            dstPtr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dstPtr++ = *src0_ptr++ * *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_4::Div(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dstPtr = dst;
    const float *srcPtr = src;

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);
        
        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = c / load_ps(srcPtr + 0);
            simd4f x1 = c / load_ps(srcPtr + 4);
            simd4f x2 = c / load_ps(srcPtr + 8);
            simd4f x3 = c / load_ps(srcPtr + 12);
        
            store_ps(x0, dstPtr + 0);
            store_ps(x1, dstPtr + 4);
            store_ps(x2, dstPtr + 8);
            store_ps(x3, dstPtr + 12);

            srcPtr += 16;
            dstPtr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dstPtr++ = constant / *srcPtr++;
        count--;
    }
}

void BE_FASTCALL SIMD_4::Div(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dstPtr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = load_ps(src0_ptr + 0);
            simd4f x1 = load_ps(src0_ptr + 4);
            simd4f x2 = load_ps(src0_ptr + 8);
            simd4f x3 = load_ps(src0_ptr + 12);

            x0 = x0 / load_ps(src1_ptr + 0);
            x1 = x1 / load_ps(src1_ptr + 4);
            x2 = x2 / load_ps(src1_ptr + 8);
            x3 = x3 / load_ps(src1_ptr + 12);
        
            store_ps(x0, dstPtr + 0);
            store_ps(x1, dstPtr + 4);
            store_ps(x2, dstPtr + 8);
            store_ps(x3, dstPtr + 12);

            src0_ptr += 16;
            src1_ptr += 16;
            dstPtr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dstPtr++ = *src0_ptr++ / *src1_ptr++;
        count--;
    }
}

float BE_FASTCALL SIMD_4::Sum(const float *src, const int count0) {
    int count = count0;
    const float *srcPtr = src;
    float ret = 0;

    if (count >= 4) {
        assert_16_byte_aligned(src);

        simd4f total = setzero_ps();

        if (count >= 16) {
            simd4f sum0 = setzero_ps();
            simd4f sum1 = setzero_ps();
            simd4f sum2 = setzero_ps();
            simd4f sum3 = setzero_ps();

            int c16 = count >> 4;
            while (c16 > 0) {
                sum0 += load_ps(srcPtr + 0);
                sum1 += load_ps(srcPtr + 4);
                sum2 += load_ps(srcPtr + 8);
                sum3 += load_ps(srcPtr + 12);

                srcPtr += 16;
                c16--;
            }

            total = (sum0 + sum1) + (sum2 + sum3);
            count &= 15;
        }

        while (count >= 4) {
            total += load_ps(srcPtr);
            srcPtr += 4;
            count -= 4;
        }

        ret = extract_ps<0>(sum_ps(total));
    }

    while (count > 0) {
        ret += *srcPtr++;
        count--;
    }

    return ret;
}

static BE_FORCE_INLINE simd4f LoadSignedShort4(const short *src) {
#if defined(HAVE_X86_SSE_INTRIN)
    const __m128i packed = _mm_loadl_epi64((const __m128i *)src);
    const __m128i sign = _mm_srai_epi16(packed, 15);
    return _mm_cvtepi32_ps(_mm_unpacklo_epi16(packed, sign));
#else
    return vcvtq_f32_s32(vmovl_s16(vld1_s16(src)));
#endif
}

void BE_FASTCALL SIMD_4::DecompressJoints(JointPose *joints, const CompressedJointPose *compressedJoints, const int *index, const int numJoints) {
    assert_16_byte_aligned(joints);

    const simd4f quatScale = set1_ps(1.0f / 32767.0f);
    const simd4f translationScale = set1_ps(1.0f / (32767.0f / CompressedJointPose::MaxBoneTranslation));
    const simd4f jointScale = set1_ps(1.0f / (32767.0f / CompressedJointPose::MaxBoneScale));

    for (int i = 0; i < numJoints; i++) {
        const int j = index[i];
        const CompressedJointPose &compressed = compressedJoints[j];

        simd4f q = LoadSignedShort4(compressed.q) * quatScale;
        q &= SIMD_4::F4_mask_xxx0;
        const simd4f qw = sqrt_ps(abs_ps(SIMD_4::F4_one - sum_ps(q * q)));
        q |= qw & SIMD_4::F4_mask_000x;

        simd4f t = LoadSignedShort4(compressed.t) * translationScale;
        t &= SIMD_4::F4_mask_xxx0;

        simd4f s = shuffle_ps<1, 2, 3, 0>(LoadSignedShort4(compressed.t + 2)) * jointScale;
        s &= SIMD_4::F4_mask_xxx0;

        store_ps(q, joints[j].q);
        store_ps(t, joints[j].t);
        store_ps(s, joints[j].s);
    }
}

BE_FORCE_INLINE void SIMD_4::LoadJointQuats4(const JointPose *joints, const int *index, simd4f &qx, simd4f &qy, simd4f &qz, simd4f &qw) {
    qx = load_ps(joints[index[0]].q);
    qy = load_ps(joints[index[1]].q);
    qz = load_ps(joints[index[2]].q);
    qw = load_ps(joints[index[3]].q);
    transpose4x4(qx, qy, qz, qw);
}

BE_FORCE_INLINE void SIMD_4::StoreJointQuats4(JointPose *joints, const int *index, const simd4f &qx, const simd4f &qy, const simd4f &qz, const simd4f &qw) {
    simd4f q0 = qx;
    simd4f q1 = qy;
    simd4f q2 = qz;
    simd4f q3 = qw;
    transpose4x4(q0, q1, q2, q3);
    store_ps(q0, joints[index[0]].q);
    store_ps(q1, joints[index[1]].q);
    store_ps(q2, joints[index[2]].q);
    store_ps(q3, joints[index[3]].q);
}

BE_FORCE_INLINE void SIMD_4::SlerpJointQuats4(simd4f &fromX, simd4f &fromY, simd4f &fromZ, simd4f &fromW, const simd4f &toX, const simd4f &toY, const simd4f &toZ, const simd4f &toW, const simd4f &lerp) {
    const simd4f vector_float_rsqrt_c0 = set1_ps(-3.0f);
    const simd4f vector_float_rsqrt_c1 = set1_ps(-0.5f);
    const simd4f vector_float_tiny = set1_ps(1e-10f);
    const simd4f vector_float_half_pi = set1_ps(Math::HalfPi);

    const simd4f vector_float_sin_c0 = set1_ps(-2.39e-08f);
    const simd4f vector_float_sin_c1 = set1_ps(2.7526e-06f);
    const simd4f vector_float_sin_c2 = set1_ps(-1.98409e-04f);
    const simd4f vector_float_sin_c3 = set1_ps(8.3333315e-03f);
    const simd4f vector_float_sin_c4 = set1_ps(-1.666666664e-01f);

    const simd4f vector_float_atan_c0 = set1_ps(0.0028662257f);
    const simd4f vector_float_atan_c1 = set1_ps(-0.0161657367f);
    const simd4f vector_float_atan_c2 = set1_ps(0.0429096138f);
    const simd4f vector_float_atan_c3 = set1_ps(-0.0752896400f);
    const simd4f vector_float_atan_c4 = set1_ps(0.1065626393f);
    const simd4f vector_float_atan_c5 = set1_ps(-0.1420889944f);
    const simd4f vector_float_atan_c6 = set1_ps(0.1999355085f);
    const simd4f vector_float_atan_c7 = set1_ps(-0.3333314528f);

    simd4f cosom = fromX * toX;
    cosom = madd_ps(fromY, toY, cosom);
    cosom = madd_ps(fromZ, toZ, cosom);
    cosom = madd_ps(fromW, toW, cosom);

    const simd4f sign = cosom & SIMD_4::F4_sign_bit;
    cosom ^= sign;

    simd4f sinSqr = nmadd_ps(cosom, cosom, SIMD_4::F4_one);
    sinSqr = max_ps(sinSqr, vector_float_tiny);

    const simd4f rsqrt = rsqrt12_ps(sinSqr);
    const simd4f rsqrtSqr = rsqrt * rsqrt;
    const simd4f rsqrtHalf = rsqrt * vector_float_rsqrt_c1;
    const simd4f rsqrtTerm = madd_ps(sinSqr, rsqrtSqr, vector_float_rsqrt_c0);
    const simd4f invSin = rsqrtHalf * rsqrtTerm;

    const simd4f sin = sinSqr * invSin;
    const simd4f minSinCos = min_ps(sin, cosom);
    const simd4f maxSinCos = max_ps(sin, cosom);
    const simd4f swapMask = (minSinCos == cosom);
    const simd4f atanSign = swapMask & SIMD_4::F4_sign_bit;
    const simd4f atanOffset = swapMask & vector_float_half_pi;

    const simd4f reciprocalEstimate = rcp12_ps(maxSinCos);
    const simd4f reciprocal = nmadd_ps(maxSinCos * reciprocalEstimate, reciprocalEstimate, reciprocalEstimate + reciprocalEstimate);
    const simd4f atanInput = (minSinCos * reciprocal) ^ atanSign;
    const simd4f atanInputSqr = atanInput * atanInput;

    simd4f atanPolynomial = madd_ps(atanInputSqr, vector_float_atan_c0, vector_float_atan_c1);
    atanPolynomial = madd_ps(atanPolynomial, atanInputSqr, vector_float_atan_c2);
    atanPolynomial = madd_ps(atanPolynomial, atanInputSqr, vector_float_atan_c3);
    atanPolynomial = madd_ps(atanPolynomial, atanInputSqr, vector_float_atan_c4);
    atanPolynomial = madd_ps(atanPolynomial, atanInputSqr, vector_float_atan_c5);
    atanPolynomial = madd_ps(atanPolynomial, atanInputSqr, vector_float_atan_c6);
    atanPolynomial = madd_ps(atanPolynomial, atanInputSqr, vector_float_atan_c7);
    atanPolynomial = madd_ps(atanPolynomial, atanInputSqr, SIMD_4::F4_one);

    const simd4f omega = madd_ps(atanPolynomial, atanInput, atanOffset);
    const simd4f omegaTo = lerp * omega;
    const simd4f omegaFrom = omega - omegaTo;
    const simd4f omegaFromSqr = omegaFrom * omegaFrom;
    const simd4f omegaToSqr = omegaTo * omegaTo;

    simd4f sinFrom = madd_ps(omegaFromSqr, vector_float_sin_c0, vector_float_sin_c1);
    simd4f sinTo = madd_ps(omegaToSqr, vector_float_sin_c0, vector_float_sin_c1);
    sinFrom = madd_ps(sinFrom, omegaFromSqr, vector_float_sin_c2);
    sinTo = madd_ps(sinTo, omegaToSqr, vector_float_sin_c2);
    sinFrom = madd_ps(sinFrom, omegaFromSqr, vector_float_sin_c3);
    sinTo = madd_ps(sinTo, omegaToSqr, vector_float_sin_c3);
    sinFrom = madd_ps(sinFrom, omegaFromSqr, vector_float_sin_c4);
    sinTo = madd_ps(sinTo, omegaToSqr, vector_float_sin_c4);
    sinFrom = madd_ps(sinFrom, omegaFromSqr, SIMD_4::F4_one) * omegaFrom * invSin;
    sinTo = madd_ps(sinTo, omegaToSqr, SIMD_4::F4_one) * omegaTo * invSin;
    sinTo ^= sign;

    fromX = madd_ps(toX, sinTo, fromX * sinFrom);
    fromY = madd_ps(toY, sinTo, fromY * sinFrom);
    fromZ = madd_ps(toZ, sinTo, fromZ * sinFrom);
    fromW = madd_ps(toW, sinTo, fromW * sinFrom);
}

BE_FORCE_INLINE void SIMD_4::NlerpJointQuats4(simd4f &fromX, simd4f &fromY, simd4f &fromZ, simd4f &fromW, const simd4f &toX, const simd4f &toY, const simd4f &toZ, const simd4f &toW, const simd4f &scaledLerp) {
    const simd4f vector_float_rsqrt_c0 = set1_ps(-3.0f);
    const simd4f vector_float_rsqrt_c1 = set1_ps(-0.5f);

    simd4f cosom = fromX * toX;
    cosom = madd_ps(fromY, toY, cosom);
    cosom = madd_ps(fromZ, toZ, cosom);
    cosom = madd_ps(fromW, toW, cosom);

    const simd4f scale = scaledLerp ^ (cosom & SIMD_4::F4_sign_bit);
    fromX = madd_ps(scale, toX, fromX);
    fromY = madd_ps(scale, toY, fromY);
    fromZ = madd_ps(scale, toZ, fromZ);
    fromW = madd_ps(scale, toW, fromW);

    simd4f lengthSqr = fromX * fromX;
    lengthSqr = madd_ps(fromY, fromY, lengthSqr);
    lengthSqr = madd_ps(fromZ, fromZ, lengthSqr);
    lengthSqr = madd_ps(fromW, fromW, lengthSqr);

    const simd4f rsqrt = rsqrt12_ps(lengthSqr);
    const simd4f invLength = (rsqrt * vector_float_rsqrt_c1) * madd_ps(lengthSqr, rsqrt * rsqrt, vector_float_rsqrt_c0);
    fromX *= invLength;
    fromY *= invLength;
    fromZ *= invLength;
    fromW *= invLength;
}

void BE_FASTCALL SIMD_4::BlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    assert_16_byte_aligned(joints);
    assert_16_byte_aligned(blendJoints);

    if (fraction <= 0.0f) {
        return;
    } else if (fraction >= 1.0f) {
        for (int i = 0; i < numJoints; i++) {
            int j = index[i];
            joints[j] = blendJoints[j];
        }
        return;
    }

    const simd4f vlerp = { fraction, fraction, fraction, fraction };

    int i = 0;
    for (; i < numJoints - 3; i += 4) {
        const int n0 = index[i + 0];
        const int n1 = index[i + 1];
        const int n2 = index[i + 2];
        const int n3 = index[i + 3];

        // Blend positions.
        simd4f jta_0 = load_ps(joints[n0].t);
        simd4f jtb_0 = load_ps(joints[n1].t);
        simd4f jtc_0 = load_ps(joints[n2].t);
        simd4f jtd_0 = load_ps(joints[n3].t);

        simd4f bta_0 = load_ps(blendJoints[n0].t);
        simd4f btb_0 = load_ps(blendJoints[n1].t);
        simd4f btc_0 = load_ps(blendJoints[n2].t);
        simd4f btd_0 = load_ps(blendJoints[n3].t);

        bta_0 -= jta_0;
        btb_0 -= jtb_0;
        btc_0 -= jtc_0;
        btd_0 -= jtd_0;

        jta_0 = madd_ps(vlerp, bta_0, jta_0);
        jtb_0 = madd_ps(vlerp, btb_0, jtb_0);
        jtc_0 = madd_ps(vlerp, btc_0, jtc_0);
        jtd_0 = madd_ps(vlerp, btd_0, jtd_0);

        store_ps(jta_0, (float *)joints[n0].t);
        store_ps(jtb_0, (float *)joints[n1].t);
        store_ps(jtc_0, (float *)joints[n2].t);
        store_ps(jtd_0, (float *)joints[n3].t);

        // Blend scales.
        simd4f jsa_0 = load_ps(joints[n0].s);
        simd4f jsb_0 = load_ps(joints[n1].s);
        simd4f jsc_0 = load_ps(joints[n2].s);
        simd4f jsd_0 = load_ps(joints[n3].s);

        simd4f bsa_0 = load_ps(blendJoints[n0].s);
        simd4f bsb_0 = load_ps(blendJoints[n1].s);
        simd4f bsc_0 = load_ps(blendJoints[n2].s);
        simd4f bsd_0 = load_ps(blendJoints[n3].s);

        bsa_0 -= jsa_0;
        bsb_0 -= jsb_0;
        bsc_0 -= jsc_0;
        bsd_0 -= jsd_0;

        jsa_0 = madd_ps(vlerp, bsa_0, jsa_0);
        jsb_0 = madd_ps(vlerp, bsb_0, jsb_0);
        jsc_0 = madd_ps(vlerp, bsc_0, jsc_0);
        jsd_0 = madd_ps(vlerp, bsd_0, jsd_0);

        store_ps(jsa_0, (float *)joints[n0].s);
        store_ps(jsb_0, (float *)joints[n1].s);
        store_ps(jsc_0, (float *)joints[n2].s);
        store_ps(jsd_0, (float *)joints[n3].s);

        // Blend quaternions.
        simd4f jqx, jqy, jqz, jqw;
        simd4f bqx, bqy, bqz, bqw;
        LoadJointQuats4(joints, index + i, jqx, jqy, jqz, jqw);
        LoadJointQuats4(blendJoints, index + i, bqx, bqy, bqz, bqw);
        SlerpJointQuats4(jqx, jqy, jqz, jqw, bqx, bqy, bqz, bqw, vlerp);
        StoreJointQuats4(joints, index + i, jqx, jqy, jqz, jqw);
    }

    for (; i < numJoints; i++) {
        int n = index[i];

        Vec3 &jointPos = joints[n].t;
        const Vec3 &blendPos = blendJoints[n].t;

        jointPos[0] += fraction * (blendPos[0] - jointPos[0]);
        jointPos[1] += fraction * (blendPos[1] - jointPos[1]);
        jointPos[2] += fraction * (blendPos[2] - jointPos[2]);

        Vec3 &jointScale = joints[n].s;
        const Vec3 &blendScale = blendJoints[n].s;

        jointScale[0] += fraction * (blendScale[0] - jointScale[0]);
        jointScale[1] += fraction * (blendScale[1] - jointScale[1]);
        jointScale[2] += fraction * (blendScale[2] - jointScale[2]);

        Quat &jointQuat = joints[n].q;
        const Quat &blendQuat = blendJoints[n].q;

        float cosom;
        float sinom;
        float omega;
        float scale0;
        float scale1;
        uint32_t signBit;

        cosom = jointQuat.x * blendQuat.x + jointQuat.y * blendQuat.y + jointQuat.z * blendQuat.z + jointQuat.w * blendQuat.w;

        signBit = (*(uint32_t *)&cosom) & 0x80000000u;

        (*(uint32_t *)&cosom) ^= signBit;

        scale0 = 1.0f - cosom * cosom;
        scale0 = (scale0 <= 0.0f) ? 1e-10f : scale0;
        sinom = Math::InvSqrt(scale0);
        omega = Math::ATan16(scale0 * sinom, cosom);
        scale0 = Math::Sin16((1.0f - fraction) * omega) * sinom;
        scale1 = Math::Sin16(fraction * omega) * sinom;

        (*(uint32_t *)&scale1) ^= signBit;

        jointQuat.x = scale0 * jointQuat.x + scale1 * blendQuat.x;
        jointQuat.y = scale0 * jointQuat.y + scale1 * blendQuat.y;
        jointQuat.z = scale0 * jointQuat.z + scale1 * blendQuat.z;
        jointQuat.w = scale0 * jointQuat.w + scale1 * blendQuat.w;
    }
}

void BE_FASTCALL SIMD_4::BlendJointsFast(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    assert_16_byte_aligned(joints);
    assert_16_byte_aligned(blendJoints);

    if (fraction <= 0.0f) {
        return;
    } else if (fraction >= 1.0f) {
        for (int i = 0; i < numJoints; i++) {
            int j = index[i];
            joints[j] = blendJoints[j];
        }
        return;
    }

    const float scaledLerp = fraction / (1.0f - fraction);
    const simd4f vlerp = { fraction, fraction, fraction, fraction };
    const simd4f vscaledLerp = { scaledLerp, scaledLerp, scaledLerp, scaledLerp };

    int i = 0;
    for (; i < numJoints - 3; i += 4) {
        const int n0 = index[i + 0];
        const int n1 = index[i + 1];
        const int n2 = index[i + 2];
        const int n3 = index[i + 3];

        // Blend positions.
        simd4f jta_0 = load_ps(joints[n0].t);
        simd4f jtb_0 = load_ps(joints[n1].t);
        simd4f jtc_0 = load_ps(joints[n2].t);
        simd4f jtd_0 = load_ps(joints[n3].t);

        simd4f bta_0 = load_ps(blendJoints[n0].t);
        simd4f btb_0 = load_ps(blendJoints[n1].t);
        simd4f btc_0 = load_ps(blendJoints[n2].t);
        simd4f btd_0 = load_ps(blendJoints[n3].t);

        bta_0 -= jta_0;
        btb_0 -= jtb_0;
        btc_0 -= jtc_0;
        btd_0 -= jtd_0;

        jta_0 = madd_ps(vlerp, bta_0, jta_0);
        jtb_0 = madd_ps(vlerp, btb_0, jtb_0);
        jtc_0 = madd_ps(vlerp, btc_0, jtc_0);
        jtd_0 = madd_ps(vlerp, btd_0, jtd_0);

        store_ps(jta_0, (float *)joints[n0].t);
        store_ps(jtb_0, (float *)joints[n1].t);
        store_ps(jtc_0, (float *)joints[n2].t);
        store_ps(jtd_0, (float *)joints[n3].t);

        // Blend scales.
        simd4f jsa_0 = load_ps(joints[n0].s);
        simd4f jsb_0 = load_ps(joints[n1].s);
        simd4f jsc_0 = load_ps(joints[n2].s);
        simd4f jsd_0 = load_ps(joints[n3].s);

        simd4f bsa_0 = load_ps(blendJoints[n0].s);
        simd4f bsb_0 = load_ps(blendJoints[n1].s);
        simd4f bsc_0 = load_ps(blendJoints[n2].s);
        simd4f bsd_0 = load_ps(blendJoints[n3].s);

        bsa_0 -= jsa_0;
        bsb_0 -= jsb_0;
        bsc_0 -= jsc_0;
        bsd_0 -= jsd_0;

        jsa_0 = madd_ps(vlerp, bsa_0, jsa_0);
        jsb_0 = madd_ps(vlerp, bsb_0, jsb_0);
        jsc_0 = madd_ps(vlerp, bsc_0, jsc_0);
        jsd_0 = madd_ps(vlerp, bsd_0, jsd_0);

        store_ps(jsa_0, (float *)joints[n0].s);
        store_ps(jsb_0, (float *)joints[n1].s);
        store_ps(jsc_0, (float *)joints[n2].s);
        store_ps(jsd_0, (float *)joints[n3].s);

        // Blend quaternions.
        simd4f jqx, jqy, jqz, jqw;
        simd4f bqx, bqy, bqz, bqw;
        LoadJointQuats4(joints, index + i, jqx, jqy, jqz, jqw);
        LoadJointQuats4(blendJoints, index + i, bqx, bqy, bqz, bqw);
        NlerpJointQuats4(jqx, jqy, jqz, jqw, bqx, bqy, bqz, bqw, vscaledLerp);
        StoreJointQuats4(joints, index + i, jqx, jqy, jqz, jqw);
    }

    for (; i < numJoints; i++) {
        const int n = index[i];

        Vec3 &jointPos = joints[n].t;
        const Vec3 &blendPos = blendJoints[n].t;

        jointPos[0] += fraction * (blendPos[0] - jointPos[0]);
        jointPos[1] += fraction * (blendPos[1] - jointPos[1]);
        jointPos[2] += fraction * (blendPos[2] - jointPos[2]);

        Vec3 &jointScale = joints[n].s;
        const Vec3 &blendScale = blendJoints[n].s;

        jointScale[0] += fraction * (blendScale[0] - jointScale[0]);
        jointScale[1] += fraction * (blendScale[1] - jointScale[1]);
        jointScale[2] += fraction * (blendScale[2] - jointScale[2]);

        Quat &jointQuat = joints[n].q;
        const Quat &blendQuat = blendJoints[n].q;

        float cosom;
        float scale;
        float s;

        cosom = jointQuat.x * blendQuat.x + jointQuat.y * blendQuat.y + jointQuat.z * blendQuat.z + jointQuat.w * blendQuat.w;

        scale = (cosom >= 0.0f) ? scaledLerp : -scaledLerp;

        jointQuat.x += scale * blendQuat.x;
        jointQuat.y += scale * blendQuat.y;
        jointQuat.z += scale * blendQuat.z;
        jointQuat.w += scale * blendQuat.w;

        s = jointQuat.x * jointQuat.x + jointQuat.y * jointQuat.y + jointQuat.z * jointQuat.z + jointQuat.w * jointQuat.w;
        s = 1.0f / sqrtf(s);

        jointQuat.x *= s;
        jointQuat.y *= s;
        jointQuat.z *= s;
        jointQuat.w *= s;
    }
}

void BE_FASTCALL SIMD_4::AdditiveBlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    assert_16_byte_aligned(joints);
    assert_16_byte_aligned(blendJoints);

    if (fraction <= 0.0f) {
        return;
    } else if (fraction >= 1.0f) {
        SIMD_Generic::AdditiveBlendJoints(joints, blendJoints, 1.0f, index, numJoints);
        return;
    }

    const simd4f lerp = set1_ps(fraction);

    int i = 0;
    for (; i + 3 < numJoints; i += 4) {
        const int n0 = index[i + 0];
        const int n1 = index[i + 1];
        const int n2 = index[i + 2];
        const int n3 = index[i + 3];

        simd4f jt0 = load_ps(joints[n0].t);
        simd4f jt1 = load_ps(joints[n1].t);
        simd4f jt2 = load_ps(joints[n2].t);
        simd4f jt3 = load_ps(joints[n3].t);

        jt0 = madd_ps(load_ps(blendJoints[n0].t), lerp, jt0);
        jt1 = madd_ps(load_ps(blendJoints[n1].t), lerp, jt1);
        jt2 = madd_ps(load_ps(blendJoints[n2].t), lerp, jt2);
        jt3 = madd_ps(load_ps(blendJoints[n3].t), lerp, jt3);

        store_ps(jt0, joints[n0].t);
        store_ps(jt1, joints[n1].t);
        store_ps(jt2, joints[n2].t);
        store_ps(jt3, joints[n3].t);

        simd4f js0 = load_ps(joints[n0].s);
        simd4f js1 = load_ps(joints[n1].s);
        simd4f js2 = load_ps(joints[n2].s);
        simd4f js3 = load_ps(joints[n3].s);

        const simd4f scale0 = madd_ps(load_ps(blendJoints[n0].s) - SIMD_4::F4_one, lerp, SIMD_4::F4_one);
        const simd4f scale1 = madd_ps(load_ps(blendJoints[n1].s) - SIMD_4::F4_one, lerp, SIMD_4::F4_one);
        const simd4f scale2 = madd_ps(load_ps(blendJoints[n2].s) - SIMD_4::F4_one, lerp, SIMD_4::F4_one);
        const simd4f scale3 = madd_ps(load_ps(blendJoints[n3].s) - SIMD_4::F4_one, lerp, SIMD_4::F4_one);

        store_ps(js0 * scale0, joints[n0].s);
        store_ps(js1 * scale1, joints[n1].s);
        store_ps(js2 * scale2, joints[n2].s);
        store_ps(js3 * scale3, joints[n3].s);

        simd4f jqx, jqy, jqz, jqw;
        simd4f bqx, bqy, bqz, bqw;
        LoadJointQuats4(joints, index + i, jqx, jqy, jqz, jqw);
        LoadJointQuats4(blendJoints, index + i, bqx, bqy, bqz, bqw);

        simd4f targetX = madd_ps(bqx, jqw, bqy * jqz);
        targetX = nmadd_ps(bqz, jqy, targetX);
        targetX = madd_ps(bqw, jqx, targetX);

        simd4f targetY = nmadd_ps(bqx, jqz, bqy * jqw);
        targetY = madd_ps(bqz, jqx, targetY);
        targetY = madd_ps(bqw, jqy, targetY);

        simd4f targetZ = madd_ps(bqx, jqy, bqz * jqw);
        targetZ = nmadd_ps(bqy, jqx, targetZ);
        targetZ = madd_ps(bqw, jqz, targetZ);

        simd4f targetW = nmadd_ps(bqx, jqx, bqw * jqw);
        targetW = nmadd_ps(bqy, jqy, targetW);
        targetW = nmadd_ps(bqz, jqz, targetW);

        SlerpJointQuats4(jqx, jqy, jqz, jqw, targetX, targetY, targetZ, targetW, lerp);
        StoreJointQuats4(joints, index + i, jqx, jqy, jqz, jqw);
    }

    if (i < numJoints) {
        SIMD_Generic::AdditiveBlendJoints(joints, blendJoints, fraction, index + i, numJoints - i);
    }
}

void BE_FASTCALL SIMD_4::ConvertJointPosesToJointMats(Mat3x4 *jointMats, const JointPose *jointPoses, const int numJoints) {
#if 0
    int i;

    for (i = 0; i < numJoints; i++) {
        // T * R * S
        // mat[0][0] = (1.0f - yy2 - zz2) * scale.x;
        // mat[0][1] = (xy2 - wz2) * scale.y;
        // mat[0][2] = (xz2 + wy2) * scale.z;
        // mat[0][3] = translation.x;

        // mat[1][0] = (xy2 + wz2) * scale.x;
        // mat[1][1] = (1.0f - xx2 - zz2) * scale.y;
        // mat[1][2] = (yz2 - wx2) * scale.z;
        // mat[1][3] = translation.y;

        // mat[2][0] = (xz2 - wy2) * scale.x;
        // mat[2][1] = (yz2 + wx2) * scale.y;
        // mat[2][2] = (1.0f - xx2 - yy2) * scale.z;
        // mat[2][3] = translation.z;

        jointMats[i].SetTQS(jointPoses[i].t, jointPoses[i].q, jointPoses[i].s);
    }
#else
    assert_16_byte_aligned(jointMats);
    assert_16_byte_aligned(jointPoses);
    assert(sizeof(JointPose) == sizeof(float) * 12);
    assert(sizeof(Mat3x4) == sizeof(float) * 12);

    const float *jointPosePtr = (float *)jointPoses;
    float *jointMatPtr = (float *)jointMats;

    const simd4f vector_float_first_sign_bit    = simd4i(0x80000000, 0x00000000, 0x00000000, 0x00000000);
    const simd4f vector_float_first_pos_half    = { +0.5f,  0.0f,  0.0f,  0.0f };
    const simd4f vector_float_first_neg_half    = { -0.5f,  0.0f,  0.0f,  0.0f };
    const simd4f vector_float_quat2mat_mad1     = { -1.0f, -1.0f, +1.0f, -1.0f };
    const simd4f vector_float_quat2mat_mad2     = { -1.0f, +1.0f, -1.0f, -1.0f };
    const simd4f vector_float_quat2mat_mad3     = { +1.0f, -1.0f, -1.0f, +1.0f };

    int i = 0;
    for (; i + 1 < numJoints; i += 2) {
        simd4f q0 = load_ps(&jointPosePtr[(i + 0) * 12 + 0]);
        simd4f t0 = load_ps(&jointPosePtr[(i + 0) * 12 + 4]);
        simd4f s0 = load_ps(&jointPosePtr[(i + 0) * 12 + 8]);

        simd4f q1 = load_ps(&jointPosePtr[(i + 1) * 12 + 0]);
        simd4f t1 = load_ps(&jointPosePtr[(i + 1) * 12 + 4]);
        simd4f s1 = load_ps(&jointPosePtr[(i + 1) * 12 + 8]);

        simd4f d0 = q0 + q0;
        simd4f d1 = q1 + q1;

        s0 = insert_ps<3>(s0, 1.0f);
        s1 = insert_ps<3>(s1, 1.0f);

        simd4f sa0 = shuffle_ps<1, 0, 0, 1>(q0);                //   y,   x,   x,   y
        simd4f sb0 = shuffle_ps<1, 1, 2, 2>(d0);                //  2y,  2y,  2z,  2z
        simd4f sc0 = shuffle_ps<2, 3, 3, 3>(q0);                //   z,   w,   w,   w
        simd4f sd0 = shuffle_ps<2, 2, 1, 0>(d0);                //  2z,  2z,  2y,  2x

        simd4f sa1 = shuffle_ps<1, 0, 0, 1>(q1);                //   y,   x,   x,   y
        simd4f sb1 = shuffle_ps<1, 1, 2, 2>(d1);                //  2y,  2y,  2z,  2z
        simd4f sc1 = shuffle_ps<2, 3, 3, 3>(q1);                //   z,   w,   w,   w
        simd4f sd1 = shuffle_ps<2, 2, 1, 0>(d1);                //  2z,  2z,  2y,  2x

        sa0 ^= vector_float_first_sign_bit;                     //  -y,   x,   x,   y
        sa1 ^= vector_float_first_sign_bit;                     //  -y,   x,   x,   y

        simd4f ma0 = sa0 * sb0 + vector_float_first_pos_half;   //  0.5 - 2yy,  2xy,  2xz,  2yz
        simd4f mb0 = sc0 * sd0 + vector_float_first_neg_half;   // -0.5 + 2zz,  2wz,  2wy,  2wx
        simd4f mc0 = vector_float_first_pos_half - q0 * d0;     //  0.5 - 2xx, -2yy, -2zz, -2ww

        simd4f ma1 = sa1 * sb1 + vector_float_first_pos_half;   //  0.5 - 2yy,  2xy,  2xz,  2yz
        simd4f mb1 = sc1 * sd1 + vector_float_first_neg_half;   // -0.5 + 2zz,  2wz,  2wy,  2wx
        simd4f mc1 = vector_float_first_pos_half - q1 * d1;     //  0.5 - 2xx, -2yy, -2zz, -2ww

        simd4f mf0 = shuffle_ps<1, 1, 0, 0>(ma0, mc0);          //        2xy,  2xy, 0.5 - 2xx, 0.5 - 2xx
        simd4f md0 = shuffle_ps<2, 0, 2, 3>(mf0, ma0);          //  0.5 - 2xx,  2xy,       2xz,       2yz
        simd4f me0 = shuffle_ps<0, 1, 2, 3>(ma0, mb0);          //  0.5 - 2yy,  2xy,       2wy,       2wx

        simd4f mf1 = shuffle_ps<1, 1, 0, 0>(ma1, mc1);          //        2xy,  2xy, 0.5 - 2xx, 0.5 - 2xx
        simd4f md1 = shuffle_ps<2, 0, 2, 3>(mf1, ma1);          //  0.5 - 2xx,  2xy,       2xz,       2yz
        simd4f me1 = shuffle_ps<0, 1, 2, 3>(ma1, mb1);          //  0.5 - 2yy,  2xy,       2wy,       2wx

        simd4f ra0 = mb0 * vector_float_quat2mat_mad1 + ma0;    // 1 - 2yy - 2zz, 2xy - 2wz, 2xz + 2wy,       n/a   // - - + -
        simd4f rb0 = mb0 * vector_float_quat2mat_mad2 + md0;    // 1 - 2xx - 2zz, 2xy + 2wz,       n/a, 2yz - 2wx   // - + - -
        simd4f rc0 = me0 * vector_float_quat2mat_mad3 + md0;    // 1 - 2xx - 2yy,       n/a, 2xz - 2wy, 2yz + 2wx   // + - - +

        simd4f ra1 = mb1 * vector_float_quat2mat_mad1 + ma1;    // 1 - 2yy - 2zz, 2xy - 2wz, 2xz + 2wy,       n/a   // - - + -
        simd4f rb1 = mb1 * vector_float_quat2mat_mad2 + md1;    // 1 - 2xx - 2zz, 2xy + 2wz,       n/a, 2yz - 2wx   // - + - -
        simd4f rc1 = me1 * vector_float_quat2mat_mad3 + md1;    // 1 - 2xx - 2yy,       n/a, 2xz - 2wy, 2yz + 2wx   // + - - +

        simd4f ta0 = shuffle_ps<2, 2, 0, 0>(ra0, t0);           //     2xz + 2wy,     2xz + 2wy, tx, tx
        simd4f tb0 = shuffle_ps<3, 3, 1, 1>(rb0, t0);           //     2yz - 2wx,     2yz - 2wx, ty, ty
        simd4f tc0 = shuffle_ps<0, 0, 2, 2>(rc0, t0);           // 1 - 2xx - 2yy, 1 - 2xx - 2yy, tz, tz

        simd4f ta1 = shuffle_ps<2, 2, 0, 0>(ra1, t1);           //     2xz + 2wy,     2xz + 2wy, tx, tx
        simd4f tb1 = shuffle_ps<3, 3, 1, 1>(rb1, t1);           //     2yz - 2wx,     2yz - 2wx, ty, ty
        simd4f tc1 = shuffle_ps<0, 0, 2, 2>(rc1, t1);           // 1 - 2xx - 2yy, 1 - 2xx - 2yy, tz, tz

        ra0 = shuffle_ps<0, 1, 0, 2>(ra0, ta0);                 // 1 - 2yy - 2zz,     2xy - 2wz,     2xz + 2wy, tx
        rb0 = shuffle_ps<1, 0, 0, 2>(rb0, tb0);                 //     2xy + 2wz, 1 - 2xx - 2zz,     2yz - 2wx, ty
        rc0 = shuffle_ps<2, 3, 0, 2>(rc0, tc0);                 //     2xz - 2wy,     2yz + 2wx, 1 - 2xx - 2yy, tz

        ra1 = shuffle_ps<0, 1, 0, 2>(ra1, ta1);                 // 1 - 2yy - 2zz,     2xy - 2wz,     2xz + 2wy, tx
        rb1 = shuffle_ps<1, 0, 0, 2>(rb1, tb1);                 //     2xy + 2wz, 1 - 2xx - 2zz,     2yz - 2wx, ty
        rc1 = shuffle_ps<2, 3, 0, 2>(rc1, tc1);                 //     2xz - 2wy,     2yz + 2wx, 1 - 2xx - 2yy, tz

        store_ps(ra0 * s0, &jointMatPtr[(i + 0) * 12 + 0]);
        store_ps(rb0 * s0, &jointMatPtr[(i + 0) * 12 + 4]);
        store_ps(rc0 * s0, &jointMatPtr[(i + 0) * 12 + 8]);

        store_ps(ra1 * s1, &jointMatPtr[(i + 1) * 12 + 0]);
        store_ps(rb1 * s1, &jointMatPtr[(i + 1) * 12 + 4]);
        store_ps(rc1 * s1, &jointMatPtr[(i + 1) * 12 + 8]);
    }

    for (; i < numJoints; i++) {
        simd4f q0 = load_ps(&jointPosePtr[(i + 0) * 12 + 0]);
        simd4f t0 = load_ps(&jointPosePtr[(i + 0) * 12 + 4]);
        simd4f s0 = load_ps(&jointPosePtr[(i + 0) * 12 + 8]);

        simd4f d0 = q0 + q0;

        s0 = insert_ps<3>(s0, 1.0f);

        simd4f sa0 = shuffle_ps<1, 0, 0, 1>(q0);                //   y,   x,   x,   y
        simd4f sb0 = shuffle_ps<1, 1, 2, 2>(d0);                //  2y,  2y,  2z,  2z
        simd4f sc0 = shuffle_ps<2, 3, 3, 3>(q0);                //   z,   w,   w,   w
        simd4f sd0 = shuffle_ps<2, 2, 1, 0>(d0);                //  2z,  2z,  2y,  2x

        sa0 ^= vector_float_first_sign_bit;                     //  -y,   x,   x,   x

        simd4f ma0 = sa0 * sb0 + vector_float_first_pos_half;   //  0.5 - 2yy,  2xy,  2xz,  2yz
        simd4f mb0 = sc0 * sd0 + vector_float_first_neg_half;   // -0.5 + 2zz,  2wz,  2wy,  2wx
        simd4f mc0 = vector_float_first_pos_half - q0 * d0;     //  0.5 - 2xx, -2yy, -2zz, -2ww

        simd4f mf0 = shuffle_ps<1, 1, 0, 0>(ma0, mc0);          //        2xy,  2xy, 0.5 - 2xx, 0.5 - 2xx
        simd4f md0 = shuffle_ps<2, 0, 2, 3>(mf0, ma0);          //  0.5 - 2xx,  2xy,       2xz,       2yz
        simd4f me0 = shuffle_ps<0, 1, 2, 3>(ma0, mb0);          //  0.5 - 2yy,  2xy,       2wy,       2wx

        simd4f ra0 = mb0 * vector_float_quat2mat_mad1 + ma0;    // 1 - 2yy - 2zz, 2xy - 2wz, 2xz + 2wy,       n/a   // - - + -
        simd4f rb0 = mb0 * vector_float_quat2mat_mad2 + md0;    // 1 - 2xx - 2zz, 2xy + 2wz,       n/a, 2yz - 2wx   // - + - -
        simd4f rc0 = me0 * vector_float_quat2mat_mad3 + md0;    // 1 - 2xx - 2yy,       n/a, 2xz - 2wy, 2yz + 2wx   // + - - +

        simd4f ta0 = shuffle_ps<2, 2, 0, 0>(ra0, t0);           //     2xz + 2wy,     2xz + 2wy, tx, tx
        simd4f tb0 = shuffle_ps<3, 3, 1, 1>(rb0, t0);           //     2yz - 2wx,     2yz - 2wx, ty, ty
        simd4f tc0 = shuffle_ps<0, 0, 2, 2>(rc0, t0);           // 1 - 2xx - 2yy, 1 - 2xx - 2yy, tz, tz

        ra0 = shuffle_ps<0, 1, 0, 2>(ra0, ta0);                 // 1 - 2yy - 2zz,     2xy - 2wz,     2xz + 2wy, tx
        rb0 = shuffle_ps<1, 0, 0, 2>(rb0, tb0);                 //     2xy + 2wz, 1 - 2xx - 2zz,     2yz - 2wx, ty
        rc0 = shuffle_ps<2, 3, 0, 2>(rc0, tc0);                 //     2xz - 2wy,     2yz + 2wx, 1 - 2xx - 2yy, tz

        store_ps(ra0 * s0, &jointMatPtr[(i + 0) * 12 + 0]);
        store_ps(rb0 * s0, &jointMatPtr[(i + 0) * 12 + 4]);
        store_ps(rc0 * s0, &jointMatPtr[(i + 0) * 12 + 8]);
    }
#endif
}

void BE_FASTCALL SIMD_4::ConvertJointMatsToJointPoses(JointPose *jointPoses, const Mat3x4 *jointMats, const int numJoints) {
    assert_16_byte_aligned(jointPoses);
    assert_16_byte_aligned(jointMats);
    assert(sizeof(JointPose) == sizeof(float) * 12);
    assert(sizeof(Mat3x4) == sizeof(float) * 12);

    const float *jointMatPtr = (const float *)jointMats;
    float *jointPosePtr = (float *)jointPoses;

    const simd4f vector_float_zero = setzero_ps();
    const simd4f vector_float_one = set1_ps(1.0f);
    const simd4f vector_float_half = set1_ps(0.5f);

    int i = 0;
    for (; i + 3 < numJoints; i += 4) {
        simd4f m00 = load_ps(&jointMatPtr[(i + 0) * 12 + 0]);
        simd4f m01 = load_ps(&jointMatPtr[(i + 1) * 12 + 0]);
        simd4f m02 = load_ps(&jointMatPtr[(i + 2) * 12 + 0]);
        simd4f m03 = load_ps(&jointMatPtr[(i + 3) * 12 + 0]);
        transpose4x4(m00, m01, m02, m03);

        simd4f m10 = load_ps(&jointMatPtr[(i + 0) * 12 + 4]);
        simd4f m11 = load_ps(&jointMatPtr[(i + 1) * 12 + 4]);
        simd4f m12 = load_ps(&jointMatPtr[(i + 2) * 12 + 4]);
        simd4f m13 = load_ps(&jointMatPtr[(i + 3) * 12 + 4]);
        transpose4x4(m10, m11, m12, m13);

        simd4f m20 = load_ps(&jointMatPtr[(i + 0) * 12 + 8]);
        simd4f m21 = load_ps(&jointMatPtr[(i + 1) * 12 + 8]);
        simd4f m22 = load_ps(&jointMatPtr[(i + 2) * 12 + 8]);
        simd4f m23 = load_ps(&jointMatPtr[(i + 3) * 12 + 8]);
        transpose4x4(m20, m21, m22, m23);

        simd4f scaleX = sqrt_ps(m00 * m00 + m10 * m10 + m20 * m20);
        simd4f scaleY = sqrt_ps(m01 * m01 + m11 * m11 + m21 * m21);
        simd4f scaleZ = sqrt_ps(m02 * m02 + m12 * m12 + m22 * m22);

        const simd4f det2_01_01 = m00 * m11 - m01 * m10;
        const simd4f det2_01_02 = m00 * m12 - m02 * m10;
        const simd4f det2_01_12 = m01 * m12 - m02 * m11;
        const simd4f determinant = m20 * det2_01_12 - m21 * det2_01_02 + m22 * det2_01_01;
        const simd4b negativeScaleMask = determinant < vector_float_zero;
        scaleX = select_ps(scaleX, -scaleX, negativeScaleMask);
        scaleY = select_ps(scaleY, -scaleY, negativeScaleMask);
        scaleZ = select_ps(scaleZ, -scaleZ, negativeScaleMask);

        const simd4f invScaleX = vector_float_one / scaleX;
        const simd4f invScaleY = vector_float_one / scaleY;
        const simd4f invScaleZ = vector_float_one / scaleZ;
        const simd4f r00 = m00 * invScaleX;
        const simd4f r01 = m01 * invScaleY;
        const simd4f r02 = m02 * invScaleZ;
        const simd4f r10 = m10 * invScaleX;
        const simd4f r11 = m11 * invScaleY;
        const simd4f r12 = m12 * invScaleZ;
        const simd4f r20 = m20 * invScaleX;
        const simd4f r21 = m21 * invScaleY;
        const simd4f r22 = m22 * invScaleZ;

        const simd4f trace = r00 + r11 + r22;
        const simd4b traceMask = trace > vector_float_zero;
        const simd4b diagonal1Mask = r11 > r00;
        const simd4f maxDiagonal01 = select_ps(r00, r11, diagonal1Mask);
        const simd4b diagonal2Mask = r22 > maxDiagonal01;
        const simd4b negativeTraceMask = !traceMask;
        const simd4b case0Mask = negativeTraceMask & !diagonal1Mask & !diagonal2Mask;
        const simd4b case1Mask = negativeTraceMask & diagonal1Mask & !diagonal2Mask;
        const simd4b case2Mask = negativeTraceMask & diagonal2Mask;

        const simd4f traceT = trace + vector_float_one;
        const simd4f traceSafeT = select_ps(vector_float_one, traceT, traceMask);
        const simd4f traceS = sqrt_ps(vector_float_one / traceSafeT) * vector_float_half;
        simd4f quatX = (r21 - r12) * traceS;
        simd4f quatY = (r02 - r20) * traceS;
        simd4f quatZ = (r10 - r01) * traceS;
        simd4f quatW = traceSafeT * traceS;

        const simd4f case0T = r00 - r11 - r22 + vector_float_one;
        const simd4f case0SafeT = select_ps(vector_float_one, case0T, case0Mask);
        const simd4f case0S = sqrt_ps(vector_float_one / case0SafeT) * vector_float_half;
        quatX = select_ps(quatX, case0SafeT * case0S, case0Mask);
        quatY = select_ps(quatY, (r10 + r01) * case0S, case0Mask);
        quatZ = select_ps(quatZ, (r20 + r02) * case0S, case0Mask);
        quatW = select_ps(quatW, (r21 - r12) * case0S, case0Mask);

        const simd4f case1T = r11 - r22 - r00 + vector_float_one;
        const simd4f case1SafeT = select_ps(vector_float_one, case1T, case1Mask);
        const simd4f case1S = sqrt_ps(vector_float_one / case1SafeT) * vector_float_half;
        quatX = select_ps(quatX, (r01 + r10) * case1S, case1Mask);
        quatY = select_ps(quatY, case1SafeT * case1S, case1Mask);
        quatZ = select_ps(quatZ, (r21 + r12) * case1S, case1Mask);
        quatW = select_ps(quatW, (r02 - r20) * case1S, case1Mask);

        const simd4f case2T = r22 - r00 - r11 + vector_float_one;
        const simd4f case2SafeT = select_ps(vector_float_one, case2T, case2Mask);
        const simd4f case2S = sqrt_ps(vector_float_one / case2SafeT) * vector_float_half;
        quatX = select_ps(quatX, (r02 + r20) * case2S, case2Mask);
        quatY = select_ps(quatY, (r12 + r21) * case2S, case2Mask);
        quatZ = select_ps(quatZ, case2SafeT * case2S, case2Mask);
        quatW = select_ps(quatW, (r10 - r01) * case2S, case2Mask);

        simd4f q0 = quatX;
        simd4f q1 = quatY;
        simd4f q2 = quatZ;
        simd4f q3 = quatW;
        transpose4x4(q0, q1, q2, q3);
        store_ps(q0, &jointPosePtr[(i + 0) * 12 + 0]);
        store_ps(q1, &jointPosePtr[(i + 1) * 12 + 0]);
        store_ps(q2, &jointPosePtr[(i + 2) * 12 + 0]);
        store_ps(q3, &jointPosePtr[(i + 3) * 12 + 0]);

        simd4f t0 = m03;
        simd4f t1 = m13;
        simd4f t2 = m23;
        simd4f t3 = vector_float_zero;
        transpose4x4(t0, t1, t2, t3);
        store_ps(t0, &jointPosePtr[(i + 0) * 12 + 4]);
        store_ps(t1, &jointPosePtr[(i + 1) * 12 + 4]);
        store_ps(t2, &jointPosePtr[(i + 2) * 12 + 4]);
        store_ps(t3, &jointPosePtr[(i + 3) * 12 + 4]);

        simd4f s0 = scaleX;
        simd4f s1 = scaleY;
        simd4f s2 = scaleZ;
        simd4f s3 = vector_float_zero;
        transpose4x4(s0, s1, s2, s3);
        store_ps(s0, &jointPosePtr[(i + 0) * 12 + 8]);
        store_ps(s1, &jointPosePtr[(i + 1) * 12 + 8]);
        store_ps(s2, &jointPosePtr[(i + 2) * 12 + 8]);
        store_ps(s3, &jointPosePtr[(i + 3) * 12 + 8]);
    }

    for (; i < numJoints; i++) {
        jointPoses[i].SetFromMat3x4(jointMats[i]);
    }
}

void BE_FASTCALL SIMD_4::TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJointIndex, const int lastJointIndex) {
    assert_16_byte_aligned(jointMats);

    simd4f pma;
    simd4f pmb;
    simd4f pmc;
    int cachedJointIndex = -1;

    for (int i = firstJointIndex; i <= lastJointIndex; i++) {
        const int parentIndex = parents[i];
        if (parentIndex < 0) {
            continue;
        }

        const float *__restrict parentMatrix = jointMats->Ptr() + parentIndex * 3 * 4;
        float *__restrict childMatrix = jointMats->Ptr() + i * 3 * 4;

        if (parentIndex != cachedJointIndex) {
            pma = load_ps(parentMatrix + 0);
            pmb = load_ps(parentMatrix + 4);
            pmc = load_ps(parentMatrix + 8);
        }

        simd4f cma = load_ps(childMatrix + 0);
        simd4f cmb = load_ps(childMatrix + 4);
        simd4f cmc = load_ps(childMatrix + 8);

        pma = lincomb3x4(pma, cma, cmb, cmc);
        pmb = lincomb3x4(pmb, cma, cmb, cmc);
        pmc = lincomb3x4(pmc, cma, cmb, cmc);

        store_ps(pma, childMatrix + 0);
        store_ps(pmb, childMatrix + 4);
        store_ps(pmc, childMatrix + 8);

        cachedJointIndex = i;
    }
}

void BE_FASTCALL SIMD_4::TransformJoints(const Mat3x4 *localJointMats, Mat3x4 *worldJointMats, const int *parents, const int firstJoint, const int lastJoint) {
    assert_16_byte_aligned(worldJointMats);

    for (int joint = firstJoint; joint <= lastJoint; joint++) {
        const int parent = parents[joint];
        if (parent < 0) {
            worldJointMats[joint] = localJointMats[joint];
            continue;
        }

        const float *__restrict parentWorldMatrix = worldJointMats->Ptr() + parent * 3 * 4;
        const float *__restrict childLocalMatrix = localJointMats->Ptr() + joint * 3 * 4;
        float *__restrict childWorldMatrix = worldJointMats->Ptr() + joint * 3 * 4;

        simd4f pma = load_ps(parentWorldMatrix + 0);
        simd4f pmb = load_ps(parentWorldMatrix + 4);
        simd4f pmc = load_ps(parentWorldMatrix + 8);

        simd4f cma = load_ps(childLocalMatrix + 0);
        simd4f cmb = load_ps(childLocalMatrix + 4);
        simd4f cmc = load_ps(childLocalMatrix + 8);

        pma = lincomb3x4(pma, cma, cmb, cmc);
        pmb = lincomb3x4(pmb, cma, cmb, cmc);
        pmc = lincomb3x4(pmc, cma, cmb, cmc);

        store_ps(pma, childWorldMatrix + 0);
        store_ps(pmb, childWorldMatrix + 4);
        store_ps(pmc, childWorldMatrix + 8);
    }
}

void BE_FASTCALL SIMD_4::UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    assert_16_byte_aligned(jointMats);

    for (int joint = lastJoint; joint >= firstJoint; joint--) {
        assert(parents[joint] < joint);
        const int parent = parents[joint];
        if (parent < 0) {
            continue;
        }

        const float *__restrict parentMatrix = jointMats->Ptr() + parent * 3 * 4;
        float *__restrict childMatrix = jointMats->Ptr() + joint * 3 * 4;

        simd4f pma = load_ps(parentMatrix + 0);
        simd4f pmb = load_ps(parentMatrix + 4);
        simd4f pmc = load_ps(parentMatrix + 8);

        simd4f cma = load_ps(childMatrix + 0);
        simd4f cmb = load_ps(childMatrix + 4);
        simd4f cmc = load_ps(childMatrix + 8);

        simd4f ta = shuffle_ps<0, 0, 0, 0>(pma);
        simd4f tb = shuffle_ps<1, 1, 1, 1>(pma);
        simd4f tc = shuffle_ps<2, 2, 2, 2>(pma);

        simd4f td = shuffle_ps<0, 0, 0, 0>(pmb);
        simd4f te = shuffle_ps<1, 1, 1, 1>(pmb);
        simd4f tf = shuffle_ps<2, 2, 2, 2>(pmb);

        simd4f tg = shuffle_ps<0, 0, 0, 0>(pmc);
        simd4f th = shuffle_ps<1, 1, 1, 1>(pmc);
        simd4f ti = shuffle_ps<2, 2, 2, 2>(pmc);

        cma = cma - (pma & SIMD_4::F4_mask_000x);
        cmb = cmb - (pmb & SIMD_4::F4_mask_000x);
        cmc = cmc - (pmc & SIMD_4::F4_mask_000x);

        pma = ta * cma;
        pmb = tb * cma;
        pmc = tc * cma;

        pma = madd_ps(td, cmb, pma);
        pmb = madd_ps(te, cmb, pmb);
        pmc = madd_ps(tf, cmb, pmc);

        pma = madd_ps(tg, cmc, pma);
        pmb = madd_ps(th, cmc, pmb);
        pmc = madd_ps(ti, cmc, pmc);

        store_ps(pma, childMatrix + 0);
        store_ps(pmb, childMatrix + 4);
        store_ps(pmc, childMatrix + 8);
    }
}

void BE_FASTCALL SIMD_4::MultiplyJoints(Mat3x4 *result, const Mat3x4 *joints1, const Mat3x4 *joints2, const int numJoints) {
#if 0
    int i;

    for (i = 0; i < numJoints; i++) {
        result[i] = joints1[i] * joints2[i];
    }
#else
    assert_16_byte_aligned(result);
    assert_16_byte_aligned(joints1);
    assert_16_byte_aligned(joints2);

    const float *joint1Ptr = (float *)joints1;
    const float *joint2Ptr = (float *)joints2;
    float *resultPtr = (float *)result;

    int i;

    for (i = 0; i < numJoints; i++) {
        simd4f ar0 = load_ps(&joint1Ptr[i * 12 + 0]);
        simd4f ar1 = load_ps(&joint1Ptr[i * 12 + 4]);
        simd4f ar2 = load_ps(&joint1Ptr[i * 12 + 8]);

        simd4f br0 = load_ps(&joint2Ptr[i * 12 + 0]);
        simd4f br1 = load_ps(&joint2Ptr[i * 12 + 4]);
        simd4f br2 = load_ps(&joint2Ptr[i * 12 + 8]);

        store_ps(lincomb3x4(ar0, br0, br1, br2), &resultPtr[i * 12 + 0]);
        store_ps(lincomb3x4(ar1, br0, br1, br2), &resultPtr[i * 12 + 4]);
        store_ps(lincomb3x4(ar2, br0, br1, br2), &resultPtr[i * 12 + 8]);
    }
#endif
}

void SIMD_4::Memcpy64B(void *dst, const void *src, const int count) {
    assert_16_byte_aligned(src);
    assert_16_byte_aligned(dst);

    const byte *srcPtr = (byte *)src;
    byte *dstPtr = (byte *)dst;
    simd4i r0, r1, r2, r3;

    int c64 = count >> 6;
    while (c64 > 0) {
        if (c64 > 1) {
            // Loads one cache line (64B) of data from next 64B block of memory to CPU prefetch buffer.
            prefetchNTA(srcPtr + 64);
        }
        
        r0 = load_si128((const int32_t *)srcPtr);
        r1 = load_si128((const int32_t *)(srcPtr + 16));
        r2 = load_si128((const int32_t *)(srcPtr + 32));
        r3 = load_si128((const int32_t *)(srcPtr + 48));

        store_si128(r0, (int32_t *)dstPtr);
        store_si128(r1, (int32_t *)(dstPtr + 16));
        store_si128(r2, (int32_t *)(dstPtr + 32));
        store_si128(r3, (int32_t *)(dstPtr + 48));

        srcPtr += 64;
        dstPtr += 64;

        c64--;
    }
}

void SIMD_4::Memcpy2KB(void *dst, const void *src, const int count) {
    assert_16_byte_aligned(src);
    assert_16_byte_aligned(dst);

    const byte *srcPtr = (byte *)src;
    byte *dstPtr = (byte *)dst;
    simd4i r0, r1, r2, r3, r4, r5, r6, r7;

    int c128;
    int c2k = count >> 11;
    while (c2k > 0) {
        c128 = 16;
        while (c128 > 0) {
            // Loads two cache lines (64B x 2) of data from next 128B block of memory to CPU prefetch buffer.
            prefetchNTA(srcPtr + 128);
            prefetchNTA(srcPtr + 192);

            r0 = load_si128((const int32_t *)(srcPtr + 0));
            r1 = load_si128((const int32_t *)(srcPtr + 16));
            r2 = load_si128((const int32_t *)(srcPtr + 32));
            r3 = load_si128((const int32_t *)(srcPtr + 48));
            r4 = load_si128((const int32_t *)(srcPtr + 64));
            r5 = load_si128((const int32_t *)(srcPtr + 80));
            r6 = load_si128((const int32_t *)(srcPtr + 96));
            r7 = load_si128((const int32_t *)(srcPtr + 112));

            store_si128(r0, (int32_t *)(dstPtr + 0));
            store_si128(r1, (int32_t *)(dstPtr + 16));
            store_si128(r2, (int32_t *)(dstPtr + 32));
            store_si128(r3, (int32_t *)(dstPtr + 48));
            store_si128(r4, (int32_t *)(dstPtr + 64));
            store_si128(r5, (int32_t *)(dstPtr + 80));
            store_si128(r6, (int32_t *)(dstPtr + 96));
            store_si128(r7, (int32_t *)(dstPtr + 112));

            srcPtr += 128;
            dstPtr += 128;

            c128--;
        }

        c2k--;
    }
}

// optimized memory copy routine that handles all alignment cases and block sizes efficiently
void BE_FASTCALL SIMD_4::Memcpy(void *dst, const void *src, const int size) {
#if 0
    memcpy(dst, src, size);
#else
    if (size > 4096 && !(((intptr_t)dst ^ (intptr_t)src) & 15)) {
        const byte *srcPtr = (const byte *)src;
        byte *dstPtr = (byte *)dst;

        // Copy up to the first 16 byte aligned boundary.
        int remainingSize = (16 - (((intptr_t)dstPtr) & 15)) & 15;
        if (remainingSize > 0) {
            memcpy(dstPtr, srcPtr, remainingSize);
            dstPtr += remainingSize;
            srcPtr += remainingSize;
        }

        remainingSize = size - remainingSize;

        // If there are multiple blocks of 2kB, ..
        if (remainingSize & ~4095) {
            SIMD_4::Memcpy2KB(dstPtr, srcPtr, remainingSize);
            srcPtr += (remainingSize & ~2047);
            dstPtr += (remainingSize & ~2047);
            remainingSize &= 2047;
        }

        // If there are blocks of 64 bytes, ..
        if (remainingSize & ~63) {
            SIMD_4::Memcpy64B(dstPtr, srcPtr, remainingSize);
            srcPtr += (remainingSize & ~63);
            dstPtr += (remainingSize & ~63);
            remainingSize &= 63;
        }

        // Copy any remaining bytes.
        memcpy(dstPtr, srcPtr, remainingSize);
    } else {
        // Use the regular one if we cannot copy 16 byte aligned.
        memcpy(dst, src, size);
    }
#endif
}

void BE_FASTCALL SIMD_4::Memset(void *dest0, const int val, const int count0) {
#if 1
    memset(dest0, val, count0);
#else
    byte *dest = (byte *)dest0;
    int count = count0;

    while (count > 0 && (((intptr_t)dest) & 15)) {
        *dest = val;
        dest++;
        count--;
    }

    if (!count) {
        return;
    }

    simd4i data = set1_epi8((char)val);

    if (count >= 128) {
        int c128 = count >> 7;
        while (c128 > 0) {
            store_si128(data, (int32_t *)dest);
            store_si128(data, (int32_t *)(dest + 16));
            store_si128(data, (int32_t *)(dest + 32));
            store_si128(data, (int32_t *)(dest + 48));
            store_si128(data, (int32_t *)(dest + 64));
            store_si128(data, (int32_t *)(dest + 80));
            store_si128(data, (int32_t *)(dest + 96));
            store_si128(data, (int32_t *)(dest + 112));

            dest += 128;
            c128--;
        }

        count &= 127;
    }

    if (count >= 16) {
        int c16 = count >> 4;
        while (c16 > 0) {
            store_si128(data, (int32_t *)dest);

            dest += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dest = val;
        dest++;
        count--;
    }
#endif
}

BE_NAMESPACE_END

#endif
