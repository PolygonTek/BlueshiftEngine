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

#if defined(HAVE_X86_AVX_INTRIN)

BE_NAMESPACE_BEGIN

const simd8f SIMD_8::F8_zero                = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
const simd8f SIMD_8::F8_one                 = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
const simd8f SIMD_8::F8_3zero_one           = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
const simd8f SIMD_8::F8_3one_zero           = { 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };
const simd8f SIMD_8::F8_half                = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
const simd8f SIMD_8::F8_255                 = { 255.0f, 255.0f, 255.0f, 255.0f, 255.0f, 255.0f, 255.0f, 255.0f };
const simd8f SIMD_8::F8_min_char            = { -128.0f, -128.0f, -128.0f, -128.0f, -128.0f, -128.0f, -128.0f, -128.0f };
const simd8f SIMD_8::F8_max_char            = { 127.0f, 127.0f, 127.0f, 127.0f, 127.0f, 127.0f, 127.0f, 127.0f };
const simd8f SIMD_8::F8_min_short           = { -32768.0f, -32768.0f, -32768.0f, -32768.0f, -32768.0f, -32768.0f, -32768.0f, -32768.0f };
const simd8f SIMD_8::F8_max_short           = { 32767.0f, 32767.0f, 32767.0f, 32767.0f, 32767.0f, 32767.0f, 32767.0f, 32767.0f };
const simd8f SIMD_8::F8_tiny                = { 1e-4f, 1e-4f, 1e-4f, 1e-4f, 1e-4f, 1e-4f, 1e-4f, 1e-4f };
const simd8f SIMD_8::F8_smallestNonDenorm   = { 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f };
const simd8f SIMD_8::F8_sign_bit            = (simd8f &)simd8i(0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000);
const simd8f SIMD_8::F8_mask_xxx0xxx0       = (simd8f &)simd8i(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000);
const simd8f SIMD_8::F8_mask_000x000x       = (simd8f &)simd8i(0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF);

