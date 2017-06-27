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

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

static const int SmallestNonDenormal            = 1 << IEEE_FLT_MANTISSA_BITS;
static const int NanValue                       = 0x7f800000;

const float	Math::Pi                            = 3.14159265358979323846f;
const float	Math::TwoPi                         = 2.0f * Pi;
const float	Math::HalfPi                        = 0.5f * Pi;
const float	Math::OneFourthPi                   = 0.25f * Pi;
const float Math::InvPi                         = 1.0f / Pi;
const float Math::InvTwoPi                      = 1.0f / TwoPi;
const float Math::E                             = 2.71828182845904523536f;
const float Math::GoldenRatio                   = 1.61803398875f;
const float Math::SqrtTwo                       = 1.41421356237309504880f;
const float Math::SqrtThree                     = 1.73205080756887729352f;
const float	Math::SqrtOneOverTwo                = 0.70710678118654752440f;
const float	Math::SqrtOneOverThree              = 0.57735026918962576450f;
const float	Math::MulDegreeToRadian             = Pi / 180.0f;
const float	Math::MulRadianToDegree             = 180.0f / Pi;
const float	Math::MulSecondToMs                 = 1000.0f;
const float	Math::MulMsToSecond                 = 0.001f;
const float	Math::Infinity                      = 1e30f;
const float Math::FloatEpsilon                  = 1.192092896e-07f;
const float Math::FloatSmallestNonDenormal      = * reinterpret_cast<const float *>(&SmallestNonDenormal);    // 1.1754944e-038f

#if defined(BE_WIN_X86_SSE_INTRIN)
const __m128 Math::SIMD_SP_zero                 = { 0.0f, 0.0f, 0.0f, 0.0f };
const __m128 Math::SIMD_SP_255                  = { 255.0f, 255.0f, 255.0f, 255.0f };
const __m128 Math::SIMD_SP_min_char             = { -128.0f, -128.0f, -128.0f, -128.0f };
const __m128 Math::SIMD_SP_max_char             = { 127.0f, 127.0f, 127.0f, 127.0f };
const __m128 Math::SIMD_SP_min_short            = { -32768.0f, -32768.0f, -32768.0f, -32768.0f };
const __m128 Math::SIMD_SP_max_short            = { 32767.0f, 32767.0f, 32767.0f, 32767.0f };
const __m128 Math::SIMD_SP_smallestNonDenorm    = { FloatSmallestNonDenormal, FloatSmallestNonDenormal, FloatSmallestNonDenormal, FloatSmallestNonDenormal };
const __m128 Math::SIMD_SP_tiny                 = { 1e-4f, 1e-4f, 1e-4f, 1e-4f };
const __m128 Math::SIMD_SP_rsqrt_c0             = { 3.0f, 3.0f, 3.0f, 3.0f };
const __m128 Math::SIMD_SP_rsqrt_c1             = { -0.5f, -0.5f, -0.5f, -0.5f };
#endif

bool        Math::initialized                   = false;
uint32_t    Math::iSqrt[SQRT_TABLE_SIZE];       // inverse square root lookup table

void Math::Init() {
    union _flint fi, fo;

    for (int i = 0; i < SQRT_TABLE_SIZE; i++) {
        fi.i        = ((EXP_BIAS-1) << EXP_POS) | (i << LOOKUP_POS);
        fo.f        = (float)(1.0 / sqrt(fi.f));
        iSqrt[i]    = ((uint32_t)(((fo.i + (1<<(SEED_POS-2))) >> SEED_POS) & 0xFF)) << SEED_POS;
    }
    
    iSqrt[SQRT_TABLE_SIZE / 2] = ((uint32_t)(0xFF)) << (SEED_POS);

    initialized = true;
}

int Math::FloatToBits(float f, int exponentBits, int mantissaBits) {
    assert(exponentBits >= 2 && exponentBits <= 8);
    assert(mantissaBits >= 2 && mantissaBits <= 23);

    int32_t maxBits = (((1 << (exponentBits - 1)) - 1) << mantissaBits) | ((1 << mantissaBits) - 1);
    int32_t minBits = (((1 <<  exponentBits     ) - 2) << mantissaBits) | 1;

    float max = BitsToFloat(maxBits, exponentBits, mantissaBits);
    float min = BitsToFloat(minBits, exponentBits, mantissaBits);

    if (f >= 0.0f) {
        if (f >= max) {
            return maxBits;
        } else if (f <= min) {
            return minBits;
        }
    } else {
        if (f <= -max) {
            return (maxBits | (1 << (exponentBits + mantissaBits)));
        } else if (f >= -min) {
            return (minBits | (1 << (exponentBits + mantissaBits)));
        }
    }

    exponentBits--;
    int32_t i = *reinterpret_cast<int32_t *>(&f);
    int32_t sign = (i >> IEEE_FLT_SIGN_BIT) & 1;
    int32_t exponent = ((i >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - IEEE_FLT_EXPONENT_BIAS;
    int32_t mantissa = i & ((1 << IEEE_FLT_MANTISSA_BITS) - 1);

    int32_t value = sign << (1 + exponentBits + mantissaBits);
    value |= ((INT32_SIGNBITSET(exponent) << exponentBits) | (abs(exponent) & ((1 << exponentBits) - 1))) << mantissaBits;
    value |= mantissa >> (IEEE_FLT_MANTISSA_BITS - mantissaBits);
    return value;
}

float Math::BitsToFloat(int i, int exponentBits, int mantissaBits) {
    static int32_t exponentSign[2] = { 1, -1 };

    assert(exponentBits >= 2 && exponentBits <= 8);
    assert(mantissaBits >= 2 && mantissaBits <= 23);

    exponentBits--;
    int32_t sign = i >> (1 + exponentBits + mantissaBits);
    int32_t exponent = ((i >> mantissaBits) & ((1 << exponentBits) - 1)) * exponentSign[(i >> (exponentBits + mantissaBits)) & 1];
    int32_t mantissa = (i & ((1 << mantissaBits) - 1)) << (IEEE_FLT_MANTISSA_BITS - mantissaBits);

    int32_t value = sign << IEEE_FLT_SIGN_BIT | (exponent + IEEE_FLT_EXPONENT_BIAS) << IEEE_FLT_MANTISSA_BITS | mantissa;
    return *reinterpret_cast<float *>(&value);
}

BE_NAMESPACE_END
