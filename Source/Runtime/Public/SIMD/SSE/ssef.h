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

BE_FORCE_INLINE ssef set_ps(float a, float b, float c, float d) {
    return _mm_set_ps(d, c, b, a);
}

BE_FORCE_INLINE ssef set1_ps(float a) {
    return _mm_set1_ps(a);
}

BE_FORCE_INLINE ssef setzero_ps() {
    return _mm_setzero_ps();
}

BE_FORCE_INLINE ssef load_ps(const float *src) {
    return _mm_load_ps(src);
}

BE_FORCE_INLINE ssef loadu_ps(const float *src) {
    return _mm_loadu_ps(src);
}

BE_FORCE_INLINE ssef load1_ps(const float *src) {
    return _mm_load1_ps(src);
}

BE_FORCE_INLINE ssef loadnt_ps(const float *src) {
#if defined (__SSE4_1__)
    return _mm_castsi128_ps(_mm_stream_load_si128((__m128i *)src));
#else
    return _mm_load_ps(src);
#endif
}

BE_FORCE_INLINE void store_ps(const ssef &a, float *dst) {
    _mm_store_ps(dst, a);
}

BE_FORCE_INLINE void storeu_ps(const ssef &a, float *dst) {
    _mm_storeu_ps(dst, a);
}

BE_FORCE_INLINE void storent_ps(const ssef &a, float *dst) {
    _mm_stream_ps(dst, a);
}

BE_FORCE_INLINE ssef epi32_to_ps(const __m128i &a) {
    return _mm_cvtepi32_ps(a);
}

BE_FORCE_INLINE ssef abs_ps(const ssef &a) {
    return _mm_and_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))); 
}

BE_FORCE_INLINE ssef sqr_ps(const ssef &a) {
    return _mm_mul_ps(a, a);
}

BE_FORCE_INLINE ssef sqrt_ps(const ssef &a) {
    return _mm_sqrt_ps(a);
}

// Returns reciprocal with 12 bits of precision.
BE_FORCE_INLINE ssef rcp12_ps(const ssef &a) {
    return _mm_rcp_ps(a); 
}

// Returns reciprocal with at least 16 bits precision.
BE_FORCE_INLINE ssef rcp16_ps(const ssef &a) {
    ssef r = _mm_rcp_ps(a);
    // Newton-Raphson approximation to improve precision.
    return _mm_mul_ps(r, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(a, r)));
}

// Returns reciprocal with close to full precision.
BE_FORCE_INLINE ssef rcp32_ps(const ssef &a) {
    ssef r = _mm_rcp_ps(a);
    // Newton-Raphson approximation to improve precision.
    r = _mm_mul_ps(r, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(a, r)));
    return _mm_mul_ps(r, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(a, r)));
}

// Divides with at least 12 bits precision.
BE_FORCE_INLINE ssef div12_ps(const ssef &a, const ssef &b) {
    return _mm_mul_ps(a, rcp12_ps(b));
}

// Divides with at least 16 bits precision.
BE_FORCE_INLINE ssef div16_ps(const ssef &a, const ssef &b) {
    return _mm_mul_ps(a, rcp16_ps(b));
}

// Divides with close to full precision.
BE_FORCE_INLINE ssef div32_ps(const ssef &a, const ssef &b) {
    return _mm_mul_ps(a, rcp32_ps(b));
}

// Returns reciprocal square root with 12 bits of precision.
BE_FORCE_INLINE ssef rsqrt12_ps(const ssef &a) {
    return _mm_rsqrt_ps(a);
}

// Returns reciprocal square root with at least 16 bits precision.
BE_FORCE_INLINE ssef rsqrt16_ps(const ssef &a) {
    ssef r = _mm_rsqrt_ps(a);
    // Newton-Raphson approximation to improve precision.
    return _mm_add_ps(_mm_mul_ps(_mm_set1_ps(1.5f), r), _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a, _mm_set1_ps(-0.5f)), r), _mm_mul_ps(r, r)));
}