void BE_FASTCALL SIMD_8::Add(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count >= 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src);

        simd8f c = set1_256ps(constant);

        int c32 = count >> 5;
        while (c32 > 0) {

            simd8f x0 = c + load_256ps(src_ptr + 0);
            simd8f x1 = c + load_256ps(src_ptr + 8);
            simd8f x2 = c + load_256ps(src_ptr + 16);
            simd8f x3 = c + load_256ps(src_ptr + 24);

            store_256ps(x0, dst_ptr + 0);
            store_256ps(x1, dst_ptr + 8);
            store_256ps(x2, dst_ptr + 16);
            store_256ps(x3, dst_ptr + 24);

            src_ptr += 32;
            dst_ptr += 32;
            c32--;
        }

        count &= 31;
    }

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = c + load_ps(src_ptr + 0);
            simd4f x1 = c + load_ps(src_ptr + 4);
            simd4f x2 = c + load_ps(src_ptr + 8);
            simd4f x3 = c + load_ps(src_ptr + 12);

            store_ps(x0, dst_ptr + 0);
            store_ps(x1, dst_ptr + 4);
            store_ps(x2, dst_ptr + 8);
            store_ps(x3, dst_ptr + 12);

            src_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = constant + *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Add(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count >= 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src0);
        assert_32_byte_aligned(src1);

        int c32 = count >> 5;
        while (c32 > 0) {

            simd8f x0 = load_256ps(src0_ptr + 0);
            simd8f x1 = load_256ps(src0_ptr + 8);
            simd8f x2 = load_256ps(src0_ptr + 16);
            simd8f x3 = load_256ps(src0_ptr + 24);

            x0 = x0 + load_256ps(src1_ptr + 0);
            x1 = x1 + load_256ps(src1_ptr + 8);
            x2 = x2 + load_256ps(src1_ptr + 16);
            x3 = x3 + load_256ps(src1_ptr + 24);

            store_256ps(x0, dst_ptr + 0);
            store_256ps(x1, dst_ptr + 8);
            store_256ps(x2, dst_ptr + 16);
            store_256ps(x3, dst_ptr + 24);

            src0_ptr += 32;
            src1_ptr += 32;
            dst_ptr += 32;
            c32--;
        }

        count &= 31;
    }

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

            store_ps(x0, dst_ptr + 0);
            store_ps(x1, dst_ptr + 4);
            store_ps(x2, dst_ptr + 8);
            store_ps(x3, dst_ptr + 12);

            src0_ptr += 16;
            src1_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Sub(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count >= 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src);

        simd8f c = set1_256ps(constant);

        int c32 = count >> 5;
        while (c32 > 0) {

            simd8f x0 = c - load_256ps(src_ptr + 0);
            simd8f x1 = c - load_256ps(src_ptr + 8);
            simd8f x2 = c - load_256ps(src_ptr + 16);
            simd8f x3 = c - load_256ps(src_ptr + 24);

            store_256ps(x0, dst_ptr + 0);
            store_256ps(x1, dst_ptr + 8);
            store_256ps(x2, dst_ptr + 16);
            store_256ps(x3, dst_ptr + 24);

            src_ptr += 32;
            dst_ptr += 32;
            c32--;
        }

        count &= 31;
    }

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = c - load_ps(src_ptr + 0);
            simd4f x1 = c - load_ps(src_ptr + 4);
            simd4f x2 = c - load_ps(src_ptr + 8);
            simd4f x3 = c - load_ps(src_ptr + 12);

            store_ps(x0, dst_ptr + 0);
            store_ps(x1, dst_ptr + 4);
            store_ps(x2, dst_ptr + 8);
            store_ps(x3, dst_ptr + 12);

            src_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = constant - *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Sub(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count >= 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src0);
        assert_32_byte_aligned(src1);

        int c32 = count >> 5;
        while (c32 > 0) {

            simd8f x0 = load_256ps(src0_ptr + 0);
            simd8f x1 = load_256ps(src0_ptr + 8);
            simd8f x2 = load_256ps(src0_ptr + 16);
            simd8f x3 = load_256ps(src0_ptr + 24);

            x0 = x0 - load_256ps(src1_ptr + 0);
            x1 = x1 - load_256ps(src1_ptr + 8);
            x2 = x2 - load_256ps(src1_ptr + 16);
            x3 = x3 - load_256ps(src1_ptr + 24);

            store_256ps(x0, dst_ptr + 0);
            store_256ps(x1, dst_ptr + 8);
            store_256ps(x2, dst_ptr + 16);
            store_256ps(x3, dst_ptr + 24);

            src0_ptr += 32;
            src1_ptr += 32;
            dst_ptr += 32;
            c32--;
        }

        count &= 31;
    }

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

            store_ps(x0, dst_ptr + 0);
            store_ps(x1, dst_ptr + 4);
            store_ps(x2, dst_ptr + 8);
            store_ps(x3, dst_ptr + 12);

            src0_ptr += 16;
            src1_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = *src0_ptr++ - *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Mul(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count >= 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src);

        simd8f c = set1_256ps(constant);

        int c32 = count >> 5;
        while (c32 > 0) {

            simd8f x0 = c * load_256ps(src_ptr + 0);
            simd8f x1 = c * load_256ps(src_ptr + 8);
            simd8f x2 = c * load_256ps(src_ptr + 16);
            simd8f x3 = c * load_256ps(src_ptr + 24);

            store_256ps(x0, dst_ptr + 0);
            store_256ps(x1, dst_ptr + 8);
            store_256ps(x2, dst_ptr + 16);
            store_256ps(x3, dst_ptr + 24);

            src_ptr += 32;
            dst_ptr += 32;
            c32--;
        }

        count &= 31;
    }

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = c * load_ps(src_ptr + 0);
            simd4f x1 = c * load_ps(src_ptr + 4);
            simd4f x2 = c * load_ps(src_ptr + 8);
            simd4f x3 = c * load_ps(src_ptr + 12);

            store_ps(x0, dst_ptr + 0);
            store_ps(x1, dst_ptr + 4);
            store_ps(x2, dst_ptr + 8);
            store_ps(x3, dst_ptr + 12);

            src_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = constant * *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Mul(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count >= 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src0);
        assert_32_byte_aligned(src1);

        int c32 = count >> 5;
        while (c32 > 0) {

            simd8f x0 = load_256ps(src0_ptr + 0);
            simd8f x1 = load_256ps(src0_ptr + 8);
            simd8f x2 = load_256ps(src0_ptr + 16);
            simd8f x3 = load_256ps(src0_ptr + 24);

            x0 = x0 * load_256ps(src1_ptr + 0);
            x1 = x1 * load_256ps(src1_ptr + 8);
            x2 = x2 * load_256ps(src1_ptr + 16);
            x3 = x3 * load_256ps(src1_ptr + 24);

            store_256ps(x0, dst_ptr + 0);
            store_256ps(x1, dst_ptr + 8);
            store_256ps(x2, dst_ptr + 16);
            store_256ps(x3, dst_ptr + 24);

            src0_ptr += 32;
            src1_ptr += 32;
            dst_ptr += 32;
            c32--;
        }

        count &= 31;
    }

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

            store_ps(x0, dst_ptr + 0);
            store_ps(x1, dst_ptr + 4);
            store_ps(x2, dst_ptr + 8);
            store_ps(x3, dst_ptr + 12);

            src0_ptr += 16;
            src1_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = *src0_ptr++ * *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Div(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count >= 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src);

        simd8f c = set1_256ps(constant);

        int c32 = count >> 5;
        while (c32 > 0) {

            simd8f x0 = c / load_256ps(src_ptr + 0);
            simd8f x1 = c / load_256ps(src_ptr + 8);
            simd8f x2 = c / load_256ps(src_ptr + 16);
            simd8f x3 = c / load_256ps(src_ptr + 24);

            store_256ps(x0, dst_ptr + 0);
            store_256ps(x1, dst_ptr + 8);
            store_256ps(x2, dst_ptr + 16);
            store_256ps(x3, dst_ptr + 24);

            src_ptr += 32;
            dst_ptr += 32;
            c32--;
        }

        count &= 31;
    }

    if (count >= 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {

            simd4f x0 = c / load_ps(src_ptr + 0);
            simd4f x1 = c / load_ps(src_ptr + 4);
            simd4f x2 = c / load_ps(src_ptr + 8);
            simd4f x3 = c / load_ps(src_ptr + 12);

            store_ps(x0, dst_ptr + 0);
            store_ps(x1, dst_ptr + 4);
            store_ps(x2, dst_ptr + 8);
            store_ps(x3, dst_ptr + 12);

            src_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = constant / *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Div(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count >= 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src0);
        assert_32_byte_aligned(src1);

        int c32 = count >> 5;
        while (c32 > 0) {

            simd8f x0 = load_256ps(src0_ptr + 0);
            simd8f x1 = load_256ps(src0_ptr + 8);
            simd8f x2 = load_256ps(src0_ptr + 16);
            simd8f x3 = load_256ps(src0_ptr + 24);

            x0 = x0 / load_256ps(src1_ptr + 0);
            x1 = x1 / load_256ps(src1_ptr + 8);
            x2 = x2 / load_256ps(src1_ptr + 16);
            x3 = x3 / load_256ps(src1_ptr + 24);

            store_256ps(x0, dst_ptr + 0);
            store_256ps(x1, dst_ptr + 8);
            store_256ps(x2, dst_ptr + 16);
            store_256ps(x3, dst_ptr + 24);

            src0_ptr += 32;
            src1_ptr += 32;
            dst_ptr += 32;
            c32--;
        }

        count &= 31;
    }

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

            store_ps(x0, dst_ptr + 0);
            store_ps(x1, dst_ptr + 4);
            store_ps(x2, dst_ptr + 8);
            store_ps(x3, dst_ptr + 12);

            src0_ptr += 16;
            src1_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = *src0_ptr++ / *src1_ptr++;
        count--;
    }
}

