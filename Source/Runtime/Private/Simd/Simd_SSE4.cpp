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
#include "Simd/Simd.h"

BE_NAMESPACE_BEGIN

#ifdef ENABLE_X86_SSE_INTRIN

void BE_FASTCALL SIMD_SSE4::Add(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        ssef c(constant);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src_ptr + 16);

            ssef x0 = c + ssef(_mm_load_ps(src_ptr + 0));
            ssef x1 = c + ssef(_mm_load_ps(src_ptr + 4));
            ssef x2 = c + ssef(_mm_load_ps(src_ptr + 8));
            ssef x3 = c + ssef(_mm_load_ps(src_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

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

void BE_FASTCALL SIMD_SSE4::Add(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

            ssef x0(_mm_load_ps(src0_ptr + 0));
            ssef x1(_mm_load_ps(src0_ptr + 4));
            ssef x2(_mm_load_ps(src0_ptr + 8));
            ssef x3(_mm_load_ps(src0_ptr + 12));

            x0 += ssef(_mm_load_ps(src1_ptr + 0));
            x1 += ssef(_mm_load_ps(src1_ptr + 4));
            x2 += ssef(_mm_load_ps(src1_ptr + 8));
            x3 += ssef(_mm_load_ps(src1_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

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

void BE_FASTCALL SIMD_SSE4::Sub(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        ssef c(constant);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);

            ssef x0 = c - ssef(_mm_load_ps(src_ptr + 0));
            ssef x1 = c - ssef(_mm_load_ps(src_ptr + 4));
            ssef x2 = c - ssef(_mm_load_ps(src_ptr + 8));
            ssef x3 = c - ssef(_mm_load_ps(src_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

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

void BE_FASTCALL SIMD_SSE4::Sub(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

            ssef x0(_mm_load_ps(src0_ptr + 0));
            ssef x1(_mm_load_ps(src0_ptr + 4));
            ssef x2(_mm_load_ps(src0_ptr + 8));
            ssef x3(_mm_load_ps(src0_ptr + 12));

            x0 -= ssef(_mm_load_ps(src1_ptr + 0));
            x1 -= ssef(_mm_load_ps(src1_ptr + 4));
            x2 -= ssef(_mm_load_ps(src1_ptr + 8));
            x3 -= ssef(_mm_load_ps(src1_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

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

void BE_FASTCALL SIMD_SSE4::Mul(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);

        ssef c(constant);
        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);

            ssef x0 = c * ssef(_mm_load_ps(src_ptr + 0));
            ssef x1 = c * ssef(_mm_load_ps(src_ptr + 4));
            ssef x2 = c * ssef(_mm_load_ps(src_ptr + 8));
            ssef x3 = c * ssef(_mm_load_ps(src_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

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

void BE_FASTCALL SIMD_SSE4::Mul(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

            ssef x0(_mm_load_ps(src0_ptr + 0));
            ssef x1(_mm_load_ps(src0_ptr + 4));
            ssef x2(_mm_load_ps(src0_ptr + 8));
            ssef x3(_mm_load_ps(src0_ptr + 12));

            x0 *= ssef(_mm_load_ps(src1_ptr + 0));
            x1 *= ssef(_mm_load_ps(src1_ptr + 4));
            x2 *= ssef(_mm_load_ps(src1_ptr + 8));
            x3 *= ssef(_mm_load_ps(src1_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

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

void BE_FASTCALL SIMD_SSE4::Div(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src);
        
        ssef c(constant);
        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);

            ssef x0 = c / ssef(_mm_load_ps(src_ptr + 0));
            ssef x1 = c / ssef(_mm_load_ps(src_ptr + 4));
            ssef x2 = c / ssef(_mm_load_ps(src_ptr + 8));
            ssef x3 = c / ssef(_mm_load_ps(src_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

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

void BE_FASTCALL SIMD_SSE4::Div(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 16) {
        assert_16_byte_aligned(dst);
        assert_16_byte_aligned(src0);
        assert_16_byte_aligned(src1);

        int c16 = count >> 4;
        while (c16 > 0) {
            //prefetchNTA(src0_ptr + 16);
            //prefetchNTA(src1_ptr + 16);

            ssef x0(_mm_load_ps(src0_ptr + 0));
            ssef x1(_mm_load_ps(src0_ptr + 4));
            ssef x2(_mm_load_ps(src0_ptr + 8));
            ssef x3(_mm_load_ps(src0_ptr + 12));

            x0 /= ssef(_mm_load_ps(src1_ptr + 0));
            x1 /= ssef(_mm_load_ps(src1_ptr + 4));
            x2 /= ssef(_mm_load_ps(src1_ptr + 8));
            x3 /= ssef(_mm_load_ps(src1_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

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

float BE_FASTCALL SIMD_SSE4::Sum(const float *src, const int count0) {
    int count = count0;
    const float *src_ptr = src;
    float ret = 0;
    
    if (count > 4) {
        assert_16_byte_aligned(src);

        int c4 = count >> 2;
        while (c4 > 0) {
            ret += reduce_add(ssef(_mm_load_ps(src_ptr)));
            src_ptr += 4;
            c4--;
        }

        count &= 3;
    }

    while (count > 0) {
        ret += *src_ptr++;
        count--;
    }

    return ret;
}

void BE_FASTCALL SIMD_SSE4::MatrixTranspose(float *dst, const float *src) {
    assert_16_byte_aligned(dst);
    assert_16_byte_aligned(src);

    ssef r0(_mm_load_ps(src + 0));
    ssef r1(_mm_load_ps(src + 4));
    ssef r2(_mm_load_ps(src + 8));
    ssef r3(_mm_load_ps(src + 12));

    ssef c0, c1, c2, c3;

    transpose(r0, r1, r2, r3, c0, c1, c2, c3);

    _mm_store_ps(dst, c0);
    _mm_store_ps(dst + 4, c1);
    _mm_store_ps(dst + 8, c2);
    _mm_store_ps(dst + 12, c3);
}

void BE_FASTCALL SIMD_SSE4::MatrixMultiply(float *dst, const float *src0, const float *src1) {
    assert_16_byte_aligned(dst);
    assert_16_byte_aligned(src0);
    assert_16_byte_aligned(src1);

    __m128 b0 = _mm_load_ps(src1);
    __m128 b1 = _mm_load_ps(src1 + 4);
    __m128 b2 = _mm_load_ps(src1 + 8);
    __m128 b3 = _mm_load_ps(src1 + 12);

    // Row 0
    __m128 a0 = _mm_load_ps(src0);
    __m128 a1 = a0;
    __m128 a2 = a0;
    __m128 a3 = a0;

    a0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0));
    a1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1));
    a2 = _mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2));
    a3 = _mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3));

    a0 = _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a2, b2));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a3, b3));
    _mm_store_ps(dst, a0);

    // Row 1
    a0 = _mm_load_ps(src0 + 4);
    a1 = a0;
    a2 = a0;
    a3 = a0;

    a0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0));
    a1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1));   
    a2 = _mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2));
    a3 = _mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3));

    a0 = _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a2, b2));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a3, b3));
    _mm_store_ps(dst + 4, a0);

    // Row 2
    a0 = _mm_load_ps(src0 + 8);
    a1 = a0;
    a2 = a0;
    a3 = a0;

    a0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0));
    a1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1));
    a2 = _mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2));
    a3 = _mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3));

    a0 = _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a2, b2));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a3, b3));
    _mm_store_ps(dst + 8, a0);

    // Row 3
    a0 = _mm_load_ps(src0 + 12);
    a1 = a0;
    a2 = a0;
    a3 = a0;

    a0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0));
    a1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1));
    a2 = _mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2));
    a3 = _mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3));

    a0 = _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a2, b2));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a3, b3));
    _mm_store_ps(dst + 12, a0);
}