// Returns reciprocal square root with close to full precision.
BE_FORCE_INLINE ssef rsqrt32_ps(const ssef &a) {
    ssef r = _mm_rsqrt_ps(a);
    // Newton-Raphson approximation to improve precision.
    r = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(1.5f), r), _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a, _mm_set1_ps(-0.5f)), r), _mm_mul_ps(r, r)));
    return _mm_add_ps(_mm_mul_ps(_mm_set1_ps(1.5f), r), _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a, _mm_set1_ps(-0.5f)), r), _mm_mul_ps(r, r)));
}

BE_FORCE_INLINE ssef operator+(const ssef &a) { return a; }
BE_FORCE_INLINE ssef operator-(const ssef &a) { return _mm_neg_ps(a.m128); }

BE_FORCE_INLINE ssef operator+(const ssef &a, const ssef &b) { return _mm_add_ps(a, b); }
BE_FORCE_INLINE ssef operator+(const ssef &a, const float &b) { return a + set1_ps(b); }
BE_FORCE_INLINE ssef operator+(const float &a, const ssef &b) { return set1_ps(a) + b; }

BE_FORCE_INLINE ssef operator-(const ssef &a, const ssef &b) { return _mm_sub_ps(a, b); }
BE_FORCE_INLINE ssef operator-(const ssef &a, const float &b) { return a - set1_ps(b); }
BE_FORCE_INLINE ssef operator-(const float &a, const ssef &b) { return set1_ps(a) - b; }

BE_FORCE_INLINE ssef operator*(const ssef &a, const ssef &b) { return _mm_mul_ps(a, b); }
BE_FORCE_INLINE ssef operator*(const ssef &a, const float &b) { return a * set1_ps(b); }
BE_FORCE_INLINE ssef operator*(const float &a, const ssef &b) { return set1_ps(a) * b; }

BE_FORCE_INLINE ssef operator/(const ssef &a, const ssef &b) { return a * rcp32_ps(b); }
BE_FORCE_INLINE ssef operator/(const ssef &a, const float &b) { return a * rcp32_ps(set1_ps(b)); }
BE_FORCE_INLINE ssef operator/(const float &a, const ssef &b) { return a * rcp32_ps(b); }

BE_FORCE_INLINE ssef operator&(const ssef &a, const ssef &b) { return _mm_and_ps(a, b); }
BE_FORCE_INLINE ssef operator&(const ssef &a, const ssei &b) { return _mm_and_ps(a, _mm_castsi128_ps(b)); }

BE_FORCE_INLINE ssef operator|(const ssef &a, const ssef &b) { return _mm_or_ps(a, b); }
BE_FORCE_INLINE ssef operator|(const ssef &a, const ssei &b) { return _mm_or_ps(a, _mm_castsi128_ps(b)); }

BE_FORCE_INLINE ssef operator^(const ssef &a, const ssef &b) { return _mm_xor_ps(a, b); }
BE_FORCE_INLINE ssef operator^(const ssef &a, const ssei &b) { return _mm_xor_ps(a, _mm_castsi128_ps(b)); }

BE_FORCE_INLINE ssef operator==(const ssef &a, const ssef &b) { return _mm_cmpeq_ps(a, b); }
BE_FORCE_INLINE ssef operator==(const ssef &a, const float &b) { return a == set1_ps(b); }
BE_FORCE_INLINE ssef operator==(const float &a, const ssef &b) { return set1_ps(a) == b; }

BE_FORCE_INLINE ssef operator!=(const ssef &a, const ssef &b) { return _mm_cmpneq_ps(a, b); }
BE_FORCE_INLINE ssef operator!=(const ssef &a, const float &b) { return a != set1_ps(b); }
BE_FORCE_INLINE ssef operator!=(const float &a, const ssef &b) { return set1_ps(a) != b; }

BE_FORCE_INLINE ssef operator<(const ssef &a, const ssef &b) { return _mm_cmplt_ps(a, b); }
BE_FORCE_INLINE ssef operator<(const ssef &a, const float &b) { return a < set1_ps(b); }
BE_FORCE_INLINE ssef operator<(const float &a, const ssef &b) { return set1_ps(a) <  b; }

BE_FORCE_INLINE ssef operator>(const ssef &a, const ssef &b) { return _mm_cmpgt_ps(a, b); }
BE_FORCE_INLINE ssef operator>(const ssef &a, const float &b) { return a > set1_ps(b); }
BE_FORCE_INLINE ssef operator>(const float &a, const ssef &b) { return set1_ps(a) > b; }

