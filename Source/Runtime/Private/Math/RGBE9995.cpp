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

#define RGB9E5_EXPONENT_BITS            5
#define RGB9E5_MANTISSA_BITS            9
#define RGB9E5_EXP_BIAS                 15
#define RGB9E5_MAX_VALID_BIASED_EXP     31

#define MAX_RGB9E5_EXP                  (RGB9E5_MAX_VALID_BIASED_EXP - RGB9E5_EXP_BIAS)
#define RGB9E5_MANTISSA_VALUES          (1 << RGB9E5_MANTISSA_BITS)
#define MAX_RGB9E5_MANTISSA             (RGB9E5_MANTISSA_VALUES - 1)
#define MAX_RGB9E5                      (((float)MAX_RGB9E5_MANTISSA) / RGB9E5_MANTISSA_VALUES * (1 << MAX_RGB9E5_EXP))
#define EPSILON_RGB9E5                  ((1.0 / RGB9E5_MANTISSA_VALUES) / (1 << RGB9E5_EXP_BIAS))

BE_NAMESPACE_BEGIN

static float ClampRange_for_rgb9e5(float x) {
    if (x > 0.0) {
        if (x >= MAX_RGB9E5) {
            return MAX_RGB9E5;
        } else {
            return x;
        }
    } else {
        /* NaN gets here too since comparisons with NaN always fail! */
        return 0.0;
    }
}

/* Ok, FloorLog2 is not correct for the denorm and zero values, but we
   are going to do a max of this value with the minimum rgb9e5 exponent
   that will hide these problem cases. */
static int FloorLog2(float x) {
    uint32_t i = reinterpret_cast<uint32_t &>(x);
    int exponent = (i >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1) - IEEE_FLT_EXPONENT_BIAS;
    return exponent;
}

RGBE9995 RGBE9995::FromColor3(float r, float g, float b) {
    float clampedR = ClampRange_for_rgb9e5(r);
    float clampedG = ClampRange_for_rgb9e5(g);
    float clampedB = ClampRange_for_rgb9e5(b);
    float maxRgb = Max3(clampedR, clampedG, clampedB);

    int sharedExp = Max(-RGB9E5_EXP_BIAS - 1, FloorLog2(maxRgb)) + 1 + RGB9E5_EXP_BIAS;
    assert(sharedExp <= RGB9E5_MAX_VALID_BIASED_EXP);
    assert(sharedExp >= 0);

    /* This pow function could be replaced by a table. */
    double denom = pow(2, sharedExp - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS);

    int maxm = Math::Rint(maxRgb / denom);
    if (maxm == MAX_RGB9E5_MANTISSA + 1) {
        denom *= 2;
        sharedExp += 1;
        assert(sharedExp <= RGB9E5_MAX_VALID_BIASED_EXP);
    } else {
        assert(maxm <= MAX_RGB9E5_MANTISSA);
    }

    int rm = Math::Rint(clampedR / denom);
    int gm = Math::Rint(clampedG / denom);
    int bm = Math::Rint(clampedB / denom);

    assert(rm <= MAX_RGB9E5_MANTISSA);
    assert(gm <= MAX_RGB9E5_MANTISSA);
    assert(bm <= MAX_RGB9E5_MANTISSA);
    assert(rm >= 0);
    assert(gm >= 0);
    assert(bm >= 0);

    RGBE9995 rgbe9995((uint32_t)(rm | (gm << 9) | (bm << 18) | (sharedExp << 27)));
    return rgbe9995;
}

Color3 RGBE9995::ToColor3() const {
    int biasedExponent = (value >> (RGB9E5_MANTISSA_BITS * 3)) & ((1 << RGB9E5_EXPONENT_BITS) - 1);
    int exponent = biasedExponent - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS;
    float scale = Math::Pow(2, exponent);
    float r = value & ((1 << RGB9E5_MANTISSA_BITS) - 1);
    float g = (value >> RGB9E5_MANTISSA_BITS) & ((1 << RGB9E5_MANTISSA_BITS) - 1);
    float b = (value >> (RGB9E5_MANTISSA_BITS * 2)) & ((1 << RGB9E5_MANTISSA_BITS) - 1);
    return Color3(r * scale, g * scale, b * scale);
}

BE_NAMESPACE_END
