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

// Multiplication first and add last. dst = a * b + c
#define _mm_madd_ps(a, b, c)            _mm_add_ps(_mm_mul_ps((a), (b)), (c))

// Multiplication first and subtract from last. dst = c - a * b
#define _mm_nmsub_ps(a, b, c)           _mm_sub_ps((c), _mm_mul_ps((a), (b)))

// Splating 4x32 bits floats using 2 bits selector.
#define _mm_splat_ps(a, i)              _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(i, i, i, i)))

// Permutation 4x32 bits floats using 4x2 bits selector.
#define _mm_perm_ps(a, imm)             _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), imm))

// Selection 4x32 bits floats using mask.
#if defined(__SSE4_1__)
    // SSE4.1
    #define _mm_sel_ps(a, b, mask)      _mm_blendv_ps(a, b, mask);
#else
    // dst = (a & !mask) | (b & mask)
    #define _mm_sel_ps(a, b, mask)      _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(mask), a), _mm_and_ps(_mm_castsi128_ps(mask), b))
#endif

// Selection 4x32 bits integer using mask.
#if defined(__SSE4_1__)
    // SSE4.1
    #define _mm_sel_si128(a, b, mask)   _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), mask));
#else
    // dst = (a & !mask) | (b & mask)
    #define _mm_sel_si128(a, b, mask)   _mm_or_si128(_mm_andnot_si128(_mm_castps_si128(mask), a), _mm_and_si128(_mm_castps_si128(mask), b))
#endif

// Shift load for two 128 bits.
// dst = (a >> imm*8) | (b << (16 - imm)*8)
#define _mm_sld_ps(a, b, imm)           _mm_castsi128_ps(_mm_or_si128(_mm_srli_si128(_mm_castps_si128(a), imm), _mm_slli_si128(_mm_castps_si128(b), 16 - imm)))

// Shift load for two 128 bits.
// dst = (a >> imm*8) | (b << (16 - imm)*8)
#define _mm_sld_si128(a, b, imm)        _mm_or_si128(_mm_srli_si128(a, imm), _mm_slli_si128(b, 16 - imm))

// Floating-point reciprocal with at least 16 bits precision.
BE_FORCE_INLINE __m128 _mm_rcp16_ps(__m128 x) {
    __m128 r = _mm_rcp_ps(x); // _mm_rcp_ps() has 12 bits of precision.
    r = _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(x, r), r));
    return r;
}

// Floating-point reciprocal with close to full precision.
BE_FORCE_INLINE __m128 _mm_rcp32_ps(__m128 x) {
    __m128 r = _mm_rcp_ps(x); // _mm_rcp_ps() has 12 bits of precision.
    // Newton-Raphson approximation.
    r = _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(x, r), r));
    r = _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(x, r), r));
    return r;
}

// Floating-point divide with at least 16 bits precision.
BE_FORCE_INLINE __m128 _mm_div16_ps(__m128 x, __m128 y) {
    return _mm_mul_ps(x, _mm_rcp16_ps(y));
}

// Floating-point divide with close to full precision.
BE_FORCE_INLINE __m128 _mm_div32_ps(__m128 x, __m128 y) {
    return _mm_mul_ps(x, _mm_rcp32_ps(y));
}

const __m128 _mm_lookupmask_ps[16] = {
    _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, 0)),
    _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, -1)),
    _mm_castsi128_ps(_mm_set_epi32(0, 0, -1, 0)),
    _mm_castsi128_ps(_mm_set_epi32(0, 0, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32(0, -1, 0, 0)),
    _mm_castsi128_ps(_mm_set_epi32(0, -1, 0, -1)),
    _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, 0)),
    _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1, 0, 0, 0)),
    _mm_castsi128_ps(_mm_set_epi32(-1, 0, 0, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1, 0, -1, 0)),
    _mm_castsi128_ps(_mm_set_epi32(-1, 0, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1, 0, 0)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1, 0, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1, -1, 0)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1, -1, -1))
};

#include "sseb.h"
#include "ssei.h"
#include "ssef.h"

BE_FORCE_INLINE void prefetchL1(const void *ptr) { _mm_prefetch((const char *)ptr, _MM_HINT_T0); }
BE_FORCE_INLINE void prefetchL2(const void *ptr) { _mm_prefetch((const char *)ptr, _MM_HINT_T1); }
BE_FORCE_INLINE void prefetchL3(const void *ptr) { _mm_prefetch((const char *)ptr, _MM_HINT_T2); }
BE_FORCE_INLINE void prefetchNTA(const void *ptr) { _mm_prefetch((const char *)ptr, _MM_HINT_NTA); }
