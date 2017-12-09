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

struct avxi {
    union {
        __m256i     m256;
        struct {
            __m128i l, h;
        };
        int64_t     pi64[4];
        int32_t     pi32[8];
        int16_t     pi16[16];
        int8_t      pi8[32];
    };

    BE_FORCE_INLINE avxi() {}
    BE_FORCE_INLINE avxi(const avxi &other) { m256 = other.m256; }
    BE_FORCE_INLINE avxi(const __m256i a) { m256 = a; }
    BE_FORCE_INLINE explicit avxi(const __m256 a) { m256 = _mm256_cvtps_epi32(a); }
    BE_FORCE_INLINE explicit avxi(const __m128i &a, const __m128i &b) { l = a; h = b; }
    BE_FORCE_INLINE explicit avxi(const int32_t *a) { m256 = _mm256_loadu_si256((__m256i *)a); }
    BE_FORCE_INLINE explicit avxi(int32_t a) { m256 = _mm256_set1_epi32(a); }
    BE_FORCE_INLINE explicit avxi(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g, int32_t h) { m256 = _mm256_set_epi32(h, g, f, e, d, c, b, a); }

    BE_FORCE_INLINE void setZero() { m256 = _mm256_setzero_si256(); }
    BE_FORCE_INLINE void setOne() { m256 = _mm256_set1_epi32(1); }

    BE_FORCE_INLINE operator const __m256i &() const { return m256; }
    BE_FORCE_INLINE operator __m256i &() { return m256; }

    BE_FORCE_INLINE const int32_t &operator[](const size_t i) const { assert(i < 4); return pi32[i]; }
    BE_FORCE_INLINE int32_t &operator[](const size_t i) { assert(i < 4); return pi32[i]; }
};

//-------------------------------------------------------------
// Unary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const avxi operator+(const avxi &a) { return a; }
BE_FORCE_INLINE const avxi operator-(const avxi &a) { return avxi(_mm_sub_epi32(_mm_setzero_si128(), a.l), _mm_sub_epi32(_mm_setzero_si128(), a.h)); }
BE_FORCE_INLINE const avxi abs(const avxi &a) { return avxi(_mm_abs_epi32(a.l), _mm_abs_epi32(a.h)); }

//-------------------------------------------------------------
// Binary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const avxi operator+(const avxi &a, const avxi &b) { return avxi(_mm_add_epi32(a.l, b.l), _mm_add_epi32(a.h, a.h)); }
BE_FORCE_INLINE const avxi operator+(const avxi &a, const int32_t &b) { return a + avxi(b); }
BE_FORCE_INLINE const avxi operator+(const int32_t &a, const avxi &b) { return avxi(a) + b; }

BE_FORCE_INLINE const avxi operator-(const avxi &a, const avxi &b) { return avxi(_mm_sub_epi32(a.l, b.l), _mm_sub_epi32(a.h, b.h)); }
BE_FORCE_INLINE const avxi operator-(const avxi &a, const int32_t &b) { return a - avxi(b); }
BE_FORCE_INLINE const avxi operator-(const int32_t &a, const avxi &b) { return avxi(a) - b; }

BE_FORCE_INLINE const avxi operator*(const avxi &a, const avxi &b) { return avxi(_mm_mullo_epi32(a.l, b.l), _mm_mullo_epi32(a.h, b.h)); }
BE_FORCE_INLINE const avxi operator*(const avxi &a, const int32_t &b) { return a * avxi(b); }
BE_FORCE_INLINE const avxi operator*(const int32_t &a, const avxi &b) { return avxi(a) * b; }

BE_FORCE_INLINE const avxi operator&(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
BE_FORCE_INLINE const avxi operator&(const avxi &a, const int32_t &b) { return a & avxi(b); }
BE_FORCE_INLINE const avxi operator&(const int32_t &a, const avxi &b) { return avxi(a) & b; }

BE_FORCE_INLINE const avxi operator|(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
BE_FORCE_INLINE const avxi operator|(const avxi &a, const int32_t &b) { return a | avxi(b); }
BE_FORCE_INLINE const avxi operator|(const int32_t &a, const avxi &b) { return avxi(a) | b; }

BE_FORCE_INLINE const avxi operator^(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b))); }
BE_FORCE_INLINE const avxi operator^(const avxi &a, const int32_t &b) { return a ^ avxi(b); }
BE_FORCE_INLINE const avxi operator^(const int32_t &a, const avxi &b) { return avxi(a) ^ b; }

BE_FORCE_INLINE const avxi operator<<(const avxi &a, const int32_t &n) { return avxi(_mm_slli_epi32(a.l, n), _mm_slli_epi32(a.h, n)); }
BE_FORCE_INLINE const avxi operator >> (const avxi &a, const int32_t &n) { return avxi(_mm_srai_epi32(a.l, n), _mm_slli_epi32(a.h, n)); }