float BE_FASTCALL SIMD_8::Sum(const float *src, const int count0) {
    int count = count0;
    const float *srcPtr = src;
    float ret = 0;

    if (count >= 4) {
        assert_16_byte_aligned(src);

        simd4f total4 = setzero_ps();

        if (count >= 8) {
            simd8f total8 = setzero_256ps();

            if (count >= 32) {
                simd8f sum0 = setzero_256ps();
                simd8f sum1 = setzero_256ps();
                simd8f sum2 = setzero_256ps();
                simd8f sum3 = setzero_256ps();

                int c32 = count >> 5;
                while (c32 > 0) {
                    sum0 += loadu_256ps(srcPtr + 0);
                    sum1 += loadu_256ps(srcPtr + 8);
                    sum2 += loadu_256ps(srcPtr + 16);
                    sum3 += loadu_256ps(srcPtr + 24);

                    srcPtr += 32;
                    c32--;
                }

                total8 = (sum0 + sum1) + (sum2 + sum3);
                count &= 31;
            }

            while (count >= 8) {
                total8 += loadu_256ps(srcPtr);
                srcPtr += 8;
                count -= 8;
            }

            total4 = extract_256ps<0>(total8) + extract_256ps<1>(total8);
        }

        if (count >= 4) {
            total4 += load_ps(srcPtr);
            srcPtr += 4;
            count -= 4;
        }

        ret = extract_ps<0>(sum_ps(total4));
    }

    while (count > 0) {
        ret += *srcPtr++;
        count--;
    }

    return ret;
}

BE_FORCE_INLINE void SIMD_8::LoadJointQuats8(const JointPose *joints, const int *index, simd8f &qx, simd8f &qy, simd8f &qz, simd8f &qw) {
    simd4f q0 = load_ps(joints[index[0]].q);
    simd4f q1 = load_ps(joints[index[1]].q);
    simd4f q2 = load_ps(joints[index[2]].q);
    simd4f q3 = load_ps(joints[index[3]].q);
    simd4f q4 = load_ps(joints[index[4]].q);
    simd4f q5 = load_ps(joints[index[5]].q);
    simd4f q6 = load_ps(joints[index[6]].q);
    simd4f q7 = load_ps(joints[index[7]].q);

    transpose4x4(q0, q1, q2, q3);
    transpose4x4(q4, q5, q6, q7);

    qx = set2x128_256ps(q0.m128, q4.m128);
    qy = set2x128_256ps(q1.m128, q5.m128);
    qz = set2x128_256ps(q2.m128, q6.m128);
    qw = set2x128_256ps(q3.m128, q7.m128);
}

BE_FORCE_INLINE void SIMD_8::StoreJointQuats8(JointPose *joints, const int *index, const simd8f &qx, const simd8f &qy, const simd8f &qz, const simd8f &qw) {
    simd4f q0 = extract_256ps<0>(qx);
    simd4f q1 = extract_256ps<0>(qy);
    simd4f q2 = extract_256ps<0>(qz);
    simd4f q3 = extract_256ps<0>(qw);
    simd4f q4 = extract_256ps<1>(qx);
    simd4f q5 = extract_256ps<1>(qy);
    simd4f q6 = extract_256ps<1>(qz);
    simd4f q7 = extract_256ps<1>(qw);

    transpose4x4(q0, q1, q2, q3);
    transpose4x4(q4, q5, q6, q7);

    store_ps(q0, joints[index[0]].q);
    store_ps(q1, joints[index[1]].q);
    store_ps(q2, joints[index[2]].q);
    store_ps(q3, joints[index[3]].q);
    store_ps(q4, joints[index[4]].q);
    store_ps(q5, joints[index[5]].q);
    store_ps(q6, joints[index[6]].q);
    store_ps(q7, joints[index[7]].q);
}

// Linearly interpolates two joint values in parallel.
BE_FORCE_INLINE void SIMD_8::LerpJointValues2(float *values0, float *values1, const float *blendValues0, const float *blendValues1, const simd8f &lerp) {
    simd8f values = set2x128_256ps(load_ps(values0).m128, load_ps(values1).m128);
    const simd8f blendValues = set2x128_256ps(load_ps(blendValues0).m128, load_ps(blendValues1).m128);
    values = madd_256ps(lerp, blendValues - values, values);
    store_ps(extract_256ps<0>(values), values0);
    store_ps(extract_256ps<1>(values), values1);
}

