// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Platform/Intrinsics.h"

BE_FORCE_INLINE ssei set_epi32(int32_t a, int32_t b, int32_t c, int32_t d) {
    return _mm_set_epi32(d, c, b, a);
}

BE_FORCE_INLINE ssei set1_epi32(int32_t a) {
    return _mm_set1_epi32(a);
}

BE_FORCE_INLINE ssei set1_epi16(int16_t a) {
    return _mm_set1_epi16(a);
}

BE_FORCE_INLINE ssei set1_epi8(int8_t a) {
    return _mm_set1_epi8(a);
}

BE_FORCE_INLINE ssei setzero_si128() {
    return _mm_setzero_si128();
}

BE_FORCE_INLINE ssei load_si128(const int32_t *src) {
    return _mm_load_si128((__m128i *)src);
}

BE_FORCE_INLINE ssei loadu_si128(const int32_t *src) {
    return _mm_loadu_si128((__m128i *)src);
}

BE_FORCE_INLINE ssei loadnt_si128(const int32_t *src) {
#if defined (__SSE4_1__)
    return _mm_stream_load_si128((__m128i *)src);
#else
    return _mm_load_si128((__m128i *)src);
#endif
}

BE_FORCE_INLINE void store_si128(const ssei &a, int32_t *dst) {
    _mm_store_si128((__m128i *)dst, a);
}

BE_FORCE_INLINE void storeu_si128(const ssei &a, int32_t *dst) {
    _mm_storeu_si128((__m128i *)dst, a);
}

BE_FORCE_INLINE void storent_si128(const ssei &a, int32_t *dst) {
    _mm_stream_si128((__m128i *)dst, a);
}

BE_FORCE_INLINE ssei ps_to_epi32(const ssef &a) {
    return _mm_cvtps_epi32(a);
}

BE_FORCE_INLINE ssei abs_epi32(const ssei &a) {
    return _mm_abs_epi32(a);
}

BE_FORCE_INLINE ssei operator+(const ssei &a) { return a; }
BE_FORCE_INLINE ssei operator-(const ssei &a) { return _mm_sub_epi32(_mm_setzero_si128(), a); }
BE_FORCE_INLINE ssei operator~(const ssei &a) { return bx_mm_inv_si128(a); }

BE_FORCE_INLINE ssei operator+(const ssei &a, const ssei &b) { return _mm_add_epi32(a, b); }
BE_FORCE_INLINE ssei operator+(const ssei &a, const int32_t &b) { return a + set1_epi32(b); }
BE_FORCE_INLINE ssei operator+(const int32_t &a, const ssei &b) { return set1_epi32(a) + b; }

BE_FORCE_INLINE ssei operator-(const ssei &a, const ssei &b) { return _mm_sub_epi32(a, b); }
BE_FORCE_INLINE ssei operator-(const ssei &a, const int32_t &b) { return a - set1_epi32(b); }
BE_FORCE_INLINE ssei operator-(const int32_t &a, const ssei &b) { return set1_epi32(a) - b; }

BE_FORCE_INLINE ssei operator*(const ssei &a, const ssei &b) { return _mm_mullo_epi32(a, b); }
BE_FORCE_INLINE ssei operator*(const ssei &a, const int32_t &b) { return a * set1_epi32(b); }
BE_FORCE_INLINE ssei operator*(const int32_t &a, const ssei &b) { return set1_epi32(a) * b; }

BE_FORCE_INLINE ssei operator&(const ssei &a, const ssei &b) { return _mm_and_si128(a, b); }
BE_FORCE_INLINE ssei operator&(const ssei &a, const int32_t &b) { return a & set1_epi32(b); }
BE_FORCE_INLINE ssei operator&(const int32_t &a, const ssei &b) { return set1_epi32(a) & b; }

BE_FORCE_INLINE ssei operator|(const ssei &a, const ssei &b) { return _mm_or_si128(a, b); }
BE_FORCE_INLINE ssei operator|(const ssei &a, const int32_t &b) { return a | set1_epi32(b); }
BE_FORCE_INLINE ssei operator|(const int32_t &a, const ssei &b) { return set1_epi32(a) | b; }

BE_FORCE_INLINE ssei operator^(const ssei &a, const ssei &b) { return _mm_xor_si128(a, b); }
BE_FORCE_INLINE ssei operator^(const ssei &a, const int32_t &b) { return a ^ set1_epi32(b); }
BE_FORCE_INLINE ssei operator^(const int32_t &a, const ssei &b) { return set1_epi32(a) ^ b; }

