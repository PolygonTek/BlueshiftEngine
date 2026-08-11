// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// Derived from Intel Embree's common/simd/smmintrin_emu.h at commit
// 9d5742f7868ca686c71c2625e21d570447401f17.
// Modified for Blueshift Engine to fix integer multiplication, floating-point
// rounding, and 64-bit lane insertion while preserving the SSE2 fallback.

#pragma once

#ifndef __GNUC__
#pragma message (" --- Blueshift remark: SSE4.1 intrinsics are emulated with SSE2 ---")
#endif

#define _MM_FROUND_TO_NEAREST_INT    0x00
#define _MM_FROUND_TO_NEG_INF        0x01
#define _MM_FROUND_TO_POS_INF        0x02
#define _MM_FROUND_TO_ZERO           0x03
#define _MM_FROUND_CUR_DIRECTION     0x04

#define _mm_blendv_ps __emu_mm_blendv_ps
BE_FORCE_INLINE __m128 _mm_blendv_ps(__m128 value, __m128 input, __m128 mask) { 
    const __m128 signMask = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(mask), 31));
    return _mm_or_ps(_mm_and_ps(signMask, input), _mm_andnot_ps(signMask, value));
}

#define _mm_blend_ps __emu_mm_blend_ps
BE_FORCE_INLINE __m128 _mm_blend_ps(__m128 value, __m128 input, const int mask) { 
    assert(mask < 0x10); return _mm_blendv_ps(value, input, _mm_lookupmask_ps[mask]);
}

#define _mm_blendv_epi8 __emu_mm_blendv_epi8
BE_FORCE_INLINE __m128i _mm_blendv_epi8(__m128i value, __m128i input, __m128i mask) { 
    const __m128i signMask = _mm_cmpgt_epi8(_mm_setzero_si128(), mask);
    return _mm_or_si128(_mm_and_si128(signMask, input), _mm_andnot_si128(signMask, value));
}

#define _mm_mullo_epi32 __emu_mm_mullo_epi32
BE_FORCE_INLINE __m128i _mm_mullo_epi32(__m128i value, __m128i input) {
    __m128i evenProducts = _mm_mul_epu32(value, input);
    __m128i oddProducts = _mm_mul_epu32(_mm_srli_si128(value, 4), _mm_srli_si128(input, 4));
    __m128i lowProducts = _mm_unpacklo_epi32(evenProducts, oddProducts);
    __m128i highProducts = _mm_unpackhi_epi32(evenProducts, oddProducts);
    return _mm_unpacklo_epi64(lowProducts, highProducts);
}

#define _mm_min_epi32 __emu_mm_min_epi32
BE_FORCE_INLINE __m128i _mm_min_epi32(__m128i value, __m128i input) { 
    return _mm_blendv_epi8(input, value, _mm_cmplt_epi32(value, input));
}

#define _mm_max_epi32 __emu_mm_max_epi32
BE_FORCE_INLINE __m128i _mm_max_epi32(__m128i value, __m128i input) { 
    return _mm_blendv_epi8(value, input, _mm_cmplt_epi32(value, input));
}

#define _mm_extract_epi32 __emu_mm_extract_epi32
BE_FORCE_INLINE int _mm_extract_epi32(__m128i input, const int index) {
    switch (index) {
    case 0: return _mm_cvtsi128_si32(input);
    case 1: return _mm_cvtsi128_si32(_mm_shuffle_epi32(input, _MM_SHUFFLE(1, 1, 1, 1)));
    case 2: return _mm_cvtsi128_si32(_mm_shuffle_epi32(input, _MM_SHUFFLE(2, 2, 2, 2)));
    case 3: return _mm_cvtsi128_si32(_mm_shuffle_epi32(input, _MM_SHUFFLE(3, 3, 3, 3)));
    default: assert(false); return 0;
  }
}

#define _mm_insert_epi32 __emu_mm_insert_epi32
BE_FORCE_INLINE __m128i _mm_insert_epi32(__m128i value, int input, const int index) { 
    assert(index >= 0 && index < 4); ((int *)&value)[index] = input; return value;
}

#define _mm_extract_ps __emu_mm_extract_ps
BE_FORCE_INLINE int _mm_extract_ps(__m128 input, const int index) {
    int32_t* ptr = (int32_t *)&input; return ptr[index];
}

