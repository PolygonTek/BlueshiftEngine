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

#ifdef __ARM_NEON__
#include "arm_neon.h"
#endif

// 4 wide NEON float type.
struct neonf {
    union {
        float32x4_t f32x4;
        int32x4_t   i32x4;
        float       f32[4];
        int32_t     i32[4];
    };

    BE_FORCE_INLINE neonf() = default;
    BE_FORCE_INLINE neonf(const float32x4_t a) { f32x4 = a; }
    BE_FORCE_INLINE neonf(float a, float b, float c, float d) { ALIGN_AS16 float data[4] = { a, b, c, d }; f32x4 = vld1q_f32(data); }
    BE_FORCE_INLINE neonf(const neonf &other) { f32x4 = other.f32x4; }
    BE_FORCE_INLINE neonf &operator=(const neonf &rhs) { f32x4 = rhs.f32x4; return *this; }

    BE_FORCE_INLINE operator const float32x4_t &() const { return f32x4; };
    BE_FORCE_INLINE operator float32x4_t &() { return f32x4; };

    BE_FORCE_INLINE operator const int32x4_t &() const { return i32x4; };
    BE_FORCE_INLINE operator int32x4_t &() { return i32x4; };

    BE_FORCE_INLINE const float &operator[](size_t index) const { assert(index < 4); return f32[index]; }
    BE_FORCE_INLINE float &operator[](size_t index) { assert(index < 4); return f32[index]; }
};

// 4 wide NEON integer type.
struct neoni {
    union {
        float32x4_t f32x4;
        int32x4_t   i32x4;
        float       f32[4];
        int32_t     i32[4];
        int16_t     i16[8];
        int8_t      i8[16];
    };

    BE_FORCE_INLINE neoni() = default;
    BE_FORCE_INLINE neoni(const int32x4_t a) { i32x4 = a; }
    BE_FORCE_INLINE neoni(int32_t a, int32_t b, int32_t c, int32_t d) { ALIGN_AS16 int32_t data[4] = { a, b, c, d }; i32x4 = vld1q_s32(data); }
    BE_FORCE_INLINE neoni(const neoni &other) { i32x4 = other.i32x4; }
    BE_FORCE_INLINE neoni &operator=(const neoni &rhs) { i32x4 = rhs.i32x4; return *this; }

    BE_FORCE_INLINE operator const float32x4_t &() const { return f32x4; };
    BE_FORCE_INLINE operator float32x4_t &() { return f32x4; };

    BE_FORCE_INLINE operator const int32x4_t &() const { return i32x4; };
    BE_FORCE_INLINE operator int32x4_t &() { return i32x4; };

    BE_FORCE_INLINE const int32_t &operator[](size_t index) const { assert(index < 4); return i32[index]; }
    BE_FORCE_INLINE int32_t &operator[](size_t index) { assert(index < 4); return i32[index]; }
};

// 4 wide NEON boolean type.
struct neonb {
    union {
        float32x4_t f32x4;
        int32x4_t   i32x4;
        float       f32[4];
        int32_t     i32[4];
    };

    BE_FORCE_INLINE neonb() = default;
    BE_FORCE_INLINE neonb(const float32x4_t a) { f32x4 = a; }
    BE_FORCE_INLINE neonb(const int32x4_t a) { i32x4 = a; }
    BE_FORCE_INLINE neonb(const neonb &other) { i32x4 = other.i32x4; }
    BE_FORCE_INLINE neonb &operator=(const neonb &rhs) { f32x4 = rhs.f32x4; return *this; }

    BE_FORCE_INLINE operator const float32x4_t &() const { return f32x4; };
    BE_FORCE_INLINE operator float32x4_t &() { return f32x4; };

    BE_FORCE_INLINE operator const int32x4_t &() const { return i32x4; };
    BE_FORCE_INLINE operator int32x4_t &() { return i32x4; };

    BE_FORCE_INLINE bool operator[](size_t index) const { assert(index < 4); return !!i32[index]; }
    BE_FORCE_INLINE int32_t &operator[](size_t index) { assert(index < 4); return i32[index]; }
};

using simd4f = neonf;
using simd4i = neoni;
using simd4b = neonb;