BE_FORCE_INLINE void SIMD_8::SlerpJointQuats8(simd8f &fromX, simd8f &fromY, simd8f &fromZ, simd8f &fromW, const simd8f &toX, const simd8f &toY, const simd8f &toZ, const simd8f &toW, const simd8f &lerp) {
    const simd8f vector_float_rsqrt_c0 = set1_256ps(-3.0f);
    const simd8f vector_float_rsqrt_c1 = set1_256ps(-0.5f);
    const simd8f vector_float_tiny = set1_256ps(1e-10f);
    const simd8f vector_float_half_pi = set1_256ps(Math::HalfPi);

    const simd8f vector_float_sin_c0 = set1_256ps(-2.39e-08f);
    const simd8f vector_float_sin_c1 = set1_256ps(2.7526e-06f);
    const simd8f vector_float_sin_c2 = set1_256ps(-1.98409e-04f);
    const simd8f vector_float_sin_c3 = set1_256ps(8.3333315e-03f);
    const simd8f vector_float_sin_c4 = set1_256ps(-1.666666664e-01f);

    const simd8f vector_float_atan_c0 = set1_256ps(0.0028662257f);
    const simd8f vector_float_atan_c1 = set1_256ps(-0.0161657367f);
    const simd8f vector_float_atan_c2 = set1_256ps(0.0429096138f);
    const simd8f vector_float_atan_c3 = set1_256ps(-0.0752896400f);
    const simd8f vector_float_atan_c4 = set1_256ps(0.1065626393f);
    const simd8f vector_float_atan_c5 = set1_256ps(-0.1420889944f);
    const simd8f vector_float_atan_c6 = set1_256ps(0.1999355085f);
    const simd8f vector_float_atan_c7 = set1_256ps(-0.3333314528f);

    simd8f cosom = fromX * toX;
    cosom = madd_256ps(fromY, toY, cosom);
    cosom = madd_256ps(fromZ, toZ, cosom);
    cosom = madd_256ps(fromW, toW, cosom);

    const simd8f sign = cosom & SIMD_8::F8_sign_bit;
    cosom ^= sign;

    simd8f sinSqr = nmadd_256ps(cosom, cosom, SIMD_8::F8_one);
    sinSqr = max_256ps(sinSqr, vector_float_tiny);

    const simd8f rsqrt = rsqrt12_256ps(sinSqr);
    const simd8f rsqrtSqr = rsqrt * rsqrt;
    const simd8f invSin = (rsqrt * vector_float_rsqrt_c1) * madd_256ps(sinSqr, rsqrtSqr, vector_float_rsqrt_c0);

    const simd8f sin = sinSqr * invSin;
    const simd8f minSinCos = min_256ps(sin, cosom);
    const simd8f maxSinCos = max_256ps(sin, cosom);
    const simd8f swapMask = (minSinCos == cosom);
    const simd8f atanSign = swapMask & SIMD_8::F8_sign_bit;
    const simd8f atanOffset = swapMask & vector_float_half_pi;

    const simd8f reciprocalEstimate = rcp12_256ps(maxSinCos);
    const simd8f reciprocal = nmadd_256ps(maxSinCos * reciprocalEstimate, reciprocalEstimate, reciprocalEstimate + reciprocalEstimate);
    const simd8f atanInput = (minSinCos * reciprocal) ^ atanSign;
    const simd8f atanInputSqr = atanInput * atanInput;

    simd8f atanPolynomial = madd_256ps(atanInputSqr, vector_float_atan_c0, vector_float_atan_c1);
    atanPolynomial = madd_256ps(atanPolynomial, atanInputSqr, vector_float_atan_c2);
    atanPolynomial = madd_256ps(atanPolynomial, atanInputSqr, vector_float_atan_c3);
    atanPolynomial = madd_256ps(atanPolynomial, atanInputSqr, vector_float_atan_c4);
    atanPolynomial = madd_256ps(atanPolynomial, atanInputSqr, vector_float_atan_c5);
    atanPolynomial = madd_256ps(atanPolynomial, atanInputSqr, vector_float_atan_c6);
    atanPolynomial = madd_256ps(atanPolynomial, atanInputSqr, vector_float_atan_c7);
    atanPolynomial = madd_256ps(atanPolynomial, atanInputSqr, SIMD_8::F8_one);

    const simd8f omega = madd_256ps(atanPolynomial, atanInput, atanOffset);
    const simd8f omegaTo = lerp * omega;
    const simd8f omegaFrom = omega - omegaTo;
    const simd8f omegaFromSqr = omegaFrom * omegaFrom;
    const simd8f omegaToSqr = omegaTo * omegaTo;

    simd8f sinFrom = madd_256ps(omegaFromSqr, vector_float_sin_c0, vector_float_sin_c1);
    simd8f sinTo = madd_256ps(omegaToSqr, vector_float_sin_c0, vector_float_sin_c1);
    sinFrom = madd_256ps(sinFrom, omegaFromSqr, vector_float_sin_c2);
    sinTo = madd_256ps(sinTo, omegaToSqr, vector_float_sin_c2);
    sinFrom = madd_256ps(sinFrom, omegaFromSqr, vector_float_sin_c3);
    sinTo = madd_256ps(sinTo, omegaToSqr, vector_float_sin_c3);
    sinFrom = madd_256ps(sinFrom, omegaFromSqr, vector_float_sin_c4);
    sinTo = madd_256ps(sinTo, omegaToSqr, vector_float_sin_c4);
    sinFrom = madd_256ps(sinFrom, omegaFromSqr, SIMD_8::F8_one) * omegaFrom * invSin;
    sinTo = madd_256ps(sinTo, omegaToSqr, SIMD_8::F8_one) * omegaTo * invSin;
    sinTo ^= sign;

    fromX = madd_256ps(toX, sinTo, fromX * sinFrom);
    fromY = madd_256ps(toY, sinTo, fromY * sinFrom);
    fromZ = madd_256ps(toZ, sinTo, fromZ * sinFrom);
    fromW = madd_256ps(toW, sinTo, fromW * sinFrom);
}

BE_FORCE_INLINE void SIMD_8::NlerpJointQuats8(simd8f &fromX, simd8f &fromY, simd8f &fromZ, simd8f &fromW, const simd8f &toX, const simd8f &toY, const simd8f &toZ, const simd8f &toW, const simd8f &scaledLerp) {
    const simd8f vector_float_rsqrt_c0 = set1_256ps(-3.0f);
    const simd8f vector_float_rsqrt_c1 = set1_256ps(-0.5f);

    simd8f cosom = fromX * toX;
    cosom = madd_256ps(fromY, toY, cosom);
    cosom = madd_256ps(fromZ, toZ, cosom);
    cosom = madd_256ps(fromW, toW, cosom);

    const simd8f scale = scaledLerp ^ (cosom & SIMD_8::F8_sign_bit);
    fromX = madd_256ps(scale, toX, fromX);
    fromY = madd_256ps(scale, toY, fromY);
    fromZ = madd_256ps(scale, toZ, fromZ);
    fromW = madd_256ps(scale, toW, fromW);

    simd8f lengthSqr = fromX * fromX;
    lengthSqr = madd_256ps(fromY, fromY, lengthSqr);
    lengthSqr = madd_256ps(fromZ, fromZ, lengthSqr);
    lengthSqr = madd_256ps(fromW, fromW, lengthSqr);

    const simd8f rsqrt = rsqrt12_256ps(lengthSqr);
    const simd8f invLength = (rsqrt * vector_float_rsqrt_c1) * madd_256ps(lengthSqr, rsqrt * rsqrt, vector_float_rsqrt_c0);
    fromX *= invLength;
    fromY *= invLength;
    fromZ *= invLength;
    fromW *= invLength;
}