void BE_FASTCALL SIMD_SSE4::TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    const __m128 vector_float_mask_keep_last = _mm_castsi128_ps(_mm_set_epi32(0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000));

    const float *__restrict firstMatrix = jointMats->Ptr() + (firstJoint + firstJoint + firstJoint - 3) * 4;

    __m128 pma = _mm_load_ps(firstMatrix + 0);
    __m128 pmb = _mm_load_ps(firstMatrix + 4);
    __m128 pmc = _mm_load_ps(firstMatrix + 8);

    for (int joint = firstJoint; joint <= lastJoint; joint++) {
        const int parent = parents[joint];
        const float *__restrict parentMatrix = jointMats->Ptr() + (parent + parent + parent) * 4;
        float *__restrict childMatrix = jointMats->Ptr() + (joint + joint + joint) * 4;

        if (parent != joint - 1) {
            pma = _mm_load_ps(parentMatrix + 0);
            pmb = _mm_load_ps(parentMatrix + 4);
            pmc = _mm_load_ps(parentMatrix + 8);
        }

        __m128 cma = _mm_load_ps(childMatrix + 0);
        __m128 cmb = _mm_load_ps(childMatrix + 4);
        __m128 cmc = _mm_load_ps(childMatrix + 8);

        __m128 ta = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pma), _MM_SHUFFLE(0, 0, 0, 0)));
        __m128 tb = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmb), _MM_SHUFFLE(0, 0, 0, 0)));
        __m128 tc = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmc), _MM_SHUFFLE(0, 0, 0, 0)));

        __m128 td = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pma), _MM_SHUFFLE(1, 1, 1, 1)));
        __m128 te = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmb), _MM_SHUFFLE(1, 1, 1, 1)));
        __m128 tf = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmc), _MM_SHUFFLE(1, 1, 1, 1)));

        __m128 tg = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pma), _MM_SHUFFLE(2, 2, 2, 2)));
        __m128 th = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmb), _MM_SHUFFLE(2, 2, 2, 2)));
        __m128 ti = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmc), _MM_SHUFFLE(2, 2, 2, 2)));

        pma = _mm_add_ps(_mm_mul_ps(ta, cma), _mm_and_ps(pma, vector_float_mask_keep_last));
        pmb = _mm_add_ps(_mm_mul_ps(tb, cma), _mm_and_ps(pmb, vector_float_mask_keep_last));
        pmc = _mm_add_ps(_mm_mul_ps(tc, cma), _mm_and_ps(pmc, vector_float_mask_keep_last));

        pma = _mm_add_ps(_mm_mul_ps(td, cmb), pma);
        pmb = _mm_add_ps(_mm_mul_ps(te, cmb), pmb);
        pmc = _mm_add_ps(_mm_mul_ps(tf, cmb), pmc);

        pma = _mm_add_ps(_mm_mul_ps(tg, cmc), pma);
        pmb = _mm_add_ps(_mm_mul_ps(th, cmc), pmb);
        pmc = _mm_add_ps(_mm_mul_ps(ti, cmc), pmc);

        _mm_store_ps(childMatrix + 0, pma);
        _mm_store_ps(childMatrix + 4, pmb);
        _mm_store_ps(childMatrix + 8, pmc);
    }
}

