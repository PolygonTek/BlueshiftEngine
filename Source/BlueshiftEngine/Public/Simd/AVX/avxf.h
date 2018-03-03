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

struct avxf {
    union {
        __m256      m256;
        float       ps[8];
        int32_t     pi32[8];
    };

    BE_FORCE_INLINE avxf() {}
    BE_FORCE_INLINE avxf(const avxf &other) { m256 = other.m256; }
    BE_FORCE_INLINE avxf(const __m256 a) { m256 = a; }
    BE_FORCE_INLINE explicit avxf(const __m256i a) { m256 = _mm256_cvtepi32_ps(a); }
    BE_FORCE_INLINE explicit avxf(const __m128 a) { m256 = _mm256_insertf128_ps(_mm256_castps128_ps256(a), a, 1); }
    BE_FORCE_INLINE explicit avxf(const __m128 a, const __m128 b) { m256 = _mm256_insertf128_ps(_mm256_castps128_ps256(a), b, 1); }
    BE_FORCE_INLINE explicit avxf(const float *a) { m256 = _mm256_loadu_ps(a); }
    BE_FORCE_INLINE explicit avxf(float a) { m256 = _mm256_set1_ps(a); }
    BE_FORCE_INLINE explicit avxf(float a, float b, float c, float d) { m256 = _mm256_set_ps(d, c, b, a, d, c, b, a); }
    BE_FORCE_INLINE explicit avxf(float a, float b, float c, float d, float e, float f, float g, float h) { m256 = _mm256_set_ps(h, g, f, e, d, c, b, a); }

    BE_FORCE_INLINE void setZero() { m256 = _mm256_setzero_ps(); }
    BE_FORCE_INLINE void setOne() { m256 = _mm256_set1_ps(1.0f); }

    BE_FORCE_INLINE avxf &operator=(const avxf &other) { m256 = other.m256; return *this; }
    BE_FORCE_INLINE operator const __m256 &() const { return m256; };
    BE_FORCE_INLINE operator __m256 &() { return m256; };

    BE_FORCE_INLINE const float &operator[](const size_t i) const { assert(i < 8); return ps[i]; }
    BE_FORCE_INLINE float &operator[](const size_t i) { assert(i < 8); return ps[i]; }
};

//-------------------------------------------------------------
// Unary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const avxf operator+(const avxf &a) { return a; }
BE_FORCE_INLINE const avxf operator-(const avxf &a) { return _mm256_xor_ps(a.m256, _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000))); }
BE_FORCE_INLINE const avxf abs(const avxf &a) { return _mm256_and_ps(a.m256, _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff))); }
BE_FORCE_INLINE const avxf rcp(const avxf &a) { return _mm256_rcp_ps(a.m256); }
BE_FORCE_INLINE const avxf rcp_nr(const avxf &a) {
    const avxf r = _mm256_rcp_ps(a.m256);
    // Newton-Raphson approximation
    return _mm256_sub_ps(_mm256_add_ps(r, r), _mm256_mul_ps(_mm256_mul_ps(r, r), a));
}
BE_FORCE_INLINE const avxf sqr(const avxf &a) { return _mm256_mul_ps(a, a); }
BE_FORCE_INLINE const avxf sqrt(const avxf &a) { return _mm256_sqrt_ps(a.m256); }
BE_FORCE_INLINE const avxf rsqrt(const avxf &a) { return _mm256_rsqrt_ps(a.m256); }
BE_FORCE_INLINE const avxf rsqrt_nr(const avxf &a) {
    const avxf r = _mm256_rsqrt_ps(a.m256);
    // Newton-Raphson approximation
    return _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(1.5f), r),
        _mm256_mul_ps(_mm256_mul_ps(_mm256_mul_ps(a, _mm256_set1_ps(-0.5f)), r), _mm256_mul_ps(r, r)));
}

//-------------------------------------------------------------
// Binary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const avxf operator+(const avxf &a, const avxf &b) { return _mm256_add_ps(a.m256, b.m256); }
BE_FORCE_INLINE const avxf operator+(const avxf &a, const float &b) { return a + avxf(b); }
BE_FORCE_INLINE const avxf operator+(const float &a, const avxf &b) { return avxf(a) + b; }

BE_FORCE_INLINE const avxf operator-(const avxf &a, const avxf &b) { return _mm256_sub_ps(a.m256, b.m256); }
BE_FORCE_INLINE const avxf operator-(const avxf &a, const float &b) { return a - avxf(b); }
BE_FORCE_INLINE const avxf operator-(const float &a, const avxf &b) { return avxf(a) - b; }