// Shifts signed 32-bit lanes left. Arithmetic and logical left shifts produce identical bit patterns.
BE_FORCE_INLINE ssei operator<<(const ssei &a, const int32_t &n) { return _mm_slli_epi32(a, n); }
// Shifts signed 32-bit lanes right with sign extension.
BE_FORCE_INLINE ssei operator>>(const ssei &a, const int32_t &n) { return _mm_srai_epi32(a, n); }

BE_FORCE_INLINE ssei operator==(const ssei &a, const ssei &b) { return _mm_cmpeq_epi32(a, b); }
BE_FORCE_INLINE ssei operator==(const ssei &a, const int32_t &b) { return a == set1_epi32(b); }
BE_FORCE_INLINE ssei operator==(const int32_t &a, const ssei &b) { return set1_epi32(a) == b; }

BE_FORCE_INLINE ssei operator!=(const ssei &a, const ssei &b) { return bx_mm_inv_si128(_mm_cmpeq_epi32(a, b)); }
BE_FORCE_INLINE ssei operator!=(const ssei &a, const int32_t &b) { return a != set1_epi32(b); }
BE_FORCE_INLINE ssei operator!=(const int32_t &a, const ssei &b) { return set1_epi32(a) != b; }

BE_FORCE_INLINE ssei operator<(const ssei &a, const ssei &b) { return _mm_cmplt_epi32(a, b); }
BE_FORCE_INLINE ssei operator<(const ssei &a, const int32_t &b) { return a < set1_epi32(b); }
BE_FORCE_INLINE ssei operator<(const int32_t &a, const ssei &b) { return set1_epi32(a) < b; }

BE_FORCE_INLINE ssei operator>(const ssei &a, const ssei &b) { return _mm_cmpgt_epi32(a, b); }
BE_FORCE_INLINE ssei operator>(const ssei &a, const int32_t &b) { return a > set1_epi32(b); }
BE_FORCE_INLINE ssei operator>(const int32_t &a, const ssei &b) { return set1_epi32(a) > b; }

BE_FORCE_INLINE ssei operator>=(const ssei &a, const ssei &b) { return bx_mm_inv_si128(_mm_cmplt_epi32(a, b)); }
BE_FORCE_INLINE ssei operator>=(const ssei &a, const int32_t &b) { return a >= set1_epi32(b); }
BE_FORCE_INLINE ssei operator>=(const int32_t &a, const ssei &b) { return set1_epi32(a) >= b; }

BE_FORCE_INLINE ssei operator<=(const ssei &a, const ssei &b) { return bx_mm_inv_si128(_mm_cmpgt_epi32(a, b)); }
BE_FORCE_INLINE ssei operator<=(const ssei &a, const int32_t &b) { return a <= set1_epi32(b); }
BE_FORCE_INLINE ssei operator<=(const int32_t &a, const ssei &b) { return set1_epi32(a) <= b; }

BE_FORCE_INLINE ssei &operator+=(ssei &a, const ssei &b) { return a = a + b; }
BE_FORCE_INLINE ssei &operator+=(ssei &a, const int32_t &b) { return a = a + b; }

BE_FORCE_INLINE ssei &operator-=(ssei &a, const ssei &b) { return a = a - b; }
BE_FORCE_INLINE ssei &operator-=(ssei &a, const int32_t &b) { return a = a - b; }

BE_FORCE_INLINE ssei &operator*=(ssei &a, const ssei &b) { return a = a * b; }
BE_FORCE_INLINE ssei &operator*=(ssei &a, const int32_t &b) { return a = a * b; }

BE_FORCE_INLINE ssei &operator&=(ssei &a, const ssei &b) { return a = a & b; }
BE_FORCE_INLINE ssei &operator&=(ssei &a, const int32_t &b) { return a = a & b; }

BE_FORCE_INLINE ssei &operator|=(ssei &a, const ssei &b) { return a = a | b; }
BE_FORCE_INLINE ssei &operator|=(ssei &a, const int32_t &b) { return a = a | b; }

// Compound signed left shift. Arithmetic and logical left shifts produce identical bit patterns.
BE_FORCE_INLINE ssei &operator<<=(ssei &a, const int32_t &b) { return a = a << b; }
// Compound signed right shift with sign extension.
BE_FORCE_INLINE ssei &operator>>=(ssei &a, const int32_t &b) { return a = a >> b; }

