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

BE_FORCE_INLINE neonf set_ps(float a, float b, float c, float d) {
    ALIGN_AS16 float data[4] = { a, b, c, d };
    return vld1q_f32(data);
}

BE_FORCE_INLINE neonf set1_ps(float a) {
    return vdupq_n_f32(a);
}

BE_FORCE_INLINE neonf setzero_ps() {
    return vdupq_n_f32(0);
}

BE_FORCE_INLINE neonf load_ps(const float *src) {
    return vld1q_f32(src);
}

BE_FORCE_INLINE neonf loadu_ps(const float *src) {
    return vld1q_f32(src);
}

BE_FORCE_INLINE neonf load1_ps(const float *src) {
    return vld1q_dup_f32(src);
}

BE_FORCE_INLINE neonf loadnt_ps(const float *src) {
    return vld1q_f32(src);
}

BE_FORCE_INLINE void store_ps(const neonf &a, float *dst) {
    vst1q_f32(dst, a);
}

BE_FORCE_INLINE void storeu_ps(const neonf &a, float *dst) {
    vst1q_f32(dst, a);
}

BE_FORCE_INLINE void storent_ps(const neonf &a, float *dst) {
    vst1q_f32(dst, a);
}

BE_FORCE_INLINE neonf epi32_to_ps(const neoni &a) {
    return vcvtq_f32_s32(a);
}

BE_FORCE_INLINE neonf abs_ps(const neonf &a) {
    return vabsq_f32(a);
}

BE_FORCE_INLINE neonf sqr_ps(const neonf &a) {
    return vmulq_f32(a, a);
}

BE_FORCE_INLINE neonf sqrt_ps(const neonf &a) {
    return vsqrtq_f32(a);
}

// Reciprocal with 12 bits of precision.
BE_FORCE_INLINE neonf rcp12_ps(const neonf &a) {
    return vrecpeq_f32(a);
}

// Reciprocal with at least 16 bits precision.
BE_FORCE_INLINE neonf rcp16_ps(const neonf &a) {
    neonf r = vrecpeq_f32(a);
    // Newton-Raphson approximation to improve precision.
    return vrecpsq_f32(r, a);
}

// Reciprocal with close to full precision.
BE_FORCE_INLINE neonf rcp32_ps(const neonf &a) {
    neonf r = vrecpeq_f32(a);
    // Newton-Raphson approximation to improve precision.
    r = vrecpsq_f32(r, a);
    return vrecpsq_f32(r, a);
}

// Divide with at least 16 bits precision.
BE_FORCE_INLINE neonf div16_ps(const neonf &a, const neonf &b) {
    return vmulq_f32(a, rcp16_ps(b));
}

// Divide with close to full precision.
BE_FORCE_INLINE neonf div32_ps(const neonf &a, const neonf &b) {
    return vmulq_f32(a, rcp32_ps(b));
}

// Reciprocal square root with 12 bits of precision.
BE_FORCE_INLINE neonf rsqrt12_ps(const neonf &a) {
    return vrsqrteq_f32(a);
}

// Reciprocal square root with at least 16 bits precision.
BE_FORCE_INLINE neonf rsqrt16_ps(const neonf &a) {
    neonf r = vrsqrteq_f32(a);
    // Newton-Raphson approximation to improve precision.
    return vrsqrtsq_f32(r, a);
}

// Reciprocal square root with close to full precision.
BE_FORCE_INLINE neonf rsqrt32_ps(const neonf &a) {
    neonf r = vrsqrteq_f32(a);
    // Newton-Raphson approximation to improve precision.
    r = vrsqrtsq_f32(r, a);
    return vrsqrtsq_f32(r, a);
}

BE_FORCE_INLINE neonf operator+(const neonf &a) { return a; }
BE_FORCE_INLINE neonf operator-(const neonf &a) { return vnegq_f32(a); }

BE_FORCE_INLINE neonf operator+(const neonf &a, const neonf &b) { return vaddq_f32(a, b); }
BE_FORCE_INLINE neonf operator+(const neonf &a, const float &b) { return a + set1_ps(b); }
BE_FORCE_INLINE neonf operator+(const float &a, const neonf &b) { return set1_ps(a) + b; }

BE_FORCE_INLINE neonf operator-(const neonf &a, const neonf &b) { return vsubq_f32(a, b); }
BE_FORCE_INLINE neonf operator-(const neonf &a, const float &b) { return a - set1_ps(b); }
BE_FORCE_INLINE neonf operator-(const float &a, const neonf &b) { return set1_ps(a) - b; }

