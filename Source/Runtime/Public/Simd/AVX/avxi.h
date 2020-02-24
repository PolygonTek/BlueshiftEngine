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

BE_FORCE_INLINE avxi set_256epi32(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g, int32_t h) {
    return _mm256_set_epi32(h, g, f, e, d, c, b, a);
}

BE_FORCE_INLINE avxi set1_256epi32(int32_t a) {
    return _mm256_set1_epi32(a);
}

BE_FORCE_INLINE avxi set2x128_256epi32(const __m128i &a, const __m128i &b) {
    return _mm256_set_m128i(b, a);
}

BE_FORCE_INLINE avxi setzero_si256() {
    return _mm256_setzero_si256();
}

BE_FORCE_INLINE avxi load_si256(const int32_t *src) {
    return _mm256_load_si256((__m256i *)src);
}

BE_FORCE_INLINE avxi loadu_si256(const int32_t *src) {
    return _mm256_loadu_si256((__m256i *)src);
}

BE_FORCE_INLINE avxi loadnt_si256(const int32_t *src) {
    return _mm256_stream_load_si256((__m256i *)src);
}

BE_FORCE_INLINE void store_si256(const avxi &a, int32_t *dst) {
    _mm256_store_si256((__m256i *)dst, a);
}

BE_FORCE_INLINE void storeu_si256(const avxi &a, int32_t *dst) {
    _mm256_storeu_si256((__m256i *)dst, a);
}

BE_FORCE_INLINE void storent_si256(const avxi &a, int32_t *dst) {
    _mm256_stream_si256((__m256i *)dst, a);
}

BE_FORCE_INLINE avxi ps_to_256epi32(const __m256 &a) {
    return _mm256_cvtps_epi32(a);
}

BE_FORCE_INLINE avxi abs_256epi32(const avxi &a) { 
    return avxi(_mm_abs_epi32(a.l128i), _mm_abs_epi32(a.h128i));
}

BE_FORCE_INLINE avxi operator+(const avxi &a) { return a; }
BE_FORCE_INLINE avxi operator-(const avxi &a) { return avxi(_mm_sub_epi32(_mm_setzero_si128(), a.l128i), _mm_sub_epi32(_mm_setzero_si128(), a.h128i)); }

BE_FORCE_INLINE avxi operator+(const avxi &a, const avxi &b) { return avxi(_mm_add_epi32(a.l128i, b.l128i), _mm_add_epi32(a.h128i, a.h128i)); }
BE_FORCE_INLINE avxi operator+(const avxi &a, const int32_t &b) { return a + set1_256epi32(b); }
BE_FORCE_INLINE avxi operator+(const int32_t &a, const avxi &b) { return set1_256epi32(a) + b; }

BE_FORCE_INLINE avxi operator-(const avxi &a, const avxi &b) { return avxi(_mm_sub_epi32(a.l128i, b.l128i), _mm_sub_epi32(a.h128i, b.h128i)); }
BE_FORCE_INLINE avxi operator-(const avxi &a, const int32_t &b) { return a - set1_256epi32(b); }
BE_FORCE_INLINE avxi operator-(const int32_t &a, const avxi &b) { return set1_256epi32(a) - b; }

BE_FORCE_INLINE avxi operator*(const avxi &a, const avxi &b) { return avxi(_mm_mullo_epi32(a.l128i, b.l128i), _mm_mullo_epi32(a.h128i, b.h128i)); }
BE_FORCE_INLINE avxi operator*(const avxi &a, const int32_t &b) { return a * set1_256epi32(b); }
BE_FORCE_INLINE avxi operator*(const int32_t &a, const avxi &b) { return set1_256epi32(a) * b; }