// Shifts signed 32-bit lanes right with sign extension.
BE_FORCE_INLINE ssei sra_epi32(const ssei &a, const int32_t &b) { return _mm_srai_epi32(a, b); }

// Shifts 32-bit lanes right with zero fill.
BE_FORCE_INLINE ssei srl_epi32(const ssei &a, const int32_t &b) { return _mm_srli_epi32(a, b); }

// Logically shifts all 32-bit lanes right by the same runtime count.
BE_FORCE_INLINE ssei srlv_epi32(const ssei &a, const int32_t &b) { return _mm_srl_epi32(a, _mm_cvtsi32_si128(b)); }

// Shifts 32-bit lanes left with zero fill. Arithmetic and logical left shifts produce identical bit patterns.
BE_FORCE_INLINE ssei sll_epi32(const ssei &a, const int32_t &b) { return _mm_slli_epi32(a, b); }

// Unpacks to (a0, b0, a1, b1).
BE_FORCE_INLINE ssei unpacklo_epi32(const ssei &a, const ssei &b) { return _mm_castps_si128(_mm_unpacklo_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b))); }

// Unpacks to (a2, b2, a3, b3).
BE_FORCE_INLINE ssei unpackhi_epi32(const ssei &a, const ssei &b) { return _mm_castps_si128(_mm_unpackhi_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b))); }

// Shuffles 4x32 bits integers using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE ssei shuffle_epi32(const ssei &a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(i3, i2, i1, i0)); }

template <> 
BE_FORCE_INLINE ssei shuffle_epi32<0, 0, 2, 2>(const ssei &a) { return _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(a))); }
template <> 
BE_FORCE_INLINE ssei shuffle_epi32<1, 1, 3, 3>(const ssei &a) { return _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(a))); }
template <> 
BE_FORCE_INLINE ssei shuffle_epi32<0, 1, 0, 1>(const ssei &a) { return _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(a))); }

// Shuffles two 4x32 bits integers using template parameters. ax, bx = [0, 3].
template <size_t a0, size_t a1, size_t b0, size_t b1>
BE_FORCE_INLINE ssei shuffle_epi32(const ssei &a, const ssei &b) {
    return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _MM_SHUFFLE(b1, b0, a1, a0)));
}

#if defined(__SSE4_1__)
template <size_t src>
BE_FORCE_INLINE int extract_epi32(const ssei &a) { return _mm_extract_epi32(a, src); }
template <size_t src>
BE_FORCE_INLINE int extract_epi8(const ssei &a) { return _mm_extract_epi8(a, src); }
#else
template <size_t src>
BE_FORCE_INLINE int extract_epi32(const ssei &a) { return a.i32[src & 3]; }
template <size_t src>
BE_FORCE_INLINE int extract_epi8(const ssei &a) { return a.i8[src & 15]; }
#endif

template <> 
BE_FORCE_INLINE int extract_epi32<0>(const ssei &a) { return _mm_cvtsi128_si32(a); }

#if defined(__SSE4_1__)
template <size_t dst>
BE_FORCE_INLINE const ssei insert_epi32(const ssei &a, const int32_t b) { return _mm_insert_epi32(a, b, dst); }
#else
template <size_t dst>
BE_FORCE_INLINE const ssei insert_epi32(const ssei &a, const int32_t b) { ssei c = a; c[dst & 3] = b; return c; }
#endif

// Selects 4x32 bits integer using mask.
BE_FORCE_INLINE ssei select_epi32(const ssei &a, const ssei &b, const sseb &mask) {
    return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), (mask)));
}

BE_FORCE_INLINE ssei min_epi32(const ssei &a, const ssei &b) { return _mm_min_epi32(a, b); }
BE_FORCE_INLINE ssei max_epi32(const ssei &a, const ssei &b) { return _mm_max_epi32(a, b); }

BE_FORCE_INLINE ssei min_epi32(const ssei &a, const int32_t &b) { return min_epi32(a, set1_epi32(b)); }
BE_FORCE_INLINE ssei min_epi32(const int32_t &a, const ssei &b) { return min_epi32(set1_epi32(a), b); }