void BE_FASTCALL SIMD_8::BlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    assert_16_byte_aligned(joints);
    assert_16_byte_aligned(blendJoints);

    if (fraction <= 0.0f) {
        return;
    } else if (fraction >= 1.0f) {
        for (int i = 0; i < numJoints; i++) {
            const int j = index[i];
            joints[j] = blendJoints[j];
        }
        return;
    }

    const simd8f lerp = set1_256ps(fraction);

    int i = 0;
    for (; i + 7 < numJoints; i += 8) {
        const int *jointIndex = index + i;
        const int n0 = jointIndex[0];
        const int n1 = jointIndex[1];
        const int n2 = jointIndex[2];
        const int n3 = jointIndex[3];
        const int n4 = jointIndex[4];
        const int n5 = jointIndex[5];
        const int n6 = jointIndex[6];
        const int n7 = jointIndex[7];

        // Blend positions.
        LerpJointValues2(joints[n0].t, joints[n1].t, blendJoints[n0].t, blendJoints[n1].t, lerp);
        LerpJointValues2(joints[n2].t, joints[n3].t, blendJoints[n2].t, blendJoints[n3].t, lerp);
        LerpJointValues2(joints[n4].t, joints[n5].t, blendJoints[n4].t, blendJoints[n5].t, lerp);
        LerpJointValues2(joints[n6].t, joints[n7].t, blendJoints[n6].t, blendJoints[n7].t, lerp);

        // Blend scales.
        LerpJointValues2(joints[n0].s, joints[n1].s, blendJoints[n0].s, blendJoints[n1].s, lerp);
        LerpJointValues2(joints[n2].s, joints[n3].s, blendJoints[n2].s, blendJoints[n3].s, lerp);
        LerpJointValues2(joints[n4].s, joints[n5].s, blendJoints[n4].s, blendJoints[n5].s, lerp);
        LerpJointValues2(joints[n6].s, joints[n7].s, blendJoints[n6].s, blendJoints[n7].s, lerp);

        simd8f jqx, jqy, jqz, jqw;
        simd8f bqx, bqy, bqz, bqw;
        LoadJointQuats8(joints, jointIndex, jqx, jqy, jqz, jqw);
        LoadJointQuats8(blendJoints, jointIndex, bqx, bqy, bqz, bqw);
        SlerpJointQuats8(jqx, jqy, jqz, jqw, bqx, bqy, bqz, bqw, lerp);
        StoreJointQuats8(joints, jointIndex, jqx, jqy, jqz, jqw);
    }

    if (i < numJoints) {
        SIMD_4::BlendJoints(joints, blendJoints, fraction, index + i, numJoints - i);
    }
}

void BE_FASTCALL SIMD_8::BlendJointsFast(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    assert_16_byte_aligned(joints);
    assert_16_byte_aligned(blendJoints);

    if (fraction <= 0.0f) {
        return;
    } else if (fraction >= 1.0f) {
        for (int i = 0; i < numJoints; i++) {
            const int j = index[i];
            joints[j] = blendJoints[j];
        }
        return;
    }

    const simd8f lerp = set1_256ps(fraction);
    const simd8f scaledLerp = set1_256ps(fraction / (1.0f - fraction));

    int i = 0;
    for (; i + 7 < numJoints; i += 8) {
        const int *jointIndex = index + i;
        const int n0 = jointIndex[0];
        const int n1 = jointIndex[1];
        const int n2 = jointIndex[2];
        const int n3 = jointIndex[3];
        const int n4 = jointIndex[4];
        const int n5 = jointIndex[5];
        const int n6 = jointIndex[6];
        const int n7 = jointIndex[7];

        // Blend positions.
        LerpJointValues2(joints[n0].t, joints[n1].t, blendJoints[n0].t, blendJoints[n1].t, lerp);
        LerpJointValues2(joints[n2].t, joints[n3].t, blendJoints[n2].t, blendJoints[n3].t, lerp);
        LerpJointValues2(joints[n4].t, joints[n5].t, blendJoints[n4].t, blendJoints[n5].t, lerp);
        LerpJointValues2(joints[n6].t, joints[n7].t, blendJoints[n6].t, blendJoints[n7].t, lerp);

        // Blend scales.
        LerpJointValues2(joints[n0].s, joints[n1].s, blendJoints[n0].s, blendJoints[n1].s, lerp);
        LerpJointValues2(joints[n2].s, joints[n3].s, blendJoints[n2].s, blendJoints[n3].s, lerp);
        LerpJointValues2(joints[n4].s, joints[n5].s, blendJoints[n4].s, blendJoints[n5].s, lerp);
        LerpJointValues2(joints[n6].s, joints[n7].s, blendJoints[n6].s, blendJoints[n7].s, lerp);

        simd8f jqx, jqy, jqz, jqw;
        simd8f bqx, bqy, bqz, bqw;
        LoadJointQuats8(joints, jointIndex, jqx, jqy, jqz, jqw);
        LoadJointQuats8(blendJoints, jointIndex, bqx, bqy, bqz, bqw);
        NlerpJointQuats8(jqx, jqy, jqz, jqw, bqx, bqy, bqz, bqw, scaledLerp);
        StoreJointQuats8(joints, jointIndex, jqx, jqy, jqz, jqw);
    }

    if (i < numJoints) {
        SIMD_4::BlendJointsFast(joints, blendJoints, fraction, index + i, numJoints - i);
    }
}

BE_FORCE_INLINE void SIMD_8::AdditiveBlendJointTranslations2(float *translations0, float *translations1, const float *blendTranslations0, const float *blendTranslations1, const simd8f &lerp) {
    simd8f translations = set2x128_256ps(load_ps(translations0).m128, load_ps(translations1).m128);
    const simd8f blendTranslations = set2x128_256ps(load_ps(blendTranslations0).m128, load_ps(blendTranslations1).m128);
    translations = madd_256ps(blendTranslations, lerp, translations);
    store_ps(extract_256ps<0>(translations), translations0);
    store_ps(extract_256ps<1>(translations), translations1);
}