void BE_FASTCALL SIMD_SSE4::UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    const __m128 vector_float_mask_keep_last = _mm_castsi128_ps(_mm_set_epi32(0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000));

    for (int joint = lastJoint; joint >= firstJoint; joint--) {
        assert(parents[joint] < joint);
        const int parent = parents[joint];
        const float *__restrict parentMatrix = jointMats->Ptr() + (parent + parent + parent) * 4;
        float *__restrict childMatrix = jointMats->Ptr() + (joint + joint + joint) * 4;

        __m128 pma = _mm_load_ps(parentMatrix + 0);
        __m128 pmb = _mm_load_ps(parentMatrix + 4);
        __m128 pmc = _mm_load_ps(parentMatrix + 8);

        __m128 cma = _mm_load_ps(childMatrix + 0);
        __m128 cmb = _mm_load_ps(childMatrix + 4);
        __m128 cmc = _mm_load_ps(childMatrix + 8);

        __m128 ta = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pma), _MM_SHUFFLE(0, 0, 0, 0)));
        __m128 tb = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pma), _MM_SHUFFLE(1, 1, 1, 1)));
        __m128 tc = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pma), _MM_SHUFFLE(2, 2, 2, 2)));

        __m128 td = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmb), _MM_SHUFFLE(0, 0, 0, 0)));
        __m128 te = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmb), _MM_SHUFFLE(1, 1, 1, 1)));
        __m128 tf = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmb), _MM_SHUFFLE(2, 2, 2, 2)));

        __m128 tg = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmc), _MM_SHUFFLE(0, 0, 0, 0)));
        __m128 th = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmc), _MM_SHUFFLE(1, 1, 1, 1)));
        __m128 ti = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pmc), _MM_SHUFFLE(2, 2, 2, 2)));

        cma = _mm_sub_ps(cma, _mm_and_ps(pma, vector_float_mask_keep_last));
        cmb = _mm_sub_ps(cmb, _mm_and_ps(pmb, vector_float_mask_keep_last));
        cmc = _mm_sub_ps(cmc, _mm_and_ps(pmc, vector_float_mask_keep_last));

        pma = _mm_mul_ps(ta, cma);
        pmb = _mm_mul_ps(tb, cma);
        pmc = _mm_mul_ps(tc, cma);

        pma = _mm_add_ps(_mm_mul_ps(td, cmb), pma);
        pmb = _mm_add_ps(_mm_mul_ps(te, cmb), pmb);
        pmc = _mm_add_ps(_mm_mul_ps(tf, cmb), pmc);

        pma = _mm_add_ps(_mm_mul_ps(tg, cmc), pma);
        pmb = _mm_add_ps(_mm_mul_ps(th, cmc), pmb);
        pmc = _mm_add_ps(_mm_mul_ps(ti, cmc), pmc);

        _mm_store_ps(childMatrix + 0, pma);
        _mm_store_ps(childMatrix + 4, pmb);
        _mm_store_ps(childMatrix + 8, pmc);
    }
}

