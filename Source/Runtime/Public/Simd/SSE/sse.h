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

// Negates 4x32 bits floats.
#define _mm_neg_ps(a)                   _mm_sub_ps(_mm_setzero_ps(), (a))

// dst = a * b + c
#ifdef __FMA__
    #define _mm_madd_ps(a, b, c)        _mm_fmadd_ps(a, b, c)
#else
    #define _mm_madd_ps(a, b, c)        _mm_add_ps(_mm_mul_ps((a), (b)), (c))
#endif

// dst = -(a * b) + c
#ifdef __FMA__
    #define _mm_nmadd_ps(a, b, c)       _mm_fnmadd_ps(a, b, c)
#else
    #define _mm_nmadd_ps(a, b, c)       _mm_sub_ps((c), _mm_mul_ps((a), (b)))
#endif

// dst = a * b - c
#ifdef __FMA__
    #define _mm_msub_ps(a, b, c)        _mm_fmsub_ps(a, b, c)
#else
    #define _mm_msub_ps(a, b, c)        _mm_sub_ps(_mm_mul_ps((a), (b)), (c))
#endif

// dst = -(a * b) - c
#ifdef __FMA__
    #define _mm_nmsub_ps(a, b, c)       _mm_fnmsub_ps(a, b, c)
#else
    #define _mm_nmsub_ps(a, b, c)       _mm_sub_ps(__mm_neg_ps(_mm_mul_ps((a), (b))), (c))
#endif

// Splats 4x32 bits floats using 2 bits selector.
#define _mm_splat_ps(a, i)              _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(i, i, i, i)))

// Permutates 4x32 bits floats using 4x2 bits selector.
#define _mm_perm_ps(a, imm)             _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), imm))

// Returns the given channel as a float.
#define _mm_x(a)                        _mm_cvtss_f32((a))
#define _mm_y(a)                        _mm_cvtss_f32(_mm_splat_ps(a, 1))
#define _mm_z(a)                        _mm_cvtss_f32(_mm_unpackhi_ps((a), (a)))
#define _mm_w(a)                        _mm_cvtss_f32(_mm_splat_ps(a, 3))

// Selects 4x32 bits floats using mask.
#if defined(__SSE4_1__)
    // SSE4.1
    #define _mm_sel_ps(a, b, mask)      _mm_blendv_ps((a), (b), (mask));
#else
    // dst = (a & !mask) | (b & mask)
    #define _mm_sel_ps(a, b, mask)      _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(mask), (a)), _mm_and_ps(_mm_castsi128_ps(mask), (b)))
#endif

// Selects 4x32 bits integer using mask.
#if defined(__SSE4_1__)
    // SSE4.1
    #define _mm_sel_si128(a, b, mask)   _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), (mask)));
#else
    // dst = (a & !mask) | (b & mask)
    #define _mm_sel_si128(a, b, mask)   _mm_or_si128(_mm_andnot_si128(_mm_castps_si128(mask), (a)), _mm_and_si128(_mm_castps_si128(mask), (b)))
#endif

// Shifts load for two 128 bits.
// dst = (a >> imm*8) | (b << (16 - imm)*8)
#define _mm_sld_ps(a, b, imm)           _mm_castsi128_ps(_mm_or_si128(_mm_srli_si128(_mm_castps_si128(a), (imm)), _mm_slli_si128(_mm_castps_si128(b), 16 - (imm))))

// Shifts load for two 128 bits.
// dst = (a >> imm*8) | (b << (16 - imm)*8)
#define _mm_sld_si128(a, b, imm)        _mm_or_si128(_mm_srli_si128((a), (imm)), _mm_slli_si128((b), 16 - (imm))))

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

// Shuffles 4x32 bits floats using template parameters. x(0), y(1), z(2), w(3).
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE __m128 _mm_shuffle_ps(__m128 a) { return _mm_perm_ps(a, _MM_SHUFFLE(i3, i2, i1, i0)); }

