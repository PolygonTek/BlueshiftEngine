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
    float *dst_ptr = dst;
    const float *src_ptr = src;

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

void BE_FASTCALL SIMD_4::Add(float *dst, const float *src0, const float *src1, const int count0) {
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

void BE_FASTCALL SIMD_4::Sub(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

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

void BE_FASTCALL SIMD_4::Sub(float *dst, const float *src0, const float *src1, const int count0) {
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

void BE_FASTCALL SIMD_4::Mul(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

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

void BE_FASTCALL SIMD_4::Mul(float *dst, const float *src0, const float *src1, const int count0) {
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

void BE_FASTCALL SIMD_4::Div(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

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

void BE_FASTCALL SIMD_4::Div(float *dst, const float *src0, const float *src1, const int count0) {
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

float BE_FASTCALL SIMD_4::Sum(const float *src, const int count0) {
    int count = count0;
    const float *src_ptr = src;
    float ret = 0;
    
    if (count > 4) {
        assert_16_byte_aligned(src);

        int c4 = count >> 2;
        while (c4 > 0) {
            ret += extract_ps<0>(sum_ps(load_ps(src_ptr)));
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

void BE_FASTCALL SIMD_4::TransposeMat4x4(float *dst, const float *src) {
    assert_16_byte_aligned(dst);
    assert_16_byte_aligned(src);

    simd4f r0 = load_ps(src + 0);
    simd4f r1 = load_ps(src + 4);
    simd4f r2 = load_ps(src + 8);
    simd4f r3 = load_ps(src + 12);

#if 0
    _MM_TRANSPOSE4_PS(r0, r1, r2, r3);
#else
    // Slightly faster than _MM_TRANSPOSE4_PS().
    transpose4x4(r0, r1, r2, r3);
#endif

    store_ps(r0, dst);
    store_ps(r1, dst + 4);
    store_ps(r2, dst + 8);
    store_ps(r3, dst + 12);
}

void BE_FASTCALL SIMD_4::MulMat3x4RM(float *dst, const float *src0, const float *src1) {
    assert_16_byte_aligned(dst);
    assert_16_byte_aligned(src0);
    assert_16_byte_aligned(src1);

    simd4f ar0 = load_ps(src0);
    simd4f ar1 = load_ps(src0 + 4);
    simd4f ar2 = load_ps(src0 + 8);

    simd4f br0 = load_ps(src1);
    simd4f br1 = load_ps(src1 + 4);
    simd4f br2 = load_ps(src1 + 8);

    simd4f cr0 = lincomb3x4(ar0, br0, br1, br2);
    simd4f cr1 = lincomb3x4(ar1, br0, br1, br2);
    simd4f cr2 = lincomb3x4(ar2, br0, br1, br2);

    store_ps(cr0, dst);
    store_ps(cr1, dst + 4);
    store_ps(cr2, dst + 8);
}

void BE_FASTCALL SIMD_4::MulMat4x4RM(float *dst, const float *src0, const float *src1) {
    assert_16_byte_aligned(dst);
    assert_16_byte_aligned(src0);
    assert_16_byte_aligned(src1);
    
    simd4f ar0 = load_ps(src0);
    simd4f ar1 = load_ps(src0 + 4);
    simd4f ar2 = load_ps(src0 + 8);
    simd4f ar3 = load_ps(src0 + 12);

    simd4f br0 = load_ps(src1);
    simd4f br1 = load_ps(src1 + 4);
    simd4f br2 = load_ps(src1 + 8);
    simd4f br3 = load_ps(src1 + 12);

    simd4f cr0 = lincomb4x4(ar0, br0, br1, br2, br3);
    simd4f cr1 = lincomb4x4(ar1, br0, br1, br2, br3);
    simd4f cr2 = lincomb4x4(ar2, br0, br1, br2, br3);
    simd4f cr3 = lincomb4x4(ar3, br0, br1, br2, br3);

    store_ps(cr0, dst);
    store_ps(cr1, dst + 4);
    store_ps(cr2, dst + 8);
    store_ps(cr3, dst + 12);
}

void BE_FASTCALL SIMD_4::MulMat4x4RMVec4(float *dst, const float *src0, const float *src1) {
    assert_16_byte_aligned(dst);
    assert_16_byte_aligned(src0);
    assert_16_byte_aligned(src1);

    simd4f v = load_ps(src1);

    simd4f result = mat4x4rowmajor_mul_vec4(src0, v);

    store_ps(result, dst);
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

    const simd4f vector_float_rsqrt_c0 = { -3.0f, -3.0f, -3.0f, -3.0f };
    const simd4f vector_float_rsqrt_c1 = { -0.5f, -0.5f, -0.5f, -0.5f };
    const simd4f vector_float_tiny = { 1e-10f, 1e-10f, 1e-10f, 1e-10f };
    const simd4f vector_float_half_pi = { Math::HalfPi, Math::HalfPi, Math::HalfPi, Math::HalfPi };

    const simd4f vector_float_sin_c0 = { -2.39e-08f, -2.39e-08f, -2.39e-08f, -2.39e-08f };
    const simd4f vector_float_sin_c1 = { 2.7526e-06f, 2.7526e-06f, 2.7526e-06f, 2.7526e-06f };
    const simd4f vector_float_sin_c2 = { -1.98409e-04f, -1.98409e-04f, -1.98409e-04f, -1.98409e-04f };
    const simd4f vector_float_sin_c3 = { 8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f, 8.3333315e-03f };
    const simd4f vector_float_sin_c4 = { -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f, -1.666666664e-01f };

    const simd4f vector_float_atan_c0 = { 0.0028662257f, 0.0028662257f, 0.0028662257f, 0.0028662257f };
    const simd4f vector_float_atan_c1 = { -0.0161657367f, -0.0161657367f, -0.0161657367f, -0.0161657367f };
    const simd4f vector_float_atan_c2 = { 0.0429096138f, 0.0429096138f, 0.0429096138f, 0.0429096138f };
    const simd4f vector_float_atan_c3 = { -0.0752896400f, -0.0752896400f, -0.0752896400f, -0.0752896400f };
    const simd4f vector_float_atan_c4 = { 0.1065626393f, 0.1065626393f, 0.1065626393f, 0.1065626393f };
    const simd4f vector_float_atan_c5 = { -0.1420889944f, -0.1420889944f, -0.1420889944f, -0.1420889944f };
    const simd4f vector_float_atan_c6 = { 0.1999355085f, 0.1999355085f, 0.1999355085f, 0.1999355085f };
    const simd4f vector_float_atan_c7 = { -0.3333314528f, -0.3333314528f, -0.3333314528f, -0.3333314528f };

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
        simd4f jqa_0 = load_ps(joints[n0].q);
        simd4f jqb_0 = load_ps(joints[n1].q);
        simd4f jqc_0 = load_ps(joints[n2].q);
        simd4f jqd_0 = load_ps(joints[n3].q);

        transpose4x4(jqa_0, jqb_0, jqc_0, jqd_0);

        simd4f bqa_0 = load_ps(blendJoints[n0].q);
        simd4f bqb_0 = load_ps(blendJoints[n1].q);
        simd4f bqc_0 = load_ps(blendJoints[n2].q);
        simd4f bqd_0 = load_ps(blendJoints[n3].q);

        transpose4x4(bqa_0, bqb_0, bqc_0, bqd_0);

        simd4f cosoma_0 = jqa_0 * bqa_0;
        simd4f cosomb_0 = jqb_0 * bqb_0;
        simd4f cosomc_0 = jqc_0 * bqc_0;
        simd4f cosomd_0 = jqd_0 * bqd_0;

        simd4f cosome_0 = cosoma_0 + cosomb_0;
        simd4f cosomf_0 = cosomc_0 + cosomd_0;
        simd4f cosomg_0 = cosome_0 + cosomf_0;

        simd4f sign_0 = cosomg_0 & SIMD_4::F4_sign_bit;
        simd4f cosom_0 = cosomg_0 ^ sign_0;
        simd4f ss_0 = nmadd_ps(cosom_0, cosom_0, SIMD_4::F4_one);

        ss_0 = max_ps(ss_0, vector_float_tiny);

        simd4f rs_0 = rsqrt12_ps(ss_0);
        simd4f sq_0 = rs_0 * rs_0;
        simd4f sh_0 = rs_0 * vector_float_rsqrt_c1;
        simd4f sx_0 = madd_ps(ss_0, sq_0, vector_float_rsqrt_c0);
        simd4f sinom_0 = sh_0 * sx_0;                       // sinom = sqrt( ss );

        ss_0 *= sinom_0;

        simd4f min_0 = min_ps(ss_0, cosom_0);
        simd4f max_0 = max_ps(ss_0, cosom_0);
        simd4f mask_0 = (min_0 == cosom_0);
        simd4f masksign_0 = mask_0 & SIMD_4::F4_sign_bit;
        simd4f maskPI_0 = mask_0 & vector_float_half_pi;

        simd4f rcpa_0 = rcp12_ps(max_0);
        simd4f rcpb_0 = max_0 * rcpa_0;
        simd4f rcpd_0 = rcpa_0 + rcpa_0;
        simd4f rcp_0 = nmadd_ps(rcpb_0, rcpa_0, rcpd_0);    // 1 / y or 1 / x
        simd4f ata_0 = min_0 * rcp_0;                       // x / y or y / x

        simd4f atb_0 = ata_0 ^ masksign_0;                  // -x / y or y / x
        simd4f atc_0 = atb_0 * atb_0;
        simd4f atd_0 = madd_ps(atc_0, vector_float_atan_c0, vector_float_atan_c1);

        atd_0 = madd_ps(atd_0, atc_0, vector_float_atan_c2);
        atd_0 = madd_ps(atd_0, atc_0, vector_float_atan_c3);
        atd_0 = madd_ps(atd_0, atc_0, vector_float_atan_c4);
        atd_0 = madd_ps(atd_0, atc_0, vector_float_atan_c5);
        atd_0 = madd_ps(atd_0, atc_0, vector_float_atan_c6);
        atd_0 = madd_ps(atd_0, atc_0, vector_float_atan_c7);
        atd_0 = madd_ps(atd_0, atc_0, SIMD_4::F4_one);

        simd4f omega_a_0 = madd_ps(atd_0, atb_0, maskPI_0);
        simd4f omega_b_0 = vlerp * omega_a_0;
        omega_a_0 -= omega_b_0;

        simd4f sinsa_0 = omega_a_0 * omega_a_0;
        simd4f sinsb_0 = omega_b_0 * omega_b_0;
        simd4f sina_0 = madd_ps(sinsa_0, vector_float_sin_c0, vector_float_sin_c1);
        simd4f sinb_0 = madd_ps(sinsb_0, vector_float_sin_c0, vector_float_sin_c1);
        sina_0 = madd_ps(sina_0, sinsa_0, vector_float_sin_c2);
        sinb_0 = madd_ps(sinb_0, sinsb_0, vector_float_sin_c2);
        sina_0 = madd_ps(sina_0, sinsa_0, vector_float_sin_c3);
        sinb_0 = madd_ps(sinb_0, sinsb_0, vector_float_sin_c3);
        sina_0 = madd_ps(sina_0, sinsa_0, vector_float_sin_c4);
        sinb_0 = madd_ps(sinb_0, sinsb_0, vector_float_sin_c4);
        sina_0 = madd_ps(sina_0, sinsa_0, SIMD_4::F4_one);
        sinb_0 = madd_ps(sinb_0, sinsb_0, SIMD_4::F4_one);
        sina_0 = sina_0 * omega_a_0;
        sinb_0 = sinb_0 * omega_b_0;
        simd4f scalea_0 = sina_0 * sinom_0;
        simd4f scaleb_0 = sinb_0 * sinom_0;

        scaleb_0 = scaleb_0 ^ sign_0;

        jqa_0 *= scalea_0;
        jqb_0 *= scalea_0;
        jqc_0 *= scalea_0;
        jqd_0 *= scalea_0;

        jqa_0 = madd_ps(bqa_0, scaleb_0, jqa_0);
        jqb_0 = madd_ps(bqb_0, scaleb_0, jqb_0);
        jqc_0 = madd_ps(bqc_0, scaleb_0, jqc_0);
        jqd_0 = madd_ps(bqd_0, scaleb_0, jqd_0);

        transpose4x4(jqa_0, jqb_0, jqc_0, jqd_0);

        store_ps(jqa_0, (float *)joints[n0].q);
        store_ps(jqb_0, (float *)joints[n1].q);
        store_ps(jqc_0, (float *)joints[n2].q);
        store_ps(jqd_0, (float *)joints[n3].q);
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

        signBit = (*(uint32_t *)&cosom) & (1 << 31);

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

    const simd4f vector_float_rsqrt_c0 = { -3.0f, -3.0f, -3.0f, -3.0f };
    const simd4f vector_float_rsqrt_c1 = { -0.5f, -0.5f, -0.5f, -0.5f };

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
        simd4f jqa_0 = load_ps(joints[n0].q);
        simd4f jqb_0 = load_ps(joints[n1].q);
        simd4f jqc_0 = load_ps(joints[n2].q);
        simd4f jqd_0 = load_ps(joints[n3].q);

        transpose4x4(jqa_0, jqb_0, jqc_0, jqd_0);

        simd4f bqa_0 = load_ps(blendJoints[n0].q);
        simd4f bqb_0 = load_ps(blendJoints[n1].q);
        simd4f bqc_0 = load_ps(blendJoints[n2].q);
        simd4f bqd_0 = load_ps(blendJoints[n3].q);

        transpose4x4(bqa_0, bqb_0, bqc_0, bqd_0);

        simd4f cosoma_0 = jqa_0 * bqa_0;
        simd4f cosomb_0 = jqb_0 * bqb_0;
        simd4f cosomc_0 = jqc_0 * bqc_0;
        simd4f cosomd_0 = jqd_0 * bqd_0;

        simd4f cosome_0 = cosoma_0 + cosomb_0;
        simd4f cosomf_0 = cosomc_0 + cosomd_0;
        simd4f cosom_0 = cosome_0 + cosomf_0;

        simd4f sign_0 = cosom_0 & SIMD_4::F4_sign_bit;

        simd4f scale_0 = vscaledLerp ^ sign_0;

        jqa_0 = madd_ps(scale_0, bqa_0, jqa_0);
        jqb_0 = madd_ps(scale_0, bqb_0, jqb_0);
        jqc_0 = madd_ps(scale_0, bqc_0, jqc_0);
        jqd_0 = madd_ps(scale_0, bqd_0, jqd_0);

        simd4f da_0 = jqa_0 * jqa_0;
        simd4f db_0 = jqb_0 * jqb_0;
        simd4f dc_0 = jqc_0 * jqc_0;
        simd4f dd_0 = jqd_0 * jqd_0;

        simd4f de_0 = da_0 + db_0;
        simd4f df_0 = dc_0 + dd_0;
        simd4f d_0 = de_0 + df_0;

        simd4f rs_0 = rsqrt12_ps(d_0);
        simd4f sq_0 = rs_0 * rs_0;
        simd4f sh_0 = rs_0 * vector_float_rsqrt_c1;
        simd4f sx_0 = madd_ps(d_0, sq_0, vector_float_rsqrt_c0);
        simd4f s_0 = sh_0 * sx_0;

        jqa_0 *= s_0;
        jqb_0 *= s_0;
        jqc_0 *= s_0;
        jqd_0 *= s_0;

        transpose4x4(jqa_0, jqb_0, jqc_0, jqd_0);

        store_ps(jqa_0, (float *)joints[n0].q);
        store_ps(jqb_0, (float *)joints[n1].q);
        store_ps(jqc_0, (float *)joints[n2].q);
        store_ps(jqd_0, (float *)joints[n3].q);
    }

    for (; i < numJoints; i++) {
        const int n = index[i];

        Vec3 &jointPos = joints[n].t;
        const Vec3 &blendPos = blendJoints[n].t;

        jointPos[0] += fraction * (blendPos[0] - jointPos[0]);
        jointPos[1] += fraction * (blendPos[1] - jointPos[1]);
        jointPos[2] += fraction * (blendPos[2] - jointPos[2]);

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
    // TODO: Implement this with SIMD code.
    int i;

    for (i = 0; i < numJoints; i++) {
        jointPoses[i].SetFromMat3x4(jointMats[i]);
    }
}

void BE_FASTCALL SIMD_4::TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    assert_16_byte_aligned(jointMats);

    const float *__restrict firstMatrix = jointMats->Ptr() + firstJoint * 3 * 4;

    simd4f pma = load_ps(firstMatrix + 0);
    simd4f pmb = load_ps(firstMatrix + 4);
    simd4f pmc = load_ps(firstMatrix + 8);

    for (int joint = firstJoint; joint <= lastJoint; joint++) {
        const int parent = parents[joint];
        if (parent < 0) {
            continue;
        }

        const float *__restrict parentMatrix = jointMats->Ptr() + parent * 3 * 4;
        float *__restrict childMatrix = jointMats->Ptr() + joint * 3 * 4;

        if (parent != joint - 1) {
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

        const float *__restrict parentMatrix = worldJointMats->Ptr() + parent * 3 * 4;
        float *__restrict childMatrix = worldJointMats->Ptr() + joint * 3 * 4;

        simd4f pma = load_ps(parentMatrix + 0);
        simd4f pmb = load_ps(parentMatrix + 4);
        simd4f pmc = load_ps(parentMatrix + 8);

        simd4f cma = load_ps(childMatrix + 0);
        simd4f cmb = load_ps(childMatrix + 4);
        simd4f cmc = load_ps(childMatrix + 8);

        pma = lincomb3x4(pma, cma, cmb, cmc);
        pmb = lincomb3x4(pmb, cma, cmb, cmc);
        pmc = lincomb3x4(pmc, cma, cmb, cmc);

        store_ps(pma, childMatrix + 0);
        store_ps(pmb, childMatrix + 4);
        store_ps(pmc, childMatrix + 8);
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

#if 1

static void SSE_Memcpy64B(void *dst, const void *src, const int count) {
    char *src_ptr = (char *)src;
    char *dst_ptr = (char *)dst;

    simd4i r0, r1, r2, r3;

    int c64 = count >> 6;
    while (c64 > 0) {
        prefetchNTA(src_ptr + 64);
        
        r0 = load_si128((int32_t *)src_ptr);
        r1 = load_si128((int32_t *)(src_ptr + 16));
        r2 = load_si128((int32_t *)(src_ptr + 32));
        r3 = load_si128((int32_t *)(src_ptr + 48));

        store_si128(r0, (int32_t *)dst_ptr);
        store_si128(r1, (int32_t *)(dst_ptr + 16));
        store_si128(r2, (int32_t *)(dst_ptr + 32));
        store_si128(r3, (int32_t *)(dst_ptr + 48));

        src_ptr += 64;
        dst_ptr += 64;

        c64--;
    }

    sfence();
}

static void SSE_Memcpy2kB(void *dst, const void *src, const int count) {
    char *src_ptr = (char *)src;
    char *dst_ptr = (char *)dst;

    simd4i r0, r1, r2, r3, r4, r5, r6, r7;

    int c128;
    int c2k = count >> 11;
    while (c2k > 0) {
        c128 = 16;
        while (c128 > 0) {
            prefetchNTA(src_ptr + 128);
            prefetchNTA(src_ptr + 192);
            
            r0 = load_si128((int32_t *)(src_ptr + 0));
            r1 = load_si128((int32_t *)(src_ptr + 16));
            r2 = load_si128((int32_t *)(src_ptr + 32));
            r3 = load_si128((int32_t *)(src_ptr + 48));
            r4 = load_si128((int32_t *)(src_ptr + 64));
            r5 = load_si128((int32_t *)(src_ptr + 80));
            r6 = load_si128((int32_t *)(src_ptr + 96));
            r7 = load_si128((int32_t *)(src_ptr + 112));

            store_si128(r0, (int32_t *)(dst_ptr + 0));
            store_si128(r1, (int32_t *)(dst_ptr + 16));
            store_si128(r2, (int32_t *)(dst_ptr + 32));
            store_si128(r3, (int32_t *)(dst_ptr + 48));
            store_si128(r4, (int32_t *)(dst_ptr + 64));
            store_si128(r5, (int32_t *)(dst_ptr + 80));
            store_si128(r6, (int32_t *)(dst_ptr + 96));
            store_si128(r7, (int32_t *)(dst_ptr + 112));
                    
            src_ptr += 128;
            dst_ptr += 128;

            c128--;
        }
                
        c2k--;
    }

    sfence();
}

static void SSE_MemcpyStream2kB(void *dst, const void *src, const int count) {
    char *tbuf = (char *)_alloca16(2048);
    char *src_ptr;
    char *src_next = (char *)src;
    char *dst_ptr = (char *)dst;
    char *dst_next;

    simd4i r0, r1, r2, r3, r4, r5, r6, r7;

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
        
            r0 = load_si128((int32_t *)src_ptr);
            r1 = load_si128((int32_t *)(src_ptr + 16));
            r2 = load_si128((int32_t *)(src_ptr + 32));
            r3 = load_si128((int32_t *)(src_ptr + 48));
            r4 = load_si128((int32_t *)(src_ptr + 64));
            r5 = load_si128((int32_t *)(src_ptr + 80));
            r6 = load_si128((int32_t *)(src_ptr + 96));
            r7 = load_si128((int32_t *)(src_ptr + 112));

            store_si128(r0, (int32_t *)dst_ptr);
            store_si128(r1, (int32_t *)(dst_ptr + 16));
            store_si128(r2, (int32_t *)(dst_ptr + 32));
            store_si128(r3, (int32_t *)(dst_ptr + 48));
            store_si128(r4, (int32_t *)(dst_ptr + 64));
            store_si128(r5, (int32_t *)(dst_ptr + 80));
            store_si128(r6, (int32_t *)(dst_ptr + 96));
            store_si128(r7, (int32_t *)(dst_ptr + 112));

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
            r0 = load_si128((int32_t *)src_ptr);
            r1 = load_si128((int32_t *)(src_ptr + 16));
            r2 = load_si128((int32_t *)(src_ptr + 32));
            r3 = load_si128((int32_t *)(src_ptr + 48));
            r4 = load_si128((int32_t *)(src_ptr + 64));
            r5 = load_si128((int32_t *)(src_ptr + 80));
            r6 = load_si128((int32_t *)(src_ptr + 96));
            r7 = load_si128((int32_t *)(src_ptr + 112));

            storent_si128(r0, (int32_t *)dst_ptr);
            storent_si128(r1, (int32_t *)(dst_ptr + 16));
            storent_si128(r2, (int32_t *)(dst_ptr + 32));
            storent_si128(r3, (int32_t *)(dst_ptr + 48));
            storent_si128(r4, (int32_t *)(dst_ptr + 64));
            storent_si128(r5, (int32_t *)(dst_ptr + 80));
            storent_si128(r6, (int32_t *)(dst_ptr + 96));
            storent_si128(r7, (int32_t *)(dst_ptr + 112));
        
            src_ptr += 128;
            dst_ptr += 128;

            c128--;
        }
                
        c2k--;
    }

    sfence();
}

// optimized memory copy routine that handles all alignment cases and block sizes efficiently
void BE_FASTCALL SIMD_4::Memcpy(void *dest0, const void *src0, const int count0) {
#if 1
    memcpy(dest0, src0, count0);
#else
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

#endif

BE_NAMESPACE_END

#endif
