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

struct ssei {
    union { 
        __m128i     m128; 
        int64_t     pi64[2]; 
        int32_t     pi32[4]; 
        int16_t     pi16[8]; 
        int8_t      pi8[16]; 
    };

    BE_FORCE_INLINE ssei() {}
    BE_FORCE_INLINE ssei(const ssei &other) { m128 = other.m128; }
    BE_FORCE_INLINE ssei &operator=(const ssei &other) { m128 = other.m128; return *this; }
    BE_FORCE_INLINE ssei(const __m128i a) { m128 = a; }
    BE_FORCE_INLINE explicit ssei(const __m128 a) { m128 = _mm_cvtps_epi32(a); }
    BE_FORCE_INLINE explicit ssei(const int32_t *a) { m128 = _mm_loadu_si128((__m128i *)a); }
    BE_FORCE_INLINE explicit ssei(int32_t a) { m128 = _mm_set1_epi32(a); }
    BE_FORCE_INLINE explicit ssei(int32_t a, int32_t b, int32_t c, int32_t d) { m128 = _mm_set_epi32(d, c, b, a); }

    BE_FORCE_INLINE void setZero() { m128 = _mm_setzero_si128(); }
    BE_FORCE_INLINE void setOne() { m128 = _mm_set1_epi32(1); }

    BE_FORCE_INLINE operator const __m128i &() const { return m128; }
    BE_FORCE_INLINE operator __m128i &() { return m128; }

    BE_FORCE_INLINE const int32_t &operator[](const size_t i) const { assert(i < 4); return pi32[i]; }
    BE_FORCE_INLINE int32_t &operator[](const size_t i) { assert(i < 4); return pi32[i]; }
};

//-------------------------------------------------------------
// Unary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const ssei operator+(const ssei &a) { return a; }
BE_FORCE_INLINE const ssei operator-(const ssei &a) { return _mm_sub_epi32(_mm_setzero_si128(), a.m128); }
BE_FORCE_INLINE const ssei abs(const ssei &a) {
#ifdef __SSSE3__
    return _mm_abs_epi32(a.m128);
#else
    __m128i mask = _mm_cmplt_epi32(a, _mm_setzero_si128()); // FFFF   where a < 0
    ssei r = _mm_xor_si128(a, mask);                        // Invert where a < 0
    mask   = _mm_srli_epi32(mask, 31);                      // 0001   where a < 0
    r      = _mm_add_epi32(r, mask);                        // Add 1  where a < 0
    return a;
#endif
}

//-------------------------------------------------------------
// Binary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const ssei operator+(const ssei &a, const ssei &b) { return _mm_add_epi32(a.m128, b.m128); }
BE_FORCE_INLINE const ssei operator+(const ssei &a, const int32_t &b) { return a + ssei(b); }
BE_FORCE_INLINE const ssei operator+(const int32_t &a, const ssei &b) { return ssei(a) + b; }

BE_FORCE_INLINE const ssei operator-(const ssei &a, const ssei &b) { return _mm_sub_epi32(a.m128, b.m128); }
BE_FORCE_INLINE const ssei operator-(const ssei &a, const int32_t &b) { return a - ssei(b); }
BE_FORCE_INLINE const ssei operator-(const int32_t &a, const ssei &b) { return ssei(a) - b; }

BE_FORCE_INLINE const ssei operator*(const ssei &a, const ssei &b) { return _mm_mullo_epi32(a.m128, b.m128); }
BE_FORCE_INLINE const ssei operator*(const ssei &a, const int32_t &b) { return a * ssei(b); }
BE_FORCE_INLINE const ssei operator*(const int32_t &a, const ssei &b) { return ssei(a) * b; }

BE_FORCE_INLINE const ssei operator&(const ssei &a, const ssei &b) { return _mm_and_si128(a.m128, b.m128); }
BE_FORCE_INLINE const ssei operator&(const ssei &a, const int32_t &b) { return a & ssei(b); }
BE_FORCE_INLINE const ssei operator&(const int32_t &a, const ssei &b) { return ssei(a) & b; }

BE_FORCE_INLINE const ssei operator|(const ssei &a, const ssei &b) { return _mm_or_si128(a.m128, b.m128); }
BE_FORCE_INLINE const ssei operator|(const ssei &a, const int32_t &b) { return a | ssei(b); }
BE_FORCE_INLINE const ssei operator|(const int32_t &a, const ssei &b) { return ssei(a) | b; }

