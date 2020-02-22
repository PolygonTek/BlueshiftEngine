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

BE_FORCE_INLINE avxb set1_256b32(bool a) {
    avxb dst;
    dst.l128i = _mm_castps_si128(_mm_lookupmask_ps[(size_t(a) << 3) | (size_t(a) << 2) | (size_t(a) << 1) | size_t(a)]);
    dst.h128i = _mm_castps_si128(_mm_lookupmask_ps[(size_t(a) << 3) | (size_t(a) << 2) | (size_t(a) << 1) | size_t(a)]);
    return dst;
}

BE_FORCE_INLINE avxb set_256b32(bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h) {
    avxb dst;
    dst.l128i = _mm_castps_si128(_mm_lookupmask_ps[(size_t(d) << 3) | (size_t(c) << 2) | (size_t(b) << 1) | size_t(a)]);
    dst.h128i = _mm_castps_si128(_mm_lookupmask_ps[(size_t(h) << 3) | (size_t(g) << 2) | (size_t(f) << 1) | size_t(e)]);
    return dst;
}

BE_FORCE_INLINE avxb ps_to_256b32(const avxf &a) {
    return _mm256_castps_si256(a);
}

BE_FORCE_INLINE avxb operator!(const avxb &a) { return _mm256_xor_ps(a, set1_256b32(true)); }

BE_FORCE_INLINE avxb operator&(const avxb &a, const avxb &b) { return _mm256_and_ps(a, b); }
BE_FORCE_INLINE avxb operator|(const avxb &a, const avxb &b) { return _mm256_or_ps(a, b); }
BE_FORCE_INLINE avxb operator^(const avxb &a, const avxb &b) { return _mm256_xor_ps(a, b); }

BE_FORCE_INLINE avxb operator&=(avxb &a, const avxb &b) { return a = a & b; }
BE_FORCE_INLINE avxb operator|=(avxb &a, const avxb &b) { return a = a | b; }
BE_FORCE_INLINE avxb operator^=(avxb &a, const avxb &b) { return a = a ^ b; }

BE_FORCE_INLINE avxb operator!=(const avxb &a, const avxb &b) { return _mm256_xor_ps(a, b); }
BE_FORCE_INLINE avxb operator==(const avxb &a, const avxb &b) { return _mm256_xor_ps(_mm256_xor_ps(a, b), set1_256b32(true)); }

// Selects 8x32 bits using mask.
BE_FORCE_INLINE avxb select_256b32(const avxb &a, const avxb &b, const avxb &mask) {
    return _mm256_blendv_ps(a, b, mask);
}

// Unpacks to (a0, b0, a1, b1, a4, b4, a5, b5).
BE_FORCE_INLINE avxb unpacklo_256b32(const avxb &a, const avxb &b) { return _mm256_unpacklo_ps(a.m256, b.m256); }

// Unpacks to (a2, b2, a3, b3, a6, b6, a7, b7).
BE_FORCE_INLINE avxb unpackhi_256b32(const avxb &a, const avxb &b) { return _mm256_unpackhi_ps(a.m256, b.m256); }

BE_FORCE_INLINE size_t movemask_256b32(const avxb &a) { return _mm256_movemask_ps(a); }

BE_FORCE_INLINE bool reduce_and_256b32(const avxb &a) { return _mm256_movemask_ps(a) == 0xf; }
BE_FORCE_INLINE bool reduce_or_256b32(const avxb &a) { return !_mm256_testz_ps(a, a); }
BE_FORCE_INLINE bool all_256b32(const avxb &b) { return _mm256_movemask_ps(b) == 0xf; }
BE_FORCE_INLINE bool any_256b32(const avxb &b) { return !_mm256_testz_ps(b, b); }
BE_FORCE_INLINE bool none_256b32(const avxb &b) { return _mm256_testz_ps(b, b) != 0; }
