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

BE_FORCE_INLINE avxf set_256ps(float a, float b, float c, float d, float e, float f, float g, float h) {
    return _mm256_set_ps(h, g, f, e, d, c, b, a);
}

BE_FORCE_INLINE avxf set1_256ps(float a) {
    return _mm256_set1_ps(a);
}

BE_FORCE_INLINE avxf set2x128_256ps(const __m128 &a, const __m128 &b) {
    return _mm256_set_m128(b, a);
}

BE_FORCE_INLINE avxf setzero_256ps() {
    return _mm256_setzero_ps();
}

BE_FORCE_INLINE avxf load_256ps(const float *a) {
    return _mm256_load_ps(a);
}

BE_FORCE_INLINE avxf loadu_256ps(const float *a) {
    return _mm256_loadu_ps(a);
}

BE_FORCE_INLINE avxf broadcast_256ss(const float *a) {
    return _mm256_broadcast_ss(a);
}

BE_FORCE_INLINE avxf broadcast_256ps(const ssef *a) {
    return _mm256_broadcast_ps(&a->m128);
}

BE_FORCE_INLINE void store_256ps(const avxf &a, float *dst) {
    _mm256_store_ps(dst, a);
}

BE_FORCE_INLINE void storeu_256ps(const avxf &a, float *dst) {
    _mm256_storeu_ps(dst, a);
}

BE_FORCE_INLINE void storent_256ps(const avxf &a, float *dst) {
    _mm256_stream_ps(dst, a);
}

BE_FORCE_INLINE avxf epi32_to_256ps(const __m256i a) {
    return _mm256_cvtepi32_ps(a);
}

BE_FORCE_INLINE avxf abs_256ps(const avxf &a) {
    return _mm256_and_ps(a, _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff)));
}

BE_FORCE_INLINE avxf sqr_256ps(const avxf &a) {
    return _mm256_mul_ps(a, a);
}

BE_FORCE_INLINE avxf sqrt_256ps(const avxf &a) {
    return _mm256_sqrt_ps(a);
}

// Returns reciprocal with 12 bits of precision.
BE_FORCE_INLINE avxf rcp12_256ps(const avxf &a) { 
    return _mm256_rcp_ps(a); 
}

// Returns reciprocal with at least 16 bits precision.
BE_FORCE_INLINE avxf rcp16_256ps(const avxf &a) {
    avxf r = _mm256_rcp_ps(a);
    // Newton-Raphson approximation to improve precision.
    return _mm256_mul_ps(r, _mm256_sub_ps(_mm256_set1_ps(2.0f), _mm256_mul_ps(a, r)));
}

// Returns reciprocal with close to full precision.
BE_FORCE_INLINE avxf rcp32_256ps(const avxf &a) {
    avxf r = _mm256_rcp_ps(a);
    // Newton-Raphson approximation to improve precision.
    r = _mm256_mul_ps(r, _mm256_sub_ps(_mm256_set1_ps(2.0f), _mm256_mul_ps(a, r)));
    return _mm256_mul_ps(r, _mm256_sub_ps(_mm256_set1_ps(2.0f), _mm256_mul_ps(a, r)));
}

// Divides with at least 12 bits precision.
BE_FORCE_INLINE avxf div12_256ps(const avxf &a, const avxf &b) {
    return _mm256_mul_ps(a, rcp12_256ps(b));
}

// Divides with at least 16 bits precision.
BE_FORCE_INLINE avxf div16_256ps(const avxf &a, const avxf &b) {
    return _mm256_mul_ps(a, rcp16_256ps(b));
}

// Divides with close to full precision.
BE_FORCE_INLINE avxf div32_256ps(const avxf &a, const avxf &b) {
    return _mm256_mul_ps(a, rcp32_256ps(b));
}

// Returns reciprocal square root with 12 bits of precision.
BE_FORCE_INLINE avxf rsqrt12_256ps(const avxf &a) { 
    return _mm256_rsqrt_ps(a); 
}

