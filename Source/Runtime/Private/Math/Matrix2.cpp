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

ALIGN_AS16 const Mat2 Mat2::zero(Vec2(0, 0), Vec2(0, 0));
ALIGN_AS16 const Mat2 Mat2::identity(Vec2(1, 0), Vec2(0, 1));

Mat2 Mat2::operator-() const & {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat2 dst;

    simd4f r0 = loadu_ps(mat[0]);

    store_ps(-r0, dst[0]);
#else
    Mat2 dst;

    dst[0][0] = -mat[0][0];
    dst[0][1] = -mat[0][1];

    dst[1][0] = -mat[1][0];
    dst[1][1] = -mat[1][1];
#endif
    return dst;
}

Mat2 &&Mat2::operator-() && {
#if defined(ENABLE_SIMD4_INTRIN)
    simd4f r0 = loadu_ps(mat[0]);

    storeu_ps(-r0, mat[0]);
#else
    mat[0][0] = -mat[0][0];
    mat[0][1] = -mat[0][1];

    mat[1][0] = -mat[1][0];
    mat[1][1] = -mat[1][1];
#endif
    return std::move(*this);
}

Mat2 &Mat2::operator*=(const Mat2 &rhs) {
#if defined(ENABLE_SIMD4_INTRIN)
    simd4f a = loadu_ps(mat[0]);
    simd4f b = loadu_ps(rhs.mat[0]);

    simd4f result = shuffle_ps<0, 0, 2, 2>(a) * shuffle_ps<0, 1, 0, 1>(b);
    result = madd_ps(shuffle_ps<1, 1, 3, 3>(a), shuffle_ps<2, 3, 2, 3>(b), result);

    storeu_ps(result, mat[0]);
#else
    float dst[2];

    dst[0] = mat[0][0] * rhs.mat[0][0] + mat[0][1] * rhs.mat[1][0];
    dst[1] = mat[0][0] * rhs.mat[0][1] + mat[0][1] * rhs.mat[1][1];
    mat[0][0] = dst[0];
    mat[0][1] = dst[1];

    dst[0] = mat[1][0] * rhs.mat[0][0] + mat[1][1] * rhs.mat[1][0];
    dst[1] = mat[1][0] * rhs.mat[0][1] + mat[1][1] * rhs.mat[1][1];
    mat[1][0] = dst[0];
    mat[1][1] = dst[1];
#endif
    return *this;
}

bool Mat2::InverseSelf() {
#if 1
    // 2+4 = 6 multiplications
    //       1 division
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
    //       2 division
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

Mat2 Mat2::FromString(const char *str) {
    Mat2 m;
    int count = sscanf(str, "%f %f %f %f", &m[0].x, &m[0].y, &m[1].x, &m[1].y);
    assert(count == m.GetDimension());
    return m;
}

BE_NAMESPACE_END
