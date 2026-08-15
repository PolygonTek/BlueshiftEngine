// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
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
#if defined(__ARM64__)
    return vcvtnq_s32_f32(a);
#else
    return vcvtq_s32_f32(round_ps(a));
#endif
}

BE_FORCE_INLINE neoni abs_epi32(const neoni &a) {
    return vabsq_s32(a);
}

BE_FORCE_INLINE neoni operator+(const neoni &a) { return a; }
BE_FORCE_INLINE neoni operator-(const neoni &a) { return vnegq_s32(a); }
BE_FORCE_INLINE neoni operator~(const neoni &a) { return vmvnq_s32(a); }

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

// Shifts signed 32-bit lanes left. Arithmetic and logical left shifts produce identical bit patterns.
BE_FORCE_INLINE neoni operator<<(const neoni &a, const int32_t &n) { return vshlq_s32(a, vdupq_n_s32(n)); }
// Shifts signed 32-bit lanes right with sign extension.
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

// Compound signed left shift. Arithmetic and logical left shifts produce identical bit patterns.
BE_FORCE_INLINE neoni &operator<<=(neoni &a, const int32_t &b) { return a = a << b; }
// Compound signed right shift with sign extension.
BE_FORCE_INLINE neoni &operator>>=(neoni &a, const int32_t &b) { return a = a >> b; }

// Shifts signed 32-bit lanes right with sign extension.
BE_FORCE_INLINE neoni sra_epi32(const neoni &a, const int32_t &b) { return vshlq_s32(a, vdupq_n_s32(-b)); }

// Shifts 32-bit lanes right with zero fill.
BE_FORCE_INLINE neoni srl_epi32(const neoni &a, const int32_t &b) { return vshlq_u32(a, vdupq_n_s32(-b)); }

// Logically shifts all 32-bit lanes right by the same runtime count.
BE_FORCE_INLINE neoni srlv_epi32(const neoni &a, const int32_t &b) { return vshlq_u32(a, vdupq_n_s32(-b)); }

// Shifts 32-bit lanes left with zero fill. Arithmetic and logical left shifts produce identical bit patterns.
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
    return vbslq_s32(mask, b, a);
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
BE_FORCE_INLINE size_t select_min_epi32(const neoni &a) { return CountTrailingZeros(bx_vmovemaskq_f32(a == vreduce_min_epi32(a))); }

// Returns index of maximum component.
BE_FORCE_INLINE size_t select_max_epi32(const neoni &a) { return CountTrailingZeros(bx_vmovemaskq_f32(a == vreduce_max_epi32(a))); }

// Returns index of minimum component with valid index mask.
BE_FORCE_INLINE size_t select_min_epi32(const neoni &a, const neonb &validmask) {
    const neoni v = select_epi32(set1_epi32(INT_MAX), a, validmask);
    return CountTrailingZeros(bx_vmovemaskq_f32(vandq_s32(validmask, v == vreduce_min_epi32(v))));
}

// Returns index of maximum component with valid index mask.
BE_FORCE_INLINE size_t select_max_epi32(const neoni &a, const neonb &validmask) {
    const neoni v = select_epi32(set1_epi32(INT_MIN), a, validmask);
    return CountTrailingZeros(bx_vmovemaskq_f32(vandq_s32(validmask, v == vreduce_max_epi32(v))));
}

// Broadcasts sums of all components.
BE_FORCE_INLINE neoni sum_epi32(const neoni &a) {
    neoni sum1 = vaddq_s32(a, vrev64q_s32(a)); // (x + y, x + y, z + w, z + w)
    neoni sum2 = vcombine_s32(vget_high_s32(sum1), vget_low_s32(sum1)); // (z + w, z + w, x + y, x + y)
    return vaddq_s32(sum1, sum2); // (x + y + z + w, x + y + z + w, x + y + z + w, x + y + z + w)
}

// Unpacks 16 unsigned 8-bit integers into four groups of 4x32-bit integers.
BE_FORCE_INLINE void unpack_epu8_to_epi32(const neoni &a, neoni &r0, neoni &r1, neoni &r2, neoni &r3) {
    const uint8x16_t u8 = vreinterpretq_u8_s32(a);
    const uint16x8_t lo16 = vmovl_u8(vget_low_u8(u8));
    const uint16x8_t hi16 = vmovl_u8(vget_high_u8(u8));

    r0 = neoni(vmovl_u16(vget_low_u16(lo16)));
    r1 = neoni(vmovl_u16(vget_high_u16(lo16)));
    r2 = neoni(vmovl_u16(vget_low_u16(hi16)));
    r3 = neoni(vmovl_u16(vget_high_u16(hi16)));
}

