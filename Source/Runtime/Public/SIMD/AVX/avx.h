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
#include "immintrin_emu.h"
#endif

#ifdef __AVX2__
    #define bx_mm256_add_epi32(a, b)        _mm256_add_epi32(a, b)
    #define bx_mm256_sub_epi32(a, b)        _mm256_sub_epi32(a, b)
    #define bx_mm256_mullo_epi32(a, b)      _mm256_mullo_epi32(a, b)
    #define bx_mm256_min_epi32(a, b)        _mm256_min_epi32(a, b)
    #define bx_mm256_max_epi32(a, b)        _mm256_max_epi32(a, b)
    #define bx_mm256_cmpeq_epi32(a, b)      _mm256_cmpeq_epi32(a, b)
    #define bx_mm256_cmpgt_epi32(a, b)      _mm256_cmpgt_epi32(a, b)
    #define bx_mm256_slli_epi32(a, b)       _mm256_slli_epi32(a, b)
    #define bx_mm256_srli_epi32(a, b)       _mm256_srli_epi32(a, b)
    #define bx_mm256_srai_epi32(a, b)       _mm256_srai_epi32(a, b)
    #define bx_mm256_and_si256(a, b)        _mm256_and_si256(a, b)
    #define bx_mm256_or_si256(a, b)         _mm256_or_si256(a, b)
    #define bx_mm256_xor_si256(a, b)        _mm256_xor_si256(a, b)
    #define bx_mm256_unpacklo_epi32(a, b)   _mm256_unpacklo_epi32(a, b)
    #define bx_mm256_unpackhi_epi32(a, b)   _mm256_unpackhi_epi32(a, b)
#else
    #define bx_mm256_add_epi32(a, b)        _mm256_set_m128i(_mm_add_epi32(_mm256_extractf128_si256(a, 0), _mm256_extractf128_si256(b, 0)), _mm_add_epi32(_mm256_extractf128_si256(a, 1), _mm256_extractf128_si256(b, 1)))
    #define bx_mm256_sub_epi32(a, b)        _mm256_set_m128i(_mm_sub_epi32(_mm256_extractf128_si256(a, 0), _mm256_extractf128_si256(b, 0)), _mm_sub_epi32(_mm256_extractf128_si256(a, 1), _mm256_extractf128_si256(b, 1)))
    #define bx_mm256_mullo_epi32(a, b)      _mm256_set_m128i(_mm_mullo_epi32(_mm256_extractf128_si256(a, 0), _mm256_extractf128_si256(b, 0)), _mm_mullo_epi32(_mm256_extractf128_si256(a, 1), _mm256_extractf128_si256(b, 1)))
    #define bx_mm256_min_epi32(a, b)        _mm256_set_m128i(_mm_min_epi32(_mm256_extractf128_si256(a, 0), _mm256_extractf128_si256(b, 0)), _mm_min_epi32(_mm256_extractf128_si256(a, 1), _mm256_extractf128_si256(b, 1)))
    #define bx_mm256_max_epi32(a, b)        _mm256_set_m128i(_mm_max_epi32(_mm256_extractf128_si256(a, 0), _mm256_extractf128_si256(b, 0)), _mm_max_epi32(_mm256_extractf128_si256(a, 1), _mm256_extractf128_si256(b, 1)))
    #define bx_mm256_cmpeq_epi32(a, b)      _mm256_set_m128i(_mm_cmpeq_epi32(_mm256_extractf128_si256(a, 0), _mm256_extractf128_si256(b, 0)), _mm_cmpeq_epi32(_mm256_extractf128_si256(a, 1), _mm256_extractf128_si256(b, 1)))
    #define bx_mm256_cmpgt_epi32(a, b)      _mm256_set_m128i(_mm_cmpgt_epi32(_mm256_extractf128_si256(a, 0), _mm256_extractf128_si256(b, 0)), _mm_cmpgt_epi32(_mm256_extractf128_si256(a, 1), _mm256_extractf128_si256(b, 1)))
    #define bx_mm256_slli_epi32(a, b)       _mm256_set_m128i(_mm_slli_epi32(_mm256_extractf128_si256(a, 0), b), _mm_slli_epi32(_mm256_extractf128_si256(a, 1), b))
    #define bx_mm256_srli_epi32(a, b)       _mm256_set_m128i(_mm_srli_epi32(_mm256_extractf128_si256(a, 0), b), _mm_srli_epi32(_mm256_extractf128_si256(a, 1), b))
    #define bx_mm256_srai_epi32(a, b)       _mm256_set_m128i(_mm_srai_epi32(_mm256_extractf128_si256(a, 0), b), _mm_srai_epi32(_mm256_extractf128_si256(a, 1), b))
    #define bx_mm256_and_si256(a, b)        _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)))
    #define bx_mm256_or_si256(a, b)         _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)))
    #define bx_mm256_xor_si256(a, b)        _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)))
    #define bx_mm256_unpacklo_epi32(a, b)   _mm256_castps_si256(_mm256_unpacklo_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)))
    #define bx_mm256_unpackhi_epi32(a, b)   _mm256_castps_si256(_mm256_unpackhi_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)))