#if 0

static void SSE_Memcpy64B(void *dst, const void *src, const int count) {
    char *src_ptr = (char *)src;
    char *dst_ptr = (char *)dst;

    __m128i r0, r1, r2, r3;

    int c64 = count >> 6;
    while (c64 > 0) {
        //prefetchNTA(src_ptr + 64);
        
        r0 = _mm_load_si128((__m128i *)src_ptr);
        r1 = _mm_load_si128((__m128i *)(src_ptr + 16));
        r2 = _mm_load_si128((__m128i *)(src_ptr + 32));
        r3 = _mm_load_si128((__m128i *)(src_ptr + 48));

        _mm_store_si128((__m128i *)dst_ptr, r0);
        _mm_store_si128((__m128i *)(dst_ptr + 16), r1);
        _mm_store_si128((__m128i *)(dst_ptr + 32), r2);
        _mm_store_si128((__m128i *)(dst_ptr + 48), r3);

        src_ptr += 64;
        dst_ptr += 64;

        c64--;
    }

    //_mm_mfence();
}

static void SSE_Memcpy2kB(void *dst, const void *src, const int count) {
    char *src_ptr = (char *)src;
    char *dst_ptr = (char *)dst;

    __m128i r0, r1, r2, r3, r4, r5, r6, r7;

    int c128;
    int c2k = count >> 11;
    while (c2k > 0) {
        c128 = 16;
        while (c128 > 0) {
            //prefetchNTA(src_ptr + 128);
            //prefetchNTA(src_ptr + 192);
            
            r0 = _mm_load_si128((__m128i *)(src_ptr + 0));
            r1 = _mm_load_si128((__m128i *)(src_ptr + 16));
            r2 = _mm_load_si128((__m128i *)(src_ptr + 32));
            r3 = _mm_load_si128((__m128i *)(src_ptr + 48));
            r4 = _mm_load_si128((__m128i *)(src_ptr + 64));
            r5 = _mm_load_si128((__m128i *)(src_ptr + 80));
            r6 = _mm_load_si128((__m128i *)(src_ptr + 96));
            r7 = _mm_load_si128((__m128i *)(src_ptr + 112));

            _mm_store_si128((__m128i *)(dst_ptr + 0), r0);
            _mm_store_si128((__m128i *)(dst_ptr + 16), r1);
            _mm_store_si128((__m128i *)(dst_ptr + 32), r2);
            _mm_store_si128((__m128i *)(dst_ptr + 48), r3);
            _mm_store_si128((__m128i *)(dst_ptr + 64), r4);
            _mm_store_si128((__m128i *)(dst_ptr + 80), r5);
            _mm_store_si128((__m128i *)(dst_ptr + 96), r6);
            _mm_store_si128((__m128i *)(dst_ptr + 112), r7);
                    
            src_ptr += 128;
            dst_ptr += 128;

            c128--;
        }
                
        c2k--;
    }

    //_mm_mfence();
}

