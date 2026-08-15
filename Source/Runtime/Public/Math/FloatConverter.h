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

#include <limits>
#include <type_traits>

#include "Math/MathCommon.h"

BE_NAMESPACE_BEGIN

/// Controls whether FromF32 emits denormal values in the target float format.
enum class DenormalMode {
    FlushToZero,
    Preserve
};

/// Signed float converter.
template <typename T, int MBits, int EBits, int EBias, int SBit>
class FloatConverter {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
    static_assert(MBits > 0 && MBits <= IEEE_FLT_MANTISSA_BITS, "MBits must fit within a float mantissa");
    static_assert(EBits > 0 && EBits < 32, "EBits must fit within a uint32_t");
    static_assert(SBit >= MBits + EBits && SBit < 32, "SBit must follow the exponent field");
    static_assert(SBit < std::numeric_limits<T>::digits, "SBit must fit within T");

public:
    static float ToF32(T x) {
        const int e = (x >> MBits) & ((1u << EBits) - 1u);
        const int m = (x & ((1u << MBits) - 1u));
        const uint32_t s = (x >> SBit) & 1u;
        uint32_t i;

        if (e == 0) {
            if (m == 0) { // zero
                i = s << IEEE_FLT_SIGN_BIT;
                return reinterpret_cast<float &>(i);
            } else { // denormal
                return Math::Ldexp((s ? -(float)m : (float)m), 1 - EBias - MBits);
            }
        } else if (e == (1u << EBits) - 1u) {
            if (m == 0) { // INF
                i = (s << IEEE_FLT_SIGN_BIT) | (((1u << IEEE_FLT_EXPONENT_BITS) - 1u) << IEEE_FLT_MANTISSA_BITS);
            } else { // NaN
                i = (s << IEEE_FLT_SIGN_BIT) | (((1u << IEEE_FLT_EXPONENT_BITS) - 1u) << IEEE_FLT_MANTISSA_BITS) | ((uint32_t)m << (IEEE_FLT_MANTISSA_BITS - MBits));
            }
            return reinterpret_cast<float &>(i);
        } else { // normal number
            return Math::Ldexp((s ? -1.0f : 1.0f) * (1.0f + m / (float)(1u << MBits)), e - EBias);
        }
    }

    static T FromF32(float x, DenormalMode denormalMode = DenormalMode::FlushToZero) {
        const uint32_t f = reinterpret_cast<uint32_t &>(x);
        const T s = (T)(f >> IEEE_FLT_SIGN_BIT) << SBit;
        const uint32_t f32Exponent = (f >> IEEE_FLT_MANTISSA_BITS) & ((1u << IEEE_FLT_EXPONENT_BITS) - 1u);
        const uint32_t f32Mantissa = f & ((1u << IEEE_FLT_MANTISSA_BITS) - 1u);
        if (f32Exponent == (1u << IEEE_FLT_EXPONENT_BITS) - 1u) {
            const uint32_t e = ((1u << EBits) - 1u) << MBits;
            if (f32Mantissa == 0) { // INF
                return (T)(s | e);
            }

            const uint32_t m = (f32Mantissa >> (IEEE_FLT_MANTISSA_BITS - MBits)) | (1u << (MBits - 1));
            return (T)(s | e | m); // quiet NaN
        }

        // Round to nearest, with ties to even.
        uint32_t roundedF = f;
        if constexpr (MBits < IEEE_FLT_MANTISSA_BITS) {
            constexpr int shift = IEEE_FLT_MANTISSA_BITS - MBits;
            const uint32_t roundingBias = ((1u << (shift - 1)) - 1u) + ((f32Mantissa >> shift) & 1u);
            roundedF += roundingBias;
        }

        const int32_t e = (int32_t)((roundedF >> IEEE_FLT_MANTISSA_BITS) & ((1u << IEEE_FLT_EXPONENT_BITS) - 1u)) - (IEEE_FLT_EXPONENT_BIAS - EBias);
        if (e <= 0) {
            if (denormalMode == DenormalMode::FlushToZero) {
                // Flush denormal values to signed zero.
                return (T)s;
            }

            const int shift = IEEE_FLT_MANTISSA_BITS + IEEE_FLT_EXPONENT_BIAS + 1 - (int)f32Exponent - EBias - MBits;
            if (shift >= 32) {
                return (T)s;
            }

            const uint32_t mantissa = (1u << IEEE_FLT_MANTISSA_BITS) | f32Mantissa;
            uint32_t m = mantissa >> shift;
            const uint32_t remainder = mantissa & ((1u << shift) - 1u);
            const uint32_t halfway = 1u << (shift - 1);
            if (remainder > halfway || (remainder == halfway && (m & 1u))) {
                ++m;
            }
            return (T)(s | m);
        }
        if (e >= (1u << EBits) - 1u) {
            // Map finite overflow to infinity.
            return (T)(s | (((1u << EBits) - 1u) << MBits));
        }
        const uint32_t m = (roundedF & ((1u << IEEE_FLT_MANTISSA_BITS) - 1u)) >> (IEEE_FLT_MANTISSA_BITS - MBits);
        return (T)(s | (e << MBits) | m);
    }
};