#define _mm_insert_ps __emu_mm_insert_ps
BE_FORCE_INLINE __m128 _mm_insert_ps(__m128 value, __m128 input, const int index) { 
    assert(index < 0x100); 
    ((float *)&value)[(index >> 4) & 0x3] = ((float *)&input)[index >> 6]; 
    return _mm_andnot_ps(_mm_lookupmask_ps[index & 0xf], value); 
}

#define _mm_round_ps __emu_mm_round_ps
BE_FORCE_INLINE __m128 _mm_round_ps(__m128 value, const int flags) {
    const __m128 zero = _mm_setzero_ps();
    const __m128 signMask = _mm_set1_ps(-0.0f);
    const __m128 absValue = _mm_andnot_ps(signMask, value);
    __m128 passthroughMask = _mm_cmpge_ps(absValue, _mm_set1_ps(8388608.0f));
    passthroughMask = _mm_or_ps(passthroughMask, _mm_cmpunord_ps(value, value));
    passthroughMask = _mm_or_ps(passthroughMask, _mm_cmpeq_ps(absValue, zero));

    const __m128 safeValue = _mm_andnot_ps(passthroughMask, value);
    const __m128i truncatedInt = _mm_cvttps_epi32(safeValue);
    const __m128 truncatedValue = _mm_cvtepi32_ps(truncatedInt);
    __m128 roundedValue;

    switch (flags & 0x7) {
    case _MM_FROUND_TO_NEAREST_INT: {
        const __m128 fraction = _mm_sub_ps(safeValue, truncatedValue);
        const __m128 absFraction = _mm_andnot_ps(signMask, fraction);
        const __m128 greaterThanHalf = _mm_cmpgt_ps(absFraction, _mm_set1_ps(0.5f));
        const __m128 equalToHalf = _mm_cmpeq_ps(absFraction, _mm_set1_ps(0.5f));
        const __m128i oddMask = _mm_cmpeq_epi32(_mm_and_si128(truncatedInt, _mm_set1_epi32(1)), _mm_set1_epi32(1));
        const __m128 adjustMask = _mm_or_ps(greaterThanHalf, _mm_and_ps(equalToHalf, _mm_castsi128_ps(oddMask)));
        const __m128i sign = _mm_srai_epi32(_mm_castps_si128(safeValue), 31);
        const __m128i unit = _mm_or_si128(sign, _mm_set1_epi32(1));
        const __m128i roundedInt = _mm_add_epi32(truncatedInt, _mm_and_si128(_mm_castps_si128(adjustMask), unit));
        roundedValue = _mm_cvtepi32_ps(roundedInt);
        break;
    }
    case _MM_FROUND_TO_NEG_INF:
        roundedValue = _mm_sub_ps(truncatedValue, _mm_and_ps(_mm_cmplt_ps(safeValue, truncatedValue), _mm_set1_ps(1.0f)));
        break;
    case _MM_FROUND_TO_POS_INF:
        roundedValue = _mm_add_ps(truncatedValue, _mm_and_ps(_mm_cmpgt_ps(safeValue, truncatedValue), _mm_set1_ps(1.0f)));
        break;
    case _MM_FROUND_TO_ZERO:
        roundedValue = truncatedValue;
        break;
    case _MM_FROUND_CUR_DIRECTION:
        roundedValue = _mm_cvtepi32_ps(_mm_cvtps_epi32(safeValue));
        break;
    default:
        assert(false);
        return value;
    }

    const __m128 roundedZeroMask = _mm_cmpeq_ps(roundedValue, zero);
    roundedValue = _mm_or_ps(roundedValue, _mm_and_ps(roundedZeroMask, _mm_and_ps(value, signMask)));
    return _mm_blendv_ps(roundedValue, value, passthroughMask);
}

#ifdef _M_X64
#define _mm_insert_epi64 __emu_mm_insert_epi64
BE_FORCE_INLINE __m128i _mm_insert_epi64(__m128i value, __int64 input, const int index) { 
    assert(index >= 0 && index < 2);
    ((__int64 *)&value)[index] = input; 
    return value; 
}

#define _mm_extract_epi64 __emu_mm_extract_epi64
BE_FORCE_INLINE __int64 _mm_extract_epi64(__m128i input, const int index) { 
    assert(size_t(index) < 2); 
    return index == 0 ? _mm_cvtsi128_si64x(input) : _mm_cvtsi128_si64x(_mm_unpackhi_epi64(input, input)); 
}
#endif // _M_X64
