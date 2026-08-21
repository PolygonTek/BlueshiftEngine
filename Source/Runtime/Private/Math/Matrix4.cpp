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

#include "Precompiled.h"
#include "Math/Math.h"
#include "Cholesky.h"
#include "LU.h"
#include "QRCP.h"
#include "JacobiRotation.h"
#include "JacobiSVD.h"
#include "SymmetricPSDSolver.h"

BE_NAMESPACE_BEGIN

ALIGN_AS32 const Mat4 Mat4::zero(Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0));
ALIGN_AS32 const Mat4 Mat4::identity(Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0), Vec4(0, 0, 0, 1));

Mat4 &Mat4::operator=(const Mat3x4 &rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f r01 = loadu_256ps(rhs[0]);
    simd4f r2 = loadu_ps(rhs[2]);
    simd8f r23 = set2x128_256ps(r2, SIMD_4::F4_3zero_one);

    storeu_256ps(r01, mat[0]);
    storeu_256ps(r23, mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f r0 = loadu_ps(rhs[0]);
    simd4f r1 = loadu_ps(rhs[1]);
    simd4f r2 = loadu_ps(rhs[2]);

    storeu_ps(r0, mat[0]);
    storeu_ps(r1, mat[1]);
    storeu_ps(r2, mat[2]);
    storeu_ps(SIMD_4::F4_3zero_one, mat[3]);
#else
    mat[0][0] = rhs[0][0];
    mat[0][1] = rhs[0][1];
    mat[0][2] = rhs[0][2];
    mat[0][3] = rhs[0][3];

    mat[1][0] = rhs[1][0];
    mat[1][1] = rhs[1][1];
    mat[1][2] = rhs[1][2];
    mat[1][3] = rhs[1][3];

    mat[2][0] = rhs[2][0];
    mat[2][1] = rhs[2][1];
    mat[2][2] = rhs[2][2];
    mat[2][3] = rhs[2][3];

    mat[3][0] = 0;
    mat[3][1] = 0;
    mat[3][2] = 0;
    mat[3][3] = 1;
#endif
    return *this;
}

Mat4 Mat4::operator-() const & {
#if defined(ENABLE_SIMD8_INTRIN)
    ALIGN_AS32 Mat4 dst;

    simd8f r01 = loadu_256ps(mat[0]);
    simd8f r23 = loadu_256ps(mat[2]);

    store_256ps(-r01, dst[0]);
    store_256ps(-r23, dst[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 dst;

    simd4f r0 = loadu_ps(mat[0]);
    simd4f r1 = loadu_ps(mat[1]);
    simd4f r2 = loadu_ps(mat[2]);
    simd4f r3 = loadu_ps(mat[3]);

    store_ps(-r0, dst[0]);
    store_ps(-r1, dst[1]);
    store_ps(-r2, dst[2]);
    store_ps(-r3, dst[3]);
#else
    Mat4 dst;

    dst[0][0] = -mat[0][0];
    dst[0][1] = -mat[0][1];
    dst[0][2] = -mat[0][2];
    dst[0][3] = -mat[0][3];

    dst[1][0] = -mat[1][0];
    dst[1][1] = -mat[1][1];
    dst[1][2] = -mat[1][2];
    dst[1][3] = -mat[1][3];

    dst[2][0] = -mat[2][0];
    dst[2][1] = -mat[2][1];
    dst[2][2] = -mat[2][2];
    dst[2][3] = -mat[2][3];

    dst[3][0] = -mat[3][0];
    dst[3][1] = -mat[3][1];
    dst[3][2] = -mat[3][2];
    dst[3][3] = -mat[3][3];
#endif
    return dst;
}

Mat4 &&Mat4::operator-() && {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f r01 = loadu_256ps(mat[0]);
    simd8f r23 = loadu_256ps(mat[2]);

    storeu_256ps(-r01, mat[0]);
    storeu_256ps(-r23, mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f r0 = loadu_ps(mat[0]);
    simd4f r1 = loadu_ps(mat[1]);
    simd4f r2 = loadu_ps(mat[2]);
    simd4f r3 = loadu_ps(mat[3]);

    storeu_ps(-r0, mat[0]);
    storeu_ps(-r1, mat[1]);
    storeu_ps(-r2, mat[2]);
    storeu_ps(-r3, mat[3]);
#else
    mat[0][0] = -mat[0][0];
    mat[0][1] = -mat[0][1];
    mat[0][2] = -mat[0][2];
    mat[0][3] = -mat[0][3];

    mat[1][0] = -mat[1][0];
    mat[1][1] = -mat[1][1];
    mat[1][2] = -mat[1][2];
    mat[1][3] = -mat[1][3];

    mat[2][0] = -mat[2][0];
    mat[2][1] = -mat[2][1];
    mat[2][2] = -mat[2][2];
    mat[2][3] = -mat[2][3];

    mat[3][0] = -mat[3][0];
    mat[3][1] = -mat[3][1];
    mat[3][2] = -mat[3][2];
    mat[3][3] = -mat[3][3];
#endif
    return std::move(*this);
}

Mat4 Mat4::operator+(const Mat4 &a) const & {
#if defined(ENABLE_SIMD8_INTRIN)
    ALIGN_AS32 Mat4 dst;

    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd8f br01 = loadu_256ps(a[0]);
    simd8f br23 = loadu_256ps(a[2]);

    store_256ps(ar01 + br01, dst[0]);
    store_256ps(ar23 + br23, dst[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f br0 = loadu_ps(a[0]);
    simd4f br1 = loadu_ps(a[1]);
    simd4f br2 = loadu_ps(a[2]);
    simd4f br3 = loadu_ps(a[3]);

    store_ps(ar0 + br0, dst[0]);
    store_ps(ar1 + br1, dst[1]);
    store_ps(ar2 + br2, dst[2]);
    store_ps(ar3 + br3, dst[3]);
#else
    Mat4 dst;

    dst[0][0] = mat[0][0] + a[0][0];
    dst[0][1] = mat[0][1] + a[0][1];
    dst[0][2] = mat[0][2] + a[0][2];
    dst[0][3] = mat[0][3] + a[0][3];

    dst[1][0] = mat[1][0] + a[1][0];
    dst[1][1] = mat[1][1] + a[1][1];
    dst[1][2] = mat[1][2] + a[1][2];
    dst[1][3] = mat[1][3] + a[1][3];

    dst[2][0] = mat[2][0] + a[2][0];
    dst[2][1] = mat[2][1] + a[2][1];
    dst[2][2] = mat[2][2] + a[2][2];
    dst[2][3] = mat[2][3] + a[2][3];

    dst[3][0] = mat[3][0] + a[3][0];
    dst[3][1] = mat[3][1] + a[3][1];
    dst[3][2] = mat[3][2] + a[3][2];
    dst[3][3] = mat[3][3] + a[3][3];
#endif
    return dst;
}

Mat4 &Mat4::operator+=(const Mat4 &rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd8f br01 = loadu_256ps(rhs[0]);
    simd8f br23 = loadu_256ps(rhs[2]);

    storeu_256ps(ar01 + br01, mat[0]);
    storeu_256ps(ar23 + br23, mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f br0 = loadu_ps(rhs[0]);
    simd4f br1 = loadu_ps(rhs[1]);
    simd4f br2 = loadu_ps(rhs[2]);
    simd4f br3 = loadu_ps(rhs[3]);

    storeu_ps(ar0 + br0, mat[0]);
    storeu_ps(ar1 + br1, mat[1]);
    storeu_ps(ar2 + br2, mat[2]);
    storeu_ps(ar3 + br3, mat[3]);
#else
    mat[0][0] += rhs[0][0];
    mat[0][1] += rhs[0][1];
    mat[0][2] += rhs[0][2];
    mat[0][3] += rhs[0][3];

    mat[1][0] += rhs[1][0];
    mat[1][1] += rhs[1][1];
    mat[1][2] += rhs[1][2];
    mat[1][3] += rhs[1][3];

    mat[2][0] += rhs[2][0];
    mat[2][1] += rhs[2][1];
    mat[2][2] += rhs[2][2];
    mat[2][3] += rhs[2][3];

    mat[3][0] += rhs[3][0];
    mat[3][1] += rhs[3][1];
    mat[3][2] += rhs[3][2];
    mat[3][3] += rhs[3][3];
#endif
    return *this;
}

Mat4 Mat4::operator-(const Mat4 &a) const & {
#if defined(ENABLE_SIMD8_INTRIN)
    ALIGN_AS32 Mat4 dst;

    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd8f br01 = loadu_256ps(a[0]);
    simd8f br23 = loadu_256ps(a[2]);

    store_256ps(ar01 - br01, dst[0]);
    store_256ps(ar23 - br23, dst[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f br0 = loadu_ps(a[0]);
    simd4f br1 = loadu_ps(a[1]);
    simd4f br2 = loadu_ps(a[2]);
    simd4f br3 = loadu_ps(a[3]);

    store_ps(ar0 - br0, dst[0]);
    store_ps(ar1 - br1, dst[1]);
    store_ps(ar2 - br2, dst[2]);
    store_ps(ar3 - br3, dst[3]);
#else
    Mat4 dst;

    dst[0][0] = mat[0][0] - a[0][0];
    dst[0][1] = mat[0][1] - a[0][1];
    dst[0][2] = mat[0][2] - a[0][2];
    dst[0][3] = mat[0][3] - a[0][3];

    dst[1][0] = mat[1][0] - a[1][0];
    dst[1][1] = mat[1][1] - a[1][1];
    dst[1][2] = mat[1][2] - a[1][2];
    dst[1][3] = mat[1][3] - a[1][3];

    dst[2][0] = mat[2][0] - a[2][0];
    dst[2][1] = mat[2][1] - a[2][1];
    dst[2][2] = mat[2][2] - a[2][2];
    dst[2][3] = mat[2][3] - a[2][3];

    dst[3][0] = mat[3][0] - a[3][0];
    dst[3][1] = mat[3][1] - a[3][1];
    dst[3][2] = mat[3][2] - a[3][2];
    dst[3][3] = mat[3][3] - a[3][3];
#endif
    return dst;
}

Mat4 &Mat4::operator-=(const Mat4 &rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd8f br01 = loadu_256ps(rhs[0]);
    simd8f br23 = loadu_256ps(rhs[2]);

    storeu_256ps(ar01 - br01, mat[0]);
    storeu_256ps(ar23 - br23, mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f br0 = loadu_ps(rhs[0]);
    simd4f br1 = loadu_ps(rhs[1]);
    simd4f br2 = loadu_ps(rhs[2]);
    simd4f br3 = loadu_ps(rhs[3]);

    storeu_ps(ar0 - br0, mat[0]);
    storeu_ps(ar1 - br1, mat[1]);
    storeu_ps(ar2 - br2, mat[2]);
    storeu_ps(ar3 - br3, mat[3]);
#else
    mat[0][0] -= rhs[0][0];
    mat[0][1] -= rhs[0][1];
    mat[0][2] -= rhs[0][2];
    mat[0][3] -= rhs[0][3];

    mat[1][0] -= rhs[1][0];
    mat[1][1] -= rhs[1][1];
    mat[1][2] -= rhs[1][2];
    mat[1][3] -= rhs[1][3];

    mat[2][0] -= rhs[2][0];
    mat[2][1] -= rhs[2][1];
    mat[2][2] -= rhs[2][2];
    mat[2][3] -= rhs[2][3];

    mat[3][0] -= rhs[3][0];
    mat[3][1] -= rhs[3][1];
    mat[3][2] -= rhs[3][2];
    mat[3][3] -= rhs[3][3];
#endif
    return *this;
}

Mat4 Mat4::operator*(float rhs) const & {
#if defined(ENABLE_SIMD8_INTRIN)
    ALIGN_AS32 Mat4 dst;

    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd8f b = set1_256ps(rhs);

    store_256ps(ar01 * b, dst[0]);
    store_256ps(ar23 * b, dst[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f b = set1_ps(rhs);

    store_ps(ar0 * b, dst[0]);
    store_ps(ar1 * b, dst[1]);
    store_ps(ar2 * b, dst[2]);
    store_ps(ar3 * b, dst[3]);
#else
    Mat4 dst;

    dst[0][0] = mat[0][0] * rhs;
    dst[0][1] = mat[0][1] * rhs;
    dst[0][2] = mat[0][2] * rhs;
    dst[0][3] = mat[0][3] * rhs;

    dst[1][0] = mat[1][0] * rhs;
    dst[1][1] = mat[1][1] * rhs;
    dst[1][2] = mat[1][2] * rhs;
    dst[1][3] = mat[1][3] * rhs;

    dst[2][0] = mat[2][0] * rhs;
    dst[2][1] = mat[2][1] * rhs;
    dst[2][2] = mat[2][2] * rhs;
    dst[2][3] = mat[2][3] * rhs;

    dst[3][0] = mat[3][0] * rhs;
    dst[3][1] = mat[3][1] * rhs;
    dst[3][2] = mat[3][2] * rhs;
    dst[3][3] = mat[3][3] * rhs;
#endif
    return dst;
}

Mat4 &Mat4::operator*=(float rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd8f b = set1_256ps(rhs);

    storeu_256ps(ar01 * b, mat[0]);
    storeu_256ps(ar23 * b, mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f b = set1_ps(rhs);

    storeu_ps(ar0 * b, mat[0]);
    storeu_ps(ar1 * b, mat[1]);
    storeu_ps(ar2 * b, mat[2]);
    storeu_ps(ar3 * b, mat[3]);
#else
    mat[0][0] *= rhs;
    mat[0][1] *= rhs;
    mat[0][2] *= rhs;
    mat[0][3] *= rhs;

    mat[1][0] *= rhs;
    mat[1][1] *= rhs;
    mat[1][2] *= rhs;
    mat[1][3] *= rhs;

    mat[2][0] *= rhs;
    mat[2][1] *= rhs;
    mat[2][2] *= rhs;
    mat[2][3] *= rhs;

    mat[3][0] *= rhs;
    mat[3][1] *= rhs;
    mat[3][2] *= rhs;
    mat[3][3] *= rhs;
#endif
    return *this;
}

Vec4 Mat4::operator*(const Vec4 &vec) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Vec4 dst;

    simd4f v = loadu_ps(vec);

    simd4f result = mat4x4rowmajoru_mul_vec4((const float *)mat, v);

    store_ps(result, dst);
#else
    Vec4 dst;

    dst[0] = mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z + mat[0].w * vec.w;
    dst[1] = mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z + mat[1].w * vec.w;
    dst[2] = mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z + mat[2].w * vec.w;
    dst[3] = mat[3].x * vec.x + mat[3].y * vec.y + mat[3].z * vec.z + mat[3].w * vec.w;
#endif
    return dst;
}

Mat4 Mat4::operator*(const Mat4 &rhs) const & {
#if defined(ENABLE_SIMD8_INTRIN)
    ALIGN_AS32 Mat4 dst;

    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd4f br0 = loadu_ps(rhs.mat[0]);
    simd4f br1 = loadu_ps(rhs.mat[1]);
    simd4f br2 = loadu_ps(rhs.mat[2]);
    simd4f br3 = loadu_ps(rhs.mat[3]);

    simd8f br00 = set2x128_256ps(br0.m128, br0.m128);
    simd8f br11 = set2x128_256ps(br1.m128, br1.m128);
    simd8f br22 = set2x128_256ps(br2.m128, br2.m128);
    simd8f br33 = set2x128_256ps(br3.m128, br3.m128);

    store_256ps(lincomb2x4x4(ar01, br00, br11, br22, br33), dst[0]);
    store_256ps(lincomb2x4x4(ar23, br00, br11, br22, br33), dst[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f br0 = loadu_ps(rhs.mat[0]);
    simd4f br1 = loadu_ps(rhs.mat[1]);
    simd4f br2 = loadu_ps(rhs.mat[2]);
    simd4f br3 = loadu_ps(rhs.mat[3]);

    store_ps(lincomb4x4(ar0, br0, br1, br2, br3), dst.mat[0]);
    store_ps(lincomb4x4(ar1, br0, br1, br2, br3), dst.mat[1]);
    store_ps(lincomb4x4(ar2, br0, br1, br2, br3), dst.mat[2]);
    store_ps(lincomb4x4(ar3, br0, br1, br2, br3), dst.mat[3]);
#else
    Mat4 dst;

    float *dstPtr = dst.Ptr();
    const float *m1Ptr = Ptr();
    const float *m2Ptr = rhs.Ptr();

    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            *dstPtr =
                m1Ptr[0] * m2Ptr[0 * Cols + c] +
                m1Ptr[1] * m2Ptr[1 * Cols + c] +
                m1Ptr[2] * m2Ptr[2 * Cols + c] +
                m1Ptr[3] * m2Ptr[3 * Cols + c];
            dstPtr++;
        }
        m1Ptr += Cols;
    }
#endif
    return dst;
}

Mat4 Mat4::operator*(const Mat3x4 &rhs) const & {
#if defined(ENABLE_SIMD8_INTRIN)
    ALIGN_AS32 Mat4 dst;

    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd4f br0 = loadu_ps(rhs.mat[0]);
    simd4f br1 = loadu_ps(rhs.mat[1]);
    simd4f br2 = loadu_ps(rhs.mat[2]);

    simd8f br00 = set2x128_256ps(br0.m128, br0.m128);
    simd8f br11 = set2x128_256ps(br1.m128, br1.m128);
    simd8f br22 = set2x128_256ps(br2.m128, br2.m128);

    store_256ps(lincomb2x3x4(ar01, br00, br11, br22), dst.mat[0]);
    store_256ps(lincomb2x3x4(ar23, br00, br11, br22), dst.mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f br0 = loadu_ps(rhs.mat[0]);
    simd4f br1 = loadu_ps(rhs.mat[1]);
    simd4f br2 = loadu_ps(rhs.mat[2]);

    store_ps(lincomb3x4(ar0, br0, br1, br2), dst.mat[0]);
    store_ps(lincomb3x4(ar1, br0, br1, br2), dst.mat[1]);
    store_ps(lincomb3x4(ar2, br0, br1, br2), dst.mat[2]);
    store_ps(lincomb3x4(ar3, br0, br1, br2), dst.mat[3]);
#else
    Mat4 dst;

    dst[0][0] = mat[0][0] * rhs[0][0] + mat[0][1] * rhs[1][0] + mat[0][2] * rhs[2][0];
    dst[0][1] = mat[0][0] * rhs[0][1] + mat[0][1] * rhs[1][1] + mat[0][2] * rhs[2][1];
    dst[0][2] = mat[0][0] * rhs[0][2] + mat[0][1] * rhs[1][2] + mat[0][2] * rhs[2][2];
    dst[0][3] = mat[0][0] * rhs[0][3] + mat[0][1] * rhs[1][3] + mat[0][2] * rhs[2][3] + mat[0][3];

    dst[1][0] = mat[1][0] * rhs[0][0] + mat[1][1] * rhs[1][0] + mat[1][2] * rhs[2][0];
    dst[1][1] = mat[1][0] * rhs[0][1] + mat[1][1] * rhs[1][1] + mat[1][2] * rhs[2][1];
    dst[1][2] = mat[1][0] * rhs[0][2] + mat[1][1] * rhs[1][2] + mat[1][2] * rhs[2][2];
    dst[1][3] = mat[1][0] * rhs[0][3] + mat[1][1] * rhs[1][3] + mat[1][2] * rhs[2][3] + mat[1][3];

    dst[2][0] = mat[2][0] * rhs[0][0] + mat[2][1] * rhs[1][0] + mat[2][2] * rhs[2][0];
    dst[2][1] = mat[2][0] * rhs[0][1] + mat[2][1] * rhs[1][1] + mat[2][2] * rhs[2][1];
    dst[2][2] = mat[2][0] * rhs[0][2] + mat[2][1] * rhs[1][2] + mat[2][2] * rhs[2][2];
    dst[2][3] = mat[2][0] * rhs[0][3] + mat[2][1] * rhs[1][3] + mat[2][2] * rhs[2][3] + mat[2][3];

    dst[3][0] = mat[3][0] * rhs[0][0] + mat[3][1] * rhs[1][0] + mat[3][2] * rhs[2][0];
    dst[3][1] = mat[3][0] * rhs[0][1] + mat[3][1] * rhs[1][1] + mat[3][2] * rhs[2][1];
    dst[3][2] = mat[3][0] * rhs[0][2] + mat[3][1] * rhs[1][2] + mat[3][2] * rhs[2][2];
    dst[3][3] = mat[3][0] * rhs[0][3] + mat[3][1] * rhs[1][3] + mat[3][2] * rhs[2][3] + mat[3][3];
#endif
    return dst;
}

Mat4 &Mat4::operator*=(const Mat4 &rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd4f br0 = loadu_ps(rhs.mat[0]);
    simd4f br1 = loadu_ps(rhs.mat[1]);
    simd4f br2 = loadu_ps(rhs.mat[2]);
    simd4f br3 = loadu_ps(rhs.mat[3]);

    simd8f br00 = set2x128_256ps(br0.m128, br0.m128);
    simd8f br11 = set2x128_256ps(br1.m128, br1.m128);
    simd8f br22 = set2x128_256ps(br2.m128, br2.m128);
    simd8f br33 = set2x128_256ps(br3.m128, br3.m128);

    storeu_256ps(lincomb2x4x4(ar01, br00, br11, br22, br33), mat[0]);
    storeu_256ps(lincomb2x4x4(ar23, br00, br11, br22, br33), mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f br0 = loadu_ps(rhs.mat[0]);
    simd4f br1 = loadu_ps(rhs.mat[1]);
    simd4f br2 = loadu_ps(rhs.mat[2]);
    simd4f br3 = loadu_ps(rhs.mat[3]);

    storeu_ps(lincomb4x4(ar0, br0, br1, br2, br3), mat[0]);
    storeu_ps(lincomb4x4(ar1, br0, br1, br2, br3), mat[1]);
    storeu_ps(lincomb4x4(ar2, br0, br1, br2, br3), mat[2]);
    storeu_ps(lincomb4x4(ar3, br0, br1, br2, br3), mat[3]);
#else
    float dst[4];

    dst[0] = mat[0][0] * rhs.mat[0][0] + mat[0][1] * rhs.mat[1][0] + mat[0][2] * rhs.mat[2][0] + mat[0][3] * rhs.mat[3][0];
    dst[1] = mat[0][0] * rhs.mat[0][1] + mat[0][1] * rhs.mat[1][1] + mat[0][2] * rhs.mat[2][1] + mat[0][3] * rhs.mat[3][1];
    dst[2] = mat[0][0] * rhs.mat[0][2] + mat[0][1] * rhs.mat[1][2] + mat[0][2] * rhs.mat[2][2] + mat[0][3] * rhs.mat[3][2];
    dst[3] = mat[0][0] * rhs.mat[0][3] + mat[0][1] * rhs.mat[1][3] + mat[0][2] * rhs.mat[2][3] + mat[0][3] * rhs.mat[3][3];
    mat[0][0] = dst[0];
    mat[0][1] = dst[1];
    mat[0][2] = dst[2];
    mat[0][3] = dst[3];

    dst[0] = mat[1][0] * rhs.mat[0][0] + mat[1][1] * rhs.mat[1][0] + mat[1][2] * rhs.mat[2][0] + mat[1][3] * rhs.mat[3][0];
    dst[1] = mat[1][0] * rhs.mat[0][1] + mat[1][1] * rhs.mat[1][1] + mat[1][2] * rhs.mat[2][1] + mat[1][3] * rhs.mat[3][1];
    dst[2] = mat[1][0] * rhs.mat[0][2] + mat[1][1] * rhs.mat[1][2] + mat[1][2] * rhs.mat[2][2] + mat[1][3] * rhs.mat[3][2];
    dst[3] = mat[1][0] * rhs.mat[0][3] + mat[1][1] * rhs.mat[1][3] + mat[1][2] * rhs.mat[2][3] + mat[1][3] * rhs.mat[3][3];
    mat[1][0] = dst[0];
    mat[1][1] = dst[1];
    mat[1][2] = dst[2];
    mat[1][3] = dst[3];

    dst[0] = mat[2][0] * rhs.mat[0][0] + mat[2][1] * rhs.mat[1][0] + mat[2][2] * rhs.mat[2][0] + mat[2][3] * rhs.mat[3][0];
    dst[1] = mat[2][0] * rhs.mat[0][1] + mat[2][1] * rhs.mat[1][1] + mat[2][2] * rhs.mat[2][1] + mat[2][3] * rhs.mat[3][1];
    dst[2] = mat[2][0] * rhs.mat[0][2] + mat[2][1] * rhs.mat[1][2] + mat[2][2] * rhs.mat[2][2] + mat[2][3] * rhs.mat[3][2];
    dst[3] = mat[2][0] * rhs.mat[0][3] + mat[2][1] * rhs.mat[1][3] + mat[2][2] * rhs.mat[2][3] + mat[2][3] * rhs.mat[3][3];
    mat[2][0] = dst[0];
    mat[2][1] = dst[1];
    mat[2][2] = dst[2];
    mat[2][3] = dst[3];

    dst[0] = mat[3][0] * rhs.mat[0][0] + mat[3][1] * rhs.mat[1][0] + mat[3][2] * rhs.mat[2][0] + mat[3][3] * rhs.mat[3][0];
    dst[1] = mat[3][0] * rhs.mat[0][1] + mat[3][1] * rhs.mat[1][1] + mat[3][2] * rhs.mat[2][1] + mat[3][3] * rhs.mat[3][1];
    dst[2] = mat[3][0] * rhs.mat[0][2] + mat[3][1] * rhs.mat[1][2] + mat[3][2] * rhs.mat[2][2] + mat[3][3] * rhs.mat[3][2];
    dst[3] = mat[3][0] * rhs.mat[0][3] + mat[3][1] * rhs.mat[1][3] + mat[3][2] * rhs.mat[2][3] + mat[3][3] * rhs.mat[3][3];
    mat[3][0] = dst[0];
    mat[3][1] = dst[1];
    mat[3][2] = dst[2];
    mat[3][3] = dst[3];
#endif
    return *this;
}

Mat4 &Mat4::operator*=(const Mat3x4 &rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f ar01 = loadu_256ps(mat[0]);
    simd8f ar23 = loadu_256ps(mat[2]);

    simd4f br0 = loadu_ps(rhs.mat[0]);
    simd4f br1 = loadu_ps(rhs.mat[1]);
    simd4f br2 = loadu_ps(rhs.mat[2]);

    simd8f br00 = set2x128_256ps(br0.m128, br0.m128);
    simd8f br11 = set2x128_256ps(br1.m128, br1.m128);
    simd8f br22 = set2x128_256ps(br2.m128, br2.m128);

    storeu_256ps(lincomb2x3x4(ar01, br00, br11, br22), mat[0]);
    storeu_256ps(lincomb2x3x4(ar23, br00, br11, br22), mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    simd4f br0 = loadu_ps(rhs.mat[0]);
    simd4f br1 = loadu_ps(rhs.mat[1]);
    simd4f br2 = loadu_ps(rhs.mat[2]);

    storeu_ps(lincomb3x4(ar0, br0, br1, br2), mat[0]);
    storeu_ps(lincomb3x4(ar1, br0, br1, br2), mat[1]);
    storeu_ps(lincomb3x4(ar2, br0, br1, br2), mat[2]);
    storeu_ps(lincomb3x4(ar3, br0, br1, br2), mat[3]);
#else
    float dst[4];

    dst[0] = mat[0][0] * rhs.mat[0][0] + mat[0][1] * rhs.mat[1][0] + mat[0][2] * rhs.mat[2][0];
    dst[1] = mat[0][0] * rhs.mat[0][1] + mat[0][1] * rhs.mat[1][1] + mat[0][2] * rhs.mat[2][1];
    dst[2] = mat[0][0] * rhs.mat[0][2] + mat[0][1] * rhs.mat[1][2] + mat[0][2] * rhs.mat[2][2];
    dst[3] = mat[0][0] * rhs.mat[0][3] + mat[0][1] * rhs.mat[1][3] + mat[0][2] * rhs.mat[2][3] + mat[0][3];
    mat[0][0] = dst[0];
    mat[0][1] = dst[1];
    mat[0][2] = dst[2];
    mat[0][3] = dst[3];

    dst[0] = mat[1][0] * rhs.mat[0][0] + mat[1][1] * rhs.mat[1][0] + mat[1][2] * rhs.mat[2][0];
    dst[1] = mat[1][0] * rhs.mat[0][1] + mat[1][1] * rhs.mat[1][1] + mat[1][2] * rhs.mat[2][1];
    dst[2] = mat[1][0] * rhs.mat[0][2] + mat[1][1] * rhs.mat[1][2] + mat[1][2] * rhs.mat[2][2];
    dst[3] = mat[1][0] * rhs.mat[0][3] + mat[1][1] * rhs.mat[1][3] + mat[1][2] * rhs.mat[2][3] + mat[1][3];
    mat[1][0] = dst[0];
    mat[1][1] = dst[1];
    mat[1][2] = dst[2];
    mat[1][3] = dst[3];

    dst[0] = mat[2][0] * rhs.mat[0][0] + mat[2][1] * rhs.mat[1][0] + mat[2][2] * rhs.mat[2][0];
    dst[1] = mat[2][0] * rhs.mat[0][1] + mat[2][1] * rhs.mat[1][1] + mat[2][2] * rhs.mat[2][1];
    dst[2] = mat[2][0] * rhs.mat[0][2] + mat[2][1] * rhs.mat[1][2] + mat[2][2] * rhs.mat[2][2];
    dst[3] = mat[2][0] * rhs.mat[0][3] + mat[2][1] * rhs.mat[1][3] + mat[2][2] * rhs.mat[2][3] + mat[2][3];
    mat[2][0] = dst[0];
    mat[2][1] = dst[1];
    mat[2][2] = dst[2];
    mat[2][3] = dst[3];

    dst[0] = mat[3][0] * rhs.mat[0][0] + mat[3][1] * rhs.mat[1][0] + mat[3][2] * rhs.mat[2][0];
    dst[1] = mat[3][0] * rhs.mat[0][1] + mat[3][1] * rhs.mat[1][1] + mat[3][2] * rhs.mat[2][1];
    dst[2] = mat[3][0] * rhs.mat[0][2] + mat[3][1] * rhs.mat[1][2] + mat[3][2] * rhs.mat[2][2];
    dst[3] = mat[3][0] * rhs.mat[0][3] + mat[3][1] * rhs.mat[1][3] + mat[3][2] * rhs.mat[2][3] + mat[3][3];
    mat[3][0] = dst[0];
    mat[3][1] = dst[1];
    mat[3][2] = dst[2];
    mat[3][3] = dst[3];
#endif
    return *this;
}

Vec4 Mat4::TransposedMulVec(const Vec4 &vec) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Vec4 dst;

    simd4f ac0 = loadu_ps(mat[0]);
    simd4f ac1 = loadu_ps(mat[1]);
    simd4f ac2 = loadu_ps(mat[2]);
    simd4f ac3 = loadu_ps(mat[3]);
    simd4f v = loadu_ps(vec);
    simd4f result = ac0 * shuffle_ps<0, 0, 0, 0>(v);
    result = madd_ps(ac1, shuffle_ps<1, 1, 1, 1>(v), result);
    result = madd_ps(ac2, shuffle_ps<2, 2, 2, 2>(v), result);
    result = madd_ps(ac3, shuffle_ps<3, 3, 3, 3>(v), result);
    store_ps(result, dst);
#else
    Vec4 dst;

    dst[0] = mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z + mat[3].x * vec.w;
    dst[1] = mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z + mat[3].y * vec.w;
    dst[2] = mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z + mat[3].z * vec.w;
    dst[3] = mat[0].w * vec.x + mat[1].w * vec.y + mat[2].w * vec.z + mat[3].w * vec.w;
#endif
    return dst;
}

Mat4 Mat4::TransposedMul(const Mat4 &a) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    transpose4x4(ar0, ar1, ar2, ar3);

    simd4f br0 = loadu_ps(a.mat[0]);
    simd4f br1 = loadu_ps(a.mat[1]);
    simd4f br2 = loadu_ps(a.mat[2]);
    simd4f br3 = loadu_ps(a.mat[3]);

    store_ps(lincomb4x4(ar0, br0, br1, br2, br3), dst.mat[0]);
    store_ps(lincomb4x4(ar1, br0, br1, br2, br3), dst.mat[1]);
    store_ps(lincomb4x4(ar2, br0, br1, br2, br3), dst.mat[2]);
    store_ps(lincomb4x4(ar3, br0, br1, br2, br3), dst.mat[3]);
#else
    Mat4 dst;

    float *dstPtr = dst.Ptr();
    const float *m1Ptr = Ptr();
    const float *m2Ptr = a.Ptr();

    for (int c = 0; c < Cols; c++) {
        for (int r = 0; r < Rows; r++) {
            *dstPtr =
                m1Ptr[0 * Cols] * m2Ptr[0 * Cols + r] +
                m1Ptr[1 * Cols] * m2Ptr[1 * Cols + r] +
                m1Ptr[2 * Cols] * m2Ptr[2 * Cols + r] +
                m1Ptr[3 * Cols] * m2Ptr[3 * Cols + r];
            dstPtr++;
        }
        m1Ptr += 1;
    }
#endif
    return dst;
}

Mat4 Mat4::TransposedMul(const Mat3x4 &a) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);
    simd4f ar3 = loadu_ps(mat[3]);

    transpose4x4(ar0, ar1, ar2, ar3);

    simd4f br0 = loadu_ps(a.mat[0]);
    simd4f br1 = loadu_ps(a.mat[1]);
    simd4f br2 = loadu_ps(a.mat[2]);

    store_ps(lincomb3x4(ar0, br0, br1, br2), dst.mat[0]);
    store_ps(lincomb3x4(ar1, br0, br1, br2), dst.mat[1]);
    store_ps(lincomb3x4(ar2, br0, br1, br2), dst.mat[2]);
    store_ps(lincomb3x4(ar3, br0, br1, br2), dst.mat[3]);
#else
    Mat4 dst;

    dst[0][0] = mat[0][0] * a[0][0] + mat[1][0] * a[1][0] + mat[2][0] * a[2][0];
    dst[0][1] = mat[0][0] * a[0][1] + mat[1][0] * a[1][1] + mat[2][0] * a[2][1];
    dst[0][2] = mat[0][0] * a[0][2] + mat[1][0] * a[1][2] + mat[2][0] * a[2][2];
    dst[0][3] = mat[0][0] * a[0][3] + mat[1][0] * a[1][3] + mat[2][0] * a[2][3] + mat[3][0];

    dst[1][0] = mat[0][1] * a[0][0] + mat[1][1] * a[1][0] + mat[2][1] * a[2][0];
    dst[1][1] = mat[0][1] * a[0][1] + mat[1][1] * a[1][1] + mat[2][1] * a[2][1];
    dst[1][2] = mat[0][1] * a[0][2] + mat[1][1] * a[1][2] + mat[2][1] * a[2][2];
    dst[1][3] = mat[0][1] * a[0][3] + mat[1][1] * a[1][3] + mat[2][1] * a[2][3] + mat[3][1];

    dst[2][0] = mat[0][2] * a[0][0] + mat[1][2] * a[1][0] + mat[2][2] * a[2][0];
    dst[2][1] = mat[0][2] * a[0][1] + mat[1][2] * a[1][1] + mat[2][2] * a[2][1];
    dst[2][2] = mat[0][2] * a[0][2] + mat[1][2] * a[1][2] + mat[2][2] * a[2][2];
    dst[2][3] = mat[0][2] * a[0][3] + mat[1][2] * a[1][3] + mat[2][2] * a[2][3] + mat[3][2];

    dst[3][0] = mat[0][3] * a[0][0] + mat[1][3] * a[1][0] + mat[2][3] * a[2][0];
    dst[3][1] = mat[0][3] * a[0][1] + mat[1][3] * a[1][1] + mat[2][3] * a[2][1];
    dst[3][2] = mat[0][3] * a[0][2] + mat[1][3] * a[1][2] + mat[2][3] * a[2][2];
    dst[3][3] = mat[0][3] * a[0][3] + mat[1][3] * a[1][3] + mat[2][3] * a[2][3] + mat[3][3];
#endif
    return dst;
}

Vec3 Mat4::TransformDir(const Vec3 &dir) const {
    return ToMat3x4().TransformDir(dir);
}

void Mat4::BatchTransformDir(Vec3 *dirArray, int numElements) const {
    ToMat3x4().BatchTransformDir(dirArray, numElements);
}

Mat4 Mat4::Transpose() const & {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat4 result;

    #ifdef HAVE_ARM_NEON_INTRIN
        float32x4x4_t m = vld4q_f32((const float32_t *)mat);
        vst1q_f32((float32_t *)result[0], m.val[0]);
        vst1q_f32((float32_t *)result[1], m.val[1]);
        vst1q_f32((float32_t *)result[2], m.val[2]);
        vst1q_f32((float32_t *)result[3], m.val[3]);
    #else
        simd4f r0 = loadu_ps(mat[0]);
        simd4f r1 = loadu_ps(mat[1]);
        simd4f r2 = loadu_ps(mat[2]);
        simd4f r3 = loadu_ps(mat[3]);

        transpose4x4(r0, r1, r2, r3);

        store_ps(r0, result[0]);
        store_ps(r1, result[1]);
        store_ps(r2, result[2]);
        store_ps(r3, result[3]);
    #endif
#else
    Mat4 result;

    for (int i = 0; i < 4; i++ ) {
        for (int j = 0; j < 4; j++ ) {
            result[i][j] = mat[j][i];
        }
    }
#endif
    return result;
}

Mat4 &Mat4::TransposeSelf() {
#if defined(ENABLE_SIMD4_INTRIN)
    #ifdef HAVE_ARM_NEON_INTRIN
        float32x4x4_t m = vld4q_f32((const float32_t *)mat);
        vst1q_f32((float32_t *)mat[0], m.val[0]);
        vst1q_f32((float32_t *)mat[1], m.val[1]);
        vst1q_f32((float32_t *)mat[2], m.val[2]);
        vst1q_f32((float32_t *)mat[3], m.val[3]);
    #else
        simd4f r0 = loadu_ps(mat[0]);
        simd4f r1 = loadu_ps(mat[1]);
        simd4f r2 = loadu_ps(mat[2]);
        simd4f r3 = loadu_ps(mat[3]);

        transpose4x4(r0, r1, r2, r3);

        storeu_ps(r0, mat[0]);
        storeu_ps(r1, mat[1]);
        storeu_ps(r2, mat[2]);
        storeu_ps(r3, mat[3]);
    #endif
#else
    float temp;
    
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            temp = mat[i][j];
            mat[i][j] = mat[j][i];
            mat[j][i] = temp;
        }
    }
#endif
    return *this;
}

Mat4 Mat4::Symmetrize() const & {
    Mat4 symmetric = *this;
    symmetric.SymmetrizeSelf();
    return symmetric;
}

Mat4 &Mat4::SymmetrizeSelf() {
    float xy = 0.5f * (mat[0][1] + mat[1][0]);
    float xz = 0.5f * (mat[0][2] + mat[2][0]);
    float xw = 0.5f * (mat[0][3] + mat[3][0]);
    float yz = 0.5f * (mat[1][2] + mat[2][1]);
    float yw = 0.5f * (mat[1][3] + mat[3][1]);
    float zw = 0.5f * (mat[2][3] + mat[3][2]);

    mat[0][1] = mat[1][0] = xy;
    mat[0][2] = mat[2][0] = xz;
    mat[0][3] = mat[3][0] = xw;
    mat[1][2] = mat[2][1] = yz;
    mat[1][3] = mat[3][1] = yw;
    mat[2][3] = mat[3][2] = zw;

    return *this;
}

Mat4 Mat4::SkewSymmetrize() const & {
    Mat4 skewSymmetric = *this;
    skewSymmetric.SkewSymmetrizeSelf();
    return skewSymmetric;
}

Mat4 &Mat4::SkewSymmetrizeSelf() {
    float xy = 0.5f * (mat[0][1] - mat[1][0]);
    float xz = 0.5f * (mat[0][2] - mat[2][0]);
    float xw = 0.5f * (mat[0][3] - mat[3][0]);
    float yz = 0.5f * (mat[1][2] - mat[2][1]);
    float yw = 0.5f * (mat[1][3] - mat[3][1]);
    float zw = 0.5f * (mat[2][3] - mat[3][2]);

    mat[0][0] = mat[1][1] = mat[2][2] = mat[3][3] = 0.0f;
    mat[0][1] = xy;
    mat[1][0] = -xy;
    mat[0][2] = xz;
    mat[2][0] = -xz;
    mat[0][3] = xw;
    mat[3][0] = -xw;
    mat[1][2] = yz;
    mat[2][1] = -yz;
    mat[1][3] = yw;
    mat[3][1] = -yw;
    mat[2][3] = zw;
    mat[3][2] = -zw;

    return *this;
}

#if defined(ENABLE_SIMD4_INTRIN)
static BE_FORCE_INLINE simd4f Mat2AdjugateSIMD(const simd4f &matrix) {
    return shuffle_ps<3, 1, 2, 0>(matrix) * set_ps(1.0f, -1.0f, -1.0f, 1.0f);
}

static BE_FORCE_INLINE simd4f Mat2MulSIMD(const simd4f &lhs, const simd4f &rhs) {
    const simd4f lhsDiagonal = shuffle_ps<0, 0, 2, 2>(lhs);
    const simd4f lhsOffDiagonal = shuffle_ps<1, 1, 3, 3>(lhs);
    const simd4f rhsTopRow = shuffle_ps<0, 1, 0, 1>(rhs);
    const simd4f rhsBottomRow = shuffle_ps<2, 3, 2, 3>(rhs);
    return madd_ps(lhsOffDiagonal, rhsBottomRow, lhsDiagonal * rhsTopRow);
}

static BE_FORCE_INLINE float Mat2DeterminantSIMD(const simd4f &matrix) {
    const simd4f products = matrix * shuffle_ps<3, 2, 1, 0>(matrix);
    return x_ps(products - shuffle_ps<1, 0, 3, 2>(products));
}
#endif

bool Mat4::InverseSelf() {
#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f row0 = loadu_ps(mat[0]);
    const simd4f row1 = loadu_ps(mat[1]);
    const simd4f row2 = loadu_ps(mat[2]);
    const simd4f row3 = loadu_ps(mat[3]);

    // Pack each row-major 2x2 block as [ m00, m01, m10, m11 ].
    const simd4f blockA = shuffle_ps<0, 1, 0, 1>(row0, row1);
    const simd4f blockB = shuffle_ps<2, 3, 2, 3>(row0, row1);
    const simd4f blockC = shuffle_ps<0, 1, 0, 1>(row2, row3);
    const simd4f blockD = shuffle_ps<2, 3, 2, 3>(row2, row3);

    const float determinantA = Mat2DeterminantSIMD(blockA);
    const float determinantB = Mat2DeterminantSIMD(blockB);
    const float determinantC = Mat2DeterminantSIMD(blockC);
    const float determinantD = Mat2DeterminantSIMD(blockD);

    const simd4f adjugateA_B = Mat2MulSIMD(Mat2AdjugateSIMD(blockA), blockB);
    const simd4f adjugateD_C = Mat2MulSIMD(Mat2AdjugateSIMD(blockD), blockC);

    simd4f blockX = determinantD * blockA - Mat2MulSIMD(blockB, adjugateD_C);
    simd4f blockY = determinantB * blockC - Mat2MulSIMD(blockD, Mat2AdjugateSIMD(adjugateA_B));
    simd4f blockZ = determinantC * blockB - Mat2MulSIMD(blockA, Mat2AdjugateSIMD(adjugateD_C));
    simd4f blockW = determinantA * blockD - Mat2MulSIMD(blockC, adjugateA_B);

    const float trace = x_ps(dot4_ps(adjugateA_B, shuffle_ps<0, 2, 1, 3>(adjugateD_C)));
    const double determinant = (double)determinantA * determinantD + (double)determinantB * determinantC - trace;
    if (Math::Abs(determinant) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    const simd4f inverseDeterminant = set1_ps((float)(1.0 / determinant));
    blockX = Mat2AdjugateSIMD(blockX) * inverseDeterminant;
    blockY = Mat2AdjugateSIMD(blockY) * inverseDeterminant;
    blockZ = Mat2AdjugateSIMD(blockZ) * inverseDeterminant;
    blockW = Mat2AdjugateSIMD(blockW) * inverseDeterminant;

    storeu_ps(shuffle_ps<0, 1, 0, 1>(blockX, blockY), mat[0]);
    storeu_ps(shuffle_ps<2, 3, 2, 3>(blockX, blockY), mat[1]);
    storeu_ps(shuffle_ps<0, 1, 0, 1>(blockZ, blockW), mat[2]);
    storeu_ps(shuffle_ps<2, 3, 2, 3>(blockZ, blockW), mat[3]);
    return true;
#else
#if 1
    // 84+4+16 = 104 multiplications
    //             1 division
    // 2x2 sub-determinants
    float det2_01_01 = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
    float det2_01_02 = mat[0][0] * mat[1][2] - mat[0][2] * mat[1][0];
    float det2_01_03 = mat[0][0] * mat[1][3] - mat[0][3] * mat[1][0];
    float det2_01_12 = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
    float det2_01_13 = mat[0][1] * mat[1][3] - mat[0][3] * mat[1][1];
    float det2_01_23 = mat[0][2] * mat[1][3] - mat[0][3] * mat[1][2];

    // 3x3 sub-determinants
    float det3_201_012 = mat[2][0] * det2_01_12 - mat[2][1] * det2_01_02 + mat[2][2] * det2_01_01;
    float det3_201_013 = mat[2][0] * det2_01_13 - mat[2][1] * det2_01_03 + mat[2][3] * det2_01_01;
    float det3_201_023 = mat[2][0] * det2_01_23 - mat[2][2] * det2_01_03 + mat[2][3] * det2_01_02;
    float det3_201_123 = mat[2][1] * det2_01_23 - mat[2][2] * det2_01_13 + mat[2][3] * det2_01_12;

    double det = (-det3_201_123 * mat[3][0] + det3_201_023 * mat[3][1] - det3_201_013 * mat[3][2] + det3_201_012 * mat[3][3]);

    if (Math::Abs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    double invDet = 1.0f / det;

    // remaining 2x2 sub-determinants
    float det2_03_01 = mat[0][0] * mat[3][1] - mat[0][1] * mat[3][0];
    float det2_03_02 = mat[0][0] * mat[3][2] - mat[0][2] * mat[3][0];
    float det2_03_03 = mat[0][0] * mat[3][3] - mat[0][3] * mat[3][0];
    float det2_03_12 = mat[0][1] * mat[3][2] - mat[0][2] * mat[3][1];
    float det2_03_13 = mat[0][1] * mat[3][3] - mat[0][3] * mat[3][1];
    float det2_03_23 = mat[0][2] * mat[3][3] - mat[0][3] * mat[3][2];

    float det2_13_01 = mat[1][0] * mat[3][1] - mat[1][1] * mat[3][0];
    float det2_13_02 = mat[1][0] * mat[3][2] - mat[1][2] * mat[3][0];
    float det2_13_03 = mat[1][0] * mat[3][3] - mat[1][3] * mat[3][0];
    float det2_13_12 = mat[1][1] * mat[3][2] - mat[1][2] * mat[3][1];
    float det2_13_13 = mat[1][1] * mat[3][3] - mat[1][3] * mat[3][1];
    float det2_13_23 = mat[1][2] * mat[3][3] - mat[1][3] * mat[3][2];

    // remaining 3x3 sub-determinants
    float det3_203_012 = mat[2][0] * det2_03_12 - mat[2][1] * det2_03_02 + mat[2][2] * det2_03_01;
    float det3_203_013 = mat[2][0] * det2_03_13 - mat[2][1] * det2_03_03 + mat[2][3] * det2_03_01;
    float det3_203_023 = mat[2][0] * det2_03_23 - mat[2][2] * det2_03_03 + mat[2][3] * det2_03_02;
    float det3_203_123 = mat[2][1] * det2_03_23 - mat[2][2] * det2_03_13 + mat[2][3] * det2_03_12;

    float det3_213_012 = mat[2][0] * det2_13_12 - mat[2][1] * det2_13_02 + mat[2][2] * det2_13_01;
    float det3_213_013 = mat[2][0] * det2_13_13 - mat[2][1] * det2_13_03 + mat[2][3] * det2_13_01;
    float det3_213_023 = mat[2][0] * det2_13_23 - mat[2][2] * det2_13_03 + mat[2][3] * det2_13_02;
    float det3_213_123 = mat[2][1] * det2_13_23 - mat[2][2] * det2_13_13 + mat[2][3] * det2_13_12;

    float det3_301_012 = mat[3][0] * det2_01_12 - mat[3][1] * det2_01_02 + mat[3][2] * det2_01_01;
    float det3_301_013 = mat[3][0] * det2_01_13 - mat[3][1] * det2_01_03 + mat[3][3] * det2_01_01;
    float det3_301_023 = mat[3][0] * det2_01_23 - mat[3][2] * det2_01_03 + mat[3][3] * det2_01_02;
    float det3_301_123 = mat[3][1] * det2_01_23 - mat[3][2] * det2_01_13 + mat[3][3] * det2_01_12;

    mat[0][0] = -det3_213_123 * invDet;
    mat[1][0] = +det3_213_023 * invDet;
    mat[2][0] = -det3_213_013 * invDet;
    mat[3][0] = +det3_213_012 * invDet;

    mat[0][1] = +det3_203_123 * invDet;
    mat[1][1] = -det3_203_023 * invDet;
    mat[2][1] = +det3_203_013 * invDet;
    mat[3][1] = -det3_203_012 * invDet;

    mat[0][2] = +det3_301_123 * invDet;
    mat[1][2] = -det3_301_023 * invDet;
    mat[2][2] = +det3_301_013 * invDet;
    mat[3][2] = -det3_301_012 * invDet;

    mat[0][3] = -det3_201_123 * invDet;
    mat[1][3] = +det3_201_023 * invDet;
    mat[2][3] = -det3_201_013 * invDet;
    mat[3][3] = +det3_201_012 * invDet;

    return true;
#elif 0
    // 4*18 = 72 multiplications
    //         4 divisions
    float *mat = reinterpret_cast<float *>(this);
    float s;
    double d, di;

    di = mat[0];
    s = di;
    mat[0] = d = 1.0f / di;
    mat[1] *= d;
    mat[2] *= d;
    mat[3] *= d;
    d = -d;
    mat[4] *= d;
    mat[8] *= d;
    mat[12] *= d;
    d = mat[4] * di;
    mat[5] += mat[1] * d;
    mat[6] += mat[2] * d;
    mat[7] += mat[3] * d;
    d = mat[8] * di;
    mat[9] += mat[1] * d;
    mat[10] += mat[2] * d;
    mat[11] += mat[3] * d;
    d = mat[12] * di;
    mat[13] += mat[1] * d;
    mat[14] += mat[2] * d;
    mat[15] += mat[3] * d;
    di = mat[5];
    s *= di;
    mat[5] = d = 1.0f / di;
    mat[4] *= d;
    mat[6] *= d;
    mat[7] *= d;
    d = -d;
    mat[1] *= d;
    mat[9] *= d;
    mat[13] *= d;
    d = mat[1] * di;
    mat[0] += mat[4] * d;
    mat[2] += mat[6] * d;
    mat[3] += mat[7] * d;
    d = mat[9] * di;
    mat[8] += mat[4] * d;
    mat[10] += mat[6] * d;
    mat[11] += mat[7] * d;
    d = mat[13] * di;
    mat[12] += mat[4] * d;
    mat[14] += mat[6] * d;
    mat[15] += mat[7] * d;
    di = mat[10];
    s *= di;
    mat[10] = d = 1.0f / di;
    mat[8] *= d;
    mat[9] *= d;
    mat[11] *= d;
    d = -d;
    mat[2] *= d;
    mat[6] *= d;
    mat[14] *= d;
    d = mat[2] * di;
    mat[0] += mat[8] * d;
    mat[1] += mat[9] * d;
    mat[3] += mat[11] * d;
    d = mat[6] * di;
    mat[4] += mat[8] * d;
    mat[5] += mat[9] * d;
    mat[7] += mat[11] * d;
    d = mat[14] * di;
    mat[12] += mat[8] * d;
    mat[13] += mat[9] * d;
    mat[15] += mat[11] * d;
    di = mat[15];
    s *= di;
    mat[15] = d = 1.0f / di;
    mat[12] *= d;
    mat[13] *= d;
    mat[14] *= d;
    d = -d;
    mat[3] *= d;
    mat[7] *= d;
    mat[11] *= d;
    d = mat[3] * di;
    mat[0] += mat[12] * d;
    mat[1] += mat[13] * d;
    mat[2] += mat[14] * d;
    d = mat[7] * di;
    mat[4] += mat[12] * d;
    mat[5] += mat[13] * d;
    mat[6] += mat[14] * d;
    d = mat[11] * di;
    mat[8] += mat[12] * d;
    mat[9] += mat[13] * d;
    mat[10] += mat[14] * d;

    return (s != 0.0f && !FLOAT_IS_NAN(s));
#else
    //  6*8+2*6 = 60 multiplications
    //      2*1 =  2 divisions
    Mat2 r0, r1, r2, r3;
    float a, det, invDet;
    float *mat = reinterpret_cast<float *>(this);

    // r0 = m0.Inverse();
    det = mat[0*4+0] * mat[1*4+1] - mat[0*4+1] * mat[1*4+0];

    if (Math::Abs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    invDet = 1.0f / det;

    r0[0][0] =   mat[1*4+1] * invDet;
    r0[0][1] = - mat[0*4+1] * invDet;
    r0[1][0] = - mat[1*4+0] * invDet;
    r0[1][1] =   mat[0*4+0] * invDet;

    // r1 = r0 * m1;
    r1[0][0] = r0[0][0] * mat[0*4+2] + r0[0][1] * mat[1*4+2];
    r1[0][1] = r0[0][0] * mat[0*4+3] + r0[0][1] * mat[1*4+3];
    r1[1][0] = r0[1][0] * mat[0*4+2] + r0[1][1] * mat[1*4+2];
    r1[1][1] = r0[1][0] * mat[0*4+3] + r0[1][1] * mat[1*4+3];

    // r2 = m2 * r1;
    r2[0][0] = mat[2*4+0] * r1[0][0] + mat[2*4+1] * r1[1][0];
    r2[0][1] = mat[2*4+0] * r1[0][1] + mat[2*4+1] * r1[1][1];
    r2[1][0] = mat[3*4+0] * r1[0][0] + mat[3*4+1] * r1[1][0];
    r2[1][1] = mat[3*4+0] * r1[0][1] + mat[3*4+1] * r1[1][1];

    // r3 = r2 - m3;
    r3[0][0] = r2[0][0] - mat[2*4+2];
    r3[0][1] = r2[0][1] - mat[2*4+3];
    r3[1][0] = r2[1][0] - mat[3*4+2];
    r3[1][1] = r2[1][1] - mat[3*4+3];

    // r3.InverseSelf();
    det = r3[0][0] * r3[1][1] - r3[0][1] * r3[1][0];

    if (Math::Abs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    invDet = 1.0f / det;

    a = r3[0][0];
    r3[0][0] =   r3[1][1] * invDet;
    r3[0][1] = - r3[0][1] * invDet;
    r3[1][0] = - r3[1][0] * invDet;
    r3[1][1] =   a * invDet;

    // r2 = m2 * r0;
    r2[0][0] = mat[2*4+0] * r0[0][0] + mat[2*4+1] * r0[1][0];
    r2[0][1] = mat[2*4+0] * r0[0][1] + mat[2*4+1] * r0[1][1];
    r2[1][0] = mat[3*4+0] * r0[0][0] + mat[3*4+1] * r0[1][0];
    r2[1][1] = mat[3*4+0] * r0[0][1] + mat[3*4+1] * r0[1][1];

    // m2 = r3 * r2;
    mat[2*4+0] = r3[0][0] * r2[0][0] + r3[0][1] * r2[1][0];
    mat[2*4+1] = r3[0][0] * r2[0][1] + r3[0][1] * r2[1][1];
    mat[3*4+0] = r3[1][0] * r2[0][0] + r3[1][1] * r2[1][0];
    mat[3*4+1] = r3[1][0] * r2[0][1] + r3[1][1] * r2[1][1];

    // m0 = r0 - r1 * m2;
    mat[0*4+0] = r0[0][0] - r1[0][0] * mat[2*4+0] - r1[0][1] * mat[3*4+0];
    mat[0*4+1] = r0[0][1] - r1[0][0] * mat[2*4+1] - r1[0][1] * mat[3*4+1];
    mat[1*4+0] = r0[1][0] - r1[1][0] * mat[2*4+0] - r1[1][1] * mat[3*4+0];
    mat[1*4+1] = r0[1][1] - r1[1][0] * mat[2*4+1] - r1[1][1] * mat[3*4+1];

    // m1 = r1 * r3;
    mat[0*4+2] = r1[0][0] * r3[0][0] + r1[0][1] * r3[1][0];
    mat[0*4+3] = r1[0][0] * r3[0][1] + r1[0][1] * r3[1][1];
    mat[1*4+2] = r1[1][0] * r3[0][0] + r1[1][1] * r3[1][0];
    mat[1*4+3] = r1[1][0] * r3[0][1] + r1[1][1] * r3[1][1];

    // m3 = -r3;
    mat[2*4+2] = -r3[0][0];
    mat[2*4+3] = -r3[0][1];
    mat[3*4+2] = -r3[1][0];
    mat[3*4+3] = -r3[1][1];

    return true;
#endif
#endif
}

void Mat4::SetFrustumRH(float left, float right, float bottom, float top, float zNear, float zFar, bool zNegativeOneToOne, bool reverseZ, bool infiniteZFar) {
    // check for division by 0
    if (left == right || top == bottom || (!infiniteZFar && zNear == zFar)) {
        return;
    }

    float n2 = 2.0f * zNear;
    float inv_rl = 1.0f / (right - left);
    float inv_tb = 1.0f / (top - bottom);

    mat[0][0] = n2 * inv_rl;
    mat[0][1] = 0.0f;
    mat[0][2] = (right + left) * inv_rl;
    mat[0][3] = 0.0f;
        
    mat[1][0] = 0.0f;
    mat[1][1] = n2 * inv_tb;
    mat[1][2] = (top + bottom) * inv_tb;
    mat[1][3] = 0.0f;

    mat[2][0] = 0.0f;
    mat[2][1] = 0.0f;
    if (infiniteZFar) {
        if (zNegativeOneToOne) {
            // Z range [-1, 1]
            mat[2][2] = reverseZ ? 1.0f : -1.0f;
            mat[2][3] = reverseZ ? 2.0f * zNear : -2.0f * zNear;
        } else {
            // Z range [0, 1]
            mat[2][2] = reverseZ ? 0.0f : -1.0f;
            mat[2][3] = reverseZ ? zNear : -zNear;
        }
    } else {
        float inv_fn = 1.0f / (zFar - zNear);
        if (zNegativeOneToOne) {
            // Z range [-1, 1]
            if (reverseZ) {
                mat[2][2] = (zFar + zNear) * inv_fn;
                mat[2][3] = (2.0f * zFar * zNear) * inv_fn;
            } else {
                mat[2][2] = -(zFar + zNear) * inv_fn;
                mat[2][3] = -(2.0f * zFar * zNear) * inv_fn;
            }
        } else {
            // Z range [0, 1]
            if (reverseZ) {
                mat[2][2] = zNear * inv_fn;
                mat[2][3] = (zFar * zNear) * inv_fn;
            } else {
                mat[2][2] = -zFar * inv_fn;
                mat[2][3] = -(zFar * zNear) * inv_fn;
            }
        }
    }

    mat[3][0] = 0.0f;
    mat[3][1] = 0.0f;
    mat[3][2] = -1.0f;
    mat[3][3] = 0.0f;
}

void Mat4::SetFrustumLH(float left, float right, float bottom, float top, float zNear, float zFar, bool zNegativeOneToOne, bool reverseZ, bool infiniteZFar) {
    // check for division by 0
    if (left == right || top == bottom || (!infiniteZFar && zNear == zFar)) {
        return;
    }

    float n2 = 2.0f * zNear;
    float inv_rl = 1.0f / (right - left);
    float inv_tb = 1.0f / (top - bottom);

    mat[0][0] = n2 * inv_rl;
    mat[0][1] = 0.0f;
    mat[0][2] = (right + left) * inv_rl;
    mat[0][3] = 0.0f;

    mat[1][0] = 0.0f;
    mat[1][1] = n2 * inv_tb;
    mat[1][2] = (top + bottom) * inv_tb;
    mat[1][3] = 0.0f;

    mat[2][0] = 0.0f;
    mat[2][1] = 0.0f;
    if (infiniteZFar) {
        if (zNegativeOneToOne) {
            // Z range [-1, 1]
            mat[2][2] = reverseZ ? -1.0f : 1.0f;
            mat[2][3] = reverseZ ? 2.0f * zNear : -2.0f * zNear;
        } else {
            // Z range [0, 1]
            mat[2][2] = reverseZ ? 0.0f : 1.0f;
            mat[2][3] = reverseZ ? zNear : -zNear;
        }
    } else {
        float inv_fn = 1.0f / (zFar - zNear);
        if (zNegativeOneToOne) {
            // Z range [-1, 1]
            if (reverseZ) {
                mat[2][2] = -(zFar + zNear) * inv_fn;
                mat[2][3] = (2.0f * zFar * zNear) * inv_fn;
            } else {
                mat[2][2] = (zFar + zNear) * inv_fn;
                mat[2][3] = -(2.0f * zFar * zNear) * inv_fn;
            }
        } else {
            // Z range [0, 1]
            if (reverseZ) {
                mat[2][2] = -zNear * inv_fn;
                mat[2][3] = (zFar * zNear) * inv_fn;
            } else {
                mat[2][2] = zFar * inv_fn;
                mat[2][3] = -(zFar * zNear) * inv_fn;
            }
        }
    }

    mat[3][0] = 0.0f;
    mat[3][1] = 0.0f;
    mat[3][2] = 1.0f;
    mat[3][3] = 0.0f;
}

void Mat4::SetPerspectiveRH(float fovy, float aspect, float zNear, float zFar, bool zNegativeOneToOne, bool reverseZ, bool infiniteZFar) {
    float top       = zNear * Math::Tan(DEG2RAD(fovy * 0.5f));
    float bottom    = -top;
    float right     = top * aspect;
    float left      = -right;

    SetFrustumRH(left, right, bottom, top, zNear, zFar, zNegativeOneToOne, reverseZ, infiniteZFar);
}

void Mat4::SetPerspectiveLH(float fovy, float aspect, float zNear, float zFar, bool zNegativeOneToOne, bool reverseZ, bool infiniteZFar) {
    float top       = zNear * Math::Tan(DEG2RAD(fovy * 0.5f));
    float bottom    = -top;
    float right     = top * aspect;
    float left      = -right;

    SetFrustumLH(left, right, bottom, top, zNear, zFar, zNegativeOneToOne, reverseZ, infiniteZFar);
}

void Mat4::SetOrthoRH(float left, float right, float bottom, float top, float zNear, float zFar, bool zNegativeOneToOne, bool reverseZ) {
    // check for division by 0
    if (left == right || top == bottom || zNear == zFar) {
        return;
    }

    float inv_rl = 1.0f / (right - left);
    float inv_tb = 1.0f / (top - bottom);
    float inv_fn = 1.0f / (zFar - zNear);

    mat[0][0] = 2.0f * inv_rl;
    mat[0][1] = 0.0f;
    mat[0][2] = 0.0f;
    mat[0][3] = -(right + left) * inv_rl;

    mat[1][0] = 0.0f;
    mat[1][1] = 2.0f * inv_tb;
    mat[1][2] = 0.0f;
    mat[1][3] = -(top + bottom) * inv_tb;

    mat[2][0] = 0.0f;
    mat[2][1] = 0.0f;
    if (zNegativeOneToOne) {
        // Z range [-1, 1]
        if (reverseZ) {
            mat[2][2] = 2.0f * inv_fn;
            mat[2][3] = (zFar + zNear) * inv_fn;
        } else {
            mat[2][2] = -2.0f * inv_fn;
            mat[2][3] = -(zFar + zNear) * inv_fn;
        }
    } else {
        // Z range [0, 1]
        if (reverseZ) {
            mat[2][2] = inv_fn;
            mat[2][3] = zFar * inv_fn;
        } else {
            mat[2][2] = -inv_fn;
            mat[2][3] = -zNear * inv_fn;
        }
    }

    mat[3][0] = 0.0f;
    mat[3][1] = 0.0f;
    mat[3][2] = 0.0f;
    mat[3][3] = 1.0f;
}

void Mat4::SetOrthoLH(float left, float right, float bottom, float top, float zNear, float zFar, bool zNegativeOneToOne, bool reverseZ) {
    // check for division by 0
    if (left == right || top == bottom || zNear == zFar) {
        return;
    }

    float inv_rl = 1.0f / (right - left);
    float inv_tb = 1.0f / (top - bottom);
    float inv_fn = 1.0f / (zFar - zNear);

    mat[0][0] = 2.0f * inv_rl;
    mat[0][1] = 0.0f;
    mat[0][2] = 0.0f;
    mat[0][3] = -(right + left) * inv_rl;

    mat[1][0] = 0.0f;
    mat[1][1] = 2.0f * inv_tb;
    mat[1][2] = 0.0f;
    mat[1][3] = -(top + bottom) * inv_tb;

    mat[2][0] = 0.0f;
    mat[2][1] = 0.0f;
    if (zNegativeOneToOne) {
        // Z range [-1, 1]
        if (reverseZ) {
            mat[2][2] = -2.0f * inv_fn;
            mat[2][3] = (zFar + zNear) * inv_fn;
        } else {
            mat[2][2] = 2.0f * inv_fn;
            mat[2][3] = -(zFar + zNear) * inv_fn;
        }
    } else {
        // Z range [0, 1]
        if (reverseZ) {
            mat[2][2] = -inv_fn;
            mat[2][3] = zFar * inv_fn;
        } else {
            mat[2][2] = inv_fn;
            mat[2][3] = -zNear * inv_fn;
        }
    }

    mat[3][0] = 0.0f;
    mat[3][1] = 0.0f;
    mat[3][2] = 0.0f;
    mat[3][3] = 1.0f;
}

//--------------------------------------------------------------------------------------------
//
// * 한점을 주어진 평면으로 반사 (reflection) 시키는 행렬을 만든다
//
// X = ( 2 * (-D*N) / (N*N) ) * N + D
//
//     | 1-2*Nx*Nx   -2*Nx*Ny   -2*Nx*Nz  2*d*Nx |
// R = |  -2*Nx*Ny  1-2*Ny*Ny   -2*Ny*Nz  2*d*Ny |
//     |  -2*Nx*Nz   -2*Ny*Nz  1-2*Nz*Nz  2*d*Nz |
//     |         0          0          0       1 |
//
//--------------------------------------------------------------------------------------------
void Mat4::SetReflect(const Plane &plane) {
    Vec3 normal = plane.normal;
    float dist = -plane.offset;

    mat[0][0] = 1.0f - 2.0f * normal.x * normal.x;
    mat[0][1] = 0.0f - 2.0f * normal.x * normal.y;
    mat[0][2] = 0.0f - 2.0f * normal.x * normal.z;
    mat[0][3] = 2.0f * dist * normal.x;

    mat[1][0] = mat[0][1];
    mat[1][1] = 1.0f - 2.0f * normal.y * normal.y;
    mat[1][2] = 0.0f - 2.0f * normal.y * normal.z;
    mat[1][3] = 2.0f * dist * normal.y;

    mat[2][0] = mat[0][2];
    mat[2][1] = mat[1][2];
    mat[2][2] = 1.0f - 2.0f * normal.z * normal.z;
    mat[2][3] = 2.0f * dist * normal.z;

    mat[3][0] = 0.0f;
    mat[3][1] = 0.0f;
    mat[3][2] = 0.0f;
    mat[3][3] = 1.0f;
}

void Mat4::SetTRS(const Vec3 &translation, const Mat3 &rotation, const Vec3 &scale) {
    // T * R * S
#if defined(ENABLE_SIMD4_INTRIN)
    const float *rotationPtr = rotation.Ptr();
    simd4f column0 = loadu_ps(rotationPtr + 0) * set1_ps(scale.x);
    simd4f column1 = loadu_ps(rotationPtr + 3) * set1_ps(scale.y);
    simd4f column2 = shuffle_ps<1, 2, 3, 0>(loadu_ps(rotationPtr + 5)) * set1_ps(scale.z);
    simd4f column3 = set_ps(translation.x, translation.y, translation.z, 0.0f);

    transpose4x4(column0, column1, column2, column3);

    storeu_ps(column0, Ptr() + 0);
    storeu_ps(column1, Ptr() + 4);
    storeu_ps(column2, Ptr() + 8);
    storeu_ps(set_ps(0.0f, 0.0f, 0.0f, 1.0f), Ptr() + 12);
#else
    mat[0][0] = rotation[0][0] * scale.x;
    mat[0][1] = rotation[1][0] * scale.y;
    mat[0][2] = rotation[2][0] * scale.z;
    mat[0][3] = translation.x;

    mat[1][0] = rotation[0][1] * scale.x;
    mat[1][1] = rotation[1][1] * scale.y;
    mat[1][2] = rotation[2][1] * scale.z;
    mat[1][3] = translation.y;

    mat[2][0] = rotation[0][2] * scale.x;
    mat[2][1] = rotation[1][2] * scale.y;
    mat[2][2] = rotation[2][2] * scale.z;
    mat[2][3] = translation.z;

    mat[3][0] = 0.0f;
    mat[3][1] = 0.0f;
    mat[3][2] = 0.0f;
    mat[3][3] = 1.0f;
#endif
}

void Mat4::SetTQS(const Vec3 &translation, const Quat &rotation, const Vec3 &scale) {
#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f firstSignBit = simd4i(0x80000000, 0x00000000, 0x00000000, 0x00000000);
    const simd4f firstPosHalf = set_ps(0.5f, 0.0f, 0.0f, 0.0f);
    const simd4f firstNegHalf = set_ps(-0.5f, 0.0f, 0.0f, 0.0f);
    const simd4f quat2MatMad1 = set_ps(-1.0f, -1.0f, 1.0f, -1.0f);
    const simd4f quat2MatMad2 = set_ps(-1.0f, 1.0f, -1.0f, -1.0f);
    const simd4f quat2MatMad3 = set_ps(1.0f, -1.0f, -1.0f, 1.0f);

    const simd4f q = loadu_ps(rotation.Ptr());
    const simd4f d = q + q;
    const simd4f t = set_ps(translation.x, translation.y, translation.z, 0.0f);
    const simd4f s = set_ps(scale.x, scale.y, scale.z, 1.0f);

    simd4f sa = shuffle_ps<1, 0, 0, 1>(q);
    const simd4f sb = shuffle_ps<1, 1, 2, 2>(d);
    const simd4f sc = shuffle_ps<2, 3, 3, 3>(q);
    const simd4f sd = shuffle_ps<2, 2, 1, 0>(d);
    sa ^= firstSignBit;

    const simd4f ma = madd_ps(sa, sb, firstPosHalf);
    const simd4f mb = madd_ps(sc, sd, firstNegHalf);
    const simd4f mc = firstPosHalf - q * d;

    const simd4f mf = shuffle_ps<1, 1, 0, 0>(ma, mc);
    const simd4f md = shuffle_ps<2, 0, 2, 3>(mf, ma);
    const simd4f me = shuffle_ps<0, 1, 2, 3>(ma, mb);

    simd4f row0 = madd_ps(mb, quat2MatMad1, ma);
    simd4f row1 = madd_ps(mb, quat2MatMad2, md);
    simd4f row2 = madd_ps(me, quat2MatMad3, md);

    const simd4f tx = shuffle_ps<2, 2, 0, 0>(row0, t);
    const simd4f ty = shuffle_ps<3, 3, 1, 1>(row1, t);
    const simd4f tz = shuffle_ps<0, 0, 2, 2>(row2, t);
    row0 = shuffle_ps<0, 1, 0, 2>(row0, tx);
    row1 = shuffle_ps<1, 0, 0, 2>(row1, ty);
    row2 = shuffle_ps<2, 3, 0, 2>(row2, tz);

    storeu_ps(row0 * s, Ptr() + 0);
    storeu_ps(row1 * s, Ptr() + 4);
    storeu_ps(row2 * s, Ptr() + 8);
    storeu_ps(set_ps(0.0f, 0.0f, 0.0f, 1.0f), Ptr() + 12);
#else
    SetTRS(translation, rotation.ToMat3(), scale);
#endif
}

void Mat4::GetTRS(Vec3 &translation, Mat3 &rotation, Vec3 &scale) const {
    ToMat3x4().GetTRS(translation, rotation, scale);
}

void Mat4::GetTQS(Vec3 &translation, Quat &rotation, Vec3 &scale) const {
    Mat3 r;
    GetTRS(translation, r, scale);
    rotation = r.ToQuat();
}

Vec3 Mat4::ToScaleVec3() const {
    return ToMat3x4().ToScaleVec3();
}

Vec3 Mat4::ToSignedScaleVec3() const {
    return ToMat3x4().ToSignedScaleVec3();
}

bool Mat4::Solve(const Vec4 &rightHandSide, Vec4 &solution, float tolerance) const {
    return LU::Solve<4>(*this, rightHandSide, solution, tolerance);
}

bool Mat4::CholeskyDecompose(Mat4 &lowerTriangular, float tolerance) const {
    float input[4][4];
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            input[row][column] = mat[row][column];
        }
    }

    float output[4][4];
    if (!Cholesky::Decompose<4>(input, output, tolerance)) {
        return false;
    }

    Mat4 lower(
        Vec4(output[0][0], output[0][1], output[0][2], output[0][3]),
        Vec4(output[1][0], output[1][1], output[1][2], output[1][3]),
        Vec4(output[2][0], output[2][1], output[2][2], output[2][3]),
        Vec4(output[3][0], output[3][1], output[3][2], output[3][3]));

    lowerTriangular = lower;
    return true;
}

bool Mat4::SolveSymmetricPSD(const Vec4 &rightHandSide, Vec4 &solution, float tolerance) const {
    return SymmetricPSDSolver::Solve<4>(*this, rightHandSide, solution, tolerance);
}

bool Mat4::QRDecomposeColumnPivoted(Mat4 &orthogonalFactor, Mat4 &upperTriangularFactor, int columnPermutation[4], int &rank, float tolerance) const {
    float input[4][4];
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            input[row][column] = mat[row][column];
        }
    }

    float outputQ[4][4];
    float outputR[4][4];
    int outputPermutation[4];
    int outputRank;
    if (!QRCP::Decompose<4>(input, outputQ, outputR, outputPermutation, outputRank, tolerance)) {
        return false;
    }

    Mat4 q(
        Vec4(outputQ[0][0], outputQ[0][1], outputQ[0][2], outputQ[0][3]),
        Vec4(outputQ[1][0], outputQ[1][1], outputQ[1][2], outputQ[1][3]),
        Vec4(outputQ[2][0], outputQ[2][1], outputQ[2][2], outputQ[2][3]),
        Vec4(outputQ[3][0], outputQ[3][1], outputQ[3][2], outputQ[3][3]));
    Mat4 r(
        Vec4(outputR[0][0], outputR[0][1], outputR[0][2], outputR[0][3]),
        Vec4(outputR[1][0], outputR[1][1], outputR[1][2], outputR[1][3]),
        Vec4(outputR[2][0], outputR[2][1], outputR[2][2], outputR[2][3]),
        Vec4(outputR[3][0], outputR[3][1], outputR[3][2], outputR[3][3]));

    orthogonalFactor = q;
    upperTriangularFactor = r;
    for (int i = 0; i < 4; i++) {
        columnPermutation[i] = outputPermutation[i];
    }
    rank = outputRank;
    return true;
}

bool Mat4::EigenDecomposeSymmetric(Vec4 &eigenValues, Mat4 &eigenVectors, float epsilon) const {
    if (epsilon <= 0.0f || !Math::IsFinite(epsilon)) {
        return false;
    }

    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            float value = mat[row][column];
            if (!Math::IsFinite(value)) {
                return false;
            }
        }
    }

    if (!IsSymmetric(epsilon)) {
        return false;
    }

    // Average the two triangles to remove the accepted symmetry tolerance error.
    float matrix[4][4];
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            matrix[row][column] = 0.5f * mat[row][column] + 0.5f * mat[column][row];
        }
    }

    float vectors[4][4] = {};
    for (int i = 0; i < 4; i++) {
        vectors[i][i] = 1.0f;
    }
    const int maxIterations = 64;
    bool converged = false;

    for (int iteration = 0; iteration < maxIterations; iteration++) {
        int p = 0;
        int q = 1;
        float maxOffDiagonal = Math::Abs(matrix[p][q]);

        for (int row = 0; row < 3; row++) {
            for (int column = row + 1; column < 4; column++) {
                float offDiagonal = Math::Abs(matrix[row][column]);
                if (offDiagonal > maxOffDiagonal) {
                    maxOffDiagonal = offDiagonal;
                    p = row;
                    q = column;
                }
            }
        }

        if (!Math::IsFinite(maxOffDiagonal)) {
            return false;
        }

        float matrixScale = maxOffDiagonal;
        for (int i = 0; i < 4; i++) {
            float diagonal = Math::Abs(matrix[i][i]);
            if (!Math::IsFinite(diagonal)) {
                return false;
            }
            if (diagonal > matrixScale) {
                matrixScale = diagonal;
            }
        }

        if (maxOffDiagonal <= epsilon * matrixScale) {
            converged = true;
            break;
        }

        JacobiRotation::Coefficients rotation = JacobiRotation::ComputeSymmetric(matrix[p][p], matrix[q][q], matrix[p][q]);
        JacobiRotation::RotateSymmetric<4>(matrix, p, q, rotation);
        JacobiRotation::RotateColumns<4>(vectors, p, q, rotation);
    }

    if (!converged) {
        float maxOffDiagonal = 0.0f;
        float matrixScale = 0.0f;

        for (int row = 0; row < 4; row++) {
            float diagonal = Math::Abs(matrix[row][row]);
            if (!Math::IsFinite(diagonal)) {
                return false;
            }
            if (diagonal > matrixScale) {
                matrixScale = diagonal;
            }

            for (int column = row + 1; column < 4; column++) {
                float offDiagonal = Math::Abs(matrix[row][column]);
                if (!Math::IsFinite(offDiagonal)) {
                    return false;
                }
                if (offDiagonal > maxOffDiagonal) {
                    maxOffDiagonal = offDiagonal;
                }
            }
        }

        if (maxOffDiagonal > matrixScale) {
            matrixScale = maxOffDiagonal;
        }
        if (maxOffDiagonal > epsilon * matrixScale) {
            return false;
        }
    }

    Vec4 values(matrix[0][0], matrix[1][1], matrix[2][2], matrix[3][3]);

    // Sort eigenvalues in descending order and keep their eigenvectors paired.
    for (int i = 0; i < 3; i++) {
        int maxIndex = i;
        for (int j = i + 1; j < 4; j++) {
            if (values[j] > values[maxIndex]) {
                maxIndex = j;
            }
        }

        if (maxIndex != i) {
            float value = values[i];
            values[i] = values[maxIndex];
            values[maxIndex] = value;

            for (int row = 0; row < 4; row++) {
                float component = vectors[row][i];
                vectors[row][i] = vectors[row][maxIndex];
                vectors[row][maxIndex] = component;
            }
        }
    }

    Mat4 resultVectors(
        Vec4(vectors[0][0], vectors[0][1], vectors[0][2], vectors[0][3]),
        Vec4(vectors[1][0], vectors[1][1], vectors[1][2], vectors[1][3]),
        Vec4(vectors[2][0], vectors[2][1], vectors[2][2], vectors[2][3]),
        Vec4(vectors[3][0], vectors[3][1], vectors[3][2], vectors[3][3]));

    // Sorting columns can change orientation; eigenvector signs are arbitrary.
    if (resultVectors.Determinant() < 0.0f) {
        for (int row = 0; row < 4; row++) {
            resultVectors[row][3] = -resultVectors[row][3];
        }
    }

    eigenValues = values;
    eigenVectors = resultVectors;
    return true;
}

bool Mat4::SingularValueDecompose(Mat4 &leftVectors, Vec4 &singularValues, Mat4 &rightVectors, float tolerance) const {
    float input[4][4];
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            input[row][column] = mat[row][column];
        }
    }

    float outputU[4][4];
    float outputSingularValues[4];
    float outputV[4][4];
    if (!JacobiSVD::Decompose<4>(input, outputU, outputSingularValues, outputV, tolerance)) {
        return false;
    }

    Mat4 u(
        Vec4(outputU[0][0], outputU[0][1], outputU[0][2], outputU[0][3]),
        Vec4(outputU[1][0], outputU[1][1], outputU[1][2], outputU[1][3]),
        Vec4(outputU[2][0], outputU[2][1], outputU[2][2], outputU[2][3]),
        Vec4(outputU[3][0], outputU[3][1], outputU[3][2], outputU[3][3]));
    Mat4 v(
        Vec4(outputV[0][0], outputV[0][1], outputV[0][2], outputV[0][3]),
        Vec4(outputV[1][0], outputV[1][1], outputV[1][2], outputV[1][3]),
        Vec4(outputV[2][0], outputV[2][1], outputV[2][2], outputV[2][3]),
        Vec4(outputV[3][0], outputV[3][1], outputV[3][2], outputV[3][3]));

    leftVectors = u;
    singularValues.Set(outputSingularValues[0], outputSingularValues[1], outputSingularValues[2], outputSingularValues[3]);
    rightVectors = v;
    return true;
}

bool Mat4::ConditionNumber(float &conditionNumber, float tolerance) const {
    return JacobiSVD::ComputeConditionNumber<4, Mat4, Vec4>(*this, conditionNumber, tolerance);
}

bool Mat4::PseudoInverse(Mat4 &pseudoInverse, float tolerance) const {
    return JacobiSVD::ComputePseudoInverse<4, Mat4, Vec4>(*this, pseudoInverse, tolerance);
}

Mat4 Mat4::FromString(const char *str) {
    Mat4 m;
    int count = sscanf(str, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
        &m[0].x, &m[0].y, &m[0].z, &m[0].w, &m[1].x, &m[1].y, &m[1].z, &m[1].w, &m[2].x, &m[2].y, &m[2].z, &m[2].w, &m[3].x, &m[3].y, &m[3].z, &m[3].w);
    assert(count == m.GetDimension());
    return m;
}

BE_NAMESPACE_END