BE_FORCE_INLINE neonf operator*(const neonf &a, const neonf &b) { return vmulq_f32(a, b); }
BE_FORCE_INLINE neonf operator*(const neonf &a, const float &b) { return a * set1_ps(b); }
BE_FORCE_INLINE neonf operator*(const float &a, const neonf &b) { return set1_ps(a) * b; }

BE_FORCE_INLINE neonf operator/(const neonf &a, const neonf &b) { return a * rcp32_ps(b); }
BE_FORCE_INLINE neonf operator/(const neonf &a, const float &b) { return a * rcp32_ps(set1_ps(b)); }
BE_FORCE_INLINE neonf operator/(const float &a, const neonf &b) { return a * rcp32_ps(b); }

BE_FORCE_INLINE neonf operator&(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b))); }
BE_FORCE_INLINE neonf operator&(const neonf &a, const neoni &b) { return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), b)); }

BE_FORCE_INLINE neonf operator|(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b))); }
BE_FORCE_INLINE neonf operator|(const neonf &a, const neoni &b) { return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), b)); }

BE_FORCE_INLINE neonf operator^(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b))); }
BE_FORCE_INLINE neonf operator^(const neonf &a, const neoni &b) { return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a), b)); }

BE_FORCE_INLINE neonf operator==(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(vceqq_f32(a, b)); }
BE_FORCE_INLINE neonf operator==(const neonf &a, const float &b) { return a == set1_ps(b); }
BE_FORCE_INLINE neonf operator==(const float &a, const neonf &b) { return set1_ps(a) == b; }

BE_FORCE_INLINE neonf operator!=(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(vmvnq_u32(vceqq_f32(a, b))); }
BE_FORCE_INLINE neonf operator!=(const neonf &a, const float &b) { return a != set1_ps(b); }
BE_FORCE_INLINE neonf operator!=(const float &a, const neonf &b) { return set1_ps(a) != b; }

BE_FORCE_INLINE neonf operator<(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(vcltq_f32(a, b)); }
BE_FORCE_INLINE neonf operator<(const neonf &a, const float &b) { return a < set1_ps(b); }
BE_FORCE_INLINE neonf operator<(const float &a, const neonf &b) { return set1_ps(a) < b; }

BE_FORCE_INLINE neonf operator>(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(vcgtq_f32(a, b)); }
BE_FORCE_INLINE neonf operator>(const neonf &a, const float &b) { return a > set1_ps(b); }
BE_FORCE_INLINE neonf operator>(const float &a, const neonf &b) { return set1_ps(a) > b; }

BE_FORCE_INLINE neonf operator>=(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(vcgeq_f32(a, b)); }
BE_FORCE_INLINE neonf operator>=(const neonf &a, const float &b) { return a >= set1_ps(b); }
BE_FORCE_INLINE neonf operator>=(const float &a, const neonf &b) { return set1_ps(a) >= b; }

BE_FORCE_INLINE neonf operator<=(const neonf &a, const neonf &b) { return vreinterpretq_f32_u32(vcleq_f32(a, b)); }
BE_FORCE_INLINE neonf operator<=(const neonf &a, const float &b) { return a <= set1_ps(b); }
BE_FORCE_INLINE neonf operator<=(const float &a, const neonf &b) { return set1_ps(a) <= b; }

BE_FORCE_INLINE neonf &operator+=(neonf &a, const neonf &b) { return a = a + b; }
BE_FORCE_INLINE neonf &operator+=(neonf &a, const float &b) { return a = a + b; }

BE_FORCE_INLINE neonf &operator-=(neonf &a, const neonf &b) { return a = a - b; }
BE_FORCE_INLINE neonf &operator-=(neonf &a, const float &b) { return a = a - b; }

BE_FORCE_INLINE neonf &operator*=(neonf &a, const neonf &b) { return a = a * b; }
BE_FORCE_INLINE neonf &operator*=(neonf &a, const float &b) { return a = a * b; }

BE_FORCE_INLINE neonf &operator/=(neonf &a, const neonf &b) { return a = a / b; }
BE_FORCE_INLINE neonf &operator/=(neonf &a, const float &b) { return a = a / b; }

BE_FORCE_INLINE neonf &operator&=(neonf &a, const neonf &b) { return a = a & b; }
BE_FORCE_INLINE neonf &operator&=(neonf &a, const neoni &b) { return a = a & b; }

