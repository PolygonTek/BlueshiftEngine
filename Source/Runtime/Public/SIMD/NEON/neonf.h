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
BE_FORCE_INLINE neonf rcp_ps(const neonf &a) {
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
BE_FORCE_INLINE neonf rsqrt_12ps(const neonf &a) {
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
BE_FORCE_INLINE neonf shuffle_ps(const neonf &a) { return vreinterpretq_f32_u32(vshuffleq_s32(vreinterpretq_u32_f32(a), _MM_SHUFFLE(i3, i2, i1, i0))); }

// Shuffles two 4x32 bits floats using template parameters. ax, bx = [0, 3].
template <size_t a0, size_t a1, size_t b0, size_t b1>
BE_FORCE_INLINE neonf shuffle_ps(const neonf &a, const neonf &b) { return vshuffleq_f32(a, b, _MM_SHUFFLE(b1, b0, a1, a0)); }

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
BE_FORCE_INLINE size_t select_min_ps(const neonf &a) { return __bsf(vmovemaskq_f32(a == vreduce_min_ps(a))); }

// Return index of maximum component.
BE_FORCE_INLINE size_t select_max_ps(const neonf &a) { return __bsf(vmovemaskq_f32(a == vreduce_max_ps(a))); }

// Return index of minimum component with valid index mask.
BE_FORCE_INLINE size_t select_min_ps(const neonf &a, const neonb &validmask) {
    const neonf v = select_ps(set1_ps(FLT_INFINITY), a, validmask);
    return __bsf(vmovemaskq_f32(validmask & (v == vreduce_min_ps(v))));
}

// Return index of maximum component with valid index mask.
BE_FORCE_INLINE size_t select_max_ps(const neonf &a, const neonb &validmask) {
    const neonf v = select_ps(set1_ps(-FLT_INFINITY), a, validmask);
    return __bsf(vmovemaskq_f32(validmask & (v == vreduce_max_ps(v))));
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