BE_FORCE_INLINE const ssei operator^(const ssei &a, const ssei &b) { return _mm_xor_si128(a.m128, b.m128); }
BE_FORCE_INLINE const ssei operator^(const ssei &a, const int32_t &b) { return a ^ ssei(b); }
BE_FORCE_INLINE const ssei operator^(const int32_t &a, const ssei &b) { return ssei(a) ^ b; }

BE_FORCE_INLINE const ssei operator<<(const ssei &a, const int32_t &n) { return _mm_slli_epi32(a.m128, n); }
BE_FORCE_INLINE const ssei operator>>(const ssei &a, const int32_t &n) { return _mm_srai_epi32(a.m128, n); }

BE_FORCE_INLINE const ssei sra(const ssei &a, const int32_t &b) { return _mm_srai_epi32(a.m128, b); }
BE_FORCE_INLINE const ssei srl(const ssei &a, const int32_t &b) { return _mm_srli_epi32(a.m128, b); }

BE_FORCE_INLINE const ssei vmin(const ssei &a, const ssei &b) { return _mm_min_epi32(a.m128, b.m128); }
BE_FORCE_INLINE const ssei vmin(const ssei &a, const int32_t &b) { return vmin(a, ssei(b)); }
BE_FORCE_INLINE const ssei vmin(const int32_t &a, const ssei &b) { return vmin(ssei(a), b); }

BE_FORCE_INLINE const ssei vmax(const ssei &a, const ssei &b) { return _mm_max_epi32(a.m128, b.m128); }
BE_FORCE_INLINE const ssei vmax(const ssei &a, const int32_t &b) { return vmax(a, ssei(b)); }
BE_FORCE_INLINE const ssei vmax(const int32_t &a, const ssei &b) { return vmax(ssei(a), b); }

//-------------------------------------------------------------
// Assignment Operators
//-------------------------------------------------------------

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

BE_FORCE_INLINE ssei &operator<<=(ssei &a, const int32_t &b) { return a = a << b; }
BE_FORCE_INLINE ssei &operator>>=(ssei &a, const int32_t &b) { return a = a >> b; }

//-------------------------------------------------------------
// Comparision Operators + Select
//-------------------------------------------------------------

BE_FORCE_INLINE const sseb operator==(const ssei &a, const ssei &b) { return _mm_castsi128_ps(_mm_cmpeq_epi32(a.m128, b.m128)); }
BE_FORCE_INLINE const sseb operator==(const ssei &a, const int32_t &b) { return a == ssei(b); }
BE_FORCE_INLINE const sseb operator==(const int32_t &a, const ssei &b) { return ssei(a) == b; }

BE_FORCE_INLINE const sseb operator!=(const ssei &a, const ssei &b) { return !(a == b); }
BE_FORCE_INLINE const sseb operator!=(const ssei &a, const int32_t &b) { return a != ssei(b); }
BE_FORCE_INLINE const sseb operator!=(const int32_t &a, const ssei &b) { return ssei(a) != b; }

BE_FORCE_INLINE const sseb operator<(const ssei &a, const ssei &b) { return _mm_castsi128_ps(_mm_cmplt_epi32(a.m128, b.m128)); }
BE_FORCE_INLINE const sseb operator<(const ssei &a, const int32_t &b) { return a < ssei(b); }
BE_FORCE_INLINE const sseb operator<(const int32_t &a, const ssei &b) { return ssei(a) <  b; }

BE_FORCE_INLINE const sseb operator>=(const ssei &a, const ssei &b) { return !(a < b); }
BE_FORCE_INLINE const sseb operator>=(const ssei &a, const int32_t &b) { return a >= ssei(b); }
BE_FORCE_INLINE const sseb operator>=(const int32_t &a, const ssei &b) { return ssei(a) >= b; }

BE_FORCE_INLINE const sseb operator>(const ssei &a, const ssei &b) { return _mm_castsi128_ps(_mm_cmpgt_epi32(a.m128, b.m128)); }
BE_FORCE_INLINE const sseb operator>(const ssei &a, const int32_t &b) { return a > ssei(b); }
BE_FORCE_INLINE const sseb operator>(const int32_t &a, const ssei &b) { return ssei(a) >  b; }

BE_FORCE_INLINE const sseb operator<=(const ssei &a, const ssei &b) { return !(a > b); }
BE_FORCE_INLINE const sseb operator<=(const ssei &a, const int32_t &b) { return a <= ssei(b); }
BE_FORCE_INLINE const sseb operator<=(const int32_t &a, const ssei &b) { return ssei(a) <= b; }