BE_FORCE_INLINE neonf &operator|=(neonf &a, const neonf &b) { return a = a | b; }
BE_FORCE_INLINE neonf &operator|=(neonf &a, const neoni &b) { return a = a | b; }

BE_FORCE_INLINE neonf &operator^=(neonf &a, const neonf &b) { return a = a ^ b; }
BE_FORCE_INLINE neonf &operator^=(neonf &a, const neoni &b) { return a = a ^ b; }

// dst = a * b + c
BE_FORCE_INLINE neonf madd_ps(const neonf &a, const neonf &b, const neonf &c) { return vmlaq_f32(c, a, b); }
// dst = a * b - c
BE_FORCE_INLINE neonf msub_ps(const neonf &a, const neonf &b, const neonf &c) { return vnegq_f32(vmlsq_f32(c, a, b)); }
// dst = -(a * b) + c
BE_FORCE_INLINE neonf nmadd_ps(const neonf &a, const neonf &b, const neonf &c) { return vmlsq_f32(c, a, b); }
// dst = -(a * b) - c
BE_FORCE_INLINE neonf nmsub_ps(const neonf &a, const neonf &b, const neonf &c) { return vnegq_f32(vmlaq_f32(c, a, b)); }

BE_FORCE_INLINE neonf floor_ps(const neonf &a) { return neonf(floorf(a[0]), floorf(a[1]), floorf(a[2]), floorf(a[3])); }
BE_FORCE_INLINE neonf ceil_ps(const neonf &a) { return neonf(ceilf(a[0]), ceilf(a[1]), ceilf(a[2]), ceilf(a[3])); }
BE_FORCE_INLINE neonf trunc_ps(const neonf &a) { return neonf(truncf(a[0]), truncf(a[1]), truncf(a[2]), truncf(a[3])); }
BE_FORCE_INLINE neonf round_ps(const neonf &a) { return neonf(roundf(a[0]), roundf(a[1]), roundf(a[2]), roundf(a[3])); }

BE_FORCE_INLINE neonf frac_ps(const neonf &a) { return a - floor_ps(a); }

// Unpack to (a0, b0, a1, b1).
BE_FORCE_INLINE neonf unpacklo_ps(const neonf &a, const neonf &b) { return vunpackloq_f32(a, b); }

// Unpack to (a2, b2, a3, b3).
BE_FORCE_INLINE neonf unpackhi_ps(const neonf &a, const neonf &b) { return vunpackhiq_f32(a, b); }

// Shuffles 4x32 bits floats using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE neonf shuffle_ps(const neonf &a) {
    float32x4_t ret = vmovq_n_f32(vgetq_lane_f32(a, i0));
    ret = vsetq_lane_f32(vgetq_lane_f32(a, i1), ret, 1);
    ret = vsetq_lane_f32(vgetq_lane_f32(a, i2), ret, 2);
    return vsetq_lane_f32(vgetq_lane_f32(a, i3), ret, 3);
}

