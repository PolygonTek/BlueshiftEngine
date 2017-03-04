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

struct sseb {
    union { 
        __m128      m128;
        int32_t     pi32[4];
    };

    BE_FORCE_INLINE sseb() {}
    BE_FORCE_INLINE sseb(const sseb &other) { m128 = other.m128; }
    BE_FORCE_INLINE sseb(const __m128 a) { m128 = a; }
    BE_FORCE_INLINE explicit sseb(bool a) { m128 = _mm_lookupmask_ps[(size_t(a) << 3) | (size_t(a) << 2) | (size_t(a) << 1) | size_t(a)]; }
    BE_FORCE_INLINE explicit sseb(bool a, bool b, bool c, bool d) { m128 = _mm_lookupmask_ps[(size_t(d) << 3) | (size_t(c) << 2) | (size_t(b) << 1) | size_t(a)]; } 

    BE_FORCE_INLINE operator const __m128 &() const { return m128; }
    BE_FORCE_INLINE operator const __m128i() const { return _mm_castps_si128(m128); }
    BE_FORCE_INLINE operator const __m128d() const { return _mm_castps_pd(m128); }

    BE_FORCE_INLINE bool operator[](const size_t i) const { assert(i < 4); return (_mm_movemask_ps(m128) >> i) & 1; }
    BE_FORCE_INLINE int32_t &operator[](const size_t i) { assert(i < 4); return pi32[i]; }
};

//-------------------------------------------------------------
// Unary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const sseb operator!(const sseb &a) { return _mm_xor_ps(a, sseb(true)); }

//-------------------------------------------------------------
// Binary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const sseb operator&(const sseb &a, const sseb &b) { return _mm_and_ps(a, b); }
BE_FORCE_INLINE const sseb operator|(const sseb &a, const sseb &b) { return _mm_or_ps(a, b); }
BE_FORCE_INLINE const sseb operator^(const sseb &a, const sseb &b) { return _mm_xor_ps(a, b); }

//-------------------------------------------------------------
// Assignment Operators
//-------------------------------------------------------------
  
BE_FORCE_INLINE const sseb operator&=(sseb &a, const sseb &b) { return a = a & b; }
BE_FORCE_INLINE const sseb operator|=(sseb &a, const sseb &b) { return a = a | b; }
BE_FORCE_INLINE const sseb operator^=(sseb &a, const sseb &b) { return a = a ^ b; }

//-------------------------------------------------------------
// Comparision Operators + Select
//-------------------------------------------------------------
  
BE_FORCE_INLINE const sseb operator!=(const sseb &a, const sseb &b) { return _mm_xor_ps(a, b); }
BE_FORCE_INLINE const sseb operator==(const sseb &a, const sseb &b) { return _mm_castsi128_ps(_mm_cmpeq_epi32(a, b)); }
  
// workaround for compiler bug in VS2008
#if defined(_MSC_VER) && (_MSC_VER < 1600)
BE_FORCE_INLINE const sseb select(const sseb &mask, const sseb &a, const sseb &b) {
    return _mm_or_ps(_mm_and_ps(mask, a), _mm_andnot_ps(mask, b));
}
#else
BE_FORCE_INLINE const sseb select(const sseb &mask, const sseb &t, const sseb &f) {
    return _mm_blendv_ps(f, t, mask);
}
#endif

//-------------------------------------------------------------
// Movement/Shifting/Shuffling
//-------------------------------------------------------------

BE_FORCE_INLINE const sseb unpacklo(const sseb &a, const sseb &b) { return _mm_unpacklo_ps(a, b); }
BE_FORCE_INLINE const sseb unpackhi(const sseb &a, const sseb &b) { return _mm_unpackhi_ps(a, b); }

// 한개의 4 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다.
template <size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const sseb shuffle(const sseb &a) {
    return _mm_shuffle_epi32(a, _MM_SHUFFLE(i3, i2, i1, i0));
}

// 두개의 4 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다.
template <size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const sseb shuffle(const sseb &a, const sseb &b) {
    return _mm_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
}

// 특수한 case 의 shuffle 은 간단한 intrinsic 으로 대응.
template<> BE_FORCE_INLINE const sseb shuffle<0, 0, 2, 2>(const sseb &a) { return _mm_moveldup_ps(a); }
template<> BE_FORCE_INLINE const sseb shuffle<1, 1, 3, 3>(const sseb &a) { return _mm_movehdup_ps(a); }
template<> BE_FORCE_INLINE const sseb shuffle<0, 1, 0, 1>(const sseb &a) { return _mm_castpd_ps(_mm_movedup_pd (a)); }

// 2-bit index 를 사용해서 src 에서 dst 로 insert 한다. 4 bit clr 로 0 으로 채우기 가능. (SSE4.1)
template <size_t dst, size_t src, size_t clr> BE_FORCE_INLINE const sseb insert(const sseb &a, const sseb &b) { return _mm_insert_ps(a, b, (dst << 4) | (src << 6) | clr); }
template <size_t dst, size_t src> BE_FORCE_INLINE const sseb insert(const sseb &a, const sseb &b) { return insert<dst, src, 0>(a, b); }
template <size_t dst> BE_FORCE_INLINE const sseb insert(const sseb &a, const bool b) { return insert<dst, 0>(a, sseb(b)); }

//-------------------------------------------------------------
// Reduction Operations
//-------------------------------------------------------------

BE_FORCE_INLINE size_t movemask(const sseb &a) { return _mm_movemask_ps(a); }

// true 테스트 함수 (비교 후 연산 결과 테스트)
BE_FORCE_INLINE bool reduce_and(const sseb &a) { return _mm_movemask_ps(a) == 0xf; }
BE_FORCE_INLINE bool reduce_or(const sseb &a) { return _mm_movemask_ps(a) != 0x0; }
BE_FORCE_INLINE bool all(const sseb &b) { return _mm_movemask_ps(b) == 0xf; }
BE_FORCE_INLINE bool any(const sseb &b) { return _mm_movemask_ps(b) != 0x0; }
BE_FORCE_INLINE bool none(const sseb &b) { return _mm_movemask_ps(b) == 0x0; }

// true 개수 리턴
#if defined(__USE_POP_COUNT__)
BE_FORCE_INLINE size_t popcnt(const sseb &a) { return __popcnt(_mm_movemask_ps(a)); }
#else
BE_FORCE_INLINE size_t popcnt(const sseb &a) { return bool(a[0]) + bool(a[1]) + bool(a[2]) + bool(a[3]); }
#endif