BE_FORCE_INLINE void SIMD_8::AdditiveBlendJointScales2(float *scales0, float *scales1, const float *blendScales0, const float *blendScales1, const simd8f &lerp) {
    const simd8f scales = set2x128_256ps(load_ps(scales0).m128, load_ps(scales1).m128);
    const simd8f blendScales = set2x128_256ps(load_ps(blendScales0).m128, load_ps(blendScales1).m128);
    const simd8f scaleFactor = madd_256ps(blendScales - SIMD_8::F8_one, lerp, SIMD_8::F8_one);
    const simd8f result = scales * scaleFactor;
    store_ps(extract_256ps<0>(result), scales0);
    store_ps(extract_256ps<1>(result), scales1);
}

void BE_FASTCALL SIMD_8::AdditiveBlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    assert_16_byte_aligned(joints);
    assert_16_byte_aligned(blendJoints);

    if (fraction <= 0.0f) {
        return;
    } else if (fraction >= 1.0f) {
        SIMD_Generic::AdditiveBlendJoints(joints, blendJoints, 1.0f, index, numJoints);
        return;
    }

    const simd8f lerp = set1_256ps(fraction);

    int i = 0;
    for (; i + 7 < numJoints; i += 8) {
        const int *jointIndex = index + i;
        const int n0 = jointIndex[0];
        const int n1 = jointIndex[1];
        const int n2 = jointIndex[2];
        const int n3 = jointIndex[3];
        const int n4 = jointIndex[4];
        const int n5 = jointIndex[5];
        const int n6 = jointIndex[6];
        const int n7 = jointIndex[7];

        // Blend translations.
        AdditiveBlendJointTranslations2(joints[n0].t, joints[n1].t, blendJoints[n0].t, blendJoints[n1].t, lerp);
        AdditiveBlendJointTranslations2(joints[n2].t, joints[n3].t, blendJoints[n2].t, blendJoints[n3].t, lerp);
        AdditiveBlendJointTranslations2(joints[n4].t, joints[n5].t, blendJoints[n4].t, blendJoints[n5].t, lerp);
        AdditiveBlendJointTranslations2(joints[n6].t, joints[n7].t, blendJoints[n6].t, blendJoints[n7].t, lerp);

        // Blend scales.
        AdditiveBlendJointScales2(joints[n0].s, joints[n1].s, blendJoints[n0].s, blendJoints[n1].s, lerp);
        AdditiveBlendJointScales2(joints[n2].s, joints[n3].s, blendJoints[n2].s, blendJoints[n3].s, lerp);
        AdditiveBlendJointScales2(joints[n4].s, joints[n5].s, blendJoints[n4].s, blendJoints[n5].s, lerp);
        AdditiveBlendJointScales2(joints[n6].s, joints[n7].s, blendJoints[n6].s, blendJoints[n7].s, lerp);

        simd8f jqx, jqy, jqz, jqw;
        simd8f bqx, bqy, bqz, bqw;
        LoadJointQuats8(joints, jointIndex, jqx, jqy, jqz, jqw);
        LoadJointQuats8(blendJoints, jointIndex, bqx, bqy, bqz, bqw);

        simd8f targetX = madd_256ps(bqx, jqw, bqy * jqz);
        targetX = nmadd_256ps(bqz, jqy, targetX);
        targetX = madd_256ps(bqw, jqx, targetX);

        simd8f targetY = nmadd_256ps(bqx, jqz, bqy * jqw);
        targetY = madd_256ps(bqz, jqx, targetY);
        targetY = madd_256ps(bqw, jqy, targetY);

        simd8f targetZ = madd_256ps(bqx, jqy, bqz * jqw);
        targetZ = nmadd_256ps(bqy, jqx, targetZ);
        targetZ = madd_256ps(bqw, jqz, targetZ);

        simd8f targetW = nmadd_256ps(bqx, jqx, bqw * jqw);
        targetW = nmadd_256ps(bqy, jqy, targetW);
        targetW = nmadd_256ps(bqz, jqz, targetW);

        SlerpJointQuats8(jqx, jqy, jqz, jqw, targetX, targetY, targetZ, targetW, lerp);
        StoreJointQuats8(joints, jointIndex, jqx, jqy, jqz, jqw);
    }

    if (i < numJoints) {
        SIMD_4::AdditiveBlendJoints(joints, blendJoints, fraction, index + i, numJoints - i);
    }
}

void BE_FASTCALL SIMD_8::TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    assert_16_byte_aligned(jointMats);

    simd8f pmab;
    simd4f pmc;
    int cachedJointIndex = -1;

    for (int joint = firstJoint; joint <= lastJoint; joint++) {
        const int parent = parents[joint];
        if (parent < 0) {
            continue;
        }

        const float *__restrict parentMatrix = jointMats->Ptr() + parent * 3 * 4;
        float *__restrict childMatrix = jointMats->Ptr() + joint * 3 * 4;

        if (parent != cachedJointIndex) {
            pmab = loadu_256ps(parentMatrix + 0);
            pmc = load_ps(parentMatrix + 8);
        }

        simd8f cmaa = broadcast_256ps(reinterpret_cast<const simd4f *>(childMatrix + 0));
        simd8f cmbb = broadcast_256ps(reinterpret_cast<const simd4f *>(childMatrix + 4));
        simd8f cmcc = broadcast_256ps(reinterpret_cast<const simd4f *>(childMatrix + 8));

        pmab = lincomb2x3x4(pmab, cmaa, cmbb, cmcc);
        pmc = lincomb3x4(pmc, extract_256ps<0>(cmaa), extract_256ps<0>(cmbb), extract_256ps<0>(cmcc));

        storeu_256ps(pmab, childMatrix + 0);
        store_ps(pmc, childMatrix + 8);

        cachedJointIndex = joint;
    }
}