BE_FORCE_INLINE const avxi sra(const avxi &a, const int32_t &b) { return avxi(_mm_srai_epi32(a.l, b), _mm_srai_epi32(a.h, b)); }
BE_FORCE_INLINE const avxi srl(const avxi &a, const int32_t &b) { return avxi(_mm_srli_epi32(a.l, b), _mm_srli_epi32(a.h, b)); }

BE_FORCE_INLINE const avxi vmin(const avxi &a, const avxi &b) { return avxi(_mm_min_epi32(a.l, b.l), _mm_min_epi32(a.h, b.h)); }
BE_FORCE_INLINE const avxi vmin(const avxi &a, const int32_t &b) { return vmin(a, avxi(b)); }
BE_FORCE_INLINE const avxi vmin(const int32_t &a, const avxi &b) { return vmin(avxi(a), b); }

BE_FORCE_INLINE const avxi vmax(const avxi &a, const avxi &b) { return avxi(_mm_max_epi32(a.l, b.l), _mm_max_epi32(a.h, b.h)); }
BE_FORCE_INLINE const avxi vmax(const avxi &a, const int32_t &b) { return vmax(a, avxi(b)); }
BE_FORCE_INLINE const avxi vmax(const int32_t &a, const avxi &b) { return vmax(avxi(a), b); }

//-------------------------------------------------------------
// Assignment Operators
//-------------------------------------------------------------

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

//-------------------------------------------------------------
// Comparision Operators + Select
//-------------------------------------------------------------

BE_FORCE_INLINE const avxb operator==(const avxi &a, const avxi &b) { return avxb(_mm_castsi128_ps(_mm_cmpeq_epi32(a.l, b.l)), _mm_castsi128_ps(_mm_cmpeq_epi32(a.h, b.h))); }
BE_FORCE_INLINE const avxb operator==(const avxi &a, const int32_t &b) { return a == avxi(b); }
BE_FORCE_INLINE const avxb operator==(const int32_t &a, const avxi &b) { return avxi(a) == b; }

BE_FORCE_INLINE const avxb operator!=(const avxi &a, const avxi &b) { return !(a == b); }
BE_FORCE_INLINE const avxb operator!=(const avxi &a, const int32_t &b) { return a != avxi(b); }
BE_FORCE_INLINE const avxb operator!=(const int32_t &a, const avxi &b) { return avxi(a) != b; }

BE_FORCE_INLINE const avxb operator<(const avxi &a, const avxi &b) { return avxb(_mm_castsi128_ps(_mm_cmplt_epi32(a.l, b.l)), _mm_castsi128_ps(_mm_cmplt_epi32(a.h, b.h))); }
BE_FORCE_INLINE const avxb operator<(const avxi &a, const int32_t &b) { return a < avxi(b); }
BE_FORCE_INLINE const avxb operator<(const int32_t &a, const avxi &b) { return avxi(a) < b; }

BE_FORCE_INLINE const avxb operator>=(const avxi &a, const avxi &b) { return !(a < b); }
BE_FORCE_INLINE const avxb operator>=(const avxi &a, const int32_t &b) { return a >= avxi(b); }
BE_FORCE_INLINE const avxb operator>=(const int32_t &a, const avxi &b) { return avxi(a) >= b; }

BE_FORCE_INLINE const avxb operator>(const avxi &a, const avxi &b) { return avxb(_mm_castsi128_ps(_mm_cmpgt_epi32(a.l, b.l)), _mm_castsi128_ps(_mm_cmpgt_epi32(a.h, b.h))); }
BE_FORCE_INLINE const avxb operator>(const avxi &a, const int32_t &b) { return a > avxi(b); }
BE_FORCE_INLINE const avxb operator>(const int32_t &a, const avxi &b) { return avxi(a) > b; }

BE_FORCE_INLINE const avxb operator<=(const avxi &a, const avxi &b) { return !(a > b); }
BE_FORCE_INLINE const avxb operator<=(const avxi &a, const int32_t &b) { return a <= avxi(b); }
BE_FORCE_INLINE const avxb operator<=(const int32_t &a, const avxi &b) { return avxi(a) <= b; }

BE_FORCE_INLINE const avxi select(const avxb &mask, const avxi &a, const avxi &b) {
    return _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(b), _mm256_castsi256_ps(a), mask));
}

//-------------------------------------------------------------
// Movement/Shifting/Shuffling
//-------------------------------------------------------------

BE_FORCE_INLINE avxi unpacklo(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_unpacklo_ps(_mm256_castsi256_ps(a.m256), _mm256_castsi256_ps(b.m256))); }
BE_FORCE_INLINE avxi unpackhi(const avxi &a, const avxi &b) { return _mm256_castps_si256(_mm256_unpackhi_ps(_mm256_castsi256_ps(a.m256), _mm256_castsi256_ps(b.m256))); }

