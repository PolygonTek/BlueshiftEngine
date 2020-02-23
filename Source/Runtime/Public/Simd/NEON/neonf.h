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
    return load_ps(src);
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

BE_FORCE_INLINE neonf epi32_to_ps(const i32x4 a) {
    return vcvtq_f32_s32(vreinterpretq_s32_m128i(a));
}

BE_FORCE_INLINE neonf abs_ps(const neonf &a) {
    return vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(a), vdupq_n_s32(0x7fffffff)));
}

BE_FORCE_INLINE neonf sqr_ps(const neonf &a) {
    return vmulq_f32(a, a);
}

BE_FORCE_INLINE neonf sqrt_ps(const neonf &a) {
    return _mm_sqrt_ps(a.f32x4);
}

// Reciprocal with 12 bits of precision.
BE_FORCE_INLINE neonf rcp_ps(const neonf &a) {
    return vrecpeq_f32(a.f32x4);
}

// Reciprocal with at least 16 bits precision.
BE_FORCE_INLINE neonf rcp16_ps(const neonf &a) {
    neonf r = vrecpeq_f32(a.f32x4);
    // Newton-Raphson approximation to improve precision.
    return vmulq_f32(r, vsubq_f32(vdupq_n_f32(2.0f), vmulq_f32(a, r)));
}

// Reciprocal with close to full precision.
BE_FORCE_INLINE neonf rcp32_ps(const neonf &a) {
    neonf r = vrecpeq_f32(a.f32x4);
    // Newton-Raphson approximation to improve precision.
    r = vmulq_f32(r, vsubq_f32(vdupq_n_f32(2.0f), vmulq_f32(a, r)));
    return vmulq_f32(r, vsubq_f32(vdupq_n_f32(2.0f), vmulq_f32(a, r)));
}

// Divide with at least 16 bits precision.
BE_FORCE_INLINE neonf div16_ps(const neonf &a, const neonf &b) {
    return vmulq_f32(a.f32x4, rcp16_ps(b.f32x4));
}

// Divide with close to full precision.
BE_FORCE_INLINE neonf div32_ps(const neonf &a, const neonf &b) {
    return vmulq_f32(a.f32x4, rcp32_ps(b.f32x4));
}

// Reciprocal square root with 12 bits of precision.
BE_FORCE_INLINE neonf rsqrt_ps(const neonf &a) {
    return vrsqrteq_f32(a.f32x4);
}

// Reciprocal square root with at least 16 bits precision.
BE_FORCE_INLINE neonf rsqrt16_ps(const neonf &a) {
    neonf r = vrsqrteq_f32(a.f32x4);
    // Newton-Raphson approximation to improve precision.
    return vaddq_f32(vmulq_f32(vdupq_n_f32(1.5f), r), vmulq_f32(vmulq_f32(vmulq_f32(a, vdupq_n_f32(-0.5f)), r), vmulq_f32(r, r)));
}

// Reciprocal square root with close to full precision.
BE_FORCE_INLINE neonf rsqrt32_ps(const neonf &a) {
    neonf r = vrsqrteq_f32(a.f32x4);
    // Newton-Raphson approximation to improve precision.
    r = vaddq_f32(vmulq_f32(vdupq_n_f32(1.5f), r), vmulq_f32(vmulq_f32(vmulq_f32(a, vdupq_n_f32(-0.5f)), r), vmulq_f32(r, r)));
    return vaddq_f32(vmulq_f32(vdupq_n_f32(1.5f), r), vmulq_f32(vmulq_f32(vmulq_f32(a, vdupq_n_f32(-0.5f)), r), vmulq_f32(r, r)));
}

BE_FORCE_INLINE neonf operator+(const neonf &a) { return a; }
BE_FORCE_INLINE neonf operator-(const neonf &a) { return vnegq_f32(a.f32x4); }