void BE_FASTCALL SIMD_8::TransformJoints(const Mat3x4 *localJointMats, Mat3x4 *worldJointMats, const int *parents, const int firstJointIndex, const int lastJointIndex) {
    assert_16_byte_aligned(localJointMats);
    assert_16_byte_aligned(worldJointMats);

    for (int i = firstJointIndex; i <= lastJointIndex; i++) {
        const int parentIndex = parents[i];
        if (parentIndex < 0) {
            worldJointMats[i] = localJointMats[i];
            continue;
        }

        const float *__restrict parentWorldMatrix = worldJointMats->Ptr() + parentIndex * 3 * 4;
        const float *__restrict childLocalMatrix = localJointMats->Ptr() + i * 3 * 4;
        float *__restrict childWorldMatrix = worldJointMats->Ptr() + i * 3 * 4;

        simd8f pmab = loadu_256ps(parentWorldMatrix + 0);
        simd4f pmc = load_ps(parentWorldMatrix + 8);

        simd8f cmaa = broadcast_256ps(reinterpret_cast<const simd4f *>(childLocalMatrix + 0));
        simd8f cmbb = broadcast_256ps(reinterpret_cast<const simd4f *>(childLocalMatrix + 4));
        simd8f cmcc = broadcast_256ps(reinterpret_cast<const simd4f *>(childLocalMatrix + 8));

        pmab = lincomb2x3x4(pmab, cmaa, cmbb, cmcc);
        pmc = lincomb3x4(pmc, extract_256ps<0>(cmaa), extract_256ps<0>(cmbb), extract_256ps<0>(cmcc));

        storeu_256ps(pmab, childWorldMatrix + 0);
        store_ps(pmc, childWorldMatrix + 8);
    }
}

void BE_FASTCALL SIMD_8::UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    for (int joint = lastJoint; joint >= firstJoint; joint--) {
        assert(parents[joint] < joint);

        const int parent = parents[joint];
        if (parent < 0) {
            continue;
        }

        const float *__restrict parentMatrix = jointMats->Ptr() + parent * 3 * 4;
        float *__restrict childMatrix = jointMats->Ptr() + joint * 3 * 4;

        simd8f pmab = loadu_256ps(parentMatrix + 0);
        simd4f pmc = loadu_ps(parentMatrix + 8);

        simd8f cmab = loadu_256ps(childMatrix + 0);
        simd4f cmc = loadu_ps(childMatrix + 8);

        cmab = cmab - (pmab & SIMD_8::F8_mask_000x000x);
        cmc = cmc - (pmc & SIMD_4::F4_mask_000x);

        simd4f pma = extract_256ps<0>(pmab);
        simd4f pmb = extract_256ps<1>(pmab);
        simd4f cma = extract_256ps<0>(cmab);
        simd4f cmb = extract_256ps<1>(cmab);

        simd8f pa01 = set2x128_256ps(shuffle_ps<0, 0, 0, 0>(pma).m128, shuffle_ps<1, 1, 1, 1>(pma).m128);
        simd8f pb01 = set2x128_256ps(shuffle_ps<0, 0, 0, 0>(pmb).m128, shuffle_ps<1, 1, 1, 1>(pmb).m128);
        simd8f pc01 = set2x128_256ps(shuffle_ps<0, 0, 0, 0>(pmc).m128, shuffle_ps<1, 1, 1, 1>(pmc).m128);

        simd8f cmaa = set2x128_256ps(cma.m128, cma.m128);
        simd8f cmbb = set2x128_256ps(cmb.m128, cmb.m128);
        simd8f cmcc = set2x128_256ps(cmc.m128, cmc.m128);

        simd8f result01 = pa01 * cmaa;
        result01 = madd_256ps(pb01, cmbb, result01);
        result01 = madd_256ps(pc01, cmcc, result01);

        simd4f result2 = shuffle_ps<2, 2, 2, 2>(pma) * cma;
        result2 = madd_ps(shuffle_ps<2, 2, 2, 2>(pmb), cmb, result2);
        result2 = madd_ps(shuffle_ps<2, 2, 2, 2>(pmc), cmc, result2);

        storeu_256ps(result01, childMatrix + 0);
        storeu_ps(result2, childMatrix + 8);
    }
}

void BE_FASTCALL SIMD_8::MultiplyJoints(Mat3x4 *result, const Mat3x4 *joints1, const Mat3x4 *joints2, const int numJoints) {
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
        simd8f ar01 = loadu_256ps(&joint1Ptr[i * 12 + 0]);
        simd4f ar2 = loadu_ps(&joint1Ptr[i * 12 + 8]);

        simd8f br00 = broadcast_256ps(reinterpret_cast<const simd4f *>(&joint2Ptr[i * 12 + 0]));
        simd8f br11 = broadcast_256ps(reinterpret_cast<const simd4f *>(&joint2Ptr[i * 12 + 4]));
        simd8f br22 = broadcast_256ps(reinterpret_cast<const simd4f *>(&joint2Ptr[i * 12 + 8]));

        storeu_256ps(lincomb2x3x4(ar01, br00, br11, br22), &resultPtr[i * 12 + 0]);
        store_ps(lincomb3x4(ar2, extract_256ps<0>(br00), extract_256ps<0>(br11), extract_256ps<0>(br22)), &resultPtr[i * 12 + 8]);
    }
#endif
}

void SIMD_8::Memcpy64B(void *dst, const void *src, const int count) {
    assert_32_byte_aligned(src);
    assert_32_byte_aligned(dst);

    const byte *srcPtr = (byte *)src;
    byte *dstPtr = (byte *)dst;
    simd8i r0, r1;

    int c64 = count >> 6;
    while (c64 > 0) {
        if (c64 > 1) {
            // Loads one cache line (64B) of data from next 64B block of memory to CPU prefetch buffer.
            prefetchNTA(srcPtr + 64);
        }

        r0 = load_si256((const int32_t *)(srcPtr + 0));
        r1 = load_si256((const int32_t *)(srcPtr + 32));

        store_si256(r0, (int32_t *)(dstPtr + 0));
        store_si256(r1, (int32_t *)(dstPtr + 32));

        srcPtr += 64;
        dstPtr += 64;

        c64--;
    }
}

