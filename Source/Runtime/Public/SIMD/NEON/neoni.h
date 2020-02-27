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

BE_FORCE_INLINE neoni set_epi32(int32_t a, int32_t b, int32_t c, int32_t d) {
    ALIGN_AS16 int32_t data[4] = { a, b, c, d };
    return vld1q_s32(data);
}

BE_FORCE_INLINE neoni set1_epi32(int32_t a) {
    return vdupq_n_s32(a);
}

BE_FORCE_INLINE neoni set1_epi16(int16_t a) {
    return vdupq_n_s16(a);
}

BE_FORCE_INLINE neoni set1_epi8(int8_t a) {
    return vdupq_n_s8(a);
}

BE_FORCE_INLINE neoni setzero_si128() {
    return vdupq_n_s32(0);
}

BE_FORCE_INLINE neoni load_si128(const int32_t *src) {
    return vld1q_s32(src);
}

BE_FORCE_INLINE neoni loadu_si128(const int32_t *src) {
    return vld1q_s32(src);
}

BE_FORCE_INLINE neoni loadnt_si128(const int32_t *src) {
    return vld1q_s32(src);
}

BE_FORCE_INLINE void store_si128(const neoni &a, int32_t *dst) {
    vst1q_s32(dst, a);
}

BE_FORCE_INLINE void storeu_si128(const neoni &a, int32_t *dst) {
    vst1q_s32(dst, a);
}

BE_FORCE_INLINE void storent_si128(const neoni &a, int32_t *dst) {
    vst1q_s32(dst, a);
}

BE_FORCE_INLINE neoni ps_to_epi32(const neonf &a) {
    return vcvtq_s32_f32(a);
}

BE_FORCE_INLINE neoni abs_epi32(const neoni &a) {
    return vqabsq_s32(a);
}

BE_FORCE_INLINE neoni operator+(const neoni &a) { return a; }
BE_FORCE_INLINE neoni operator-(const neoni &a) { return vnegq_s32(a); }

BE_FORCE_INLINE neoni operator+(const neoni &a, const neoni &b) { return vaddq_s32(a, b); }
BE_FORCE_INLINE neoni operator+(const neoni &a, const int32_t &b) { return a + set1_epi32(b); }
BE_FORCE_INLINE neoni operator+(const int32_t &a, const neoni &b) { return set1_epi32(a) + b; }

BE_FORCE_INLINE neoni operator-(const neoni &a, const neoni &b) { return vsubq_s32(a, b); }
BE_FORCE_INLINE neoni operator-(const neoni &a, const int32_t &b) { return a - set1_epi32(b); }
BE_FORCE_INLINE neoni operator-(const int32_t &a, const neoni &b) { return set1_epi32(a) - b; }

BE_FORCE_INLINE neoni operator*(const neoni &a, const neoni &b) { return vmulq_s32(a, b); }
BE_FORCE_INLINE neoni operator*(const neoni &a, const int32_t &b) { return a * set1_epi32(b); }
BE_FORCE_INLINE neoni operator*(const int32_t &a, const neoni &b) { return set1_epi32(a) * b; }

BE_FORCE_INLINE neoni operator&(const neoni &a, const neoni &b) { return vandq_u32(a, b); }
BE_FORCE_INLINE neoni operator&(const neoni &a, const int32_t &b) { return a & set1_epi32(b); }
BE_FORCE_INLINE neoni operator&(const int32_t &a, const neoni &b) { return set1_epi32(a) & b; }

BE_FORCE_INLINE neoni operator|(const neoni &a, const neoni &b) { return vorrq_u32(a, b); }
BE_FORCE_INLINE neoni operator|(const neoni &a, const int32_t &b) { return a | set1_epi32(b); }
BE_FORCE_INLINE neoni operator|(const int32_t &a, const neoni &b) { return set1_epi32(a) | b; }

BE_FORCE_INLINE neoni operator^(const neoni &a, const neoni &b) { return veorq_u32(a, b); }
BE_FORCE_INLINE neoni operator^(const neoni &a, const int32_t &b) { return a ^ set1_epi32(b); }
BE_FORCE_INLINE neoni operator^(const int32_t &a, const neoni &b) { return set1_epi32(a) ^ b; }

BE_FORCE_INLINE neoni operator<<(const neoni &a, const int32_t &n) { return vshlq_u32(a, vdupq_n_s32(n)); }
BE_FORCE_INLINE neoni operator>>(const neoni &a, const int32_t &n) { return vshlq_s32(a, vdupq_n_s32(-n)); }