static void SSE_MemcpyStream2kB(void *dst, const void *src, const int count) {
    char *tbuf = (char *)_alloca16(2048);
    char *src_ptr;
    char *src_next = (char *)src;
    char *dst_ptr = (char *)dst;
    char *dst_next;

    __m128i r0, r1, r2, r3, r4, r5, r6, r7;

    int c128;
    int c2k = count >> 11;
    while (c2k > 0) {
        // copy 2k into temporary buffer
        dst_next = dst_ptr;
        src_ptr = src_next;
        dst_ptr = tbuf;

        c128 = 16;
        while (c128 > 0) {
            prefetchNTA(src_ptr + 128);
            prefetchNTA(src_ptr + 192);
        
            r0 = _mm_load_si128((__m128i *)src_ptr);
            r1 = _mm_load_si128((__m128i *)(src_ptr + 16));
            r2 = _mm_load_si128((__m128i *)(src_ptr + 32));
            r3 = _mm_load_si128((__m128i *)(src_ptr + 48));
            r4 = _mm_load_si128((__m128i *)(src_ptr + 64));
            r5 = _mm_load_si128((__m128i *)(src_ptr + 80));
            r6 = _mm_load_si128((__m128i *)(src_ptr + 96));
            r7 = _mm_load_si128((__m128i *)(src_ptr + 112));

            _mm_store_si128((__m128i *)dst_ptr, r0);
            _mm_store_si128((__m128i *)(dst_ptr + 16), r1);
            _mm_store_si128((__m128i *)(dst_ptr + 32), r2);
            _mm_store_si128((__m128i *)(dst_ptr + 48), r3);
            _mm_store_si128((__m128i *)(dst_ptr + 64), r4);
            _mm_store_si128((__m128i *)(dst_ptr + 80), r5);
            _mm_store_si128((__m128i *)(dst_ptr + 96), r6);
            _mm_store_si128((__m128i *)(dst_ptr + 112), r7);

            src_ptr += 128;
            dst_ptr += 128;

            c128--;
        }

        // Now copy from L1 to system memory
        src_next = src_ptr;
        src_ptr = tbuf;
        dst_ptr = dst_next;
    
        c128 = 16;
        while (c128 > 0) {
            r0 = _mm_load_si128((__m128i *)src_ptr);
            r1 = _mm_load_si128((__m128i *)(src_ptr + 16));
            r2 = _mm_load_si128((__m128i *)(src_ptr + 32));
            r3 = _mm_load_si128((__m128i *)(src_ptr + 48));
            r4 = _mm_load_si128((__m128i *)(src_ptr + 64));
            r5 = _mm_load_si128((__m128i *)(src_ptr + 80));
            r6 = _mm_load_si128((__m128i *)(src_ptr + 96));
            r7 = _mm_load_si128((__m128i *)(src_ptr + 112));

            _mm_stream_si128((__m128i *)dst_ptr, r0);
            _mm_stream_si128((__m128i *)(dst_ptr + 16), r1);
            _mm_stream_si128((__m128i *)(dst_ptr + 32), r2);
            _mm_stream_si128((__m128i *)(dst_ptr + 48), r3);
            _mm_stream_si128((__m128i *)(dst_ptr + 64), r4);
            _mm_stream_si128((__m128i *)(dst_ptr + 80), r5);
            _mm_stream_si128((__m128i *)(dst_ptr + 96), r6);
            _mm_stream_si128((__m128i *)(dst_ptr + 112), r7);
        
            src_ptr += 128;
            dst_ptr += 128;

            c128--;
        }
                
        c2k--;
    }

    _mm_mfence();
}