// Returns reciprocal square root with at least 16 bits precision.
BE_FORCE_INLINE avxf rsqrt16_256ps(const avxf &a) {
    avxf r = _mm256_rsqrt_ps(a);
    // Newton-Raphson approximation to improve precision.
    return _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(1.5f), r), _mm256_mul_ps(_mm256_mul_ps(_mm256_mul_ps(a, _mm256_set1_ps(-0.5f)), r), _mm256_mul_ps(r, r)));
}

// Returns reciprocal square root with close to full precision.
BE_FORCE_INLINE avxf rsqrt32_256ps(const avxf &a) {
    avxf r = _mm256_rsqrt_ps(a);
    // Newton-Raphson approximation to improve precision.
    r = _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(1.5f), r), _mm256_mul_ps(_mm256_mul_ps(_mm256_mul_ps(a, _mm256_set1_ps(-0.5f)), r), _mm256_mul_ps(r, r)));
    return _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(1.5f), r), _mm256_mul_ps(_mm256_mul_ps(_mm256_mul_ps(a, _mm256_set1_ps(-0.5f)), r), _mm256_mul_ps(r, r)));
}

BE_FORCE_INLINE avxf operator+(const avxf &a) { return a; }
BE_FORCE_INLINE avxf operator-(const avxf &a) { return _mm256_neg_ps(a.m256); }

BE_FORCE_INLINE avxf operator+(const avxf &a, const avxf &b) { return _mm256_add_ps(a, b); }
BE_FORCE_INLINE avxf operator+(const avxf &a, const float &b) { return a + set1_256ps(b); }
BE_FORCE_INLINE avxf operator+(const float &a, const avxf &b) { return set1_256ps(a) + b; }

BE_FORCE_INLINE avxf operator-(const avxf &a, const avxf &b) { return _mm256_sub_ps(a, b); }
BE_FORCE_INLINE avxf operator-(const avxf &a, const float &b) { return a - set1_256ps(b); }
BE_FORCE_INLINE avxf operator-(const float &a, const avxf &b) { return set1_256ps(a) - b; }

BE_FORCE_INLINE avxf operator*(const avxf &a, const avxf &b) { return _mm256_mul_ps(a, b); }
BE_FORCE_INLINE avxf operator*(const avxf &a, const float &b) { return a * set1_256ps(b); }
BE_FORCE_INLINE avxf operator*(const float &a, const avxf &b) { return set1_256ps(a) * b; }

BE_FORCE_INLINE avxf operator/(const avxf &a, const avxf &b) { return a * rcp32_256ps(b); }
BE_FORCE_INLINE avxf operator/(const avxf &a, const float &b) { return a * rcp32_256ps(set1_256ps(b)); }
BE_FORCE_INLINE avxf operator/(const float &a, const avxf &b) { return a * rcp32_256ps(b); }

BE_FORCE_INLINE avxf operator&(const avxf &a, const avxf &b) { return _mm256_and_ps(a, b); }
BE_FORCE_INLINE avxf operator&(const avxf &a, const avxi &b) { return _mm256_and_ps(a, _mm256_castsi256_ps(b)); }

BE_FORCE_INLINE avxf operator|(const avxf &a, const avxf &b) { return _mm256_or_ps(a, b); }
BE_FORCE_INLINE avxf operator|(const avxf &a, const avxi &b) { return _mm256_or_ps(a, _mm256_castsi256_ps(b)); }

BE_FORCE_INLINE avxf operator^(const avxf &a, const avxf &b) { return _mm256_xor_ps(a, b); }
BE_FORCE_INLINE avxf operator^(const avxf &a, const avxi &b) { return _mm256_xor_ps(a, _mm256_castsi256_ps(b)); }

BE_FORCE_INLINE avxf operator==(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a, b, _CMP_EQ_UQ); }
BE_FORCE_INLINE avxf operator==(const avxf &a, const float &b) { return a == set1_256ps(b); }
BE_FORCE_INLINE avxf operator==(const float &a, const avxf &b) { return set1_256ps(a) == b; }

BE_FORCE_INLINE avxf operator!=(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a, b, _CMP_NEQ_UQ); }
BE_FORCE_INLINE avxf operator!=(const avxf &a, const float &b) { return a != set1_256ps(b); }
BE_FORCE_INLINE avxf operator!=(const float &a, const avxf &b) { return set1_256ps(a) != b; }

