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

#if defined(ENABLE_X86_AVX_INTRINSICS)
#include "AVX/avx.h"
#else
#error "No SIMD8 intrinsics"
#endif

BE_NAMESPACE_BEGIN

// Dual linear combination.
// r0 = a[0] * b0 + a[1] * b1 + a[2] * b2 + a[3] * b3
// r1 = a[4] * b0 + a[5] * b1 + a[6] * b2 + a[7] * b3
BE_FORCE_INLINE simd8f lincomb4x8(simd8f a, const simd8f &br00, const simd8f &br11, const simd8f &br22, const simd8f &br33) {
    simd8f result = shuffle_256ps<0, 0, 0, 0>(a) * br00;
    result += shuffle_256ps<1, 1, 1, 1>(a) * br11;
    result += shuffle_256ps<2, 2, 2, 2>(a) * br22;
    result += shuffle_256ps<3, 3, 3, 3>(a) * br33;
    return result;
}

// Transposes two 4x4 matrices.
BE_FORCE_INLINE void mat4x4_transpose2(const simd8f &r0, const simd8f &r1, const simd8f &r2, const simd8f &r3, simd8f &c0, simd8f &c1, simd8f &c2, simd8f &c3) {
    simd8f l02 = unpacklo_256ps(r0, r2); // m00, m20, m01, m21, m04, m24, m05, m25
    simd8f h02 = unpackhi_256ps(r0, r2); // m02, m22, m03, m23, m06, m26, m07, m27
    simd8f l13 = unpacklo_256ps(r1, r3); // m10, m30, m11, m31, m14, m34, m15, m35
    simd8f h13 = unpackhi_256ps(r1, r3); // m12, m32, m13, m33, m16, m36, m17, m37
    c0 = unpacklo_256ps(l02, l13); // m00, m10, m20, m30, m04, m14, m24, m34
    c1 = unpackhi_256ps(l02, l13); // m01, m11, m21, m31, m05, m15, m25, m35
    c2 = unpacklo_256ps(h02, h13); // m02, m12, m22, m32, m06, m16, m26, m36
    c3 = unpackhi_256ps(h02, h13); // m03, m13, m23, m33, m07, m17, m27, m37
}

// Transposes 8x8 matrix.
BE_FORCE_INLINE void mat8x8_transpose(const simd8f &r0, const simd8f &r1, const simd8f &r2, const simd8f &r3, const simd8f &r4, const simd8f &r5, const simd8f &r6, const simd8f &r7,
    simd8f &c0, simd8f &c1, simd8f &c2, simd8f &c3, simd8f &c4, simd8f &c5, simd8f &c6, simd8f &c7) {
    simd8f h0, h1, h2, h3;
    mat4x4_transpose2(r0, r1, r2, r3, h0, h1, h2, h3);
    simd8f h4, h5, h6, h7;
    mat4x4_transpose2(r4, r5, r6, r7, h4, h5, h6, h7);
    c0 = shuffle_256ps<0, 2>(h0, h4);
    c1 = shuffle_256ps<0, 2>(h1, h5);
    c2 = shuffle_256ps<0, 2>(h2, h6);
    c3 = shuffle_256ps<0, 2>(h3, h7);
    c4 = shuffle_256ps<1, 3>(h0, h4);
    c5 = shuffle_256ps<1, 3>(h1, h5);
    c6 = shuffle_256ps<1, 3>(h2, h6);
    c7 = shuffle_256ps<1, 3>(h3, h7);
    simd8f l02 = unpacklo_256ps(r0, r2);
    simd8f h02 = unpackhi_256ps(r0, r2);
    simd8f l13 = unpacklo_256ps(r1, r3);
    simd8f h13 = unpackhi_256ps(r1, r3);
    c0 = unpacklo_256ps(l02, l13);
    c1 = unpackhi_256ps(l02, l13);
    c2 = unpacklo_256ps(h02, h13);
}

class SIMD_8 : public SIMD_4 {
public:
    SIMD_8() = default;
    SIMD_8(CpuId cpuid) { this->cpuid = cpuid; }

    virtual const char * BE_FASTCALL    GetName() const { return "SIMD 8"; }

    virtual void BE_FASTCALL            Matrix4x4Multiply(float *dst, const float *src0, const float *src1);

    static const simd8f                 F8_zero;
    static const simd8f                 F8_one;
    static const simd8f                 F8_half;
    static const simd8f                 F8_255;
    static const simd8f                 F8_min_char;
    static const simd8f                 F8_max_char;
    static const simd8f                 F8_min_short;
    static const simd8f                 F8_max_short;
    static const simd8f                 F8_tiny;
    static const simd8f                 F8_smallestNonDenorm;
    static const simd8f                 F8_sign_bit;
};

BE_NAMESPACE_END