BE_FORCE_INLINE avxi operator&(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
BE_FORCE_INLINE avxi operator&(const avxi &a, const int32_t &b) { return a & set1_256epi32(b); }
BE_FORCE_INLINE avxi operator&(const int32_t &a, const avxi &b) { return set1_256epi32(a) & b; }

BE_FORCE_INLINE avxi operator|(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
BE_FORCE_INLINE avxi operator|(const avxi &a, const int32_t &b) { return a | set1_256epi32(b); }
BE_FORCE_INLINE avxi operator|(const int32_t &a, const avxi &b) { return set1_256epi32(a) | b; }

BE_FORCE_INLINE avxi operator^(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
BE_FORCE_INLINE avxi operator^(const avxi &a, const int32_t &b) { return a ^ set1_256epi32(b); }
BE_FORCE_INLINE avxi operator^(const int32_t &a, const avxi &b) { return set1_256epi32(a) ^ b; }

BE_FORCE_INLINE avxi operator<<(const avxi &a, const int32_t &n) { return avxi(_mm_slli_epi32(a.l128i, n), _mm_slli_epi32(a.h128i, n)); }
BE_FORCE_INLINE avxi operator>>(const avxi &a, const int32_t &n) { return avxi(_mm_srai_epi32(a.l128i, n), _mm_slli_epi32(a.h128i, n)); }

BE_FORCE_INLINE avxi operator==(const avxi &a, const avxi &b) { return avxi(_mm_cmpeq_epi32(a.l128i, b.l128i), _mm_cmpeq_epi32(a.h128i, b.h128i)); }
BE_FORCE_INLINE avxi operator==(const avxi &a, const int32_t &b) { return a == set1_256epi32(b); }
BE_FORCE_INLINE avxi operator==(const int32_t &a, const avxi &b) { return set1_256epi32(a) == b; }

BE_FORCE_INLINE avxi operator!=(const avxi &a, const avxi &b) { return avxi(_mm_inv_si128(_mm_cmpeq_epi32(a.l128i, b.l128i)), _mm_inv_si128(_mm_cmpeq_epi32(a.h128i, b.h128i))); }
BE_FORCE_INLINE avxi operator!=(const avxi &a, const int32_t &b) { return a != set1_256epi32(b); }
BE_FORCE_INLINE avxi operator!=(const int32_t &a, const avxi &b) { return set1_256epi32(a) != b; }

BE_FORCE_INLINE avxi operator<(const avxi &a, const avxi &b) { return avxi(_mm_cmplt_epi32(a.l128i, b.l128i), _mm_cmplt_epi32(a.h128i, b.h128i)); }
BE_FORCE_INLINE avxi operator<(const avxi &a, const int32_t &b) { return a < set1_256epi32(b); }
BE_FORCE_INLINE avxi operator<(const int32_t &a, const avxi &b) { return set1_256epi32(a) < b; }

BE_FORCE_INLINE avxi operator>(const avxi &a, const avxi &b) { return avxi(_mm_cmpgt_epi32(a.l128i, b.l128i), _mm_cmpgt_epi32(a.h128i, b.h128i)); }
BE_FORCE_INLINE avxi operator>(const avxi &a, const int32_t &b) { return a > set1_256epi32(b); }
BE_FORCE_INLINE avxi operator>(const int32_t &a, const avxi &b) { return set1_256epi32(a) > b; }

BE_FORCE_INLINE avxi operator>=(const avxi &a, const avxi &b) { return avxi(_mm_inv_si128(_mm_cmplt_epi32(a.l128i, b.l128i)), _mm_inv_si128(_mm_cmplt_epi32(a.h128i, b.h128i))); }
BE_FORCE_INLINE avxi operator>=(const avxi &a, const int32_t &b) { return a >= set1_256epi32(b); }
BE_FORCE_INLINE avxi operator>=(const int32_t &a, const avxi &b) { return set1_256epi32(a) >= b; }

BE_FORCE_INLINE avxi operator<=(const avxi &a, const avxi &b) { return avxi(_mm_inv_si128(_mm_cmpgt_epi32(a.l128i, b.l128i)), _mm_inv_si128(_mm_cmpgt_epi32(a.h128i, b.h128i))); }
BE_FORCE_INLINE avxi operator<=(const avxi &a, const int32_t &b) { return a <= set1_256epi32(b); }
BE_FORCE_INLINE avxi operator<=(const int32_t &a, const avxi &b) { return set1_256epi32(a) <= b; }

BE_FORCE_INLINE avxi &operator+=(avxi &a, const avxi &b) { return a = a + b; }
BE_FORCE_INLINE avxi &operator+=(avxi &a, const int32_t &b) { return a = a + b; }

BE_FORCE_INLINE avxi &operator-=(avxi &a, const avxi &b) { return a = a - b; }
BE_FORCE_INLINE avxi &operator-=(avxi &a, const int32_t &b) { return a = a - b; }

BE_FORCE_INLINE avxi &operator*=(avxi &a, const avxi &b) { return a = a * b; }
BE_FORCE_INLINE avxi &operator*=(avxi &a, const int32_t &b) { return a = a * b; }

BE_FORCE_INLINE avxi &operator&=(avxi &a, const avxi &b) { return a = a & b; }
BE_FORCE_INLINE avxi &operator&=(avxi &a, const int32_t &b) { return a = a & b; }

BE_FORCE_INLINE avxi &operator|=(avxi &a, const avxi &b) { return a = a | b; }
BE_FORCE_INLINE avxi &operator|=(avxi &a, const int32_t &b) { return a = a | b; }

BE_FORCE_INLINE avxi &operator<<=(avxi &a, const int32_t &b) { return a = a << b; }
BE_FORCE_INLINE avxi &operator>>=(avxi &a, const int32_t &b) { return a = a >> b; }

// Shifts right arithmetic.
BE_FORCE_INLINE avxi sra_256epi32(const avxi &a, const int32_t &b) { return avxi(_mm_srai_epi32(a.l128i, b), _mm_srai_epi32(a.h128i, b)); }

// Shifts right logical.
BE_FORCE_INLINE avxi srl_256epi32(const avxi &a, const int32_t &b) { return avxi(_mm_srli_epi32(a.l128i, b), _mm_srli_epi32(a.h128i, b)); }

// Shifts left logical.
BE_FORCE_INLINE avxi sll_256epi32(const avxi &a, const int32_t &b) { return avxi(_mm_slli_epi32(a.l128i, b), _mm_slli_epi32(a.h128i, b)); }

// Unpacks to (a0, b0, a1, b1, a4, b4, a5, b5).
BE_FORCE_INLINE avxi unpacklo_256epi32(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_unpacklo_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }

// Unpacks to (a2, b2, a3, b3, a6, b6, a7, b7).
BE_FORCE_INLINE avxi unpackhi_256epi32(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_unpackhi_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }

// Shuffles 2x128 bits packed integers using template parameters. ix = [0(2), 1(3)].
template <size_t i0, size_t i1>
BE_FORCE_INLINE avxi shuffle_256epi32(const avxi &a) {
    return _mm256_permute2f128_si256(a, a, (i1 << 4) | (i0));
}

// Shuffles two 2x128 bits packed integers using template parameters. ix = [0(2), 1(3)].
template <size_t i0, size_t i1>
BE_FORCE_INLINE avxi shuffle_256epi32(const avxi &a, const avxi &b) {
    return _mm256_permute2f128_si256(a, b, (i1 << 4) | (i0));
}

// Shuffles 4x32 bits integers for each 128 bits data using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE avxi shuffle_256epi32(const avxi &a) {
    return _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(a), _MM_SHUFFLE(i3, i2, i1, i0)));
}