BE_FORCE_INLINE avxf operator<(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a, b, _CMP_NGE_UQ); }
BE_FORCE_INLINE avxf operator<(const avxf &a, const float &b) { return a < set1_256ps(b); }
BE_FORCE_INLINE avxf operator<(const float &a, const avxf &b) { return set1_256ps(a) < b; }

BE_FORCE_INLINE avxf operator>(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a, b, _CMP_NLE_UQ); }
BE_FORCE_INLINE avxf operator>(const avxf &a, const float &b) { return a > set1_256ps(b); }
BE_FORCE_INLINE avxf operator>(const float &a, const avxf &b) { return set1_256ps(a) > b; }

BE_FORCE_INLINE avxf operator>=(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a, b, _CMP_NLT_UQ); }
BE_FORCE_INLINE avxf operator>=(const avxf &a, const float &b) { return a >= set1_256ps(b); }
BE_FORCE_INLINE avxf operator>=(const float &a, const avxf &b) { return set1_256ps(a) >= b; }

BE_FORCE_INLINE avxf operator<=(const avxf &a, const avxf &b) { return _mm256_cmp_ps(a, b, _CMP_NGT_UQ); }
BE_FORCE_INLINE avxf operator<=(const avxf &a, const float &b) { return a <= set1_256ps(b); }
BE_FORCE_INLINE avxf operator<=(const float &a, const avxf &b) { return set1_256ps(a) <= b; }

BE_FORCE_INLINE avxf &operator+=(avxf &a, const avxf &b) { return a = a + b; }
BE_FORCE_INLINE avxf &operator+=(avxf &a, const float &b) { return a = a + b; }

BE_FORCE_INLINE avxf &operator-=(avxf &a, const avxf &b) { return a = a - b; }
BE_FORCE_INLINE avxf &operator-=(avxf &a, const float &b) { return a = a - b; }

BE_FORCE_INLINE avxf &operator*=(avxf &a, const avxf &b) { return a = a * b; }
BE_FORCE_INLINE avxf &operator*=(avxf &a, const float &b) { return a = a * b; }

BE_FORCE_INLINE avxf &operator/=(avxf &a, const avxf &b) { return a = a / b; }
BE_FORCE_INLINE avxf &operator/=(avxf &a, const float &b) { return a = a / b; }

BE_FORCE_INLINE avxf &operator&=(avxf &a, const avxf &b) { return a = a & b; }
BE_FORCE_INLINE avxf &operator&=(avxf &a, const avxi &b) { return a = a & b; }

BE_FORCE_INLINE avxf &operator|=(avxf &a, const avxf &b) { return a = a | b; }
BE_FORCE_INLINE avxf &operator|=(avxf &a, const avxi &b) { return a = a | b; }

BE_FORCE_INLINE avxf &operator^=(avxf &a, const avxf &b) { return a = a ^ b; }
BE_FORCE_INLINE avxf &operator^=(avxf &a, const avxi &b) { return a = a ^ b; }

// dst = a * b + c
BE_FORCE_INLINE avxf madd_256ps(const avxf &a, const avxf &b, const avxf &c) { return _mm256_madd_ps(a.m256, b.m256, c.m256); }
// dst = -(a * b) + c
BE_FORCE_INLINE avxf nmadd_256ps(const avxf &a, const avxf &b, const avxf &c) { return _mm256_nmadd_ps(a.m256, b.m256, c.m256); }
// dst = a * b - c
BE_FORCE_INLINE avxf msub_256ps(const avxf &a, const avxf &b, const avxf &c) { return _mm256_msub_ps(a.m256, b.m256, c.m256); }
// dst = -(a * b) - c
BE_FORCE_INLINE avxf nmsub_256ps(const avxf &a, const avxf &b, const avxf &c) { return _mm256_nmsub_ps(a.m256, b.m256, c.m256); }

