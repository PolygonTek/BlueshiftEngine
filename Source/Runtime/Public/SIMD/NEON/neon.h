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

#if defined(__ARM__) && defined(__NEON__)
    // MSVC specific header
    #ifdef _M_ARM64
        #include <arm64_neon.h>
    #else
        #include <arm_neon.h>
    #endif
#endif

#if defined(__ARM64__)
    #define vsplatq_f32(a, imm)     vdupq_laneq_f32((a), (imm))
#else
    #define vsplatq_f32(a, imm)     vdupq_n_f32(vgetq_lane_f32((a), (imm)))
#endif

// Takes the upper 64 bits of a and places it in the low end of the result
// Takes the lower 64 bits of a and places it into the high end of the result.
BE_FORCE_INLINE float32x4_t vpermq_f32_1032(float32x4_t a) {
    float32x2_t a32 = vget_high_f32(a);
    float32x2_t a10 = vget_low_f32(a);
    return vcombine_f32(a32, a10);
}

// takes the lower two 32-bit values from a and swaps them and places in low end
// of result takes the higher two 32 bit values from a and swaps them and places
// in high end of result.
BE_FORCE_INLINE float32x4_t vpermq_f32_2301(float32x4_t a) {
    return vrev64q_f32(a);
}

// rotates the least significant 32 bits into the most signficant 32 bits, and
// shifts the rest down
BE_FORCE_INLINE float32x4_t vpermq_f32_0321(float32x4_t a) {
    return vextq_f32(a, a, 1);
}

// rotates the most significant 32 bits into the least signficant 32 bits, and
// shifts the rest up
BE_FORCE_INLINE float32x4_t vpermq_f32_2103(float32x4_t a) {
    return vextq_f32(a, a, 3);
}

// gets the lower 64 bits of a, and places it in the upper 64 bits
// gets the lower 64 bits of a and places it in the lower 64 bits
BE_FORCE_INLINE float32x4_t vpermq_f32_1010(float32x4_t a) {
    float32x2_t a10 = vget_low_f32(a);
    return vcombine_f32(a10, a10);
}

// gets the lower 64 bits of a, swaps the 0 and 1 elements, and places it in the
// lower 64 bits gets the lower 64 bits of a, and places it in the upper 64 bits
BE_FORCE_INLINE float32x4_t vpermq_f32_1001(float32x4_t a) {
    float32x2_t a01 = vrev64_f32(vget_low_f32(a));
    float32x2_t a10 = vget_low_f32(a);
    return vcombine_f32(a01, a10);
}

// gets the lower 64 bits of a, swaps the 0 and 1 elements and places it in the
// upper 64 bits gets the lower 64 bits of a, swaps the 0 and 1 elements, and
// places it in the lower 64 bits
BE_FORCE_INLINE float32x4_t vpermq_f32_0101(float32x4_t a) {
    float32x2_t a01 = vrev64_f32(vget_low_f32(a));
    return vcombine_f32(a01, a01);
}

BE_FORCE_INLINE float32x4_t vpermq_f32_2211(float32x4_t a) {
    float32x2_t a11 = vdup_lane_f32(vget_low_f32(a), 1);
    float32x2_t a22 = vdup_lane_f32(vget_high_f32(a), 0);
    return vcombine_f32(a11, a22);
}

BE_FORCE_INLINE float32x4_t vpermq_f32_0122(float32x4_t a) {
    float32x2_t a22 = vdup_lane_f32(vget_high_f32(a), 0);
    float32x2_t a01 = vrev64_f32(vget_low_f32(a));
    return vcombine_f32(a22, a01);
}