template <> BE_FORCE_INLINE __m128 _mm_shuffle_ps<0, 0, 0, 0>(__m128 a) { return _mm_splat_ps(a, 0); }
template <> BE_FORCE_INLINE __m128 _mm_shuffle_ps<1, 1, 1, 1>(__m128 a) { return _mm_splat_ps(a, 1); }
template <> BE_FORCE_INLINE __m128 _mm_shuffle_ps<2, 2, 2, 2>(__m128 a) { return _mm_splat_ps(a, 2); }
template <> BE_FORCE_INLINE __m128 _mm_shuffle_ps<3, 3, 3, 3>(__m128 a) { return _mm_splat_ps(a, 3); }
template <> BE_FORCE_INLINE __m128 _mm_shuffle_ps<0, 1, 0, 1>(__m128 a) { return _mm_movelh_ps(a, a); }
template <> BE_FORCE_INLINE __m128 _mm_shuffle_ps<2, 3, 2, 3>(__m128 a) { return _mm_movehl_ps(a, a); }

#ifdef __SSE3__
// _mm_moveldup_ps and _mm_movehdup_ps are better than shuffle, since they don't destroy the input operands (under non-AVX).
template <> BE_FORCE_INLINE __m128 _mm_shuffle_ps<0, 0, 2, 2>(__m128 a) { return _mm_movehdup_ps(a); }
template <> BE_FORCE_INLINE __m128 _mm_shuffle_ps<1, 1, 3, 3>(__m128 a) { return _mm_moveldup_ps(a); }
#endif

// Shuffles two 4x32 bits floats using template parameters. x(0), y(1), z(2), w(3).
template <size_t a0, size_t a1, size_t b0, size_t b1>
BE_FORCE_INLINE __m128 _mm_shuffle_ps(__m128 a, __m128 b) { return _mm_shuffle_ps(a, b, _MM_SHUFFLE(b1, b0, a1, a0)); }

// Shuffles 4x32 bits integers using template parameters. x(0), y(1), z(2), w(3).
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE __m128i _mm_shuffle_si128(__m128i a) { return _mm_shuffle_epi32(a, _MM_SHUFFLE(i3, i2, i1, i0)); }

template<> BE_FORCE_INLINE __m128i _mm_shuffle_si128<0, 0, 2, 2>(__m128i a) { return _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(a))); }
template<> BE_FORCE_INLINE __m128i _mm_shuffle_si128<1, 1, 3, 3>(__m128i a) { return _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(a))); }
template<> BE_FORCE_INLINE __m128i _mm_shuffle_si128<0, 1, 0, 1>(__m128i a) { return _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(a))); }

// Shuffles two 4x32 bits integers using template parameters. x(0), y(1), z(2), w(3).
template <size_t a0, size_t a1, size_t b0, size_t b1>
BE_FORCE_INLINE __m128i shuffle(__m128i a, __m128i b) {
    return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _MM_SHUFFLE(b1, b0, a1, a0)));
}

// Sums up all components.
BE_FORCE_INLINE __m128 _mm_sum_ps(__m128 a) {
    a = _mm_hadd_ps(a, a); // (x + y, z + w, x + y, z + w)
    return _mm_hadd_ps(a, a); // (x + y + z + w, x + y + z + w, x + y + z + w, x + y + z + w)
}

// Dot product.
BE_FORCE_INLINE float _mm_dot4_ps(__m128 a, __m128 b) {
    __m128 mul = _mm_mul_ps(a, b);
    return _mm_x(_mm_sum_ps(mul));
}

// Cross product.
BE_FORCE_INLINE __m128 _mm_cross_ps(__m128 a, __m128 b) {
    __m128 a_yzxw = _mm_shuffle_ps<1, 2, 0, 3>(a); // (a.y, a.z, a.x, a.w)
    __m128 b_yzxw = _mm_shuffle_ps<1, 2, 0, 3>(b); // (b.y, b.z, b.x, b.w)
    __m128 ab_yzxw = _mm_mul_ps(a_yzxw, b); // (a.y * b.x, a.z * b.y, a.x * b.z, a.w * b.w)

    return _mm_shuffle_ps<1, 2, 0, 3>(_mm_msub_ps(b_yzxw, a, ab_yzxw)); // (a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, 0)
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
