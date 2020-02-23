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

#if defined(__AVX__)
#include <immintrin.h>
#else
#include "../immintrin_emu.h"
#endif

// Negates 4x32 bits floats.
#define _mm256_neg_ps(a)                _mm256_xor_ps(a, _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)))

// dst = a * b + c
#ifdef __FMA__
    #define _mm256_madd_ps(a, b, c)     _mm256_fmadd_ps(a, b, c)
#else
    #define _mm256_madd_ps(a, b, c)     _mm256_add_ps(_mm256_mul_ps(a, b), c)
#endif

// dst = -(a * b) + c
#ifdef __FMA__
    #define _mm256_nmadd_ps(a, b, c)    _mm256_fnmadd_ps(a, b, c)
#else
    #define _mm256_nmadd_ps(a, b, c)    _mm256_sub_ps(c, _mm256_mul_ps(a, b))
#endif

// dst = a * b - c
#ifdef __FMA__
    #define _mm256_msub_ps(a, b, c)     _mm256_fmsub_ps(a, b, c)
#else
    #define _mm256_msub_ps(a, b, c)     _mm256_sub_ps(_mm256_mul_ps(a, b), c)
#endif

// dst = -(a * b) - c
#ifdef __FMA__
    #define _mm256_nmsub_ps(a, b, c)    _mm256_fnmsub_ps(a, b, c)
#else
    #define _mm256_nmsub_ps(a, b, c)    _mm256_sub_ps(_mm256_neg_ps(_mm256_mul_ps(a, b)), c)
#endif

// 8 wide AVX float type.
struct avxf {
    union {
        __m256 m256;
        __m256i m256i;
        union {
            struct { __m128 l128; __m128 h128; };
            struct { __m128i l128i; __m128i h128i; };
        };
        float f32[8];
        int32_t i32[8];
    };

    BE_FORCE_INLINE avxf() = default;
    BE_FORCE_INLINE avxf(const __m256 a) { m256 = a; }
    BE_FORCE_INLINE avxf(const __m256i a) { m256i = a; }
    BE_FORCE_INLINE avxf(const __m128 &a, const __m128 &b) { l128 = a; h128 = b; }
    BE_FORCE_INLINE avxf(const __m128i &a, const __m128i &b) { l128i = a; h128i = b; }
    BE_FORCE_INLINE avxf(float a, float b, float c, float d, float e, float f, float g, float h) { m256 = _mm256_set_ps(h, g, f, e, d, c, b, a); }

    BE_FORCE_INLINE avxf(const avxf &other) { m256 = other.m256; }
    BE_FORCE_INLINE avxf &operator=(const avxf &rhs) { m256 = rhs.m256; return *this; }

    BE_FORCE_INLINE operator const __m256 &() const { return m256; };
    BE_FORCE_INLINE operator __m256 &() { return m256; };

    BE_FORCE_INLINE operator const __m256i &() const { return m256i; };
    BE_FORCE_INLINE operator __m256i &() { return m256i; };

    BE_FORCE_INLINE const float &operator[](const size_t index) const { assert(index < 8); return f32[index]; }
    BE_FORCE_INLINE float &operator[](const size_t index) { assert(index < 8); return f32[index]; }
};

// 8 wide AVX integer type.
struct avxi {
    union {
        __m256 m256;
        __m256i m256i;
        union {
            struct { __m128 l128; __m128 h128; };
            struct { __m128i l128i; __m128i h128i; };
        };
        int32_t i32[8];
    };

    BE_FORCE_INLINE avxi() = default;
    BE_FORCE_INLINE avxi(const __m256 a) { m256 = a; }
    BE_FORCE_INLINE avxi(const __m256i a) { m256i = a; }
    BE_FORCE_INLINE avxi(const __m128 &a, const __m128 &b) { l128 = a; h128 = b; }
    BE_FORCE_INLINE avxi(const __m128i &a, const __m128i &b) { l128i = a; h128i = b; }
    BE_FORCE_INLINE avxi(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g, int32_t h) { m256i = _mm256_set_epi32(h, g, f, e, d, c, b, a); }

    BE_FORCE_INLINE avxi(const avxi &other) { m256i = other.m256i; }
    BE_FORCE_INLINE avxi &operator=(const avxi &rhs) { m256i = rhs.m256i; return *this; }

    BE_FORCE_INLINE operator const __m256 &() const { return m256; };
    BE_FORCE_INLINE operator __m256 &() { return m256; };

    BE_FORCE_INLINE operator const __m256i &() const { return m256i; };
    BE_FORCE_INLINE operator __m256i &() { return m256i; };

    BE_FORCE_INLINE const int32_t &operator[](const size_t index) const { assert(index < 8); return i32[index]; }
    BE_FORCE_INLINE int32_t &operator[](const size_t index) { assert(index < 8); return i32[index]; }
};

// 8 wide AVX boolean type.
struct avxb {
    union {
        __m256 m256;
        __m256i m256i;
        union {
            struct { __m128i l128i; __m128i h128i; };
            struct { __m128 l128; __m128  h128; };
        };
        float f32[8];
        int32_t i32[8];
    };

    BE_FORCE_INLINE avxb() = default;
    BE_FORCE_INLINE avxb(const __m256 a) { m256 = a; }
    BE_FORCE_INLINE avxb(const __m256i a) { m256i = a; }
    BE_FORCE_INLINE avxb(const __m128 &a, const __m128 &b) { l128 = a; h128 = b; }
    BE_FORCE_INLINE avxb(const __m128i &a, const __m128i &b) { l128i = a; h128i = b; }
    BE_FORCE_INLINE avxb(bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h) { l128i = sseb(a, b, c, d); h128i = sseb(e, f, g, h); }

    BE_FORCE_INLINE avxb(const avxb &other) { m256 = other.m256; }
    BE_FORCE_INLINE avxb &operator=(const avxb &rhs) { m256 = rhs.m256; return *this; }

    BE_FORCE_INLINE operator const __m256 &() const { return m256; };
    BE_FORCE_INLINE operator __m256 &() { return m256; };

    BE_FORCE_INLINE operator const __m256i &() const { return m256i; };
    BE_FORCE_INLINE operator __m256i &() { return m256i; };

    BE_FORCE_INLINE bool operator[](const size_t index) const { assert(index < 8); return (_mm256_movemask_ps(m256) >> index) & 1; }
    BE_FORCE_INLINE int32_t &operator[](const size_t index) { assert(index < 8); return i32[index]; }
};

using simd8f = avxf;
using simd8i = avxi;
using simd8b = avxb;

#include "avxb.h"
#include "avxf.h"
#include "avxi.h"
