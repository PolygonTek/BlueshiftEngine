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

#include "Platform/Intrinsics.h"

struct ssef {
    union {
        __m128      m128;
        __m128i     m128i;
    };

    BE_FORCE_INLINE ssef() = default;
    BE_FORCE_INLINE ssef(const ssef &other) { m128 = other.m128; }
    BE_FORCE_INLINE ssef &operator=(const ssef &other) { m128 = other.m128; return *this; }

    BE_FORCE_INLINE ssef(const __m128 a) { m128 = a; }

    BE_FORCE_INLINE explicit ssef(const __m128i a) { m128 = _mm_cvtepi32_ps(a); }
    BE_FORCE_INLINE explicit ssef(const float *a) { m128 = _mm_loadu_ps(a); }
    BE_FORCE_INLINE explicit ssef(float a) { m128 = _mm_set1_ps(a); }
    BE_FORCE_INLINE explicit ssef(float a, float b, float c, float d) { m128 = _mm_set_ps(d, c, b, a); }

    BE_FORCE_INLINE void setZero() { m128 = _mm_setzero_ps(); }
    BE_FORCE_INLINE void setOne() { m128 = _mm_set1_ps(1.0f); }

    BE_FORCE_INLINE operator const __m128 &() const { return m128; };
};

//-------------------------------------------------------------
// Unary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const ssef operator+(const ssef &a) { return a; }
BE_FORCE_INLINE const ssef operator-(const ssef &a) { return _mm_xor_ps(a.m128, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))); }
BE_FORCE_INLINE const ssef abs(const ssef &a) { return _mm_and_ps(a.m128, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))); }
BE_FORCE_INLINE const ssef rcp(const ssef &a) { return _mm_rcp_ps(a.m128); }
BE_FORCE_INLINE const ssef rcp_nr(const ssef &a) { 
    const ssef r = _mm_rcp_ps(a.m128);
    // Newton-Raphson approximation
    return _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(r, r), a));
}
BE_FORCE_INLINE const ssef sqr(const ssef &a) { return _mm_mul_ps(a, a); }
BE_FORCE_INLINE const ssef sqrt(const ssef &a) { return _mm_sqrt_ps(a.m128); }
BE_FORCE_INLINE const ssef rsqrt(const ssef &a) { return _mm_rsqrt_ps(a.m128); }
BE_FORCE_INLINE const ssef rsqrt_nr(const ssef &a) {
    const ssef r = _mm_rsqrt_ps(a.m128);
    // Newton-Raphson approximation
    return _mm_add_ps(_mm_mul_ps(_mm_set1_ps(1.5f), r), 
        _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a, _mm_set1_ps(-0.5f)), r), _mm_mul_ps(r, r)));
}

//-------------------------------------------------------------
// Binary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const ssef operator+(const ssef &a, const ssef &b) { return _mm_add_ps(a.m128, b.m128); }
BE_FORCE_INLINE const ssef operator+(const ssef &a, const float &b) { return a + ssef(b); }
BE_FORCE_INLINE const ssef operator+(const float &a, const ssef &b) { return ssef(a) + b; }

BE_FORCE_INLINE const ssef operator-(const ssef &a, const ssef &b) { return _mm_sub_ps(a.m128, b.m128); }
BE_FORCE_INLINE const ssef operator-(const ssef &a, const float &b) { return a - ssef(b); }
BE_FORCE_INLINE const ssef operator-(const float &a, const ssef &b) { return ssef(a) - b; }

BE_FORCE_INLINE const ssef operator*(const ssef &a, const ssef &b) { return _mm_mul_ps(a.m128, b.m128); }
BE_FORCE_INLINE const ssef operator*(const ssef &a, const float &b) { return a * ssef(b); }
BE_FORCE_INLINE const ssef operator*(const float &a, const ssef &b) { return ssef(a) * b; }

BE_FORCE_INLINE const ssef operator/(const ssef &a, const ssef &b) { return a * rcp_nr(b); }
BE_FORCE_INLINE const ssef operator/(const ssef &a, const float &b) { return a * rcp_nr(ssef(b)); }
BE_FORCE_INLINE const ssef operator/(const float &a, const ssef &b) { return a * rcp_nr(b); }

BE_FORCE_INLINE const ssef operator^(const ssef &a, const ssef &b) { return _mm_xor_ps(a.m128, b.m128); }
BE_FORCE_INLINE const ssef operator^(const ssef &a, const ssei &b) { return _mm_xor_ps(a.m128, _mm_castsi128_ps(b.m128i)); }

BE_FORCE_INLINE const ssef vmin(const ssef &a, const ssef &b) { return _mm_min_ps(a.m128, b.m128); }
BE_FORCE_INLINE const ssef vmin(const ssef &a, const float &b) { return _mm_min_ps(a.m128, ssef(b)); }
BE_FORCE_INLINE const ssef vmin(const float &a, const ssef &b) { return _mm_min_ps(ssef(a), b.m128); }