#endif

// Compares packed 32-bit integers in a and b for less-than.
#define bx_mm256_cmplt_epi32(a, b)          bx_mm256_cmpgt_epi32(b, a)

// Inverts 256 bits.
#define bx_mm256_inv_si256(a)               bx_mm256_xor_si256(a, _mm256_set1_epi32(0xFFFFFFFF))

// Negates 256 bits.
#define bx_mm256_neg_epi32(a)               bx_mm256_xor_si256(a, _mm256_set1_epi32(0x80000000))

// Negates 4x32 bits floats.
#define bx_mm256_neg_ps(a)                  _mm256_xor_ps(a, _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)))

#ifdef __FMA__
    // dst = a * b + c
    #define bx_mm256_madd_ps(a, b, c)       _mm256_fmadd_ps(a, b, c)
    // dst = -(a * b) + c
    #define bx_mm256_nmadd_ps(a, b, c)      _mm256_fnmadd_ps(a, b, c)
    // dst = a * b - c
    #define bx_mm256_msub_ps(a, b, c)       _mm256_fmsub_ps(a, b, c)
    // dst = -(a * b) - c
    #define bx_mm256_nmsub_ps(a, b, c)      _mm256_fnmsub_ps(a, b, c)
#else
    // dst = a * b + c
    #define bx_mm256_madd_ps(a, b, c)       _mm256_add_ps(_mm256_mul_ps(a, b), c)
    // dst = -(a * b) + c
    #define bx_mm256_nmadd_ps(a, b, c)      _mm256_sub_ps(c, _mm256_mul_ps(a, b))
    // dst = a * b - c
    #define bx_mm256_msub_ps(a, b, c)       _mm256_sub_ps(_mm256_mul_ps(a, b), c)
    // dst = -(a * b) - c
    #define bx_mm256_nmsub_ps(a, b, c)      _mm256_sub_ps(bx_mm256_neg_ps(_mm256_mul_ps(a, b)), c)
#endif

struct avxf;
struct avxi;
struct avxb;

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

    BE_FORCE_INLINE operator const avxi &() const { return *reinterpret_cast<const avxi *>(this); };
    BE_FORCE_INLINE operator avxi &() { return *reinterpret_cast<avxi *>(this); };

    BE_FORCE_INLINE operator const avxb &() const { return *reinterpret_cast<const avxb *>(this); };
    BE_FORCE_INLINE operator avxb &() { return *reinterpret_cast<avxb *>(this); };

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

    BE_FORCE_INLINE operator const avxf &() const { return *reinterpret_cast<const avxf *>(this); };
    BE_FORCE_INLINE operator avxf &() { return *reinterpret_cast<avxf *>(this); };

    BE_FORCE_INLINE operator const avxb &() const { return *reinterpret_cast<const avxb *>(this); };
    BE_FORCE_INLINE operator avxb &() { return *reinterpret_cast<avxb *>(this); };

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

    BE_FORCE_INLINE operator const avxf &() const { return *reinterpret_cast<const avxf *>(this); };
    BE_FORCE_INLINE operator avxf &() { return *reinterpret_cast<avxf *>(this); };

    BE_FORCE_INLINE operator const avxi &() const { return *reinterpret_cast<const avxi *>(this); };
    BE_FORCE_INLINE operator avxi &() { return *reinterpret_cast<avxi *>(this); };

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
