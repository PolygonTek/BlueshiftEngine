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

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __SSE3__
#include <pmmintrin.h>
#endif

#ifdef __SSSE3__
#include <tmmintrin.h>
#endif

#if defined(__SSE4_1__) || defined(__SSE4_2__)
#include <smmintrin.h>
#else
#include "../smmintrin_emu.h"
#endif

#if defined(__AES__) || defined(__PCLMUL__)
#include <wmmintrin.h>
#endif

const __m128 _mm_lookupmask_ps[16] = {
    _mm_castsi128_ps(_mm_set_epi32( 0,  0,  0,  0)),
    _mm_castsi128_ps(_mm_set_epi32( 0,  0,  0, -1)),
    _mm_castsi128_ps(_mm_set_epi32( 0,  0, -1,  0)),
    _mm_castsi128_ps(_mm_set_epi32( 0,  0, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32( 0, -1,  0,  0)),
    _mm_castsi128_ps(_mm_set_epi32( 0, -1,  0, -1)),
    _mm_castsi128_ps(_mm_set_epi32( 0, -1, -1,  0)),
    _mm_castsi128_ps(_mm_set_epi32( 0, -1, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1,  0,  0,  0)),
    _mm_castsi128_ps(_mm_set_epi32(-1,  0,  0, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1,  0, -1,  0)),
    _mm_castsi128_ps(_mm_set_epi32(-1,  0, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1,  0,  0)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1,  0, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1, -1,  0)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1, -1, -1))
};

const __m128d mm_lookupmask_pd[4] = {
    _mm_castsi128_pd(_mm_set_epi32( 0,  0,  0,  0)),
    _mm_castsi128_pd(_mm_set_epi32( 0,  0, -1, -1)),
    _mm_castsi128_pd(_mm_set_epi32(-1, -1,  0,  0)),
    _mm_castsi128_pd(_mm_set_epi32(-1, -1, -1, -1))
};

// Negates 4x32 bits floats.
#define _mm_neg_ps(a)               _mm_xor_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))) // _mm_sub_ps(_mm_setzero_ps(), (a))

// dst = a * b + c
#ifdef __FMA__
    #define _mm_madd_ps(a, b, c)    _mm_fmadd_ps(a, b, c)
#else
    #define _mm_madd_ps(a, b, c)    _mm_add_ps(_mm_mul_ps(a, b), c)
#endif

// dst = -(a * b) + c
#ifdef __FMA__
    #define _mm_nmadd_ps(a, b, c)   _mm_fnmadd_ps(a, b, c)
#else
    #define _mm_nmadd_ps(a, b, c)   _mm_sub_ps(c, _mm_mul_ps(a, b))
#endif

// dst = a * b - c
#ifdef __FMA__
    #define _mm_msub_ps(a, b, c)    _mm_fmsub_ps(a, b, c)
#else
    #define _mm_msub_ps(a, b, c)    _mm_sub_ps(_mm_mul_ps(a, b), c)
#endif

// dst = -(a * b) - c
#ifdef __FMA__
    #define _mm_nmsub_ps(a, b, c)   _mm_fnmsub_ps(a, b, c)
#else
    #define _mm_nmsub_ps(a, b, c)   _mm_sub_ps(_mm_neg_ps(_mm_mul_ps(a, b)), c)
#endif

// Splats 4x32 bits floats using 2 bits selector.
#define _mm_splat_ps(a, i)          _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(i, i, i, i)))

// Permutates 4x32 bits floats using 4x2 bits selector.
#define _mm_perm_ps(a, imm)         _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), imm))

// Shifts load for two 128 bits.
// dst = (a >> imm*8) | (b << (16 - imm)*8)
#define _mm_sld_ps(a, b, imm)       _mm_castsi128_ps(_mm_or_si128(_mm_srli_si128(_mm_castps_si128(a), imm), _mm_slli_si128(_mm_castps_si128(b), 16 - (imm))))

// Shifts load for two 128 bits.
// dst = (a >> imm*8) | (b << (16 - imm)*8)
#define _mm_sld_si128(a, b, imm)    _mm_or_si128(_mm_srli_si128(a, imm), _mm_slli_si128(b, 16 - (imm))))