// Unpacks 16 signed 8-bit integers into four groups of 4x32-bit integers.
BE_FORCE_INLINE void unpack_epi8_to_epi32(const neoni &a, neoni &r0, neoni &r1, neoni &r2, neoni &r3) {
    const int8x16_t i8 = vreinterpretq_s8_s32(a);
    const int16x8_t lo16 = vmovl_s8(vget_low_s8(i8));
    const int16x8_t hi16 = vmovl_s8(vget_high_s8(i8));

    r0 = neoni(vmovl_s16(vget_low_s16(lo16)));
    r1 = neoni(vmovl_s16(vget_high_s16(lo16)));
    r2 = neoni(vmovl_s16(vget_low_s16(hi16)));
    r3 = neoni(vmovl_s16(vget_high_s16(hi16)));
}

// Unpacks 8 unsigned 16-bit integers into two groups of 4x32-bit integers.
BE_FORCE_INLINE void unpack_epu16_to_epi32(const neoni &a, neoni &r0, neoni &r1) {
    const uint16x8_t u16 = vreinterpretq_u16_s32(a);

    r0 = neoni(vmovl_u16(vget_low_u16(u16)));
    r1 = neoni(vmovl_u16(vget_high_u16(u16)));
}

// Unpacks 8 signed 16-bit integers into two groups of 4x32-bit integers.
BE_FORCE_INLINE void unpack_epi16_to_epi32(const neoni &a, neoni &r0, neoni &r1) {
    const int16x8_t i16 = vreinterpretq_s16_s32(a);

    r0 = neoni(vmovl_s16(vget_low_s16(i16)));
    r1 = neoni(vmovl_s16(vget_high_s16(i16)));
}

// Converts four floats to signed integers by truncating toward zero.
BE_FORCE_INLINE neoni trunc_ps_to_epi32(const neonf &a) {
    return vcvtq_s32_f32(a);
}

// Input values must be in [0, 4294967040].
BE_FORCE_INLINE neoni trunc_ps_to_epu32(const neonf &a) {
    return neoni(vreinterpretq_s32_u32(vcvtq_u32_f32(a)));
}

// Converts four floats to signed integers by rounding toward negative infinity.
BE_FORCE_INLINE neoni floor_ps_to_epi32(const neonf &a) {
    const int32x4_t truncated = vcvtq_s32_f32(a);
    const float32x4_t truncatedFloat = vcvtq_f32_s32(truncated);
    const uint32x4_t adjustMask = vcltq_f32(a, truncatedFloat);
    const int32x4_t adjustment = vreinterpretq_s32_u32(
        vandq_u32(adjustMask, vdupq_n_u32(1)));

    return neoni(vsubq_s32(truncated, adjustment));
}

// Saturates and packs four groups of 4x32-bit integers into 16 unsigned bytes.
BE_FORCE_INLINE neoni packus_epi32_to_epu8(const neoni &a, const neoni &b, const neoni &c, const neoni &d) {
    const int16x8_t lo16 = vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
    const int16x8_t hi16 = vcombine_s16(vqmovn_s32(c), vqmovn_s32(d));
    const uint8x16_t u8 = vcombine_u8(vqmovun_s16(lo16), vqmovun_s16(hi16));

    return neoni(vreinterpretq_s32_u8(u8));
}

// Saturates and packs four groups of 4x32-bit integers into 16 signed bytes.
BE_FORCE_INLINE neoni packs_epi32_to_epi8(const neoni &a, const neoni &b, const neoni &c, const neoni &d) {
    const int16x8_t lo16 = vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
    const int16x8_t hi16 = vcombine_s16(vqmovn_s32(c), vqmovn_s32(d));
    const int8x16_t i8 = vcombine_s8(vqmovn_s16(lo16), vqmovn_s16(hi16));

    return neoni(vreinterpretq_s32_s8(i8));
}

// Saturates and packs two groups of 4x32-bit integers into 8 unsigned 16-bit integers.
BE_FORCE_INLINE neoni packus_epi32_to_epu16(const neoni &a, const neoni &b) {
    const uint16x8_t u16 = vcombine_u16(vqmovun_s32(a), vqmovun_s32(b));

    return neoni(vreinterpretq_s32_u16(u16));
}

// Saturates and packs two groups of 4x32-bit integers into 8 signed 16-bit integers.
BE_FORCE_INLINE neoni packs_epi32_to_epi16(const neoni &a, const neoni &b) {
    const int16x8_t i16 = vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));

    return neoni(vreinterpretq_s32_s16(i16));
}
