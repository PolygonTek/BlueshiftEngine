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

static const float shConsts[] = { 0.282095f, 0.488603f, 1.092548f, 0.315392f, 0.546274f, 0.590044f, 2.890611f, 0.457046f, 0.373176f };

// number of terms = n^2
// l = [0, n - 1]
// m = [-(n - 1), +(n - 1)]
void SphericalHarmonics::EvalBasis(int n, const Vec3 &dir, float *basisEval) {
    assert(n >= 1 && n <= 4);

    basisEval[0] = shConsts[0]; // l = 0, m = 0
    if (n == 1) {
        return;
    }

    float x = -dir.x;
    float y = -dir.y;
    float z = dir.z;

    basisEval[1] = shConsts[1] * y; // l = 1, m = -1
    basisEval[2] = shConsts[1] * z; // l = 1, m = 0
    basisEval[3] = shConsts[1] * x; // l = 1, m = 1
    if (n == 2) {
        return;
    }

    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float yz = y * z;
    float xz = x * z;

    basisEval[4] = shConsts[2] * xy; // l = 2, m = -2
    basisEval[5] = shConsts[2] * yz; // l = 2, m = -1
    basisEval[6] = shConsts[3] * (3.0f * zz - 1.0f); // l = 2, m = 0
    basisEval[7] = shConsts[2] * xz; // l = 2, m = 1
    basisEval[8] = shConsts[4] * (xx - yy); // l = 2, m = 2
    if (n == 3) {
        return;
    }

    float xxx = xx * x;
    float yyy = yy * y;
    float zzz = zz * z;
    float xxy = xx * y;
    float xxz = xx * z;
    float yyz = yy * z;
    float yyx = yy * x;
    float zzx = zz * x;
    float zzy = zz * y;
    float xyz = xy * z;

    basisEval[9] = shConsts[5] * (3.0f * xxy - yyy); // l = 3, m = -3
    basisEval[10] = shConsts[6] * xyz; // l = 3, m = -2
    basisEval[11] = shConsts[7] * (5.0f * zzy - y); // l = 3, m = -1
    basisEval[12] = shConsts[8] * (5.0f * zzz - 3.0f * z); // l = 3, m = 0
    basisEval[13] = shConsts[7] * (5.0f * zzx - x); // l = 3, m = 1
    basisEval[14] = shConsts[6] * (xxz - yyz); // l = 3, m = 2
    basisEval[15] = shConsts[5] * (xxx - 3.0f * yyx); // l = 3, m = 3
    if (n == 4) {
        return;
    }

    assert(0);
}

// To produce result of this function
// convert spherical function cos(theta) / PI to ZH coefficients
// and then convert it to SH coefficients
double SphericalHarmonics::Lambert_Al_Evaluator(int l) {
    if (l < 0) { // bogus case
        return 0.0;
    }

    if ((l & 1) == 1) { // odd case
        if (l == 1) {
            return 2.0 / 3.0;
        } else {
            return 0.0;
        }
    } else { // even case
        double l_fac = Math::Factorial((unsigned int)l);
        double l_over2_fac = Math::Factorial((unsigned int)(l >> 1));
        double denominator = (l + 2) * (l - 1);
        double sign = ((l >> 1) & 1) ? 1.0f : -1.0f;  // -1^(l/2 - 1) = -1 when l is a multiple of 4, 1 for other multiples of 2
        double exp2_l = (1 << (unsigned int)l);
        return (sign * 2.0 * l_fac) / (denominator * exp2_l * l_over2_fac);
    }
}

BE_NAMESPACE_END