BE_FORCE_INLINE const avxf operator*(const avxf &a, const avxf &b) { return _mm256_mul_ps(a.m256, b.m256); }
BE_FORCE_INLINE const avxf operator*(const avxf &a, const float &b) { return a * avxf(b); }
BE_FORCE_INLINE const avxf operator*(const float &a, const avxf &b) { return avxf(a) * b; }

BE_FORCE_INLINE const avxf operator/(const avxf &a, const avxf &b) { return a * rcp_nr(b); }
BE_FORCE_INLINE const avxf operator/(const avxf &a, const float &b) { return a * rcp_nr(avxf(b)); }
BE_FORCE_INLINE const avxf operator/(const float &a, const avxf &b) { return a * rcp_nr(b); }

BE_FORCE_INLINE const avxf operator^(const avxf &a, const avxf &b) { return _mm256_xor_ps(a.m256, b.m256); }
BE_FORCE_INLINE const avxf operator^(const avxf &a, const avxi &b) { return _mm256_xor_ps(a.m256, _mm256_castsi256_ps(b.m256)); }

BE_FORCE_INLINE const avxf vmin(const avxf &a, const avxf &b) { return _mm256_min_ps(a.m256, b.m256); }
BE_FORCE_INLINE const avxf vmin(const avxf &a, const float &b) { return _mm256_min_ps(a.m256, avxf(b)); }
BE_FORCE_INLINE const avxf vmin(const float &a, const avxf &b) { return _mm256_min_ps(avxf(a), b.m256); }

BE_FORCE_INLINE const avxf vmax(const avxf &a, const avxf &b) { return _mm256_max_ps(a.m256, b.m256); }
BE_FORCE_INLINE const avxf vmax(const avxf &a, const float &b) { return _mm256_max_ps(a.m256, avxf(b)); }
BE_FORCE_INLINE const avxf vmax(const float &a, const avxf &b) { return _mm256_max_ps(avxf(a), b.m256); }

//-------------------------------------------------------------
// Assignment Operators
//-------------------------------------------------------------

BE_FORCE_INLINE avxf &operator+=(avxf &a, const avxf &b) { return a = a + b; }
BE_FORCE_INLINE avxf &operator+=(avxf &a, const float &b) { return a = a + b; }

BE_FORCE_INLINE avxf &operator-=(avxf &a, const avxf &b) { return a = a - b; }
BE_FORCE_INLINE avxf &operator-=(avxf &a, const float &b) { return a = a - b; }

BE_FORCE_INLINE avxf &operator*=(avxf &a, const avxf &b) { return a = a * b; }
BE_FORCE_INLINE avxf &operator*=(avxf &a, const float &b) { return a = a * b; }

BE_FORCE_INLINE avxf &operator/=(avxf &a, const avxf &b) { return a = a / b; }
BE_FORCE_INLINE avxf &operator/=(avxf &a, const float &b) { return a = a / b; }

//-------------------------------------------------------------
// Comparision Operators + Select
//-------------------------------------------------------------

BE_FORCE_INLINE const avxb operator==(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a.m256, b.m256, _CMP_EQ_UQ); }
BE_FORCE_INLINE const avxb operator==(const avxf &a, const float &b) { return a == avxf(b); }
BE_FORCE_INLINE const avxb operator==(const float &a, const avxf &b) { return avxf(a) == b; }

BE_FORCE_INLINE const avxb operator!=(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a.m256, b.m256, _CMP_NEQ_UQ); }
BE_FORCE_INLINE const avxb operator!=(const avxf &a, const float &b) { return a != avxf(b); }
BE_FORCE_INLINE const avxb operator!=(const float &a, const avxf &b) { return avxf(a) != b; }

BE_FORCE_INLINE const avxb operator<(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a.m256, b.m256, _CMP_NGE_UQ); }
BE_FORCE_INLINE const avxb operator<(const avxf &a, const float &b) { return a < avxf(b); }
BE_FORCE_INLINE const avxb operator<(const float &a, const avxf &b) { return avxf(a) < b; }

BE_FORCE_INLINE const avxb operator>=(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a.m256, b.m256, _CMP_NLT_UQ); }
BE_FORCE_INLINE const avxb operator>=(const avxf &a, const float &b) { return a >= avxf(b); }
BE_FORCE_INLINE const avxb operator>=(const float &a, const avxf &b) { return avxf(a) >= b; }

BE_FORCE_INLINE const avxb operator>(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a.m256, b.m256, _CMP_NLE_UQ); }
BE_FORCE_INLINE const avxb operator>(const avxf &a, const float &b) { return a > avxf(b); }
BE_FORCE_INLINE const avxb operator>(const float &a, const avxf &b) { return avxf(a) > b; }

BE_FORCE_INLINE const avxb operator<=(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a.m256, b.m256, _CMP_NGT_UQ); }
BE_FORCE_INLINE const avxb operator<=(const avxf &a, const float &b) { return a <= avxf(b); }
BE_FORCE_INLINE const avxb operator<=(const float &a, const avxf &b) { return avxf(a) <= b; }