void SIMD_8::Memcpy2KB(void *dst, const void *src, const int count) {
    assert_32_byte_aligned(src);
    assert_32_byte_aligned(dst);

    const byte *srcPtr = (byte *)src;
    byte *dstPtr = (byte *)dst;
    simd8i r0, r1, r2, r3, r4, r5, r6, r7;

    int c256;
    int c2k = count >> 11;
    while (c2k > 0) {
        c256 = 8;
        while (c256 > 0) {
            // Loads two cache lines (64B x 4) of data from next 256B block of memory to CPU prefetch buffer.
            prefetchNTA(srcPtr + 256);
            prefetchNTA(srcPtr + 320);
            prefetchNTA(srcPtr + 384);
            prefetchNTA(srcPtr + 448);

            r0 = load_si256((const int32_t *)(srcPtr + 0));
            r1 = load_si256((const int32_t *)(srcPtr + 32));
            r2 = load_si256((const int32_t *)(srcPtr + 64));
            r3 = load_si256((const int32_t *)(srcPtr + 96));
            r4 = load_si256((const int32_t *)(srcPtr + 128));
            r5 = load_si256((const int32_t *)(srcPtr + 160));
            r6 = load_si256((const int32_t *)(srcPtr + 192));
            r7 = load_si256((const int32_t *)(srcPtr + 224));

            store_si256(r0, (int32_t *)(dstPtr + 0));
            store_si256(r1, (int32_t *)(dstPtr + 32));
            store_si256(r2, (int32_t *)(dstPtr + 64));
            store_si256(r3, (int32_t *)(dstPtr + 96));
            store_si256(r4, (int32_t *)(dstPtr + 128));
            store_si256(r5, (int32_t *)(dstPtr + 160));
            store_si256(r6, (int32_t *)(dstPtr + 192));
            store_si256(r7, (int32_t *)(dstPtr + 224));

            srcPtr += 256;
            dstPtr += 256;

            c256--;
        }

        c2k--;
    }
}

void BE_FASTCALL SIMD_8::Memcpy(void *dst, const void *src, const int size) {
#if 0
    memcpy(dst, src, size);
#else
    if (size > 4096 && !(((intptr_t)dst ^ (intptr_t)src) & 31)) {
        const byte *srcPtr = (const byte *)src;
        byte *dstPtr = (byte *)dst;

        // Copy up to the first 32 byte aligned boundary.
        int remainingSize = (32 - (((intptr_t)dstPtr) & 31)) & 31;
        if (remainingSize > 0) {
            memcpy(dstPtr, srcPtr, remainingSize);
            dstPtr += remainingSize;
            srcPtr += remainingSize;
        }

        remainingSize = size - remainingSize;

        // If there are multiple blocks of 2kB, ..
        if (remainingSize & ~4095) {
            SIMD_8::Memcpy2KB(dstPtr, srcPtr, remainingSize);
            srcPtr += (remainingSize & ~2047);
            dstPtr += (remainingSize & ~2047);
            remainingSize &= 2047;
        }

        // If there are blocks of 64 bytes, ..
        if (remainingSize & ~63) {
            SIMD_8::Memcpy64B(dstPtr, srcPtr, remainingSize);
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

void BE_FASTCALL SIMD_8::Memset(void *dest0, const int val, const int count0) {
#if 0
    memset(dest0, val, count0);
#else
    byte *dest = (byte *)dest0;
    int count = count0;

    while (count > 0 && (((intptr_t)dest) & 31)) {
        *dest = val;
        dest++;
        count--;
    }

    if (!count) {
        return;
    }

    simd8i data = set1_256epi8((char)val);

    if (count >= 512) {
        int c512 = count >> 9;
        while (c512 > 0) {
            store_si256(data, (int32_t *)dest);
            store_si256(data, (int32_t *)(dest + 32));
            store_si256(data, (int32_t *)(dest + 64));
            store_si256(data, (int32_t *)(dest + 96));
            store_si256(data, (int32_t *)(dest + 128));
            store_si256(data, (int32_t *)(dest + 160));
            store_si256(data, (int32_t *)(dest + 192));
            store_si256(data, (int32_t *)(dest + 224));
            store_si256(data, (int32_t *)(dest + 256));
            store_si256(data, (int32_t *)(dest + 288));
            store_si256(data, (int32_t *)(dest + 320));
            store_si256(data, (int32_t *)(dest + 352));
            store_si256(data, (int32_t *)(dest + 384));
            store_si256(data, (int32_t *)(dest + 416));
            store_si256(data, (int32_t *)(dest + 448));
            store_si256(data, (int32_t *)(dest + 480));

            dest += 512;
            c512--;
        }

        count &= 511;
    }

    if (count >= 256) {
        int c256 = count >> 8;
        while (c256 > 0) {
            store_si256(data, (int32_t *)dest);
            store_si256(data, (int32_t *)(dest + 32));
            store_si256(data, (int32_t *)(dest + 64));
            store_si256(data, (int32_t *)(dest + 96));
            store_si256(data, (int32_t *)(dest + 128));
            store_si256(data, (int32_t *)(dest + 160));
            store_si256(data, (int32_t *)(dest + 192));
            store_si256(data, (int32_t *)(dest + 224));

            dest += 256;
            c256--;
        }

        count &= 255;
    }

    if (count >= 128) {
        int c128 = count >> 7;
        while (c128 > 0) {
            store_si256(data, (int32_t *)dest);
            store_si256(data, (int32_t *)(dest + 32));
            store_si256(data, (int32_t *)(dest + 64));
            store_si256(data, (int32_t *)(dest + 96));

            dest += 128;
            c128--;
        }

        count &= 127;
    }

    if (count >= 64) {
        int c64 = count >> 6;
        while (c64 > 0) {
            store_si256(data, (int32_t *)dest);
            store_si256(data, (int32_t *)(dest + 32));

            dest += 64;
            c64--;
        }

        count &= 63;
    }

    if (count >= 32) {
        int c16 = count >> 5;
        while (c16 > 0) {
            store_si256(data, (int32_t *)dest);

            dest += 32;
            c16--;
        }

        count &= 31;
    }

    while (count > 0) {
        *dest = val;
        dest++;
        count--;
    }
#endif
}

BE_NAMESPACE_END

#endif // HAVE_X86_AVX_INTRIN