BE_FORCE_INLINE ssef operator>=(const ssef &a, const ssef &b) { return _mm_cmpge_ps(a, b); }
BE_FORCE_INLINE ssef operator>=(const ssef &a, const float &b) { return a >= set1_ps(b); }
BE_FORCE_INLINE ssef operator>=(const float &a, const ssef &b) { return set1_ps(a) >= b; }

BE_FORCE_INLINE ssef operator<=(const ssef &a, const ssef &b) { return _mm_cmple_ps(a, b); }
BE_FORCE_INLINE ssef operator<=(const ssef &a, const float &b) { return a <= set1_ps(b); }
BE_FORCE_INLINE ssef operator<=(const float &a, const ssef &b) { return set1_ps(a) <= b; }

BE_FORCE_INLINE ssef &operator+=(ssef &a, const ssef &b) { return a = a + b; }
BE_FORCE_INLINE ssef &operator+=(ssef &a, const float &b) { return a = a + b; }

BE_FORCE_INLINE ssef &operator-=(ssef &a, const ssef &b) { return a = a - b; }
BE_FORCE_INLINE ssef &operator-=(ssef &a, const float &b) { return a = a - b; }

BE_FORCE_INLINE ssef &operator*=(ssef &a, const ssef &b) { return a = a * b; }
BE_FORCE_INLINE ssef &operator*=(ssef &a, const float &b) { return a = a * b; }

BE_FORCE_INLINE ssef &operator/=(ssef &a, const ssef &b) { return a = a / b; }
BE_FORCE_INLINE ssef &operator/=(ssef &a, const float &b) { return a = a / b; }

BE_FORCE_INLINE ssef &operator&=(ssef &a, const ssef &b) { return a = a & b; }
BE_FORCE_INLINE ssef &operator&=(ssef &a, const ssei &b) { return a = a & b; }

BE_FORCE_INLINE ssef &operator|=(ssef &a, const ssef &b) { return a = a | b; }
BE_FORCE_INLINE ssef &operator|=(ssef &a, const ssei &b) { return a = a | b; }

BE_FORCE_INLINE ssef &operator^=(ssef &a, const ssef &b) { return a = a ^ b; }
BE_FORCE_INLINE ssef &operator^=(ssef &a, const ssei &b) { return a = a ^ b; }

// dst = a * b + c
BE_FORCE_INLINE ssef madd_ps(const ssef &a, const ssef &b, const ssef &c) { return _mm_madd_ps(a.m128, b.m128, c.m128); }
// dst = a * b - c
BE_FORCE_INLINE ssef msub_ps(const ssef &a, const ssef &b, const ssef &c) { return _mm_msub_ps(a.m128, b.m128, c.m128); }
// dst = -(a * b) + c
BE_FORCE_INLINE ssef nmadd_ps(const ssef &a, const ssef &b, const ssef &c) { return _mm_nmadd_ps(a.m128, b.m128, c.m128); }
// dst = -(a * b) - c
BE_FORCE_INLINE ssef nmsub_ps(const ssef &a, const ssef &b, const ssef &c) { return _mm_nmsub_ps(a.m128, b.m128, c.m128); }

BE_FORCE_INLINE ssef floor_ps(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF); }
BE_FORCE_INLINE ssef ceil_ps(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF); }
BE_FORCE_INLINE ssef trunc_ps(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_ZERO); }
BE_FORCE_INLINE ssef round_ps(const ssef &a) { return _mm_round_ps(a, _MM_FROUND_TO_NEAREST_INT); }

BE_FORCE_INLINE ssef frac_ps(const ssef &a) { return a - floor_ps(a); }

// Unpacks to (a0, b0, a1, b1).
BE_FORCE_INLINE ssef unpacklo_ps(const ssef &a, const ssef &b) { return _mm_unpacklo_ps(a, b); }

// Unpacks to (a2, b2, a3, b3).
BE_FORCE_INLINE ssef unpackhi_ps(const ssef &a, const ssef &b) { return _mm_unpackhi_ps(a, b); }

// Shuffles 4x32 bits floats using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE ssef shuffle_ps(const ssef &a) { return _mm_perm_ps(a, _MM_SHUFFLE(i3, i2, i1, i0)); }