BE_FORCE_INLINE const avxf select(const avxb &mask, const avxf &t, const avxf &f) {
    return _mm256_blendv_ps(f, t, mask);
}

//-------------------------------------------------------------
// Rounding Functions
//-------------------------------------------------------------

BE_FORCE_INLINE const avxf round_near(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_NEAREST_INT); }
BE_FORCE_INLINE const avxf round_down(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_NEG_INF); }
BE_FORCE_INLINE const avxf round_up(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_POS_INF); }
BE_FORCE_INLINE const avxf round_zero(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_ZERO); }
BE_FORCE_INLINE const avxf floor(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_NEG_INF); }
BE_FORCE_INLINE const avxf ceil(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_POS_INF); }

//-------------------------------------------------------------
// Movement/Shifting/Shuffling
//-------------------------------------------------------------

BE_FORCE_INLINE avxf unpacklo(const avxf &a, const avxf &b) { return _mm256_unpacklo_ps(a.m256, b.m256); }
BE_FORCE_INLINE avxf unpackhi(const avxf &a, const avxf &b) { return _mm256_unpackhi_ps(a.m256, b.m256); }

// 한개의 2 packed 128 bit operand 에 대한 shuffle.
template<size_t i0, size_t i1> BE_FORCE_INLINE const avxf shuffle(const avxf &a) {
    return _mm256_permute2f128_ps(a, a, (i1 << 4) | (i0));
}

// 두개의 2 packed 128 bit operand 에 대한 shuffle.
template<size_t i0, size_t i1> BE_FORCE_INLINE const avxf shuffle(const avxf &a, const avxf &b) {
    return _mm256_permute2f128_ps(a, b, (i1 << 4) | (i0));
}

// 한개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0> BE_FORCE_INLINE const avxf shuffle(const avxf &a) {
    return _mm256_permute_ps(a, _MM_SHUFFLE(i0, i0, i0, i0));
}

// 한개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const avxf shuffle(const avxf &b) {
    return _mm256_permute_ps(b, _MM_SHUFFLE(i3, i2, i1, i0));
}

// 두개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const avxf shuffle(const avxf &a, const avxf &b) {
    return _mm256_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
}

// 특수한 case 의 shuffle 은 간단한 intrinsic 으로 대응.
template<> BE_FORCE_INLINE const avxf shuffle<0, 0>(const avxf &b) { return _mm256_broadcast_ps((const __m128 *)&b.m256); } // NOTE: CHECK BUG!
template<> BE_FORCE_INLINE const avxf shuffle<0, 0, 2, 2>(const avxf &b) { return _mm256_moveldup_ps(b); }
template<> BE_FORCE_INLINE const avxf shuffle<1, 1, 3, 3>(const avxf &b) { return _mm256_movehdup_ps(b); }
template<> BE_FORCE_INLINE const avxf shuffle<0, 1, 0, 1>(const avxf &b) { return _mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(b))); }

// 128 bit float 값을 하위/상위 128 bit 에 insert.
template<size_t i> BE_FORCE_INLINE const avxf insert(const avxf &a, const ssef &b) { return _mm256_insertf128_ps(a, b, i); }

// 256 bit float 에서 하위/상위 128 bit 를 extract.
template<size_t i> BE_FORCE_INLINE const ssef extract(const avxf &a) { return _mm256_extractf128_ps(a, i); }
BE_FORCE_INLINE const ssef extract0(const avxf &a) { return _mm256_extractf128_ps(a, 0); }
BE_FORCE_INLINE const ssef extract1(const avxf &a) { return _mm256_extractf128_ps(a, 1); }

BE_FORCE_INLINE void transpose4(const avxf &r0, const avxf &r1, const avxf &r2, const avxf &r3, avxf &c0, avxf &c1, avxf &c2, avxf &c3) {
    avxf l02 = unpacklo(r0, r2); // m00, m20, m01, m21, m04, m24, m05, m25
    avxf h02 = unpackhi(r0, r2); // m02, m22, m03, m23, m06, m26, m07, m27
    avxf l13 = unpacklo(r1, r3); // m10, m30, m11, m31, m14, m34, m15, m35
    avxf h13 = unpackhi(r1, r3); // m12, m32, m13, m33, m16, m36, m17, m37
    c0 = unpacklo(l02, l13); // m00, m10, m20, m30, m04, m14, m24, m34
    c1 = unpackhi(l02, l13); // m01, m11, m21, m31, m05, m15, m25, m35
    c2 = unpacklo(h02, h13); // m02, m12, m22, m32, m06, m16, m26, m36
    c3 = unpackhi(h02, h13); // m03, m13, m23, m33, m07, m17, m27, m37
}