BE_FORCE_INLINE void prefetchL1(const void *ptr) { _mm_prefetch((const char *)ptr, _MM_HINT_T0); }
BE_FORCE_INLINE void prefetchL2(const void *ptr) { _mm_prefetch((const char *)ptr, _MM_HINT_T1); }
BE_FORCE_INLINE void prefetchL3(const void *ptr) { _mm_prefetch((const char *)ptr, _MM_HINT_T2); }
BE_FORCE_INLINE void prefetchNTA(const void *ptr) { _mm_prefetch((const char *)ptr, _MM_HINT_NTA); }
BE_FORCE_INLINE void sfence() { _mm_sfence(); }

// 4 wide SSE float type.
struct ssef {
    union {
        __m128 m128;
        __m128i m128i;
        float f32[4];
        int32_t i32[4];
    };

    BE_FORCE_INLINE ssef() = default;
    BE_FORCE_INLINE ssef(const __m128 a) { m128 = a; }
    BE_FORCE_INLINE ssef(float a, float b, float c, float d) { m128 = _mm_set_ps(d, c, b, a); }

    BE_FORCE_INLINE ssef(const ssef &other) { m128 = other.m128; }
    BE_FORCE_INLINE ssef &operator=(const ssef &rhs) { m128 = rhs.m128; return *this; }

    BE_FORCE_INLINE operator const __m128 &() const { return m128; };
    BE_FORCE_INLINE operator __m128 &() { return m128; };

    BE_FORCE_INLINE operator const __m128i &() const { return m128i; };
    BE_FORCE_INLINE operator __m128i &() { return m128i; };

    BE_FORCE_INLINE const float &operator[](size_t index) const { assert(index < 4); return f32[index]; }
    BE_FORCE_INLINE float &operator[](size_t index) { assert(index < 4); return f32[index]; }
};

// 4 wide SSE integer type.
struct ssei {
    union {
        __m128 m128;
        __m128i m128i;
        float f32[4];
        int32_t i32[4];
        int16_t i16[8];
        int8_t i8[16];
    };

    BE_FORCE_INLINE ssei() = default;
    BE_FORCE_INLINE ssei(const __m128i a) { m128i = a; }
    BE_FORCE_INLINE ssei(int32_t a, int32_t b, int32_t c, int32_t d) { m128i = _mm_set_epi32(d, c, b, a); }

    BE_FORCE_INLINE ssei(const ssei &other) { m128i = other.m128i; }
    BE_FORCE_INLINE ssei &operator=(const ssei &rhs) { m128i = rhs.m128i; return *this; }

    BE_FORCE_INLINE operator const __m128 &() const { return m128; };
    BE_FORCE_INLINE operator __m128 &() { return m128; };

    BE_FORCE_INLINE operator const __m128i &() const { return m128i; };
    BE_FORCE_INLINE operator __m128i &() { return m128i; };

    BE_FORCE_INLINE const int32_t &operator[](size_t index) const { assert(index < 4); return i32[index]; }
    BE_FORCE_INLINE int32_t &operator[](size_t index) { assert(index < 4); return i32[index]; }
};

// 4 wide SSE boolean type.
struct sseb {
    union {
        __m128 m128;
        __m128i m128i;
        float f32[4];
        int32_t i32[4];
    };

    BE_FORCE_INLINE sseb() = default;
    BE_FORCE_INLINE sseb(const __m128 a) { m128 = a; }
    BE_FORCE_INLINE sseb(const __m128i a) { m128i = a; }
    BE_FORCE_INLINE sseb(bool a, bool b, bool c, bool d) { m128 = _mm_lookupmask_ps[(size_t(d) << 3) | (size_t(c) << 2) | (size_t(b) << 1) | size_t(a)]; }

    BE_FORCE_INLINE sseb(const sseb &other) { m128i = other.m128i; }
    BE_FORCE_INLINE sseb &operator=(const sseb &rhs) { m128 = rhs.m128; return *this; }

    BE_FORCE_INLINE operator const __m128 &() const { return m128; };
    BE_FORCE_INLINE operator __m128 &() { return m128; };

    BE_FORCE_INLINE operator const __m128i &() const { return m128i; };
    BE_FORCE_INLINE operator __m128i &() { return m128i; };

    BE_FORCE_INLINE bool operator[](size_t index) const { assert(index < 4); return (_mm_movemask_ps(m128) >> index) & 1; }
    BE_FORCE_INLINE int32_t &operator[](size_t index) { assert(index < 4); return i32[index]; }
};

using simd4f = ssef;
using simd4i = ssei;
using simd4b = sseb;

#include "sseb.h"
#include "ssef.h"
#include "ssei.h"