// optimized memory copy routine that handles all alignment cases and block sizes efficiently
void BE_FASTCALL SIMD_SSE4::Memcpy(void *dest0, const void *src0, const int count0) {
    if (count0 > 4096 && !(((intptr_t)dest0 ^ (intptr_t)src0) & 15)) {
        byte *dest = (byte *)dest0;
        byte *src = (byte *)src0;

        // copy up to the first 16 byte aligned boundary
        int count = ((intptr_t)dest) & 15;
        if (count > 0) {
            memcpy(dest, src, count);
            dest += count;
            src += count;
        }

        count = count0 - count;

        // if there are multiple blocks of 2kB
        if (count & ~4095) {
            SSE_Memcpy2kB(dest, src, count);
            src += (count & ~2047);
            dest += (count & ~2047);
            count &= 2047;
        }

        // if there are blocks of 64 bytes
        if (count & ~63) {
            SSE_Memcpy64B(dest, src, count);
            src += (count & ~63);
            dest += (count & ~63);
            count &= 63;
        }

        // copy any remaining bytes
        memcpy(dest, src, count);
    } else {
        // use the regular one if we cannot copy 16 byte aligned
        memcpy(dest0, src0, count0);
    }
}

void BE_FASTCALL SIMD_SSE4::Memset(void *dest0, const int val, const int count0) {
    byte *dest = (byte *)dest0;
    int	count = count0;

    while (count > 0 && (((intptr_t)dest) & 15)) {
        *dest = val;
        dest++;
        count--;
    }

    if (!count) {
        return;
    }

    __m128i data = _mm_set1_epi8((char)val);

    if (count >= 128) {
        int c128 = count >> 7;
        while (c128 > 0) {
            _mm_store_si128((__m128i *)dest, data);
            _mm_store_si128((__m128i *)(dest + 16), data);
            _mm_store_si128((__m128i *)(dest + 32), data);
            _mm_store_si128((__m128i *)(dest + 48), data);
            _mm_store_si128((__m128i *)(dest + 64), data);
            _mm_store_si128((__m128i *)(dest + 80), data);
            _mm_store_si128((__m128i *)(dest + 96), data);
            _mm_store_si128((__m128i *)(dest + 112), data);

            dest += 128;
            c128--;
        }

        count &= 127;
    }

    if (count >= 16) {
        int c16 = count >> 4;
        while (c16 > 0) {
            _mm_store_si128((__m128i *)dest, data);

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
}

#endif

#endif // ENABLE_X86_SSE_INTRIN

BE_NAMESPACE_END
