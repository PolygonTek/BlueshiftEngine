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

BE_FORCE_INLINE neonf epi32_to_ps(const __m128i a) {
    return vcvtq_f32_s32(vreinterpretq_s32_m128i(a));
}

BE_FORCE_INLINE neonf abs_ps(const neonf &a) {
    return vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(a.m128), vdupq_n_s32(0x7fffffff)));
}

BE_FORCE_INLINE neonf sqr_ps(const neonf &a) {
    return vmulq_f32(a, a);
}