template <size_t i0>
BE_FORCE_INLINE ssef shuffle_ps(const ssef &a) { return _mm_splat_ps(a, i0); }
template <>
BE_FORCE_INLINE ssef shuffle_ps<0, 1, 0, 1>(const ssef &a) { return _mm_movelh_ps(a, a); }
template <> 
BE_FORCE_INLINE ssef shuffle_ps<2, 3, 2, 3>(const ssef &a) { return _mm_movehl_ps(a, a); }

// _mm_moveldup_ps and _mm_movehdup_ps are better than shuffle, since they don't destroy the input operands (under non-AVX).
template <> 
BE_FORCE_INLINE ssef shuffle_ps<0, 0, 2, 2>(const ssef &a) { return _mm_movehdup_ps(a); }
template <> 
BE_FORCE_INLINE ssef shuffle_ps<1, 1, 3, 3>(const ssef &a) { return _mm_moveldup_ps(a); }

// Shuffles two 4x32 bits floats using template parameters. ax, bx = [0, 3].
template <size_t a0, size_t a1, size_t b0, size_t b1>
BE_FORCE_INLINE ssef shuffle_ps(const ssef &a, const ssef &b) { return _mm_shuffle_ps(a, b, _MM_SHUFFLE(b1, b0, a1, a0)); }

// Extracts component value with the given index. i = [0, 3].
#if defined(__SSE4_1__) && !defined(__GNUC__)
// https://stackoverflow.com/questions/5526658/intel-sse-why-does-mm-extract-ps-return-int-instead-of-float
template <size_t i>
BE_FORCE_INLINE float extract_ps(const ssef &a) {
    union floatpun { int i; float f; } fp;
    fp.i = _mm_extract_ps(a, i);
    return fp.f;
}
#else
template <size_t i>
// NOTE: The _mm_cvtss_f32 intrinsic is free, it does not generate instructions, it only makes the compiler reinterpret the xmm register as a float.
BE_FORCE_INLINE float extract_ps(const ssef &a) { return _mm_cvtss_f32(shuffle_ps<i, i, i, i>(a)); }
#endif

template <>
BE_FORCE_INLINE float extract_ps<0>(const ssef &a) { return _mm_cvtss_f32(a); }

// Given a 4-channel single-precision ssef variable, returns the first channel 'x' as a float.
BE_FORCE_INLINE float x_ps(const ssef &a) { return extract_ps<0>(a); }

// Given a 4-channel single-precision ssef variable, returns the first channel 'y' as a float.
BE_FORCE_INLINE float y_ps(const ssef &a) { return extract_ps<1>(a); }

// Given a 4-channel single-precision ssef variable, returns the first channel 'z' as a float.
BE_FORCE_INLINE float z_ps(const ssef &a) { return extract_ps<2>(a); }

// Given a 4-channel single-precision ssef variable, returns the first channel 'w' as a float.
BE_FORCE_INLINE float w_ps(const ssef &a) { return extract_ps<3>(a); }

// Inserts [32*src, 32*src+31] bits of b to a in [32*dst, 32*dst+31] bits with clear mask.
template <int src, int dst, int clearmask>
BE_FORCE_INLINE ssef insert_ps(const ssef &a, const ssef &b) { return _mm_insert_ps(a, b, (src << 6) | (dst << 4) | clearmask); }

// Inserts [32*src, 32*src+31] bits of b to a in [32*dst, 32*dst+31] bits.
template <int src, int dst>
BE_FORCE_INLINE ssef insert_ps(const ssef &a, const ssef &b) { return insert_ps<src, dst, 0>(a, b); }

// Inserts b to a in [32*dst, 32*dst+31] bits.
template <int dst>
BE_FORCE_INLINE ssef insert_ps(const ssef &a, const float b) { return insert_ps<0, dst>(a, _mm_set_ss(b)); }

// Selects 4x32 bits floats using mask.
BE_FORCE_INLINE ssef select_ps(const ssef &a, const ssef &b, const sseb &mask) {
    return _mm_blendv_ps(a, b, mask);
}