BE_FORCE_INLINE float32x4_t vpermq_f32_3332(float32x4_t a) {
    float32x2_t a32 = vget_high_f32(a);
    float32x2_t a33 = vdup_lane_f32(vget_high_f32(a), 1);
    return vcombine_f32(a32, a33);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_0000(float32x4_t a, float32x4_t b) {
    float32x2_t a00 = vdup_lane_f32(vget_low_f32(a), 0);
    float32x2_t b00 = vdup_lane_f32(vget_low_f32(b), 0);
    return vcombine_f32(a00, b00);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_1111(float32x4_t a, float32x4_t b) {
    float32x2_t a11 = vdup_lane_f32(vget_low_f32(a), 1);
    float32x2_t b11 = vdup_lane_f32(vget_low_f32(b), 1);
    return vcombine_f32(a11, b11);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_2222(float32x4_t a, float32x4_t b) {
    float32x2_t a22 = vdup_lane_f32(vget_high_f32(a), 0);
    float32x2_t b22 = vdup_lane_f32(vget_high_f32(b), 0);
    return vcombine_f32(a22, b22);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_3333(float32x4_t a, float32x4_t b) {
    float32x2_t a33 = vdup_lane_f32(vget_high_f32(a), 1);
    float32x2_t b33 = vdup_lane_f32(vget_high_f32(b), 1);
    return vcombine_f32(a33, b33);
}

// Takes the upper 64 bits of a and places it in the low end of the result
// Takes the lower 64 bits of b and places it into the high end of the result.
BE_FORCE_INLINE float32x4_t vshufq_f32_1032(float32x4_t a, float32x4_t b) {
    float32x2_t a32 = vget_high_f32(a);
    float32x2_t b10 = vget_low_f32(b);
    return vcombine_f32(a32, b10);
}

// takes the lower two 32-bit values from a and swaps them and places in high
// end of result takes the higher two 32 bit values from b and swaps them and
// places in low end of result.
BE_FORCE_INLINE float32x4_t vshufq_f32_2301(float32x4_t a, float32x4_t b) {
    float32x2_t a01 = vrev64_f32(vget_low_f32(a));
    float32x2_t b23 = vrev64_f32(vget_high_f32(b));
    return vcombine_f32(a01, b23);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_0321(float32x4_t a, float32x4_t b) {
    float32x2_t a21 = vget_high_f32(vextq_f32(a, a, 3));
    float32x2_t b03 = vget_low_f32(vextq_f32(b, b, 3));
    return vcombine_f32(a21, b03);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_2103(float32x4_t a, float32x4_t b) {
    float32x2_t a03 = vget_low_f32(vextq_f32(a, a, 3));
    float32x2_t b21 = vget_high_f32(vextq_f32(b, b, 3));
    return vcombine_f32(a03, b21);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_1010(float32x4_t a, float32x4_t b) {
    float32x2_t a10 = vget_low_f32(a);
    float32x2_t b10 = vget_low_f32(b);
    return vcombine_f32(a10, b10);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_1001(float32x4_t a, float32x4_t b) {
    float32x2_t a01 = vrev64_f32(vget_low_f32(a));
    float32x2_t b10 = vget_low_f32(b);
    return vcombine_f32(a01, b10);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_0101(float32x4_t a, float32x4_t b) {
    float32x2_t a01 = vrev64_f32(vget_low_f32(a));
    float32x2_t b01 = vrev64_f32(vget_low_f32(b));
    return vcombine_f32(a01, b01);
}

// keeps the low 64 bits of b in the low and puts the high 64 bits of a in the high
BE_FORCE_INLINE float32x4_t vshufq_f32_3210(float32x4_t a, float32x4_t b) {
    float32x2_t a10 = vget_low_f32(a);
    float32x2_t b32 = vget_high_f32(b);
    return vcombine_f32(a10, b32);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_0011(float32x4_t a, float32x4_t b) {
    float32x2_t a11 = vdup_lane_f32(vget_low_f32(a), 1);
    float32x2_t b00 = vdup_lane_f32(vget_low_f32(b), 0);
    return vcombine_f32(a11, b00);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_0022(float32x4_t a, float32x4_t b) {
    float32x2_t a22 = vdup_lane_f32(vget_high_f32(a), 0);
    float32x2_t b00 = vdup_lane_f32(vget_low_f32(b), 0);
    return vcombine_f32(a22, b00);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_2200(float32x4_t a, float32x4_t b) {
    float32x2_t a00 = vdup_lane_f32(vget_low_f32(a), 0);
    float32x2_t b22 = vdup_lane_f32(vget_high_f32(b), 0);
    return vcombine_f32(a00, b22);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_3202(float32x4_t a, float32x4_t b) {
    float32_t a0 = vgetq_lane_f32(a, 0);
    float32x2_t a22 = vdup_lane_f32(vget_high_f32(a), 0);
    float32x2_t a02 = vset_lane_f32(a0, a22, 1); /* TODO: use vzip ?*/
    float32x2_t b32 = vget_high_f32(b);
    return vcombine_f32(a02, b32);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_3232(float32x4_t a, float32x4_t b) {
    float32x2_t a32 = vget_high_f32(a);
    float32x2_t b32 = vget_high_f32(b);
    return vcombine_f32(b32, a32);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_1133(float32x4_t a, float32x4_t b) {
    float32x2_t a33 = vdup_lane_f32(vget_high_f32(a), 1);
    float32x2_t b11 = vdup_lane_f32(vget_low_f32(b), 1);
    return vcombine_f32(a33, b11);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_2010(float32x4_t a, float32x4_t b) {
    float32x2_t a10 = vget_low_f32(a);
    float32_t b2 = vgetq_lane_f32(b, 2);
    float32x2_t b00 = vdup_lane_f32(vget_low_f32((b)), 0);
    float32x2_t b20 = vset_lane_f32(b2, b00, 1);
    return vcombine_f32(a10, b20);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_2001(float32x4_t a, float32x4_t b) {
    float32x2_t a01 = vrev64_f32(vget_low_f32(a));
    float32_t b2 = vgetq_lane_f32(b, 2);
    float32x2_t b00 = vdup_lane_f32(vget_low_f32(b), 0);
    float32x2_t b20 = vset_lane_f32(b2, b00, 1);
    return vcombine_f32(a01, b20);
}

BE_FORCE_INLINE float32x4_t vshufq_f32_2032(float32x4_t a, float32x4_t b) {
    float32x2_t a32 = vget_high_f32(a);
    float32_t b2 = vgetq_lane_f32(b, 2);
    float32x2_t b00 = vdup_lane_f32(vget_low_f32(b), 0);
    float32x2_t b20 = vset_lane_f32(b2, b00, 1);
    return vcombine_f32(a32, b20);
}

// Shuffle packed 8-bit integers in a according to shuffle control mask in the
// corresponding 8-bit element of b, and store the results in dst.
// https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_shuffle_epi8&expand=5146
BE_FORCE_INLINE int32x4_t vshufq_s8(int32x4_t a, int32x4_t b) {
#if defined(__ARM64__)
    int8x16_t tbl = vreinterpretq_s8_s32(a);   // input a
    uint8x16_t idx = vreinterpretq_u8_s32(b);  // input b
    uint8_t __attribute__((aligned(16))) mask[16] = { 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F };
    uint8x16_t idx_masked = vandq_u8(idx, vld1q_u8(mask));  // avoid using meaningless bits

    return vreinterpretq_s32_s8(vqtbl1q_s8(tbl, idx_masked));
#else
    uint8_t *tbl = (uint8_t *)&a;  // input a
    uint8_t *idx = (uint8_t *)&b;  // input b
    int32_t r[4];

    r[0] = ((idx[3] & 0x80) ? 0 : tbl[idx[3] % 16]) << 24;
    r[0] |= ((idx[2] & 0x80) ? 0 : tbl[idx[2] % 16]) << 16;
    r[0] |= ((idx[1] & 0x80) ? 0 : tbl[idx[1] % 16]) << 8;
    r[0] |= ((idx[0] & 0x80) ? 0 : tbl[idx[0] % 16]);

    r[1] = ((idx[7] & 0x80) ? 0 : tbl[idx[7] % 16]) << 24;
    r[1] |= ((idx[6] & 0x80) ? 0 : tbl[idx[6] % 16]) << 16;
    r[1] |= ((idx[5] & 0x80) ? 0 : tbl[idx[5] % 16]) << 8;
    r[1] |= ((idx[4] & 0x80) ? 0 : tbl[idx[4] % 16]);

    r[2] = ((idx[11] & 0x80) ? 0 : tbl[idx[11] % 16]) << 24;
    r[2] |= ((idx[10] & 0x80) ? 0 : tbl[idx[10] % 16]) << 16;
    r[2] |= ((idx[9] & 0x80) ? 0 : tbl[idx[9] % 16]) << 8;
    r[2] |= ((idx[8] & 0x80) ? 0 : tbl[idx[8] % 16]);

    r[3] = ((idx[15] & 0x80) ? 0 : tbl[idx[15] % 16]) << 24;
    r[3] |= ((idx[14] & 0x80) ? 0 : tbl[idx[14] % 16]) << 16;
    r[3] |= ((idx[13] & 0x80) ? 0 : tbl[idx[13] % 16]) << 8;
    r[3] |= ((idx[12] & 0x80) ? 0 : tbl[idx[12] % 16]);

    return vld1q_s32(r);
#endif
}

#if !defined(__ARM64__)
BE_FORCE_INLINE float32x4_t vsqrtq_f32(float32x4_t s) {
    float32x4_t x = vrsqrteq_f32(s);
    // Code to handle sqrt(0).
    // If the input to sqrtf() is zero, a zero will be returned.
    // If the input to vrsqrteq_f32() is zero, positive infinity is returned.
    const uint32x4_t vec_p_inf = vdupq_n_u32(0x7F800000);
    // check for divide by zero
    const uint32x4_t div_by_zero = vceqq_u32(vec_p_inf, vreinterpretq_u32_f32(x));
    // zero out the positive infinity results
    x = vreinterpretq_f32_u32(vandq_u32(vmvnq_u32(div_by_zero), vreinterpretq_u32_f32(x)));
    // from arm documentation
    // The Newton-Raphson iteration:
    //     x[n+1] = x[n] * (3 - d * (x[n] * x[n])) / 2)
    // converges to (1/sqrt(d)) if x0 is the result of VRSQRTE applied to d.
    //
    // Note: The precision did not improve after 2 iterations.
    x = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x, x), s), x);
    x = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x, x), s), x);

    // sqrt(s) = s * 1/sqrt(s)
    return vmulq_f32(s, x);
}
#endif

// Unpack to (a0, b0, a1, b1).
BE_FORCE_INLINE float32x4_t vunpackloq_f32(const float32x4_t a, const float32x4_t b) {
    float32x2_t al = vget_low_f32(a);
    float32x2_t bl = vget_low_f32(b);
    float32x2x2_t result = vzip_f32(al, bl);
    return vcombine_f32(result.val[0], result.val[1]);
}

// Unpack to (a2, b2, a3, b3).
BE_FORCE_INLINE float32x4_t vunpackhiq_f32(const float32x4_t a, const float32x4_t b) {
    float32x2_t ah = vget_high_f32(a);
    float32x2_t bh = vget_high_f32(b);
    float32x2x2_t result = vzip_f32(ah, bh);
    return vcombine_f32(result.val[0], result.val[1]);
}

// Creates a 4-bit mask from the most significant bits of the four
// single-precision, floating-point values.
// https://msdn.microsoft.com/en-us/library/vstudio/4490ys29(v=vs.100).aspx
BE_FORCE_INLINE int vmovemaskq_f32(float32x4_t a) {
#if 0 /* C version */
    uint32x4_t &ia = *(uint32x4_t *)&a;
    return (ia[0] >> 31) | ((ia[1] >> 30) & 2) | ((ia[2] >> 29) & 4) |
        ((ia[3] >> 28) & 8);
#endif
    static const uint32x4_t movemask = { 1, 2, 4, 8 };
    static const uint32x4_t highbit = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
    uint32x4_t t0 = vreinterpretq_u32_f32(a);
    uint32x4_t t1 = vtstq_u32(t0, highbit);
    uint32x4_t t2 = vandq_u32(t1, movemask);
    uint32x2_t t3 = vorr_u32(vget_low_u32(t2), vget_high_u32(t2));
    return vget_lane_u32(t3, 0) | vget_lane_u32(t3, 1);
}

// Loads one cache line of data from address p to a location closer to the processor.
BE_FORCE_INLINE void prefetchL1(const void *p) { __builtin_prefetch(p, 0, 3); }
BE_FORCE_INLINE void prefetchL2(const void *p) { __builtin_prefetch(p, 0, 2); }
BE_FORCE_INLINE void prefetchL3(const void *p) { __builtin_prefetch(p, 0, 1); }
BE_FORCE_INLINE void prefetchNTA(const void *p) { __builtin_prefetch(p, 0, 0); }

// Guarantees that every preceding store is globally visible before any subsequent store.
BE_FORCE_INLINE void sfence() { __sync_synchronize(); }

struct neonf;
struct neoni;
struct neonb;

// 4 wide NEON float type.
struct neonf {
    union {
        float32x4_t f32x4;
        int32x4_t   i32x4;
        uint32x4_t  u32x4;
        float       f32[4];
        int32_t     i32[4];
    };

    BE_FORCE_INLINE neonf() = default;
    BE_FORCE_INLINE neonf(const float32x4_t a) { f32x4 = a; }
    BE_FORCE_INLINE neonf(const int32x4_t a) { i32x4 = a; }
    BE_FORCE_INLINE neonf(const uint32x4_t a) { u32x4 = a; }
    BE_FORCE_INLINE neonf(float a, float b, float c, float d) { ALIGN_AS16 float data[4] = { a, b, c, d }; f32x4 = vld1q_f32(data); }

    BE_FORCE_INLINE neonf(const neonf &other) { f32x4 = other.f32x4; }
    BE_FORCE_INLINE neonf &operator=(const neonf &rhs) { f32x4 = rhs.f32x4; return *this; }

    BE_FORCE_INLINE operator const neoni &() const { return *reinterpret_cast<const neoni *>(this); };
    BE_FORCE_INLINE operator neoni &() { return *reinterpret_cast<neoni *>(this); };

    BE_FORCE_INLINE operator const neonb &() const { return *reinterpret_cast<const neonb *>(this); };
    BE_FORCE_INLINE operator neonb &() { return *reinterpret_cast<neonb *>(this); };

    BE_FORCE_INLINE operator const float32x4_t &() const { return f32x4; };
    BE_FORCE_INLINE operator float32x4_t &() { return f32x4; };

    BE_FORCE_INLINE operator const int32x4_t &() const { return i32x4; };
    BE_FORCE_INLINE operator int32x4_t &() { return i32x4; };

    BE_FORCE_INLINE operator const uint32x4_t &() const { return u32x4; };
    BE_FORCE_INLINE operator uint32x4_t &() { return u32x4; };

    BE_FORCE_INLINE const float &operator[](size_t index) const { assert(index < 4); return f32[index]; }
    BE_FORCE_INLINE float &operator[](size_t index) { assert(index < 4); return f32[index]; }
};

// 4 wide NEON integer type.
struct neoni {
    union {
        float32x4_t f32x4;
        int32x4_t   i32x4;
        uint32x4_t  u32x4;
        int16x8_t   i16x8;
        uint16x8_t  u16x8;
        int8x16_t   i8x16;
        uint8x16_t  u8x16;
        float       f32[4];
        int32_t     i32[4];
        int16_t     i16[8];
        int8_t      i8[16];
    };

    BE_FORCE_INLINE neoni() = default;
    BE_FORCE_INLINE neoni(const float32x4_t a) { f32x4 = a; }
    BE_FORCE_INLINE neoni(const int32x4_t a) { i32x4 = a; }
    BE_FORCE_INLINE neoni(const uint32x4_t a) { u32x4 = a; }
    BE_FORCE_INLINE neoni(const int16x8_t a) { i16x8 = a; }
    BE_FORCE_INLINE neoni(const uint16x8_t a) { u16x8 = a; }
    BE_FORCE_INLINE neoni(const int8x16_t a) { i8x16 = a; }
    BE_FORCE_INLINE neoni(const uint8x16_t a) { u8x16 = a; }
    BE_FORCE_INLINE neoni(int32_t a, int32_t b, int32_t c, int32_t d) { ALIGN_AS16 int32_t data[4] = { a, b, c, d }; i32x4 = vld1q_s32(data); }

    BE_FORCE_INLINE neoni(const neoni &other) { i32x4 = other.i32x4; }
    BE_FORCE_INLINE neoni &operator=(const neoni &rhs) { i32x4 = rhs.i32x4; return *this; }

    BE_FORCE_INLINE operator const neonf &() const { return *reinterpret_cast<const neonf *>(this); };
    BE_FORCE_INLINE operator neonf &() { return *reinterpret_cast<neonf *>(this); };

    BE_FORCE_INLINE operator const neonb &() const { return *reinterpret_cast<const neonb *>(this); };
    BE_FORCE_INLINE operator neonb &() { return *reinterpret_cast<neonb *>(this); };

    BE_FORCE_INLINE operator const float32x4_t &() const { return f32x4; };
    BE_FORCE_INLINE operator float32x4_t &() { return f32x4; };

    BE_FORCE_INLINE operator const int32x4_t &() const { return i32x4; };
    BE_FORCE_INLINE operator int32x4_t &() { return i32x4; };

    BE_FORCE_INLINE operator const uint32x4_t &() const { return u32x4; };
    BE_FORCE_INLINE operator uint32x4_t &() { return u32x4; };

    BE_FORCE_INLINE operator const int16x8_t &() const { return i16x8; };
    BE_FORCE_INLINE operator int16x8_t &() { return i16x8; };

    BE_FORCE_INLINE operator const uint16x8_t &() const { return u16x8; };
    BE_FORCE_INLINE operator uint16x8_t &() { return u16x8; };

    BE_FORCE_INLINE operator const int8x16_t &() const { return i8x16; };
    BE_FORCE_INLINE operator int8x16_t &() { return i8x16; };

    BE_FORCE_INLINE operator const uint8x16_t &() const { return u8x16; };
    BE_FORCE_INLINE operator uint8x16_t &() { return u8x16; };

    BE_FORCE_INLINE const int32_t &operator[](size_t index) const { assert(index < 4); return i32[index]; }
    BE_FORCE_INLINE int32_t &operator[](size_t index) { assert(index < 4); return i32[index]; }
};

// 4 wide NEON boolean type.
struct neonb {
    union {
        float32x4_t f32x4;
        int32x4_t   i32x4;
        uint32x4_t  u32x4;
        float       f32[4];
        int32_t     i32[4];
        uint32_t    u32[4];
    };

    BE_FORCE_INLINE neonb() = default;
    BE_FORCE_INLINE neonb(const float32x4_t a) { f32x4 = a; }
    BE_FORCE_INLINE neonb(const int32x4_t a) { i32x4 = a; }
    BE_FORCE_INLINE neonb(const uint32x4_t a) { u32x4 = a; }
    BE_FORCE_INLINE neonb(bool a, bool b, bool c, bool d) { ALIGN_AS16 uint32_t data[4] = { a ? 0xFFFFFFFF : 0, b ? 0xFFFFFFFF : 0, c ? 0xFFFFFFFF : 0, d ? 0xFFFFFFFF : 0 }; u32x4 = vld1q_u32(data); }

    BE_FORCE_INLINE neonb(const neonb &other) { u32x4 = other.u32x4; }
    BE_FORCE_INLINE neonb &operator=(const neonb &rhs) { u32x4 = rhs.u32x4; return *this; }

    BE_FORCE_INLINE operator const neonf &() const { return *reinterpret_cast<const neonf *>(this); };
    BE_FORCE_INLINE operator neonf &() { return *reinterpret_cast<neonf *>(this); };

    BE_FORCE_INLINE operator const neoni &() const { return *reinterpret_cast<const neoni *>(this); };
    BE_FORCE_INLINE operator neoni &() { return *reinterpret_cast<neoni *>(this); };

    BE_FORCE_INLINE operator const float32x4_t &() const { return f32x4; };
    BE_FORCE_INLINE operator float32x4_t &() { return f32x4; };

    BE_FORCE_INLINE operator const int32x4_t &() const { return i32x4; };
    BE_FORCE_INLINE operator int32x4_t &() { return i32x4; };

    BE_FORCE_INLINE operator const uint32x4_t &() const { return u32x4; };
    BE_FORCE_INLINE operator uint32x4_t &() { return u32x4; };

    BE_FORCE_INLINE bool operator[](size_t index) const { assert(index < 4); return !!i32[index]; }
    BE_FORCE_INLINE int32_t &operator[](size_t index) { assert(index < 4); return i32[index]; }
};

using simd4f = neonf;
using simd4i = neoni;
using simd4b = neonb;

#include "neonb.h"
#include "neonf.h"
#include "neoni.h"
