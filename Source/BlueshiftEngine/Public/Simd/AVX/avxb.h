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

struct avxb {
    union {
        __m256      m256;
        int32_t     pi32[8];
    };

    BE_FORCE_INLINE avxb() {}
    BE_FORCE_INLINE avxb(const avxb &other) { m256 = other.m256; }
    BE_FORCE_INLINE avxb(const __m256 a) { m256 = a; }
    BE_FORCE_INLINE explicit avxb(const sseb &a) { m256 = _mm256_insertf128_ps(_mm256_castps128_ps256(a), a, 1); }
    BE_FORCE_INLINE explicit avxb(const sseb &a, const sseb &b) { m256 = _mm256_insertf128_ps(_mm256_castps128_ps256(a), b, 1); }
    BE_FORCE_INLINE explicit avxb(bool a) { m256 = avxb(sseb(a), sseb(a)); }
    BE_FORCE_INLINE explicit avxb(bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h) { m256 = avxb(sseb(a, b, c, d), sseb(e, f, g, h)); }

    BE_FORCE_INLINE operator const __m256 &() const { return m256; }
    BE_FORCE_INLINE operator const __m256i() const { return _mm256_castps_si256(m256); }
    BE_FORCE_INLINE operator const __m256d() const { return _mm256_castps_pd(m256); }

    BE_FORCE_INLINE bool operator[](const size_t i) const { assert(i < 8); return (_mm256_movemask_ps(m256) >> i) & 1; }
    BE_FORCE_INLINE int32_t &operator[](const size_t i) { assert(i < 8); return pi32[i]; }
};

//-------------------------------------------------------------
// Unary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const avxb operator!(const avxb &a) { return _mm256_xor_ps(a, avxb(true)); }

//-------------------------------------------------------------
// Binary Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const avxb operator&(const avxb &a, const avxb &b) { return _mm256_and_ps(a, b); }
BE_FORCE_INLINE const avxb operator|(const avxb &a, const avxb &b) { return _mm256_or_ps(a, b); }
BE_FORCE_INLINE const avxb operator^(const avxb &a, const avxb &b) { return _mm256_xor_ps(a, b); }

//-------------------------------------------------------------
// Assignment Operators
//-------------------------------------------------------------

BE_FORCE_INLINE const avxb operator&=(avxb &a, const avxb &b) { return a = a & b; }
BE_FORCE_INLINE const avxb operator|=(avxb &a, const avxb &b) { return a = a | b; }
BE_FORCE_INLINE const avxb operator^=(avxb &a, const avxb &b) { return a = a ^ b; }

//-------------------------------------------------------------
// Comparision Operators + Select
//-------------------------------------------------------------

BE_FORCE_INLINE const avxb operator!=(const avxb &a, const avxb &b) { return _mm256_xor_ps(a, b); }
BE_FORCE_INLINE const avxb operator==(const avxb &a, const avxb &b) { return _mm256_xor_ps(_mm256_xor_ps(a, b), avxb(true)); }

BE_FORCE_INLINE const avxb select(const avxb &mask, const avxb &t, const avxb &f) {
    return _mm256_blendv_ps(f, t, mask);
}

//-------------------------------------------------------------
// Movement/Shifting/Shuffling
//-------------------------------------------------------------

BE_FORCE_INLINE const avxb unpacklo(const avxb &a, const avxb &b) { return _mm256_unpacklo_ps(a, b); }
BE_FORCE_INLINE const avxb unpackhi(const avxb &a, const avxb &b) { return _mm256_unpackhi_ps(a, b); }

// 한개의 2 packed 128 bit operand 에 대한 shuffle.
template<size_t i0, size_t i1> BE_FORCE_INLINE const avxb shuffle(const avxb &a) {
    return _mm256_permute2f128_ps(a, a, (i1 << 4) | (i0));
}

// 두개의 2 packed 128 bit operand 에 대한 shuffle.
template<size_t i0, size_t i1> BE_FORCE_INLINE const avxb shuffle(const avxb &a, const avxb &b) {
    return _mm256_permute2f128_ps(a, b, (i1 << 4) | (i0));
}

// 한개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0> BE_FORCE_INLINE const avxb shuffle(const avxb &a) {
    return _mm256_permute_ps(a, _MM_SHUFFLE(i0, i0, i0, i0));
}

// 한개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const avxb shuffle(const avxb &b) {
    return _mm256_permute_ps(b, _MM_SHUFFLE(i3, i2, i1, i0));
}

// 두개의 8 packed 32 bit operand 에 대한 shuffle. 4 개의 2 bit index 를 이용한다. 상위 128 bit 는 index + 4 로 shuffle 된다.
template<size_t i0, size_t i1, size_t i2, size_t i3> BE_FORCE_INLINE const avxb shuffle(const avxb &a, const avxb &b) {
    return _mm256_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
}

// 특수한 case 의 shuffle 은 간단한 intrinsic 으로 대응.
template<> BE_FORCE_INLINE const avxb shuffle<0, 0, 2, 2>(const avxb &b) { return _mm256_moveldup_ps(b); }
template<> BE_FORCE_INLINE const avxb shuffle<1, 1, 3, 3>(const avxb &b) { return _mm256_movehdup_ps(b); }
template<> BE_FORCE_INLINE const avxb shuffle<0, 1, 0, 1>(const avxb &b) { return _mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(b))); }

// 128 bit float 값을 하위/상위 128 bit 에 insert.
template<size_t i> BE_FORCE_INLINE const avxb insert(const avxb &a, const sseb &b) { return _mm256_insertf128_ps(a, b, i); }

// 256 bit float 에서 하위/상위 128 bit 를 extract.
template<size_t i> BE_FORCE_INLINE const sseb extract(const avxb &a) { return _mm256_extractf128_ps(a, i); }

//-------------------------------------------------------------
// Reduction Operations
//-------------------------------------------------------------

#if defined(__USE_POP_COUNT__)
BE_FORCE_INLINE size_t popcnt(const avxb &a) { return __popcnt(_mm256_movemask_ps(a)); }
#else
BE_FORCE_INLINE size_t popcnt(const avxb &a) { return bool(a[0]) + bool(a[1]) + bool(a[2]) + bool(a[3]) + bool(a[4]) + bool(a[5]) + bool(a[6]) + bool(a[7]); }
#endif

BE_FORCE_INLINE bool reduce_and(const avxb &a) { return _mm256_movemask_ps(a) == 0xf; }
BE_FORCE_INLINE bool reduce_or(const avxb &a) { return !_mm256_testz_ps(a, a); }
BE_FORCE_INLINE bool all(const avxb &b) { return _mm256_movemask_ps(b) == 0xf; }
BE_FORCE_INLINE bool any(const avxb &b) { return !_mm256_testz_ps(b, b); }
BE_FORCE_INLINE bool none(const avxb &b) { return _mm256_testz_ps(b, b) != 0; }

BE_FORCE_INLINE size_t movemask(const avxb &a) { return _mm256_movemask_ps(a); }