BE_FORCE_INLINE avxf floor_256ps(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_NEG_INF); }
BE_FORCE_INLINE avxf ceil_256ps(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_POS_INF); }
BE_FORCE_INLINE avxf trunc_256ps(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_ZERO); }
BE_FORCE_INLINE avxf round_256ps(const avxf &a) { return _mm256_round_ps(a, _MM_FROUND_TO_NEAREST_INT); }
BE_FORCE_INLINE avxf frac_256ps(const avxf &a) { return a - floor_256ps(a); }

// Unpacks to (a0, b0, a1, b1, a4, b4, a5, b5).
BE_FORCE_INLINE avxf unpacklo_256ps(const avxf &a, const avxf &b) { return _mm256_unpacklo_ps(a.m256, b.m256); }

// Unpacks to (a2, b2, a3, b3, a6, b6, a7, b7).
BE_FORCE_INLINE avxf unpackhi_256ps(const avxf &a, const avxf &b) { return _mm256_unpackhi_ps(a.m256, b.m256); }

// Shuffles 2x128 bits packed floats using template parameters. ix = [0(2), 1(3)].
template <size_t i0, size_t i1>
BE_FORCE_INLINE avxf shuffle_256ps(const avxf &a) {
    return _mm256_permute2f128_ps(a, a, (i1 << 4) | (i0));
}

// Shuffles two 2x128 bits packed floats using template parameters. ix = [0, 3].
template <size_t i0, size_t i1>
BE_FORCE_INLINE avxf shuffle_256ps(const avxf &a, const avxf &b) {
    return _mm256_permute2f128_ps(a, b, (i1 << 4) | (i0));
}

// Shuffles 4x32 bits floats for each 128 bits data using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE avxf shuffle_256ps(const avxf &b) {
    return _mm256_permute_ps(b, _MM_SHUFFLE(i3, i2, i1, i0));
}

template <size_t i0>
BE_FORCE_INLINE avxf shuffle_256ps(const avxf &a) {
    return _mm256_permute_ps(a, _MM_SHUFFLE(i0, i0, i0, i0));
}

// Shuffles two 4x32 bits floats for each 128 bits data using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE avxf shuffle_256ps(const avxf &a, const avxf &b) {
    return _mm256_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
}

template <> 
BE_FORCE_INLINE avxf shuffle_256ps<0, 0, 2, 2>(const avxf &a) { return _mm256_moveldup_ps(a); }
template <> 
BE_FORCE_INLINE avxf shuffle_256ps<1, 1, 3, 3>(const avxf &a) { return _mm256_movehdup_ps(a); }
template <> 
BE_FORCE_INLINE avxf shuffle_256ps<0, 1, 0, 1>(const avxf &a) { return _mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(a))); }

// Extracts 128 bits value with the given index. i = [0, 1].
template <size_t i>
BE_FORCE_INLINE ssef extract_256ps(const avxf &a) { return _mm256_extractf128_ps(a, i); }

// Inserts lower 128 bits of b to a in [128*dst, 128*dst+127] bits. dst = [0, 1].
template <size_t dst>
BE_FORCE_INLINE avxf insert_256ps(const avxf &a, const ssef &b) { return _mm256_insertf128_ps(a, b, dst); }

// Selects 8x32 bits floats using mask.
BE_FORCE_INLINE avxf select_256ps(const avxf &a, const avxf &b, const avxb &mask) { return _mm256_blendv_ps(a, b, mask); }

BE_FORCE_INLINE avxf min_256ps(const avxf &a, const avxf &b) { return _mm256_min_ps(a.m256, b.m256); }
BE_FORCE_INLINE avxf min_256ps(const avxf &a, const float &b) { return _mm256_min_ps(a.m256, set1_256ps(b)); }
BE_FORCE_INLINE avxf min_256ps(const float &a, const avxf &b) { return _mm256_min_ps(set1_256ps(a), b.m256); }

BE_FORCE_INLINE avxf max_256ps(const avxf &a, const avxf &b) { return _mm256_max_ps(a.m256, b.m256); }
BE_FORCE_INLINE avxf max_256ps(const avxf &a, const float &b) { return _mm256_max_ps(a.m256, set1_256ps(b)); }
BE_FORCE_INLINE avxf max_256ps(const float &a, const avxf &b) { return _mm256_max_ps(set1_256ps(a), b.m256); }

