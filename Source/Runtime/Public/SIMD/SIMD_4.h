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

#pragma once

#if defined(HAVE_X86_SSE_INTRIN)
#include "SSE/sse.h"
#elif defined(HAVE_ARM_NEON_INTRIN)
#include "NEON/neon.h"
#else
#error "No SIMD4 intrinsics"
#endif

BE_NAMESPACE_BEGIN

class SIMD_4 : public SIMD_Generic {
public:
    SIMD_4() = default;
    SIMD_4(CpuId cpuid) { this->cpuid = cpuid; }

    virtual const char * BE_FASTCALL    GetName() const override { return "SIMD 4"; }

    virtual void BE_FASTCALL            Add(float *dst, const float constant, const float *src, const int count) override;
    virtual void BE_FASTCALL            Add(float *dst, const float *src0, const float *src1, const int count) override;
    virtual void BE_FASTCALL            Sub(float *dst, const float constant, const float *src, const int count) override;
    virtual void BE_FASTCALL            Sub(float *dst, const float *src0, const float *src1, const int count) override;
    virtual void BE_FASTCALL            Mul(float *dst, const float constant, const float *src, const int count) override;
    virtual void BE_FASTCALL            Mul(float *dst, const float *src0, const float *src1, const int count) override;
    virtual void BE_FASTCALL            Div(float *dst, const float constant, const float *src, const int count) override;
    virtual void BE_FASTCALL            Div(float *dst, const float *src0, const float *src1, const int count) override;

    virtual float BE_FASTCALL           Sum(const float *src, const int count) override;

    virtual void BE_FASTCALL            TransposeMat4x4(float *dst, const float *src) override;

    virtual void BE_FASTCALL            MulMat3x4RM(float *dst, const float *src0, const float *src1) override;
    virtual void BE_FASTCALL            MulMat4x4RM(float *dst, const float *src0, const float *src1) override;
    virtual void BE_FASTCALL            MulMat4x4RMVec4(float *dst, const float *src0, const float *src1) override;

    virtual void BE_FASTCALL            Memcpy(void *dst, const void *src, const int count) override;
    virtual void BE_FASTCALL            Memset(void *dst, const int val, const int count) override;

    virtual void BE_FASTCALL            BlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) override;
    virtual void BE_FASTCALL            BlendJointsFast(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) override;
    virtual void BE_FASTCALL            ConvertJointPosesToJointMats(Mat3x4 *jointMats, const JointPose *jointPoses, const int numJoints) override;
    virtual void BE_FASTCALL            ConvertJointMatsToJointPoses(JointPose *jointPoses, const Mat3x4 *jointMats, const int numJoints) override;
    virtual void BE_FASTCALL            TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) override;
    virtual void BE_FASTCALL            UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) override;
    virtual void BE_FASTCALL            MultiplyJoints(Mat3x4 *result, const Mat3x4 *joints1, const Mat3x4 *joints2, const int numJoints) override;

    static const simd4f                 F4_zero;
    static const simd4f                 F4_one;
    static const simd4f                 F4_half;
    static const simd4f                 F4_255;
    static const simd4f                 F4_min_char;
    static const simd4f                 F4_max_char;
    static const simd4f                 F4_min_short;
    static const simd4f                 F4_max_short;
    static const simd4f                 F4_tiny;
    static const simd4f                 F4_smallestNonDenorm;
    static const simd4f                 F4_sign_bit;
    static const simd4f                 F4_mask_000x;
};

// Cross product.
BE_FORCE_INLINE simd4f cross_ps(const simd4f &a, const simd4f &b) {
    simd4f a_yzxw = shuffle_ps<1, 2, 0, 3>(a); // (a.y, a.z, a.x, a.w)
    simd4f b_yzxw = shuffle_ps<1, 2, 0, 3>(b); // (b.y, b.z, b.x, b.w)
    simd4f ab_yzxw = a_yzxw * b; // (a.y * b.x, a.z * b.y, a.x * b.z, a.w * b.w)

    return shuffle_ps<1, 2, 0, 3>(msub_ps(b_yzxw, a, ab_yzxw)); // (a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, 0)
}

// Linear combination.
// r0 = a[0] * b0 + a[1] * b1 + a[2] * b2 + a[3] * b3
BE_FORCE_INLINE simd4f lincomb4x4(const simd4f &a, const simd4f &br0, const simd4f &br1, const simd4f &br2, const simd4f &br3) {
    simd4f result = shuffle_ps<0, 0, 0, 0>(a) * br0;
    result = madd_ps(shuffle_ps<1, 1, 1, 1>(a), br1, result);
    result = madd_ps(shuffle_ps<2, 2, 2, 2>(a), br2, result);
    result = madd_ps(shuffle_ps<3, 3, 3, 3>(a), br3, result);
    return result;
}

// Linear combination.
// r0 = a[0] * b0 + a[1] * b1 + a[2] * b2 + a[3] * (0, 0, 0, 1)
BE_FORCE_INLINE simd4f lincomb3x4(const simd4f &a, const simd4f &br0, const simd4f &br1, const simd4f &br2) {
    simd4f result = shuffle_ps<0, 0, 0, 0>(a) * br0;
    result = madd_ps(shuffle_ps<1, 1, 1, 1>(a), br1, result);
    result = madd_ps(shuffle_ps<2, 2, 2, 2>(a), br2, result);
    result += (a & SIMD_4::F4_mask_000x);
    return result;
}

// M(3x4) * v(4)
BE_FORCE_INLINE simd4f mat3x4rowmajor_mul_vec4(const float *mat, const simd4f &v) {
    assert_16_byte_aligned(mat);
    simd4f ar0 = load_ps(mat);
    simd4f ar1 = load_ps(mat + 4);
    simd4f ar2 = load_ps(mat + 8);
    simd4f x = ar0 * v;
    simd4f y = ar1 * v;
    simd4f z = ar2 * v;
    simd4f w = (v & SIMD_4::F4_mask_000x);
    simd4f tmp1 = hadd_ps(x, y); // x0+x1, x2+x3, y0+y1, y2+y3
    simd4f tmp2 = hadd_ps(z, w); // z0+z1, z2+z3, w0+w1, w2+w3
    return hadd_ps(tmp1, tmp2); // x0+x1+x2+x3, y0+y1+y2+y3, z0+z1+z2+z3, w0+w1+w2+w3
}

// M(4x4) * v(4)
BE_FORCE_INLINE simd4f mat4x4colmajor_mul_vec4(const float *mat, const simd4f &v) {
    assert_16_byte_aligned(mat);
    simd4f ac0 = load_ps(mat);
    simd4f ac1 = load_ps(mat + 4);
    simd4f ac2 = load_ps(mat + 8);
    simd4f ac3 = load_ps(mat + 12);
    simd4f result = ac0 * shuffle_ps<0, 0, 0, 0>(v);
    result = madd_ps(ac1, shuffle_ps<1, 1, 1, 1>(v), result);
    result = madd_ps(ac2, shuffle_ps<2, 2, 2, 2>(v), result);
    return madd_ps(ac3, shuffle_ps<3, 3, 3, 3>(v), result);
}

BE_NAMESPACE_END