/// Unsigned float converter.
template <typename T, int MBits, int EBits, int EBias>
class FloatConverter<T, MBits, EBits, EBias, 0> {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
    static_assert(MBits > 0 && MBits <= IEEE_FLT_MANTISSA_BITS, "MBits must fit within a float mantissa");
    static_assert(EBits > 0 && EBits < 32, "EBits must fit within a uint32_t");
    static_assert(MBits + EBits <= 32, "Mantissa and exponent fields must fit within a uint32_t");
    static_assert(MBits + EBits <= std::numeric_limits<T>::digits, "Mantissa and exponent fields must fit within T");

public:
    static float ToF32(T x) {
        const int e = (x >> MBits) & ((1u << EBits) - 1u);
        const int m = (x & ((1u << MBits) - 1u));
        uint32_t i;

        if (e == 0) {
            if (m == 0) { // zero
                return 0;
            } else { // denormal
                return Math::Ldexp((float)m, 1 - EBias - MBits);
            }
        } else if (e == (1u << EBits) - 1u) {
            if (m == 0) { // INF
                i = (((1u << IEEE_FLT_EXPONENT_BITS) - 1u) << IEEE_FLT_MANTISSA_BITS);
            } else { // NaN
                i = (((1u << IEEE_FLT_EXPONENT_BITS) - 1u) << IEEE_FLT_MANTISSA_BITS) | ((uint32_t)m << (IEEE_FLT_MANTISSA_BITS - MBits));
            }
            return reinterpret_cast<float &>(i);
        } else { // normal number
            return Math::Ldexp(1.0f + m / (float)(1u << MBits), e - EBias);
        }
    }

    static T FromF32(float x, DenormalMode denormalMode = DenormalMode::FlushToZero) {
        const uint32_t f = *reinterpret_cast<uint32_t *>(&x);
        const uint32_t f32Sign = f >> IEEE_FLT_SIGN_BIT;
        const uint32_t f32Exponent = (f >> IEEE_FLT_MANTISSA_BITS) & ((1u << IEEE_FLT_EXPONENT_BITS) - 1u);
        const uint32_t f32Mantissa = f & ((1u << IEEE_FLT_MANTISSA_BITS) - 1u);
        if (f32Exponent == (1u << IEEE_FLT_EXPONENT_BITS) - 1u) {
            const uint32_t e = ((1u << EBits) - 1u) << MBits;
            if (f32Mantissa == 0) { // INF
                return f32Sign ? 0 : (T)e;
            }

            const uint32_t m = (f32Mantissa >> (IEEE_FLT_MANTISSA_BITS - MBits)) | (1u << (MBits - 1));
            return (T)(e | m); // quiet NaN
        }

        // Clamp negative finite values and negative zero to zero.
        if (f32Sign) {
            return 0;
        }

        // Round to nearest, with ties to even.
        uint32_t roundedF = f;
        if constexpr (MBits < IEEE_FLT_MANTISSA_BITS) {
            constexpr int shift = IEEE_FLT_MANTISSA_BITS - MBits;
            const uint32_t roundingBias = ((1u << (shift - 1)) - 1u) + ((f32Mantissa >> shift) & 1u);
            roundedF += roundingBias;
        }

        const int32_t e = (int32_t)((roundedF >> IEEE_FLT_MANTISSA_BITS) & ((1u << IEEE_FLT_EXPONENT_BITS) - 1u)) - (IEEE_FLT_EXPONENT_BIAS - EBias);
        if (e <= 0) {
            if (denormalMode == DenormalMode::FlushToZero) {
                // Flush denormal values to zero.
                return 0;
            }

            const int shift = IEEE_FLT_MANTISSA_BITS + IEEE_FLT_EXPONENT_BIAS + 1 - (int)f32Exponent - EBias - MBits;
            if (shift >= 32) {
                return 0;
            }

            const uint32_t mantissa = (1u << IEEE_FLT_MANTISSA_BITS) | f32Mantissa;
            uint32_t m = mantissa >> shift;
            const uint32_t remainder = mantissa & ((1u << shift) - 1u);
            const uint32_t halfway = 1u << (shift - 1);
            if (remainder > halfway || (remainder == halfway && (m & 1u))) {
                ++m;
            }
            return (T)m;
        }
        if (e >= (1u << EBits) - 1u) {
            // Map finite overflow to infinity.
            return (T)(((1u << EBits) - 1u) << MBits);
        }
        const uint32_t m = (roundedF & ((1u << IEEE_FLT_MANTISSA_BITS) - 1u)) >> (IEEE_FLT_MANTISSA_BITS - MBits);
        return (T)((e << MBits) | m);
    }
};

using float16_t = unsigned short;

using F16Converter = FloatConverter<float16_t, IEEE_FLT16_MANTISSA_BITS, IEEE_FLT16_EXPONENT_BITS, IEEE_FLT16_EXPONENT_BIAS, IEEE_FLT16_SIGN_BIT>;
using F11Converter = FloatConverter<uint32_t, 6, 5, 15, 0>;
using F10Converter = FloatConverter<uint32_t, 5, 5, 15, 0>;

BE_NAMESPACE_END