BE_FORCE_INLINE neoni operator==(const neoni &a, const neoni &b) { return vceqq_u32(a, b); }
BE_FORCE_INLINE neoni operator==(const neoni &a, const int32_t &b) { return a == set1_epi32(b); }
BE_FORCE_INLINE neoni operator==(const int32_t &a, const neoni &b) { return set1_epi32(a) == b; }

BE_FORCE_INLINE neoni operator!=(const neoni &a, const neoni &b) { return vmvnq_u32(vceqq_u32(a, b)); }
BE_FORCE_INLINE neoni operator!=(const neoni &a, const int32_t &b) { return a != set1_epi32(b); }
BE_FORCE_INLINE neoni operator!=(const int32_t &a, const neoni &b) { return set1_epi32(a) != b; }

BE_FORCE_INLINE neoni operator<(const neoni &a, const neoni &b) { return vcltq_s32(a, b); }
BE_FORCE_INLINE neoni operator<(const neoni &a, const int32_t &b) { return a < set1_epi32(b); }
BE_FORCE_INLINE neoni operator<(const int32_t &a, const neoni &b) { return set1_epi32(a) < b; }

BE_FORCE_INLINE neoni operator>(const neoni &a, const neoni &b) { return vcgtq_s32(a, b); }
BE_FORCE_INLINE neoni operator>(const neoni &a, const int32_t &b) { return a > set1_epi32(b); }
BE_FORCE_INLINE neoni operator>(const int32_t &a, const neoni &b) { return set1_epi32(a) > b; }

BE_FORCE_INLINE neoni operator>=(const neoni &a, const neoni &b) { return vmvnq_u32(vcltq_s32(a, b)); }
BE_FORCE_INLINE neoni operator>=(const neoni &a, const int32_t &b) { return a >= set1_epi32(b); }
BE_FORCE_INLINE neoni operator>=(const int32_t &a, const neoni &b) { return set1_epi32(a) >= b; }

BE_FORCE_INLINE neoni operator<=(const neoni &a, const neoni &b) { return vmvnq_u32(vcgtq_s32(a, b)); }
BE_FORCE_INLINE neoni operator<=(const neoni &a, const int32_t &b) { return a <= set1_epi32(b); }
BE_FORCE_INLINE neoni operator<=(const int32_t &a, const neoni &b) { return set1_epi32(a) <= b; }

BE_FORCE_INLINE neoni &operator+=(neoni &a, const neoni &b) { return a = a + b; }
BE_FORCE_INLINE neoni &operator+=(neoni &a, const int32_t &b) { return a = a + b; }

BE_FORCE_INLINE neoni &operator-=(neoni &a, const neoni &b) { return a = a - b; }
BE_FORCE_INLINE neoni &operator-=(neoni &a, const int32_t &b) { return a = a - b; }

BE_FORCE_INLINE neoni &operator*=(neoni &a, const neoni &b) { return a = a * b; }
BE_FORCE_INLINE neoni &operator*=(neoni &a, const int32_t &b) { return a = a * b; }

BE_FORCE_INLINE neoni &operator&=(neoni &a, const neoni &b) { return a = a & b; }
BE_FORCE_INLINE neoni &operator&=(neoni &a, const int32_t &b) { return a = a & b; }

BE_FORCE_INLINE neoni &operator|=(neoni &a, const neoni &b) { return a = a | b; }
BE_FORCE_INLINE neoni &operator|=(neoni &a, const int32_t &b) { return a = a | b; }

BE_FORCE_INLINE neoni &operator<<=(neoni &a, const int32_t &b) { return a = a << b; }
BE_FORCE_INLINE neoni &operator>>=(neoni &a, const int32_t &b) { return a = a >> b; }

// Shifts right arithmetic.
BE_FORCE_INLINE neoni sra_epi32(const neoni &a, const int32_t &b) { return vshlq_s32(a, vdupq_n_s32(-b)); }

// Shifts right logical.
BE_FORCE_INLINE neoni srl_epi32(const neoni &a, const int32_t &b) { return vshlq_u32(a, vdupq_n_s32(-b)); }

// Shifts left logical.
BE_FORCE_INLINE neoni sll_epi32(const neoni &a, const int32_t &b) { return vshlq_u32(a, vdupq_n_s32(b)); }

// Unpacks to (a0, b0, a1, b1).
BE_FORCE_INLINE neoni unpacklo_epi32(const neoni &a, const neoni &b) { 
    return vreinterpretq_s32_f32(unpacklo_ps(vreinterpretq_f32_s32(a), vreinterpretq_f32_s32(b)));
}

// Unpacks to (a2, b2, a3, b3).
BE_FORCE_INLINE neoni unpackhi_epi32(const neoni &a, const neoni &b) { 
    return vreinterpretq_s32_f32(unpackhi_ps(vreinterpretq_f32_s32(a), vreinterpretq_f32_s32(b)));
}

