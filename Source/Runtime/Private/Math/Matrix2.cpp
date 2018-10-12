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

const Mat2 Mat2::zero(Vec2(0, 0), Vec2(0, 0));
const Mat2 Mat2::identity(Vec2(1, 0), Vec2(0, 1));

Mat2 Mat2::FromString(const char *str) {
    Mat2 m;
    sscanf(str, "%f %f %f %f", &m[0].x, &m[0].y, &m[1].x, &m[1].y);
    return m;
}

bool Mat2::InverseSelf() {
#if 1
    // 2+4 = 6 multiplications
    //		 1 division
    double det, invDet, a;

    det = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

    if (Math::Fabs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    invDet = 1.0f / det;

    a = mat[0][0];
    mat[0][0] =   mat[1][1] * invDet;
    mat[0][1] = - mat[0][1] * invDet;
    mat[1][0] = - mat[1][0] * invDet;
    mat[1][1] =   a * invDet;

    return true;
#else
    // 2*4 = 8 multiplications
    //		 2 division
    float *mat = reinterpret_cast<float *>(this);
    double d, di;
    float s;

    di = mat[0];
    s = di;
    mat[0*2+0] = d = 1.0f / di;
    mat[0*2+1] *= d;
    d = -d;
    mat[1*2+0] *= d;
    d = mat[1*2+0] * di;
    mat[1*2+1] += mat[0*2+1] * d;
    di = mat[1*2+1];
    s *= di;
    mat[1*2+1] = d = 1.0f / di;
    mat[1*2+0] *= d;
    d = -d;
    mat[0*2+1] *= d;
    d = mat[0*2+1] * di;
    mat[0*2+0] += mat[1*2+0] * d;

    return (s != 0.0f && !FLOAT_IS_NAN(s));
#endif
}

BE_NAMESPACE_END
