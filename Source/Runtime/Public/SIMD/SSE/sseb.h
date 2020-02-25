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

BE_FORCE_INLINE sseb set1_b32(bool a) {
    return _mm_lookupmask_ps[(size_t(a) << 3) | (size_t(a) << 2) | (size_t(a) << 1) | size_t(a)];
}

BE_FORCE_INLINE sseb set_b32(bool a, bool b, bool c, bool d) {
    return _mm_lookupmask_ps[(size_t(d) << 3) | (size_t(c) << 2) | (size_t(b) << 1) | size_t(a)];
}

BE_FORCE_INLINE sseb ps_to_b32(const ssef &a) {
    return _mm_castps_si128(a);
}

BE_FORCE_INLINE sseb operator!(const sseb &a) { return _mm_xor_ps(a, set1_b32(true)); }

BE_FORCE_INLINE sseb operator&(const sseb &a, const sseb &b) { return _mm_and_ps(a, b); }
BE_FORCE_INLINE sseb operator|(const sseb &a, const sseb &b) { return _mm_or_ps(a, b); }
BE_FORCE_INLINE sseb operator^(const sseb &a, const sseb &b) { return _mm_xor_ps(a, b); }
  
BE_FORCE_INLINE sseb operator&=(sseb &a, const sseb &b) { return a = a & b; }
BE_FORCE_INLINE sseb operator|=(sseb &a, const sseb &b) { return a = a | b; }
BE_FORCE_INLINE sseb operator^=(sseb &a, const sseb &b) { return a = a ^ b; }

BE_FORCE_INLINE sseb operator!=(const sseb &a, const sseb &b) { return _mm_xor_ps(a, b); }
BE_FORCE_INLINE sseb operator==(const sseb &a, const sseb &b) { return _mm_castsi128_ps(_mm_cmpeq_epi32(a, b)); }

// Selects 4x32 bits using mask.
BE_FORCE_INLINE sseb select_b32(const sseb &a, const sseb &b, const sseb &mask) {
#if defined(__SSE4_1__)
    return _mm_blendv_ps(a, b, mask);
#else
    // dst = (a & !mask) | (b & mask)
    return _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(mask), a), _mm_and_ps(_mm_castsi128_ps(mask), b));
#endif
}

// Unpacks to (a0, b0, a1, b1).
BE_FORCE_INLINE sseb unpacklo_b32(const sseb &a, const sseb &b) { return _mm_unpacklo_ps(a, b); }

// Unpacks to (a2, b2, a3, b3).
BE_FORCE_INLINE sseb unpackhi_b32(const sseb &a, const sseb &b) { return _mm_unpacklo_ps(a, b); }

BE_FORCE_INLINE size_t movemask_b32(const sseb &a) { return _mm_movemask_ps(a); }

BE_FORCE_INLINE bool reduce_and_b32(const sseb &a) { return movemask_b32(a) == 0xf; }
BE_FORCE_INLINE bool reduce_or_b32(const sseb &a) { return movemask_b32(a) != 0x0; }
BE_FORCE_INLINE bool all_b32(const sseb &a) { return movemask_b32(a) == 0xf; }
BE_FORCE_INLINE bool any_b32(const sseb &a) { return movemask_b32(a) != 0x0; }
BE_FORCE_INLINE bool none_b32(const sseb &a) { return movemask_b32(a) == 0x0; }