BE_FORCE_INLINE ssei max_epi32(const ssei &a, const int32_t &b) { return max_epi32(a, set1_epi32(b)); }
BE_FORCE_INLINE ssei max_epi32(const int32_t &a, const ssei &b) { return max_epi32(set1_epi32(a), b); }

// Broadcasts minimum value of all 4 components.
BE_FORCE_INLINE ssei vreduce_min_epi32(const ssei &a) {
    ssei h = min_epi32(shuffle_epi32<1, 0, 3, 2>(a), a);
    return min_epi32(shuffle_epi32<2, 3, 0, 1>(h), h);
}

// Broadcasts maximum value of all 4 components.
BE_FORCE_INLINE ssei vreduce_max_epi32(const ssei &a) { 
    ssei h = max_epi32(shuffle_epi32<1, 0, 3, 2>(a), a);
    return max_epi32(shuffle_epi32<2, 3, 0, 1>(h), h);
}

BE_FORCE_INLINE int reduce_min_epi32(const ssei &a) { return extract_epi32<0>(vreduce_min_epi32(a)); }
BE_FORCE_INLINE int reduce_max_epi32(const ssei &a) { return extract_epi32<0>(vreduce_max_epi32(a)); }

// Returns index of minimum component.
BE_FORCE_INLINE size_t select_min_epi32(const ssei &a) { return CountTrailingZeros((uint32_t)_mm_movemask_ps(a == vreduce_min_epi32(a))); }

// Returns index of maximum component.
BE_FORCE_INLINE size_t select_max_epi32(const ssei &a) { return CountTrailingZeros((uint32_t)_mm_movemask_ps(a == vreduce_max_epi32(a))); }

// Returns index of minimum component with valid index mask.
BE_FORCE_INLINE size_t select_min_epi32(const ssei &a, const sseb &validmask) {
    const ssei v = select_epi32(set1_epi32(INT_MAX), a, validmask);
    return CountTrailingZeros((uint32_t)_mm_movemask_ps(_mm_and_ps(validmask.m128, _mm_castsi128_ps(v == vreduce_min_epi32(v)))));
}

// Returns index of maximum component with valid index mask.
BE_FORCE_INLINE size_t select_max_epi32(const ssei &a, const sseb &validmask) {
    const ssei v = select_epi32(set1_epi32(INT_MIN), a, validmask);
    return CountTrailingZeros((uint32_t)_mm_movemask_ps(_mm_and_ps(validmask.m128, _mm_castsi128_ps(v == vreduce_max_epi32(v)))));
}

// Broadcasts sums of all components.
BE_FORCE_INLINE ssei sum_epi32(const ssei &a) {
    __m128i hadd = _mm_hadd_epi32(a, a); // (x + y, z + w, x + y, z + w)
    return _mm_hadd_epi32(hadd, hadd); // (x + y + z + w, x + y + z + w, x + y + z + w, x + y + z + w)
}

// Unpacks 16 unsigned 8-bit integers into four groups of 4x32-bit integers.
BE_FORCE_INLINE void unpack_epu8_to_epi32(const ssei &a, ssei &r0, ssei &r1, ssei &r2, ssei &r3) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i lo16 = _mm_unpacklo_epi8(a, zero);
    const __m128i hi16 = _mm_unpackhi_epi8(a, zero);

    r0 = ssei(_mm_unpacklo_epi16(lo16, zero));
    r1 = ssei(_mm_unpackhi_epi16(lo16, zero));
    r2 = ssei(_mm_unpacklo_epi16(hi16, zero));
    r3 = ssei(_mm_unpackhi_epi16(hi16, zero));
}

// Unpacks 16 signed 8-bit integers into four groups of 4x32-bit integers.
BE_FORCE_INLINE void unpack_epi8_to_epi32(const ssei &a, ssei &r0, ssei &r1, ssei &r2, ssei &r3) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i sign8 = _mm_cmpgt_epi8(zero, a);
    const __m128i lo16 = _mm_unpacklo_epi8(a, sign8);
    const __m128i hi16 = _mm_unpackhi_epi8(a, sign8);
    const __m128i loSign16 = _mm_srai_epi16(lo16, 15);
    const __m128i hiSign16 = _mm_srai_epi16(hi16, 15);

    r0 = ssei(_mm_unpacklo_epi16(lo16, loSign16));
    r1 = ssei(_mm_unpackhi_epi16(lo16, loSign16));
    r2 = ssei(_mm_unpacklo_epi16(hi16, hiSign16));
    r3 = ssei(_mm_unpackhi_epi16(hi16, hiSign16));
}