BE_FORCE_INLINE const ssef vmax(const ssef &a, const ssef &b) { return _mm_max_ps(a.m128, b.m128); }
BE_FORCE_INLINE const ssef vmax(const ssef &a, const float &b) { return _mm_max_ps(a.m128, ssef(b)); }
BE_FORCE_INLINE const ssef vmax(const float &a, const ssef &b) { return _mm_max_ps(ssef(a), b.m128); }

//-------------------------------------------------------------
// Assignment Operators
//-------------------------------------------------------------

BE_FORCE_INLINE ssef &operator+=(ssef &a, const ssef &b) { return a = a + b; }
BE_FORCE_INLINE ssef &operator+=(ssef &a, const float &b) { return a = a + b; }

BE_FORCE_INLINE ssef &operator-=(ssef &a, const ssef &b) { return a = a - b; }
BE_FORCE_INLINE ssef &operator-=(ssef &a, const float &b) { return a = a - b; }
  
BE_FORCE_INLINE ssef &operator*=(ssef &a, const ssef &b) { return a = a * b; }
BE_FORCE_INLINE ssef &operator*=(ssef &a, const float &b) { return a = a * b; }

BE_FORCE_INLINE ssef &operator/=(ssef &a, const ssef &b) { return a = a / b; }
BE_FORCE_INLINE ssef &operator/=(ssef &a, const float &b) { return a = a / b; }

//-------------------------------------------------------------
// Comparision Operators + Select
//-------------------------------------------------------------

BE_FORCE_INLINE const sseb operator==(const ssef &a, const ssef &b) { return _mm_cmpeq_ps(a.m128, b.m128); }
BE_FORCE_INLINE const sseb operator==(const ssef &a, const float &b) { return a == ssef(b); }
BE_FORCE_INLINE const sseb operator==(const float &a, const ssef &b) { return ssef(a) == b; }

BE_FORCE_INLINE const sseb operator!=(const ssef &a, const ssef &b) { return _mm_cmpneq_ps(a.m128, b.m128); }
BE_FORCE_INLINE const sseb operator!=(const ssef &a, const float &b) { return a != ssef(b); }
BE_FORCE_INLINE const sseb operator!=(const float &a, const ssef &b) { return ssef(a) != b; }

BE_FORCE_INLINE const sseb operator<(const ssef &a, const ssef &b) { return _mm_cmplt_ps(a.m128, b.m128); }
BE_FORCE_INLINE const sseb operator<(const ssef &a, const float &b) { return a <  ssef(b); }
BE_FORCE_INLINE const sseb operator<(const float &a, const ssef &b) { return ssef(a) <  b; }

BE_FORCE_INLINE const sseb operator>=(const ssef &a, const ssef &b) { return _mm_cmpnlt_ps(a.m128, b.m128); }
BE_FORCE_INLINE const sseb operator>=(const ssef &a, const float &b) { return a >= ssef(b); }
BE_FORCE_INLINE const sseb operator>=(const float &a, const ssef &b) { return ssef(a) >= b; }

BE_FORCE_INLINE const sseb operator>(const ssef &a, const ssef &b) { return _mm_cmpnle_ps(a.m128, b.m128); }
BE_FORCE_INLINE const sseb operator>(const ssef &a, const float &b) { return a > ssef(b); }
BE_FORCE_INLINE const sseb operator>(const float &a, const ssef &b) { return ssef(a) > b; }

BE_FORCE_INLINE const sseb operator<=(const ssef &a, const ssef &b) { return _mm_cmple_ps(a.m128, b.m128); }
BE_FORCE_INLINE const sseb operator<=(const ssef &a, const float &b) { return a <= ssef(b); }
BE_FORCE_INLINE const sseb operator<=(const float &a, const ssef &b) { return ssef(a) <= b; }

//-------------------------------------------------------------
// Rounding Functions SSE4.1
//-------------------------------------------------------------

BE_FORCE_INLINE const ssef round_near(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_NEAREST_INT); }
BE_FORCE_INLINE const ssef round_down(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF); }
BE_FORCE_INLINE const ssef round_up(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF); }
BE_FORCE_INLINE const ssef round_zero(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_ZERO); }
BE_FORCE_INLINE const ssef floor(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF); }
BE_FORCE_INLINE const ssef ceil(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF); }

//-------------------------------------------------------------
// Movement/Shifting/Shuffling
//-------------------------------------------------------------

BE_FORCE_INLINE ssef unpacklo(const ssef &a, const ssef &b) { return _mm_unpacklo_ps(a.m128, b.m128); }
BE_FORCE_INLINE ssef unpackhi(const ssef &a, const ssef &b) { return _mm_unpackhi_ps(a.m128, b.m128); }