template <>
BE_FORCE_INLINE neonf shuffle_ps<0, 0, 0, 0>(const neonf &a) {
    return vsplatq_f32(a, 0);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 1, 1, 1>(const neonf &a) {
    return vsplatq_f32(a, 1);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 2, 2, 2>(const neonf &a) {
    return vsplatq_f32(a, 2);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<3, 3, 3, 3>(const neonf &a) {
    return vsplatq_f32(a, 3);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 3, 0, 1>(const neonf &a) {
    return vpermq_f32_1032(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 0, 3, 2>(const neonf &a) {
    return vpermq_f32_2301(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 2, 3, 0>(const neonf &a) {
    return vpermq_f32_0321(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<3, 0, 1, 2>(const neonf &a) {
    return vpermq_f32_2103(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<0, 1, 0, 1>(const neonf &a) {
    return vpermq_f32_1010(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 0, 0, 1>(const neonf &a) {
    return vpermq_f32_1001(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 0, 1, 0>(const neonf &a) {
    return vpermq_f32_0101(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 1, 2, 2>(const neonf &a) {
    return vpermq_f32_2211(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 2, 1, 0>(const neonf &a) {
    return vpermq_f32_0122(a);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 3, 3, 3>(const neonf &a) {
    return vpermq_f32_3332(a);
}

// Shuffles two 4x32 bits floats using template parameters. ax, bx = [0, 3].
template <size_t a0, size_t a1, size_t b0, size_t b1>
BE_FORCE_INLINE neonf shuffle_ps(const neonf &a, const neonf &b) {
    float32x4_t ret = vmovq_n_f32(vgetq_lane_f32(a, a0));
    ret = vsetq_lane_f32(vgetq_lane_f32(a, a1), ret, 1);
    ret = vsetq_lane_f32(vgetq_lane_f32(b, b0), ret, 2);
    return vsetq_lane_f32(vgetq_lane_f32(b, b1), ret, 3);
}

template <>
BE_FORCE_INLINE neonf shuffle_ps<0, 0, 0, 0>(const neonf &a, const neonf &b) {
    return vshufq_f32_0000(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 1, 1, 1>(const neonf &a, const neonf &b) {
    return vshufq_f32_1111(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 2, 2, 2>(const neonf &a, const neonf &b) {
    return vshufq_f32_2222(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<3, 3, 3, 3>(const neonf &a, const neonf &b) {
    return vshufq_f32_3333(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 3, 0, 1>(const neonf &a, const neonf &b) {
    return vshufq_f32_1032(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 0, 3, 2>(const neonf &a, const neonf &b) {
    return vshufq_f32_2301(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 2, 3, 0>(const neonf &a, const neonf &b) {
    return vshufq_f32_0321(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<3, 0, 1, 2>(const neonf &a, const neonf &b) {
    return vshufq_f32_2103(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<0, 1, 0, 1>(const neonf &a, const neonf &b) {
    return vshufq_f32_1010(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 0, 0, 1>(const neonf &a, const neonf &b) {
    return vshufq_f32_1001(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 0, 1, 0>(const neonf &a, const neonf &b) {
    return vshufq_f32_0101(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<0, 1, 2, 3>(const neonf &a, const neonf &b) {
    return vshufq_f32_3210(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 1, 0, 0>(const neonf &a, const neonf &b) {
    return vshufq_f32_0011(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 2, 0, 0>(const neonf &a, const neonf &b) {
    return vshufq_f32_0022(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<0, 0, 2, 2>(const neonf &a, const neonf &b) {
    return vshufq_f32_2200(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 0, 2, 3>(const neonf &a, const neonf &b) {
    return vshufq_f32_3202(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 3, 2, 3>(const neonf &a, const neonf &b) {
    return vshufq_f32_3232(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<3, 3, 1, 1>(const neonf &a, const neonf &b) {
    return vshufq_f32_1133(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<0, 1, 0, 2>(const neonf &a, const neonf &b) {
    return vshufq_f32_2010(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 0, 0, 2>(const neonf &a, const neonf &b) {
    return vshufq_f32_2001(a, b);
}
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 3, 0, 2>(const neonf &a, const neonf &b) {
    return vshufq_f32_2032(a, b);
}

template <size_t i>
BE_FORCE_INLINE float extract_ps(const neonf &a) {
    return vgetq_lane_f32(a, i);
}

// Given a 4-channel single-precision neonf variable, returns the first channel 'x' as a float.
BE_FORCE_INLINE float x_ps(const neonf &a) { return extract_ps<0>(a); }

// Given a 4-channel single-precision neonf variable, returns the first channel 'y' as a float.
BE_FORCE_INLINE float y_ps(const neonf &a) { return extract_ps<1>(a); }

// Given a 4-channel single-precision neonf variable, returns the first channel 'z' as a float.
BE_FORCE_INLINE float z_ps(const neonf &a) { return extract_ps<2>(a); }

// Given a 4-channel single-precision neonf variable, returns the first channel 'w' as a float.
BE_FORCE_INLINE float w_ps(const neonf &a) { return extract_ps<3>(a); }

// Insert [32*src, 32*src+31] bits of b to a in [32*dst, 32*dst+31] bits.
template <int src, int dst>
BE_FORCE_INLINE neonf insert_ps(const neonf &a, const neonf &b) {
    neonf c = a;
    c[dst & 3] = b[src & 3];
    return c;
}

// Insert b to a in [32*dst, 32*dst+31] bits.
template <int dst>
BE_FORCE_INLINE neonf insert_ps(const neonf &a, float b) {
    neonf c = a;
    c[dst & 3] = b;
    return c;
}

// Select 4x32 bits floats using mask.
BE_FORCE_INLINE neonf select_ps(const neonf &a, const neonf &b, const neonb &mask) {
    return vbslq_f32(mask, a, b);
}

BE_FORCE_INLINE neonf min_ps(const neonf &a, const neonf &b) { return vminq_f32(a, b); }
BE_FORCE_INLINE neonf min_ps(const neonf &a, const float &b) { return vminq_f32(a, set1_ps(b)); }
BE_FORCE_INLINE neonf min_ps(const float &a, const neonf &b) { return vminq_f32(set1_ps(a), b); }

BE_FORCE_INLINE neonf max_ps(const neonf &a, const neonf &b) { return vmaxq_f32(a, b); }
BE_FORCE_INLINE neonf max_ps(const neonf &a, const float &b) { return vmaxq_f32(a, set1_ps(b)); }
BE_FORCE_INLINE neonf max_ps(const float &a, const neonf &b) { return vmaxq_f32(set1_ps(a), b); }

// Broadcast minimum value of all 4 components.
BE_FORCE_INLINE neonf vreduce_min_ps(const neonf &a) {
    neonf h = min_ps(shuffle_ps<1, 0, 3, 2>(a), a);
    return min_ps(shuffle_ps<2, 3, 0, 1>(h), h);
}

// Broadcast maximum value of all 4 components.
BE_FORCE_INLINE neonf vreduce_max_ps(const neonf &a) {
    neonf h = max_ps(shuffle_ps<1, 0, 3, 2>(a), a);
    return max_ps(shuffle_ps<2, 3, 0, 1>(h), h);
}

BE_FORCE_INLINE float reduce_min_ps(const neonf &a) { return vgetq_lane_f32(vreduce_min_ps(a), 0); }
BE_FORCE_INLINE float reduce_max_ps(const neonf &a) { return vgetq_lane_f32(vreduce_max_ps(a), 0); }

// Return index of minimum component.
BE_FORCE_INLINE size_t select_min_ps(const neonf &a) { return CountTrailingZeros(vmovemaskq_f32(a == vreduce_min_ps(a))); }

// Return index of maximum component.
BE_FORCE_INLINE size_t select_max_ps(const neonf &a) { return CountTrailingZeros(vmovemaskq_f32(a == vreduce_max_ps(a))); }

// Return index of minimum component with valid index mask.
BE_FORCE_INLINE size_t select_min_ps(const neonf &a, const neonb &validmask) {
    const neonf v = select_ps(set1_ps(FLT_INFINITY), a, validmask);
    return CountTrailingZeros(vmovemaskq_f32(vandq_u32(validmask.u32x4, (v == vreduce_min_ps(v)).u32x4)));
}

// Return index of maximum component with valid index mask.
BE_FORCE_INLINE size_t select_max_ps(const neonf &a, const neonb &validmask) {
    const neonf v = select_ps(set1_ps(-FLT_INFINITY), a, validmask);
    return CountTrailingZeros(vmovemaskq_f32(vandq_u32(validmask.u32x4, (v == vreduce_max_ps(v)).u32x4)));
}

// Broadcast sums of all components.
BE_FORCE_INLINE neonf sum_ps(const neonf &a) {
    neonf sum1 = vaddq_f32(a, vrev64q_f32(a)); // (x + y, x + y, z + w, z + w)
    neonf sum2 = vcombine_f32(vget_high_f32(sum1), vget_low_f32(sum1)); // (z + w, z + w, x + y, x + y)
    return vaddq_f32(sum1, sum2); // (x + y + z + w, x + y + z + w, x + y + z + w, x + y + z + w)
}

// Broadcast dot4 product.
BE_FORCE_INLINE neonf dot4_ps(const neonf &a, const neonf &b) {
    return sum_ps(a * b);
}

// Transposes 4x4 matrix.
BE_FORCE_INLINE void transpose4x4(neonf &r0, neonf &r1, neonf &r2, neonf &r3) {
    float32x4x2_t r01 = vtrnq_f32(r0, r1); // (m00, m10, m02, m12), (m01, m11, m03, m13)
    float32x4x2_t r23 = vtrnq_f32(r2, r3); // (m20, m30, m22, m32), (m21, m31, m23, m33)
    r0 = vcombine_f32(vget_low_f32(r01.val[0]), vget_low_f32(r23.val[0])); // (m00, m10, m20, m30)
    r1 = vcombine_f32(vget_low_f32(r01.val[1]), vget_low_f32(r23.val[1])); // (m01, m11, m21, m31)
    r2 = vcombine_f32(vget_high_f32(r01.val[0]), vget_high_f32(r23.val[0])); // (m02, m12, m22, m32)
    r3 = vcombine_f32(vget_high_f32(r01.val[1]), vget_high_f32(r23.val[1])); // (m03, m13, m23, m33)
}