BE_FORCE_INLINE ssef min_ps(const ssef &a, const ssef &b) { return _mm_min_ps(a, b); }
BE_FORCE_INLINE ssef min_ps(const ssef &a, const float &b) { return _mm_min_ps(a, set1_ps(b)); }
BE_FORCE_INLINE ssef min_ps(const float &a, const ssef &b) { return _mm_min_ps(set1_ps(a), b); }

BE_FORCE_INLINE ssef max_ps(const ssef &a, const ssef &b) { return _mm_max_ps(a, b); }
BE_FORCE_INLINE ssef max_ps(const ssef &a, const float &b) { return _mm_max_ps(a, set1_ps(b)); }
BE_FORCE_INLINE ssef max_ps(const float &a, const ssef &b) { return _mm_max_ps(set1_ps(a), b); }

// Broadcasts minimum value of all 4 components.
BE_FORCE_INLINE ssef vreduce_min_ps(const ssef &a) {
    ssef h = min_ps(shuffle_ps<1, 0, 3, 2>(a), a);
    return min_ps(shuffle_ps<2, 3, 0, 1>(h), h);
}

// Broadcasts maximum value of all 4 components.
BE_FORCE_INLINE ssef vreduce_max_ps(const ssef &a) {
    ssef h = max_ps(shuffle_ps<1, 0, 3, 2>(a), a);
    return max_ps(shuffle_ps<2, 3, 0, 1>(h), h);
}

BE_FORCE_INLINE float reduce_min_ps(const ssef &a) { return _mm_cvtss_f32(vreduce_min_ps(a)); }
BE_FORCE_INLINE float reduce_max_ps(const ssef &a) { return _mm_cvtss_f32(vreduce_max_ps(a)); }

// Returns index of minimum component.
BE_FORCE_INLINE size_t select_min_ps(const ssef &a) { return CountTrailingZeros(_mm_movemask_ps(a == vreduce_min_ps(a))); }

// Returns index of maximum component.
BE_FORCE_INLINE size_t select_max_ps(const ssef &a) { return CountTrailingZeros(_mm_movemask_ps(a == vreduce_max_ps(a))); }

// Returns index of minimum component with valid index mask.
BE_FORCE_INLINE size_t select_min_ps(const ssef &a, const sseb &validmask) {
    const ssef v = select_ps(set1_ps(FLT_INFINITY), a, validmask);
    return CountTrailingZeros(_mm_movemask_ps(_mm_and_ps(validmask.m128, (v == vreduce_min_ps(v)))));
}

// Returns index of maximum component with valid index mask.
BE_FORCE_INLINE size_t select_max_ps(const ssef &a, const sseb &validmask) {
    const ssef v = select_ps(set1_ps(-FLT_INFINITY), a, validmask);
    return CountTrailingZeros(_mm_movemask_ps(_mm_and_ps(validmask.m128, (v == vreduce_max_ps(v)))));
}

// Broadcasts sums of all components.
BE_FORCE_INLINE ssef sum_ps(const ssef &a) {
    __m128 hadd = _mm_hadd_ps(a, a); // (x + y, z + w, x + y, z + w)
    return _mm_hadd_ps(hadd, hadd); // (x + y + z + w, x + y + z + w, x + y + z + w, x + y + z + w)
}

// Broadcasts dot4 product.
BE_FORCE_INLINE ssef dot4_ps(const ssef &a, const ssef &b) {
#if defined(__SSE4_1__)
    return _mm_dp_ps(a, b, 0xF1);
#else
    return sum_ps(a * b);
#endif
}

// Transposes 4x4 matrix.
BE_FORCE_INLINE void transpose4x4(ssef &r0, ssef &r1, ssef &r2, ssef &r3) {
    ssef l02 = unpacklo_ps(r0, r2); // m00, m20, m01, m21
    ssef h02 = unpackhi_ps(r0, r2); // m02, m22, m03, m23
    ssef l13 = unpacklo_ps(r1, r3); // m10, m30, m11, m31
    ssef h13 = unpackhi_ps(r1, r3); // m12, m32, m13, m33
    r0 = unpacklo_ps(l02, l13); // m00, m10, m20, m30
    r1 = unpackhi_ps(l02, l13); // m01, m11, m21, m31
    r2 = unpacklo_ps(h02, h13); // m02, m12, m22, m32
    r3 = unpackhi_ps(h02, h13); // m03, m13, m23, m33
}
