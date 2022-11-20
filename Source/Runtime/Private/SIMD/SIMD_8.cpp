// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
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

    if (count > 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src);

        simd8f c = set1_256ps(constant);

        int c32 = count >> 5;
        while (c32 > 0) {
            //prefetchNTA(src_ptr + 16);

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

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src_ptr + 16);

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

    if (count > 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src0);
        assert_32_byte_aligned(src1);

        int c32 = count >> 5;
        while (c32 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

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

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

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

    if (count > 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src);

        simd8f c = set1_256ps(constant);

        int c32 = count >> 5;
        while (c32 > 0) {
            //prefetchNTA(src0_ptr + 16);

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

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);

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
        *dst_ptr++ = constant + *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Sub(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src0);
        assert_32_byte_aligned(src1);

        int c32 = count >> 5;
        while (c32 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

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

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

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
        *dst_ptr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Mul(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src);

        simd8f c = set1_256ps(constant);

        int c32 = count >> 5;
        while (c32 > 0) {
            //prefetchNTA(src0_ptr + 16);

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

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);

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
        *dst_ptr++ = constant + *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Mul(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src0);
        assert_32_byte_aligned(src1);

        int c32 = count >> 5;
        while (c32 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

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

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

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
        *dst_ptr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Div(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src);

        simd8f c = set1_256ps(constant);

        int c32 = count >> 5;
        while (c32 > 0) {
            //prefetchNTA(src0_ptr + 16);

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

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        simd4f c = set1_ps(constant);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);

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
        *dst_ptr++ = constant + *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::Div(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 32) {
        assert_32_byte_aligned(dst);
        assert_32_byte_aligned(src0);
        assert_32_byte_aligned(src1);

        int c32 = count >> 5;
        while (c32 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

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

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

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
        *dst_ptr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_8::MulMat3x4RM(float *dst, const float *src0, const float *src1) {
    assert_32_byte_aligned(dst);
    assert_32_byte_aligned(src0);
    assert_32_byte_aligned(src1);

    simd8f ar01 = load_256ps(src0);
    simd4f ar2 = load_ps(src0 + 8);

    simd8f br00 = broadcast_256ps((simd4f *)src1);
    simd8f br11 = broadcast_256ps((simd4f *)(src1 + 4));
    simd8f br22 = broadcast_256ps((simd4f *)(src1 + 8));

    store_256ps(lincomb2x3x4(ar01, br00, br11, br22), dst);
    store_ps(lincomb3x4(ar2, extract_256ps<0>(br00), extract_256ps<0>(br11), extract_256ps<0>(br22)), dst + 8);
}

void BE_FASTCALL SIMD_8::MulMat4x4RM(float *dst, const float *src0, const float *src1) {
    assert_32_byte_aligned(dst);
    assert_32_byte_aligned(src0);
    assert_32_byte_aligned(src1);

    simd8f ar01 = load_256ps(src0);
    simd8f ar23 = load_256ps(src0 + 8);

    simd8f br00 = broadcast_256ps((simd4f *)src1);
    simd8f br11 = broadcast_256ps((simd4f *)(src1 + 4));
    simd8f br22 = broadcast_256ps((simd4f *)(src1 + 8));
    simd8f br33 = broadcast_256ps((simd4f *)(src1 + 12));

    store_256ps(lincomb2x4x4(ar01, br00, br11, br22, br33), dst);
    store_256ps(lincomb2x4x4(ar23, br00, br11, br22, br33), dst + 8);
}

void BE_FASTCALL SIMD_8::Memset(void *dest0, const int val, const int count0) {
#if 1
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

void BE_FASTCALL SIMD_8::TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    assert_16_byte_aligned(jointMats);

    const float *__restrict firstMatrix = jointMats->Ptr() + firstJoint * 3 * 4;

    simd8f pmab = loadu_256ps(firstMatrix + 0);
    simd4f pmc = load_ps(firstMatrix + 8);

    for (int joint = firstJoint; joint <= lastJoint; joint++) {
        const int parent = parents[joint];
        if (parent < 0) {
            continue;
        }

        const float *__restrict parentMatrix = jointMats->Ptr() + parent * 3 * 4;
        float *__restrict childMatrix = jointMats->Ptr() + joint * 3 * 4;

        if (parent != joint - 1) {
            pmab = loadu_256ps(parentMatrix + 0);
            pmc = load_ps(parentMatrix + 8);
        }

        simd8f cmaa = broadcast_256ps((simd4f *)(childMatrix + 0));
        simd8f cmbb = broadcast_256ps((simd4f *)(childMatrix + 4));
        simd8f cmcc = broadcast_256ps((simd4f *)(childMatrix + 8));

        pmab = lincomb2x3x4(pmab, cmaa, cmbb, cmcc);
        pmc = lincomb3x4(pmc, extract_256ps<0>(cmaa), extract_256ps<0>(cmbb), extract_256ps<0>(cmcc));

        storeu_256ps(pmab, childMatrix + 0);
        store_ps(pmc, childMatrix + 8);
    }
}

void BE_FASTCALL SIMD_8::TransformJoints(const Mat3x4 *localJointMats, Mat3x4 *worldJointMats, const int *parents, const int firstJointIndex, const int lastJointIndex) {
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

        simd8f cmaa = broadcast_256ps((simd4f *)(childLocalMatrix + 0));
        simd8f cmbb = broadcast_256ps((simd4f *)(childLocalMatrix + 4));
        simd8f cmcc = broadcast_256ps((simd4f *)(childLocalMatrix + 8));

        pmab = lincomb2x3x4(pmab, cmaa, cmbb, cmcc);
        pmc = lincomb3x4(pmc, extract_256ps<0>(cmaa), extract_256ps<0>(cmbb), extract_256ps<0>(cmcc));

        storeu_256ps(pmab, childWorldMatrix + 0);
        store_ps(pmc, childWorldMatrix + 8);
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

        simd8f br00 = broadcast_256ps((simd4f *)&joint2Ptr[i * 12 + 0]);
        simd8f br11 = broadcast_256ps((simd4f *)&joint2Ptr[i * 12 + 4]);
        simd8f br22 = broadcast_256ps((simd4f *)&joint2Ptr[i * 12 + 8]);

        storeu_256ps(lincomb2x3x4(ar01, br00, br11, br22), &resultPtr[i * 12 + 0]);
        store_ps(lincomb3x4(ar2, extract_256ps<0>(br00), extract_256ps<0>(br11), extract_256ps<0>(br22)), &resultPtr[i * 12 + 8]);
    }
#endif
}

BE_NAMESPACE_END

#endif // HAVE_X86_AVX_INTRIN