template <size_t i0>
BE_FORCE_INLINE avxi shuffle_256epi32(const avxi &a) {
    return _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(a), _MM_SHUFFLE(i0, i0, i0, i0)));
}

// Shuffles two 4x32 bits integers for each 128 bits data using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE avxi shuffle_256epi32(const avxi &a, const avxi &b) {
    return _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b), _MM_SHUFFLE(i3, i2, i1, i0)));
}

template <>
BE_FORCE_INLINE avxi shuffle_256epi32<0, 0, 2, 2>(const avxi &a) { return _mm256_castps_si256(_mm256_moveldup_ps(_mm256_castsi256_ps(a))); }
template <>
BE_FORCE_INLINE avxi shuffle_256epi32<1, 1, 3, 3>(const avxi &a) { return _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(a))); }
template <>
BE_FORCE_INLINE avxi shuffle_256epi32<0, 1, 0, 1>(const avxi &a) { return _mm256_castps_si256(_mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(_mm256_castsi256_ps(a))))); }

// Extracts 128 bits value with the given index. i = [0, 1].
template <size_t i>
BE_FORCE_INLINE ssei extract_256epi32(const avxi &a) { return _mm256_extractf128_si256(a, i); }

// Inserts lower 128 bits of b to a in [128*dst, 128*dst+127] bits. dst = [0, 1].
template <size_t i>
BE_FORCE_INLINE avxi insert_256epi32(const avxi &a, const ssef &b) { return _mm256_insertf128_si256(a, b, i); }

// Selects 8x32 bits integer using mask.
BE_FORCE_INLINE avxi select_256epi32(const avxi &a, const avxi &b, const avxb &mask) {
    return _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(b), _mm256_castsi256_ps(a), mask));
}