// workaround for compiler bug in VS2008
#if defined(_MSC_VER) && (_MSC_VER < 1600)
template <size_t i> BE_FORCE_INLINE float extract(const ssef &a) { return a[i]; }
#else
template <size_t i> BE_FORCE_INLINE float extract(const ssef &a) { return _mm_cvtss_f32(_mm_shuffle_ps<i, i, i, i>(a)); }
#endif

// 2-bit index 를 사용해서 src 에서 dst 로 insert 한다. 4 bit clr 로 0 으로 채우기 가능.
#if __SSE4_1__
template <size_t dst, size_t src, size_t clr> 
BE_FORCE_INLINE const ssef insert(const ssef &a, const ssef &b) { return _mm_insert_ps(a, b, (dst << 4) | (src << 6) | clr); }

template <size_t dst, size_t src> 
BE_FORCE_INLINE const ssef insert(const ssef &a, const ssef &b) { return insert<dst, src, 0>(a, b); }

template <size_t dst> 
BE_FORCE_INLINE const ssef insert(const ssef &a, const float b) { return insert<dst, 0>(a, _mm_set_ss(b)); }
#endif

BE_FORCE_INLINE void transpose(const ssef &r0, const ssef &r1, const ssef &r2, const ssef &r3, ssef &c0, ssef &c1, ssef &c2, ssef &c3) {
    ssef l02 = unpacklo(r0, r2); // m00, m20, m01, m21
    ssef h02 = unpackhi(r0, r2); // m02, m22, m03, m23
    ssef l13 = unpacklo(r1, r3); // m10, m30, m11, m31
    ssef h13 = unpackhi(r1, r3); // m12, m32, m13, m33
    c0 = unpacklo(l02, l13); // m00, m10, m20, m30
    c1 = unpackhi(l02, l13); // m01, m11, m21, m31
    c2 = unpacklo(h02, h13); // m02, m12, m22, m32
    c3 = unpackhi(h02, h13); // m03, m13, m23, m33
}

BE_FORCE_INLINE void transpose(const ssef &r0, const ssef &r1, const ssef &r2, const ssef &r3, ssef &c0, ssef &c1, ssef &c2) {
    ssef l02 = unpacklo(r0, r2); // m00, m20, m01, m21
    ssef h02 = unpackhi(r0, r2); // m02, m22, m03, m23
    ssef l13 = unpacklo(r1, r3); // m10, m30, m11, m31
    ssef h13 = unpackhi(r1, r3); // m12, m32, m13, m33
    c0 = unpacklo(l02, l13); // m00, m10, m20, m30
    c1 = unpackhi(l02, l13); // m01, m11, m21, m31
    c2 = unpacklo(h02, h13); // m02, m12, m22, m32
}

//-------------------------------------------------------------
// Reduction Operations
//-------------------------------------------------------------

BE_FORCE_INLINE const ssef vreduce_min(const ssef &v) { ssef h = vmin(_mm_shuffle_ps<1, 0, 3, 2>(v), v); return vmin(_mm_shuffle_ps<2, 3, 0, 1>(h), h); }
BE_FORCE_INLINE const ssef vreduce_max(const ssef &v) { ssef h = vmax(_mm_shuffle_ps<1, 0, 3, 2>(v), v); return vmax(_mm_shuffle_ps<2, 3, 0, 1>(h), h); }
BE_FORCE_INLINE const ssef vreduce_add(const ssef &v) { ssef h = _mm_shuffle_ps<1, 0, 3, 2>(v) + v ; return _mm_shuffle_ps<2, 3, 0, 1>(h) + h ; }
//BE_FORCE_INLINE const ssef vreduce_add(const ssef &v) { ssef h = _mm_hadd_ps(v.m128, v.m128); return _mm_hadd_ps(h.m128, h.m128); }

// 4 개의 float 의 minimum/maximum/addition 을 구해서 float 으로 리턴.
BE_FORCE_INLINE float reduce_min(const ssef &v) { return _mm_cvtss_f32(vreduce_min(v)); }
BE_FORCE_INLINE float reduce_max(const ssef &v) { return _mm_cvtss_f32(vreduce_max(v)); }
BE_FORCE_INLINE float reduce_add(const ssef &v) { return _mm_cvtss_f32(vreduce_add(v)); }

// 4 개의 float 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min(const ssef &v) { return __bsf(movemask(v == vreduce_min(v))); }
BE_FORCE_INLINE size_t select_max(const ssef &v) { return __bsf(movemask(v == vreduce_max(v))); }

// mask 된 4 개의 float 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min(const sseb &validmask, const ssef &v) {
    const ssef a = _mm_sel_ps(ssef(FLT_INFINITY), v, validmask);
    return __bsf(movemask(validmask & (a == vreduce_min(a))));
}
BE_FORCE_INLINE size_t select_max(const sseb &validmask, const ssef &v) {
    const ssef a = _mm_sel_ps(ssef(-FLT_INFINITY), v, validmask);
    return __bsf(movemask(validmask & (a == vreduce_max(a))));
}