// Unpacks 8 unsigned 16-bit integers into two groups of 4x32-bit integers.
BE_FORCE_INLINE void unpack_epu16_to_epi32(const ssei &a, ssei &r0, ssei &r1) {
    const __m128i zero = _mm_setzero_si128();

    r0 = ssei(_mm_unpacklo_epi16(a, zero));
    r1 = ssei(_mm_unpackhi_epi16(a, zero));
}

// Unpacks 8 signed 16-bit integers into two groups of 4x32-bit integers.
BE_FORCE_INLINE void unpack_epi16_to_epi32(const ssei &a, ssei &r0, ssei &r1) {
    const __m128i sign = _mm_srai_epi16(a, 15);

    r0 = ssei(_mm_unpacklo_epi16(a, sign));
    r1 = ssei(_mm_unpackhi_epi16(a, sign));
}

// Converts four floats to signed integers by truncating toward zero.
BE_FORCE_INLINE ssei trunc_ps_to_epi32(const ssef &a) {
    return _mm_cvttps_epi32(a);
}

// Input values must be in [0, 4294967040].
BE_FORCE_INLINE ssei trunc_ps_to_epu32(const ssef &a) {
    const ssef offset = set1_ps(2147483648.0f);
    const sseb highMask = a >= offset;
    const ssef adjusted = select_ps(a, a - offset, highMask);
    const ssei value = trunc_ps_to_epi32(adjusted);

    return select_epi32(
        value,
        value | set1_epi32(INT32_MIN),
        highMask);
}

// Converts four floats to signed integers by rounding toward negative infinity.
BE_FORCE_INLINE ssei floor_ps_to_epi32(const ssef &a) {
    const __m128i truncated = _mm_cvttps_epi32(a);
    const __m128 truncatedFloat = _mm_cvtepi32_ps(truncated);
    const __m128 adjustMask = _mm_cmplt_ps(a, truncatedFloat);
    const __m128i adjustment = _mm_and_si128(
        _mm_castps_si128(adjustMask),
        _mm_set1_epi32(1));

    return _mm_sub_epi32(truncated, adjustment);
}

// Saturates and packs four groups of 4x32-bit integers into 16 unsigned bytes.
BE_FORCE_INLINE ssei packus_epi32_to_epu8(const ssei &a, const ssei &b, const ssei &c, const ssei &d) {
    const __m128i lo16 = _mm_packs_epi32(a, b);
    const __m128i hi16 = _mm_packs_epi32(c, d);

    return _mm_packus_epi16(lo16, hi16);
}

// Saturates and packs four groups of 4x32-bit integers into 16 signed bytes.
BE_FORCE_INLINE ssei packs_epi32_to_epi8(const ssei &a, const ssei &b, const ssei &c, const ssei &d) {
    const __m128i lo16 = _mm_packs_epi32(a, b);
    const __m128i hi16 = _mm_packs_epi32(c, d);

    return _mm_packs_epi16(lo16, hi16);
}

// Saturates and packs two groups of 4x32-bit integers into 8 unsigned 16-bit integers.
BE_FORCE_INLINE ssei packus_epi32_to_epu16(const ssei &a, const ssei &b) {
#if defined(__SSE4_1__) || defined(__SSE4_2__)
    return _mm_packus_epi32(a, b);
#else
    const __m128i zero = _mm_setzero_si128();
    const __m128i maxValue = _mm_set1_epi32(UINT16_MAX);
    const __m128i bias32 = _mm_set1_epi32(0x8000);
    const __m128i bias16 = _mm_set1_epi16(INT16_MIN);
    const __m128i aClamped = _mm_min_epi32(_mm_max_epi32(a, zero), maxValue);
    const __m128i bClamped = _mm_min_epi32(_mm_max_epi32(b, zero), maxValue);

    return _mm_add_epi16(
        _mm_packs_epi32(_mm_sub_epi32(aClamped, bias32), _mm_sub_epi32(bClamped, bias32)),
        bias16);
#endif
}

// Saturates and packs two groups of 4x32-bit integers into 8 signed 16-bit integers.
BE_FORCE_INLINE ssei packs_epi32_to_epi16(const ssei &a, const ssei &b) {
    return _mm_packs_epi32(a, b);
}
