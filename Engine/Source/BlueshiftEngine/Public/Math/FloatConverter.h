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

BE_NAMESPACE_BEGIN

/// Signed float converter.
template <typename T, int MBits, int EBits, int EBias, int SBit>
class FloatConverter {
public:
    static float ToF32(T x) {
        const int e = (x >> MBits) & ((1 << EBits) - 1);
        const int m = (x & ((1 << MBits) - 1));
        const int s = (x >> SBit) & 1;
        uint32_t i;

        if (e == 0) {
            if (m == 0) { // zero
                i = s << IEEE_FLT_SIGN_BIT;
                return reinterpret_cast<float &>(i);
            } else { // denormal
                return (s ? -1.0f : 1.0f) * (m / (float)(1 << MBits)) * powf(2.0f, -(EBias - 1));
            }
        } else if (e == (1 << EBits) - 1) {
            if (m == 0) { // INF
                i = (s << IEEE_FLT_SIGN_BIT) | (((1 << EBits) - 1) << MBits);
            } else { // NaN
                i = (s << IEEE_FLT_SIGN_BIT) | (((1 << EBits) - 1) << MBits) | ((1 << MBits) - 1);
            }
            return reinterpret_cast<float &>(i);
        } else { // normal number
            return (s ? -1.0f : 1.0f) * (1.0f + m / (float)(1 << MBits)) * powf(2.0f, (float)(e - EBias));
        }
    }

    static T FromF32(float x) {
        const uint32_t f = reinterpret_cast<uint32_t &>(x);
        const int32_t e = ((f >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - (IEEE_FLT_EXPONENT_BIAS - EBias);
        const uint32_t s = (f >> IEEE_FLT_SIGN_BIT) << SBit;
        const uint32_t m = (f & ((1 << IEEE_FLT_MANTISSA_BITS) - 1)) >> (IEEE_FLT_MANTISSA_BITS - MBits);

        if (e <= 0) {
            // denormals are zero
            return 0;
        }
        if (e >= (1 << EBits) - 1) {
            // map +INF to largest number, -INF to smallest number
            return (T)(s | ((1 << EBits) - 2) | ((1 << MBits) - 1));
        }
        return (T)(s | (e << MBits) | m);
    }
};

/// Unsigned float converter.
template <typename T, int MBits, int EBits, int EBias>
class FloatConverter<T, MBits, EBits, EBias, 0> {
public:
    static float ToF32(T x) {
        const int e = (x >> MBits) & ((1 << EBits) - 1);
        const int m = (x & ((1 << MBits) - 1));
        uint32_t i;

        if (e == 0) {
            if (m == 0) { // zero
                return 0;
            } else { // denormal
                return (m / (float)(1 << MBits)) * powf(2.0f, -(EBias - 1));
            }
        } else if (e == (1 << EBits) - 1) {
            if (m == 0) { // INF
                i = (((1 << EBits) - 1) << MBits);
            } else { // NaN
                i = (((1 << EBits) - 1) << MBits) | ((1 << MBits) - 1);
            }
            return reinterpret_cast<float &>(i);
        } else { // normal number
            return (1.0f + m / (float)(1 << MBits)) * powf(2.0f, (float)(e - EBias));
        }
    }

    static T FromF32(float x) {
        const uint32_t f = *reinterpret_cast<uint32_t *>(&x);
        const int32_t e = ((f >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - (IEEE_FLT_EXPONENT_BIAS - EBias);
        const uint32_t m = (f & ((1 << IEEE_FLT_MANTISSA_BITS) - 1)) >> (IEEE_FLT_MANTISSA_BITS - MBits);

        if (e <= 0) {
            // denormals are zero
            return 0;
        }
        if (e >= (1 << EBits) - 1) {
            // map +INF to largest number, -INF to smallest number
            return (T)(((1 << EBits) - 2) | ((1 << MBits) - 1));
        }
        return (T)((e << MBits) | m);
    }
};

typedef unsigned short float16_t;

using F16Converter = FloatConverter<float16_t, IEEE_FLT16_MANTISSA_BITS, IEEE_FLT16_EXPONENT_BITS, IEEE_FLT16_EXPONENT_BIAS, IEEE_FLT16_SIGN_BIT>;
using F11Converter = FloatConverter<uint32_t, 6, 5, 15, 0>;
using F10Converter = FloatConverter<uint32_t, 5, 5, 15, 0>;

BE_NAMESPACE_END