BE_FORCE_INLINE neonf operator+(const neonf &a, const neonf &b) { return vaddq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf operator+(const neonf &a, const float &b) { return a + set1_ps(b); }
BE_FORCE_INLINE neonf operator+(const float &a, const neonf &b) { return set1_ps(a) + b; }

BE_FORCE_INLINE neonf operator-(const neonf &a, const neonf &b) { return vsubq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf operator-(const neonf &a, const float &b) { return a - set1_ps(b); }
BE_FORCE_INLINE neonf operator-(const float &a, const neonf &b) { return set1_ps(a) - b; }

BE_FORCE_INLINE neonf operator*(const neonf &a, const neonf &b) { return vmulq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf operator*(const neonf &a, const float &b) { return a * set1_ps(b); }
BE_FORCE_INLINE neonf operator*(const float &a, const neonf &b) { return set1_ps(a) * b; }

BE_FORCE_INLINE neonf operator/(const neonf &a, const neonf &b) { return a * rcp32_ps(b); }
BE_FORCE_INLINE neonf operator/(const neonf &a, const float &b) { return a * rcp32_ps(set1_ps(b)); }
BE_FORCE_INLINE neonf operator/(const float &a, const neonf &b) { return a * rcp32_ps(b); }

BE_FORCE_INLINE neonf operator&(const neonf &a, const neonf &b) { return _mm_and_ps(a.m128, b.m128); }
BE_FORCE_INLINE neonf operator&(const neonf &a, const neoni &b) { return _mm_and_ps(a.m128, _mm_castsi128_ps(b.m128i)); }

BE_FORCE_INLINE neonf operator|(const neonf &a, const neonf &b) { return _mm_or_ps(a.m128, b.m128); }
BE_FORCE_INLINE neonf operator|(const neonf &a, const neoni &b) { return _mm_or_ps(a.m128, _mm_castsi128_ps(b.m128i)); }

BE_FORCE_INLINE neonf operator^(const neonf &a, const neonf &b) { return veorq_s32(a.i32x4, b.i32x4); }
BE_FORCE_INLINE neonf operator^(const neonf &a, const neoni &b) { return veorq_s32(a.i32x4, b.i32x4); }

BE_FORCE_INLINE neonf operator==(const neonf &a, const neonf &b) { return vceqq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf operator==(const neonf &a, const float &b) { return a == set1_ps(b); }
BE_FORCE_INLINE neonf operator==(const float &a, const neonf &b) { return set1_ps(a) == b; }

BE_FORCE_INLINE neonf operator!=(const neonf &a, const neonf &b) { return vmvnq_u32(vceqq_f32(a.f32x4, b.f32x4)); }
BE_FORCE_INLINE neonf operator!=(const neonf &a, const float &b) { return a != set1_ps(b); }
BE_FORCE_INLINE neonf operator!=(const float &a, const neonf &b) { return set1_ps(a) != b; }

BE_FORCE_INLINE neonf operator<(const neonf &a, const neonf &b) { return vcltq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf operator<(const neonf &a, const float &b) { return a < set1_ps(b); }
BE_FORCE_INLINE neonf operator<(const float &a, const neonf &b) { return set1_ps(a) < b; }

BE_FORCE_INLINE neonf operator>(const neonf &a, const neonf &b) { return vcgtq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf operator>(const neonf &a, const float &b) { return a > set1_ps(b); }
BE_FORCE_INLINE neonf operator>(const float &a, const neonf &b) { return set1_ps(a) > b; }

BE_FORCE_INLINE neonf operator>=(const neonf &a, const neonf &b) { return vcgeq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf operator>=(const neonf &a, const float &b) { return a >= set1_ps(b); }
BE_FORCE_INLINE neonf operator>=(const float &a, const neonf &b) { return set1_ps(a) >= b; }

BE_FORCE_INLINE neonf operator<=(const neonf &a, const neonf &b) { return vcleq_f32(a.f32x4, b.f32x4); }
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
BE_FORCE_INLINE neonf madd_ps(const neonf &a, const neonf &b, const neonf &c) { return vmlaq_f32(c.f32x4, a.f32x4, b.f32x4); }
// dst = -(a * b) + c
BE_FORCE_INLINE neonf nmadd_ps(const neonf &a, const neonf &b, const neonf &c) { return vmlsq_f32(c.f32x4, a.f32x4, b.f32x4); }
// dst = a * b - c
BE_FORCE_INLINE neonf msub_ps(const neonf &a, const neonf &b, const neonf &c) { return neg_ps(vmlsq_f32(c.f32x4, a.f32x4, b.f32x4)); }
// dst = -(a * b) - c
BE_FORCE_INLINE neonf nmsub_ps(const neonf &a, const neonf &b, const neonf &c) { return neg_ps(vmlaq_f32(c.f32x4, a.f32x4, b.f32x4)); }

#ifdef __SSE4_1__
BE_FORCE_INLINE neonf floor_ps(const neonf &a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF); }
BE_FORCE_INLINE neonf ceil_ps(const neonf &a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF); }
BE_FORCE_INLINE neonf trunc_ps(const neonf &a) { return _mm_round_ps(a, _MM_FROUND_TO_ZERO); }
BE_FORCE_INLINE neonf round_ps(const neonf &a) { return _mm_round_ps(a, _MM_FROUND_TO_NEAREST_INT); }
#else
BE_FORCE_INLINE neonf floor_ps(const neonf &a) { return neonf(floorf(a[0]), floorf(a[1]), floorf(a[2]), floorf(a[3])); }
BE_FORCE_INLINE neonf ceil_ps(const neonf &a) { return neonf(ceilf(a[0]), ceilf(a[1]), ceilf(a[2]), ceilf(a[3])); }
BE_FORCE_INLINE neonf trunc_ps(const neonf &a) { return neonf(truncf(a[0]), truncf(a[1]), truncf(a[2]), truncf(a[3])); }
BE_FORCE_INLINE neonf round_ps(const neonf &a) { return neonf(roundf(a[0]), roundf(a[1]), roundf(a[2]), roundf(a[3])); }
#endif

BE_FORCE_INLINE neonf frac_ps(const neonf &a) { return a - floor_ps(a); }

// Unpack to (a0, b0, a1, b1).
BE_FORCE_INLINE neonf unpacklo_ps(const neonf &a, const neonf &b) { return _mm_unpacklo_ps(a.f32x4, b.f32x4); }

// Unpack to (a2, b2, a3, b3).
BE_FORCE_INLINE neonf unpackhi_ps(const neonf &a, const neonf &b) { return _mm_unpackhi_ps(a.f32x4, b.f32x4); }

// Shuffles 4x32 bits floats using template parameters. ix = [0, 3].
template <size_t i0, size_t i1, size_t i2, size_t i3>
BE_FORCE_INLINE neonf shuffle_ps(const neonf &a) { return _mm_perm_ps(a, _MM_SHUFFLE(i3, i2, i1, i0)); }

template <>
BE_FORCE_INLINE neonf shuffle_ps<0, 0, 0, 0>(const neonf &a) { return vdupq_lane_f32(vget_low_f32(a), 0); }
template <>
BE_FORCE_INLINE neonf shuffle_ps<1, 1, 1, 1>(const neonf &a) { return vdupq_lane_f32(vget_low_f32(a), 1); }
template <>
BE_FORCE_INLINE neonf shuffle_ps<2, 2, 2, 2>(const neonf &a) { return vdupq_lane_f32(vget_high_f32(a), 0); }
template <>
BE_FORCE_INLINE neonf shuffle_ps<3, 3, 3, 3>(const neonf &a) { return vdupq_lane_f32(vget_high_f32(a), 1); }

// Shuffles two 4x32 bits floats using template parameters. ax, bx = [0, 3].
template <size_t a0, size_t a1, size_t b0, size_t b1>
BE_FORCE_INLINE neonf shuffle_ps(const neonf &a, const neonf &b) { return _mm_shuffle_ps(a, b, _MM_SHUFFLE(b1, b0, a1, a0)); }

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
BE_FORCE_INLINE neonf insert_ps(const neonf &a, const neonf &b) { neonf c = a; c[dst & 3] = b[src & 3]; return c; }

// Insert b to a in [32*dst, 32*dst+31] bits.
template <int dst>
BE_FORCE_INLINE neonf insert_ps(const neonf &a, float b) { neonf c = a; c[dst & 3] = b; return c; }

// Select 4x32 bits floats using mask.
BE_FORCE_INLINE neonf select_ps(const neonf &a, const neonf &b, const neonb &mask) {
    // dst = (a & !mask) | (b & mask)
    return _mm_or_ps(_mm_andnot_ps(_mm_castsi128_ps(mask), a), _mm_and_ps(_mm_castsi128_ps(mask), b));
}

BE_FORCE_INLINE neonf min_ps(const neonf &a, const neonf &b) { return vminq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf min_ps(const neonf &a, const float &b) { return vminq_f32(a.f32x4, set1_ps(b)); }
BE_FORCE_INLINE neonf min_ps(const float &a, const neonf &b) { return vminq_f32(set1_ps(a), b.f32x4); }

BE_FORCE_INLINE neonf max_ps(const neonf &a, const neonf &b) { return vmaxq_f32(a.f32x4, b.f32x4); }
BE_FORCE_INLINE neonf max_ps(const neonf &a, const float &b) { return vmaxq_f32(a.f32x4, set1_ps(b)); }
BE_FORCE_INLINE neonf max_ps(const float &a, const neonf &b) { return vmaxq_f32(set1_ps(a), b.f32x4); }

// Broadcast minimum value of all 4 components.
BE_FORCE_INLINE neonf vreduce_min_ps(const neonf &v) {
    neonf h = min_ps(shuffle_ps<1, 0, 3, 2>(v), v);
    return min_ps(shuffle_ps<2, 3, 0, 1>(h), h);
}

// Broadcast maximum value of all 4 components.
BE_FORCE_INLINE neonf vreduce_max_ps(const neonf &v) {
    neonf h = max_ps(shuffle_ps<1, 0, 3, 2>(v), v);
    return max_ps(shuffle_ps<2, 3, 0, 1>(h), h);
}

BE_FORCE_INLINE float reduce_min_ps(const neonf &v) { return _mm_cvtss_f32(vreduce_min_ps(v)); }
BE_FORCE_INLINE float reduce_max_ps(const neonf &v) { return _mm_cvtss_f32(vreduce_max_ps(v)); }

// Return index of minimum component.
BE_FORCE_INLINE size_t select_min_ps(const neonf &v) { return __bsf(_mm_movemask_ps(v == vreduce_min_ps(v))); }

// Return index of maximum component.
BE_FORCE_INLINE size_t select_max_ps(const neonf &v) { return __bsf(_mm_movemask_ps(v == vreduce_max_ps(v))); }

// Return index of minimum component with valid index mask.
BE_FORCE_INLINE size_t select_min_ps(const neonf &v, const neonb &validmask) {
    const neonf a = select_ps(set1_ps(FLT_INFINITY), v, validmask);
    return __bsf(_mm_movemask_ps(validmask & (a == vreduce_min_ps(a))));
}

// Return index of maximum component with valid index mask.
BE_FORCE_INLINE size_t select_max_ps(const neonf &v, const neonb &validmask) {
    const neonf a = select_ps(set1_ps(-FLT_INFINITY), v, validmask);
    return __bsf(_mm_movemask_ps(validmask & (a == vreduce_max_ps(a))));
}

// Broadcast sums of all components.
BE_FORCE_INLINE neonf sum_ps(const neonf &a) {
    __m128 hadd = _mm_hadd_ps(a, a); // (x + y, z + w, x + y, z + w)
    return _mm_hadd_ps(hadd, hadd); // (x + y + z + w, x + y + z + w, x + y + z + w, x + y + z + w)
}

// Broadcast dot4 product.
BE_FORCE_INLINE neonf dot4_ps(const neonf &a, const neonf &b) {
    return sum_ps(a * b);
}