// Shuffles 4x32 bits integers using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE neoni shuffle_epi32(const neoni &a) {
    return shuffle_ps<i0, i1, i2, i3>(a.f32x4).i32x4;
}

// Shuffles two 4x32 bits integers using template parameters. ax, bx = [0, 3].
template <size_t a0, size_t a1, size_t b0, size_t b1>
BE_FORCE_INLINE neoni shuffle_epi32(const neoni &a, const neoni &b) {
    return shuffle_ps<a0, a1, b0, b1>(a.f32x4, b.f32x4).i32x4;
}

template <size_t src>
BE_FORCE_INLINE int extract_epi32(const neoni &a) { return vgetq_lane_s32(a, src); }
template <size_t src>
BE_FORCE_INLINE int extract_epi8(const neoni &a) { return vgetq_lane_s8(a, src); }

template <size_t dst>
BE_FORCE_INLINE const neoni insert_epi32(const neoni &a, const int32_t b) {
    neoni c = a;
    c[dst & 3] = b;
    return c;
}

// Selects 4x32 bits integer using mask.
BE_FORCE_INLINE neoni select_epi32(const neoni &a, const neoni &b, const neonb &mask) {
    return vbslq_s32(mask, a, b);
}

BE_FORCE_INLINE neoni min_epi32(const neoni &a, const neoni &b) { return vminq_s32(a, b); }
BE_FORCE_INLINE neoni max_epi32(const neoni &a, const neoni &b) { return vmaxq_s32(a, b); }

BE_FORCE_INLINE neoni min_epi32(const neoni &a, const int32_t &b) { return vminq_s32(a, set1_epi32(b)); }
BE_FORCE_INLINE neoni min_epi32(const int32_t &a, const neoni &b) { return vminq_s32(set1_epi32(a), b); }

BE_FORCE_INLINE neoni max_epi32(const neoni &a, const int32_t &b) { return vmaxq_s32(a, set1_epi32(b)); }
BE_FORCE_INLINE neoni max_epi32(const int32_t &a, const neoni &b) { return vmaxq_s32(set1_epi32(a), b); }

// Broadcasts minimum value of all 4 components.
BE_FORCE_INLINE neoni vreduce_min_epi32(const neoni &a) {
    neoni h = min_epi32(shuffle_epi32<1, 0, 3, 2>(a), a);
    return min_epi32(shuffle_epi32<2, 3, 0, 1>(h), h);
}

// Broadcasts maximum value of all 4 components.
BE_FORCE_INLINE neoni vreduce_max_epi32(const neoni &a) {
    neoni h = max_epi32(shuffle_epi32<1, 0, 3, 2>(a), a);
    return max_epi32(shuffle_epi32<2, 3, 0, 1>(h), h);
}

BE_FORCE_INLINE int reduce_min_epi32(const neoni &a) { return extract_epi32<0>(vreduce_min_epi32(a)); }
BE_FORCE_INLINE int reduce_max_epi32(const neoni &a) { return extract_epi32<0>(vreduce_max_epi32(a)); }

// Returns index of minimum component.
BE_FORCE_INLINE size_t select_min_epi32(const neoni &a) { return CountTrailingZeros(vmovemaskq_f32(a == vreduce_min_epi32(a))); }

// Returns index of maximum component.
BE_FORCE_INLINE size_t select_max_epi32(const neoni &a) { return CountTrailingZeros(vmovemaskq_f32(a == vreduce_max_epi32(a))); }

// Returns index of minimum component with valid index mask.
BE_FORCE_INLINE size_t select_min_epi32(const neoni &a, const neonb &validmask) {
    const neoni v = select_epi32(set1_epi32(INT_MAX), a, validmask);
    return CountTrailingZeros(vmovemaskq_f32(vandq_s32(validmask, v == vreduce_min_epi32(v))));
}

// Returns index of maximum component with valid index mask.
BE_FORCE_INLINE size_t select_max_epi32(const neoni &a, const neonb &validmask) {
    const neoni v = select_epi32(set1_epi32(INT_MIN), a, validmask);
    return CountTrailingZeros(vmovemaskq_f32(vandq_s32(validmask, v == vreduce_max_epi32(v))));
}

// Broadcasts sums of all components.
BE_FORCE_INLINE neoni sum_epi32(const neoni &a) {
    neoni sum1 = vaddq_s32(a, vrev64q_s32(a)); // (x + y, x + y, z + w, z + w)
    neoni sum2 = vcombine_s32(vget_high_s32(sum1), vget_low_s32(sum1)); // (z + w, z + w, x + y, x + y)
    return vaddq_s32(sum1, sum2); // (x + y + z + w, x + y + z + w, x + y + z + w, x + y + z + w)
}