BE_FORCE_INLINE avxi min_256epi32(const avxi &a, const avxi &b) { return avxi(_mm_min_epi32(a.l128i, b.l128i), _mm_min_epi32(a.h128i, b.h128i)); }
BE_FORCE_INLINE avxi min_256epi32(const avxi &a, const int32_t &b) { return min_256epi32(a, set1_256epi32(b)); }
BE_FORCE_INLINE avxi min_256epi32(const int32_t &a, const avxi &b) { return min_256epi32(set1_256epi32(a), b); }

BE_FORCE_INLINE avxi max_256epi32(const avxi &a, const avxi &b) { return avxi(_mm_max_epi32(a.l128i, b.l128i), _mm_max_epi32(a.h128i, b.h128i)); }
BE_FORCE_INLINE avxi max_256epi32(const avxi &a, const int32_t &b) { return max_256epi32(a, set1_256epi32(b)); }
BE_FORCE_INLINE avxi max_256epi32(const int32_t &a, const avxi &b) { return max_256epi32(set1_256epi32(a), b); }

// (m01, m01, m23, m23, m45, m45, m67, m67)
BE_FORCE_INLINE avxi vreduce_min2_256epi32(const avxi &a) { return min_256epi32(shuffle_256epi32<1, 0, 3, 2>(a), a); }

// (m0123, m0123, m0123, m0123, m4567, m4567, m4567, m4567)
BE_FORCE_INLINE avxi vreduce_min4_256epi32(const avxi &a) { avxi h = vreduce_min2_256epi32(a); return min_256epi32(shuffle_256epi32<2, 3, 0, 1>(h), h); }

// (m01234567, m01234567, m01234567, m01234567, m01234567, m01234567, m01234567, m01234567)
BE_FORCE_INLINE avxi vreduce_min_256epi32(const avxi &a) { avxi h = vreduce_min4_256epi32(a); return min_256epi32(shuffle_256epi32<1, 0>(h), h); }

// (m01, m01, m23, m23, m45, m45, m67, m67)
BE_FORCE_INLINE avxi vreduce_max2_256epi32(const avxi &a) { return max_256epi32(shuffle_256epi32<1, 0, 3, 2>(a), a); }

// (m0123, m0123, m0123, m0123, m4567, m4567, m4567, m4567)
BE_FORCE_INLINE avxi vreduce_max4_256epi32(const avxi &a) { avxi h = vreduce_max2_256epi32(a); return max_256epi32(shuffle_256epi32<2, 3, 0, 1>(h), h); }

// (m01234567, m01234567, m01234567, m01234567, m01234567, m01234567, m01234567, m01234567)
BE_FORCE_INLINE avxi vreduce_max_256epi32(const avxi &a) { avxi h = vreduce_max4_256epi32(a); return max_256epi32(shuffle_256epi32<1, 0>(h), h); }

// Returns minimum value of all 8 components.
BE_FORCE_INLINE int reduce_min_256epi32(const avxi &a) { return extract_epi32<0>(extract_256epi32<0>(vreduce_min_256epi32(a))); }

// Returns maximum value of all 8 components.
BE_FORCE_INLINE int reduce_max_256epi32(const avxi &a) { return extract_epi32<0>(extract_256epi32<0>(vreduce_max_256epi32(a))); }

// Returns index of minimum component.
BE_FORCE_INLINE size_t select_min_256epi32(const avxi &a) { return __bsf(_mm256_movemask_ps(a == vreduce_min_256epi32(a))); }

// Returns index of maximum component.
BE_FORCE_INLINE size_t select_max_256epi32(const avxi &a) { return __bsf(_mm256_movemask_ps(a == vreduce_max_256epi32(a))); }

// mask 된 8 개의 packed float 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min_256epi32(const avxi &a, const avxb &validmask) {
    const avxi v = select_256epi32(set1_256epi32(INT_MAX), a, validmask);
    return __bsf(_mm256_movemask_ps(_mm256_and_ps(validmask.m256, (v == vreduce_min_256epi32(v)))));
}
BE_FORCE_INLINE size_t select_max_256epi32(const avxi &a, const avxb &validmask) {
    const avxi v = select_256epi32(set1_256epi32(INT_MIN), a, validmask);
    return __bsf(_mm256_movemask_ps(_mm256_and_ps(validmask.m256, (v == vreduce_max_256epi32(v)))));
}