// workaround for compiler bug in VS2008
#if defined(_MSC_VER) && (_MSC_VER < 1600)
BE_FORCE_INLINE const ssei select(const sseb &mask, const ssei &a, const ssei &b) { 
    return _mm_castps_si128(_mm_or_ps(_mm_and_ps(mask, _mm_castsi128_ps(a)), _mm_andnot_ps(mask, _mm_castsi128_ps(b)))); 
}
#else
BE_FORCE_INLINE const ssei select(const sseb &mask, const ssei &a, const ssei &b) { 
    return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(b), _mm_castsi128_ps(a), mask)); 
}
#endif

//-------------------------------------------------------------
// Movement/Shifting/Shuffling
//-------------------------------------------------------------

BE_FORCE_INLINE ssei unpacklo(const ssei &a, const ssei &b) { return _mm_castps_si128(_mm_unpacklo_ps(_mm_castsi128_ps(a.m128), _mm_castsi128_ps(b.m128))); }
BE_FORCE_INLINE ssei unpackhi(const ssei &a, const ssei &b) { return _mm_castps_si128(_mm_unpackhi_ps(_mm_castsi128_ps(a.m128), _mm_castsi128_ps(b.m128))); }

// 한개의 4 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다.
template <size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const ssei shuffle(const ssei &a) {
    return _mm_shuffle_epi32(a, _MM_SHUFFLE(i3, i2, i1, i0));
}

// 두개의 4 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다.
template <size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const ssei shuffle(const ssei &a, const ssei &b) {
    return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _MM_SHUFFLE(i3, i2, i1, i0)));
}

// 특수한 case 의 shuffle 은 간단한 intrinsic 으로 대응.
template<> BE_FORCE_INLINE const ssei shuffle<0, 0, 2, 2>(const ssei &a) { return _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(a))); }
template<> BE_FORCE_INLINE const ssei shuffle<1, 1, 3, 3>(const ssei &a) { return _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(a))); }
template<> BE_FORCE_INLINE const ssei shuffle<0, 1, 0, 1>(const ssei &a) { return _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(a))); }

// workaround for compiler bug in VS2008
#if defined(_MSC_VER) && (_MSC_VER < 1600)
template <size_t src> BE_FORCE_INLINE int extract(const ssei &b) { return b[src]; }
#else
// 0~3 index 를 사용해서 extract 한다. (SSE4.1)
template <size_t src> BE_FORCE_INLINE int extract(const ssei &b) { return _mm_extract_epi32(b, src); }
#endif
// 0~3 index 를 사용해서 insert 한다. (SSE4.1)
template <size_t dst> BE_FORCE_INLINE const ssei insert(const ssei &a, const int32_t b) { return _mm_insert_epi32(a, b, dst); }

//-------------------------------------------------------------
// Reduction Operations
//-------------------------------------------------------------

// 4 개의 integer 의 minimum/maximum/addition 을 구한다.
BE_FORCE_INLINE const ssei vreduce_min(const ssei &v) { ssei h = vmin(shuffle<1, 0, 3, 2>(v), v); return vmin(shuffle<2, 3, 0, 1>(h), h); }
BE_FORCE_INLINE const ssei vreduce_max(const ssei &v) { ssei h = vmax(shuffle<1, 0, 3, 2>(v), v); return vmax(shuffle<2, 3, 0, 1>(h), h); }
BE_FORCE_INLINE const ssei vreduce_add(const ssei &v) { ssei h = shuffle<1, 0, 3, 2>(v) + v ; return shuffle<2, 3, 0, 1>(h) + h ; }

// 4 개의 integer 의 minimum/maximum/addition 을 구해서 float 으로 리턴.
BE_FORCE_INLINE int reduce_min(const ssei &v) { return extract<0>(vreduce_min(v)); }
BE_FORCE_INLINE int reduce_max(const ssei &v) { return extract<0>(vreduce_max(v)); }
BE_FORCE_INLINE int reduce_add(const ssei &v) { return extract<0>(vreduce_add(v)); }

// 4 개의 integer 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min(const ssei &v) { return __bsf(movemask(v == vreduce_min(v))); }
BE_FORCE_INLINE size_t select_max(const ssei &v) { return __bsf(movemask(v == vreduce_max(v))); }

// mask 된 4 개의 integer 중에서 minimum/maximum 값의 bit index 를 리턴.
BE_FORCE_INLINE size_t select_min(const sseb &valid, const ssei &v) {
    const ssei a = select(valid, v, ssei(INT_MAX)); 
    return __bsf(movemask(valid & (a == vreduce_min(a)))); 
}
BE_FORCE_INLINE size_t select_max(const sseb &valid, const ssei &v) {
    const ssei a = select(valid, v, ssei(INT_MIN)); 
    return __bsf(movemask(valid & (a == vreduce_max(a)))); 
}