// 한개의 2 packed 128 bit operand 에 대한 shuffle.
template<size_t i0, size_t i1> BE_FORCE_INLINE const avxi shuffle(const avxi &a) {
    return _mm256_permute2f128_si256(a, a, (i1 << 4) | (i0));
}

// 두개의 2 packed 128 bit operand 에 대한 shuffle.
template<size_t i0, size_t i1> BE_FORCE_INLINE const avxi shuffle(const avxi &a, const avxi &b) {
    return _mm256_permute2f128_si256(a, b, (i1 << 4) | (i0));
}

// 한개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0> BE_FORCE_INLINE const avxi shuffle(const avxi &a) {
    return _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(a), _MM_SHUFFLE(i0, i0, i0, i0)));
}

// 한개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const avxi shuffle(const avxi &b) {
    return _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(b), _MM_SHUFFLE(i3, i2, i1, i0)));
}

// 두개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const avxi shuffle(const avxi &a, const avxi &b) {
    return _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b), _MM_SHUFFLE(i3, i2, i1, i0)));
}

// 특수한 case 의 shuffle 은 간단한 intrinsic 으로 대응.
template<> BE_FORCE_INLINE const avxi shuffle<0, 0, 2, 2>(const avxi &b) { return _mm256_castps_si256(_mm256_moveldup_ps(_mm256_castsi256_ps(b))); }
template<> BE_FORCE_INLINE const avxi shuffle<1, 1, 3, 3>(const avxi &b) { return _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b))); }
template<> BE_FORCE_INLINE const avxi shuffle<0, 1, 0, 1>(const avxi &b) { return _mm256_castps_si256(_mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(_mm256_castsi256_ps(b))))); }

// 128 bit float 값을 하위/상위 128 bit 에 insert.
template<size_t i> BE_FORCE_INLINE const avxi insert(const avxi &a, const ssef &b) { return _mm256_insertf128_si256(a, b, i); }

// 256 bit float 에서 하위/상위 128 bit 를 extract.
template<size_t i> BE_FORCE_INLINE const ssei extract(const avxi &a) { return _mm256_extractf128_si256(a, i); }

//-------------------------------------------------------------
// Reduction Operations
//-------------------------------------------------------------

// 8 개의 packed float 의 minimum/maximum/addition 을 구한다.
BE_FORCE_INLINE const avxi vreduce_min2(const avxi &v) { return vmin(shuffle<1, 0, 3, 2>(v), v); }
BE_FORCE_INLINE const avxi vreduce_min4(const avxi &v) { avxi h = vreduce_min2(v); return vmin(shuffle<2, 3, 0, 1>(h), h); }
BE_FORCE_INLINE const avxi vreduce_min(const avxi &v) { avxi h = vreduce_min4(v); return vmin(shuffle<1, 0>(h), h); }

BE_FORCE_INLINE const avxi vreduce_max2(const avxi &v) { return vmax(shuffle<1, 0, 3, 2>(v), v); }
BE_FORCE_INLINE const avxi vreduce_max4(const avxi &v) { avxi h = vreduce_max2(v); return vmax(shuffle<2, 3, 0, 1>(h), h); }
BE_FORCE_INLINE const avxi vreduce_max(const avxi &v) { avxi h = vreduce_max4(v); return vmax(shuffle<1, 0>(h), h); }

BE_FORCE_INLINE const avxi vreduce_add2(const avxi &v) { return shuffle<1, 0, 3, 2>(v) + v; }
BE_FORCE_INLINE const avxi vreduce_add4(const avxi &v) { avxi h = vreduce_add2(v); return shuffle<2, 3, 0, 1>(h) + h; }
BE_FORCE_INLINE const avxi vreduce_add(const avxi &v) { avxi h = vreduce_add4(v); return shuffle<1, 0>(h) + h; }

// 8 개의 packed float 의 minimum/maximum/addition 을 구해서 float 으로 리턴.
BE_FORCE_INLINE int reduce_min(const avxi &v) { return extract<0>(extract<0>(vreduce_min(v))); }
BE_FORCE_INLINE int reduce_max(const avxi &v) { return extract<0>(extract<0>(vreduce_max(v))); }
BE_FORCE_INLINE int reduce_add(const avxi &v) { return extract<0>(extract<0>(vreduce_add(v))); }

// 8 개의 packed float 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min(const avxi &v) { return __bsf(movemask(v == vreduce_min(v))); }
BE_FORCE_INLINE size_t select_max(const avxi &v) { return __bsf(movemask(v == vreduce_max(v))); }

// mask 된 8 개의 packed float 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min(const avxb &valid, const avxi &v) {
    const avxi a = select(valid, v, avxi(INT_MAX));
    return __bsf(movemask(valid & (a == vreduce_min(a))));
}
BE_FORCE_INLINE size_t select_max(const avxb &valid, const avxi &v) {
    const avxi a = select(valid, v, avxi(INT_MIN));
    return __bsf(movemask(valid & (a == vreduce_max(a))));
}