BE_FORCE_INLINE void transpose(const avxf &r0, const avxf &r1, const avxf &r2, const avxf &r3, const avxf &r4, const avxf &r5, const avxf &r6, const avxf &r7,
    avxf &c0, avxf &c1, avxf &c2, avxf &c3, avxf &c4, avxf &c5, avxf &c6, avxf &c7) {
    avxf h0, h1, h2, h3; transpose4(r0, r1, r2, r3, h0, h1, h2, h3);
    avxf h4, h5, h6, h7; transpose4(r4, r5, r6, r7, h4, h5, h6, h7);
    c0 = shuffle<0, 2>(h0, h4);
    c1 = shuffle<0, 2>(h1, h5);
    c2 = shuffle<0, 2>(h2, h6);
    c3 = shuffle<0, 2>(h3, h7);
    c4 = shuffle<1, 3>(h0, h4);
    c5 = shuffle<1, 3>(h1, h5);
    c6 = shuffle<1, 3>(h2, h6);
    c7 = shuffle<1, 3>(h3, h7);
    avxf l02 = unpacklo(r0, r2);
    avxf h02 = unpackhi(r0, r2);
    avxf l13 = unpacklo(r1, r3);
    avxf h13 = unpackhi(r1, r3);
    c0 = unpacklo(l02, l13);
    c1 = unpackhi(l02, l13);
    c2 = unpacklo(h02, h13);
}

//-------------------------------------------------------------
// Reduction Operations
//-------------------------------------------------------------

// 8 개의 packed float 의 minimum/maximum/addition 을 구한다.
BE_FORCE_INLINE const avxf vreduce_min2(const avxf &v) { return vmin(shuffle<1, 0, 3, 2>(v), v); } // 01 01 23 23 45 45 67 67
BE_FORCE_INLINE const avxf vreduce_min4(const avxf &v) { avxf h = vreduce_min2(v); return vmin(shuffle<2, 3, 0, 1>(h), h); } // 0123 0123 0123 0123 4567 4567 4567 4567..
BE_FORCE_INLINE const avxf vreduce_min(const avxf &v) { avxf h = vreduce_min4(v); return vmin(shuffle<1, 0>(h), h); } // 01234567 01234567..

BE_FORCE_INLINE const avxf vreduce_max2(const avxf &v) { return vmax(shuffle<1, 0, 3, 2>(v), v); }
BE_FORCE_INLINE const avxf vreduce_max4(const avxf &v) { avxf h = vreduce_max2(v); return vmax(shuffle<2, 3, 0, 1>(h), h); }
BE_FORCE_INLINE const avxf vreduce_max(const avxf &v) { avxf h = vreduce_max4(v); return vmax(shuffle<1, 0>(h), h); }

BE_FORCE_INLINE const avxf vreduce_add2(const avxf &v) { return shuffle<1, 0, 3, 2>(v) + v; }
BE_FORCE_INLINE const avxf vreduce_add4(const avxf &v) { avxf h = vreduce_add2(v); return shuffle<2, 3, 0, 1>(h) + h; }
BE_FORCE_INLINE const avxf vreduce_add(const avxf &v) { avxf h = vreduce_add4(v); return shuffle<1, 0>(h) + h; }
//BE_FORCE_INLINE const avxf vreduce_add(const avxf &v)  { __m256 h = _mm256_hadd_ps(v.m256, v.m256); h = _mm256_hadd_ps(h, h); return _mm256_hadd_ps(h, h); }

// 8 개의 packed float 의 minimum/maximum/addition 을 구해서 float 으로 리턴.
BE_FORCE_INLINE float reduce_min(const avxf &v) { return _mm_cvtss_f32(extract<0>(vreduce_min(v))); }
BE_FORCE_INLINE float reduce_max(const avxf &v) { return _mm_cvtss_f32(extract<0>(vreduce_max(v))); }
BE_FORCE_INLINE float reduce_add(const avxf &v) { return _mm_cvtss_f32(extract<0>(vreduce_add(v))); }

// 8 개의 packed float 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min(const avxf &v) { return __bsf(movemask(v == vreduce_min(v))); }
BE_FORCE_INLINE size_t select_max(const avxf &v) { return __bsf(movemask(v == vreduce_max(v))); }

// mask 된 8 개의 packed float 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min(const avxb &valid, const avxf &v) {
    const avxf a = select(valid, v, avxf(FLT_INFINITY));
    return __bsf(movemask(valid & (a == vreduce_min(a))));
}
BE_FORCE_INLINE size_t select_max(const avxb &valid, const avxf &v) {
    const avxf a = select(valid, v, avxf(-FLT_INFINITY));
    return __bsf(movemask(valid & (a == vreduce_max(a))));
}