// (m01, m01, m23, m23, m45, m45, m67, m67)
BE_FORCE_INLINE avxf vreduce_min2_256ps(const avxf &a) { return min_256ps(shuffle_256ps<1, 0, 3, 2>(a), a); }

// (m0123, m0123, m0123, m0123, m4567, m4567, m4567, m4567)
BE_FORCE_INLINE avxf vreduce_min4_256ps(const avxf &a) { avxf h = vreduce_min2_256ps(a); return min_256ps(shuffle_256ps<2, 3, 0, 1>(h), h); } 

// (m01234567, m01234567, m01234567, m01234567, m01234567, m01234567, m01234567, m01234567)
BE_FORCE_INLINE avxf vreduce_min_256ps(const avxf &a) { avxf h = vreduce_min4_256ps(a); return min_256ps(shuffle_256ps<1, 0>(h), h); }

// (m01, m01, m23, m23, m45, m45, m67, m67)
BE_FORCE_INLINE avxf vreduce_max2_256ps(const avxf &a) { return max_256ps(shuffle_256ps<1, 0, 3, 2>(a), a); }

// (m0123, m0123, m0123, m0123, m4567, m4567, m4567, m4567)
BE_FORCE_INLINE avxf vreduce_max4_256ps(const avxf &a) { avxf h = vreduce_max2_256ps(a); return max_256ps(shuffle_256ps<2, 3, 0, 1>(h), h); }

// (m01234567, m01234567, m01234567, m01234567, m01234567, m01234567, m01234567, m01234567)
BE_FORCE_INLINE avxf vreduce_max_256ps(const avxf &a) { avxf h = vreduce_max4_256ps(a); return max_256ps(shuffle_256ps<1, 0>(h), h); }

// Returns minimum value of all 8 components.
BE_FORCE_INLINE float reduce_min_256ps(const avxf &a) { return _mm_cvtss_f32(extract_256ps<0>(vreduce_min_256ps(a))); }

// Returns maximum value of all 8 components.
BE_FORCE_INLINE float reduce_max_256ps(const avxf &a) { return _mm_cvtss_f32(extract_256ps<0>(vreduce_max_256ps(a))); }

// Returns index of minimum component.
BE_FORCE_INLINE size_t select_min_256ps(const avxf &a) { return __bsf(_mm256_movemask_ps(a == vreduce_min_256ps(a))); }

// Returns index of maximum component.
BE_FORCE_INLINE size_t select_max_256ps(const avxf &a) { return __bsf(_mm256_movemask_ps(a == vreduce_max_256ps(a))); }

// Returns index of minimum component with valid index mask.
BE_FORCE_INLINE size_t select_min_256ps(const avxf &v, const avxb &validmask) {
    const avxf a = select_256ps(set1_256ps(FLT_INFINITY), v, validmask);
    return __bsf(_mm256_movemask_ps(_mm256_and_ps(validmask.m256, (a == vreduce_min_256ps(a)))));
}

// Returns index of maximum component with valid index mask.
BE_FORCE_INLINE size_t select_max_256ps(const avxf &v, const avxb &validmask) {
    const avxf a = select_256ps(set1_256ps(-FLT_INFINITY), v, validmask);
    return __bsf(_mm256_movemask_ps(_mm256_and_ps(validmask.m256, (a == vreduce_max_256ps(a)))));
}

// Broadcasts sums of all components for each 128 bits packed floats.
BE_FORCE_INLINE avxf sum_256ps(const avxf &a) {
    __m256 hadd = _mm256_hadd_ps(a, a); // (x1 + y1, z1 + w1, x1 + y1, z1 + w1, x2 + y2, z2 + w2, x2 + y2, z2 + w2)
    return _mm256_hadd_ps(hadd, hadd); // (x1 + y1 + z1 + w1, x1 + y1 + z1 + w1, x2 + y2 + z2 + w2, x2 + y2 + z2 + w2)
}
