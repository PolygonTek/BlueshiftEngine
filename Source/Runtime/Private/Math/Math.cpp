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

static const uint32_t SmallestNonDenormal       = 1 << IEEE_FLT_MANTISSA_BITS;
static const uint32_t NanValue                  = 0x7f800000;

const float Math::FloatSmallestNonDenormal      = *reinterpret_cast<const float *>(&SmallestNonDenormal);    // 1.1754944e-038f

const __m128 Math::SIMD_SP_smallestNonDenorm    = { FloatSmallestNonDenormal, FloatSmallestNonDenormal, FloatSmallestNonDenormal, FloatSmallestNonDenormal };

bool        Math::initialized = false;
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
