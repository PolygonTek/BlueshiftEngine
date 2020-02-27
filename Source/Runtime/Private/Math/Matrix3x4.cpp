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

ALIGN_AS16 const Mat3x4 Mat3x4::zero(Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0));
ALIGN_AS16 const Mat3x4 Mat3x4::identity(Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0));

//---------------------------------------------------
//
//        |  1  0  0  tx | | m00  m01  m02  m03 |
// T M  = |  0  1  0  ty | | m10  m11  m12  m13 |
//        |  0  0  1  tz | | m20  m21  m22  m23 |
//
//---------------------------------------------------

Mat3x4 &Mat3x4::Translate(float tx, float ty, float tz) {
    mat[0][3] += tx;
    mat[1][3] += ty;
    mat[2][3] += tz;

    return *this;
}

//---------------------------------------------------
//
//       | m00  m01  m02  m03 | |  1  0  0  tx |
// M T = | m10  m11  m12  m13 | |  0  1  0  ty |
//       | m20  m21  m22  m23 | |  0  0  1  tz |
//
//---------------------------------------------------

Mat3x4 &Mat3x4::TranslateRight(float tx, float ty, float tz) {
    mat[0][3] += mat[0][0] * tx + mat[0][1] * ty + mat[0][2] * tz;
    mat[1][3] += mat[1][0] * tx + mat[1][1] * ty + mat[1][2] * tz;
    mat[2][3] += mat[2][0] * tx + mat[2][1] * ty + mat[2][2] * tz;

    return *this;
}

//---------------------------------------------------
//
//       | sx   0   0 | | m00  m01  m02  m03 |
// S M = |  0  sy   0 | | m10  m11  m12  m13 |
//       |  0   0  sz | | m20  m21  m22  m23 |
//
//---------------------------------------------------

Mat3x4 &Mat3x4::Scale(float sx, float sy, float sz) {
    mat[0] *= sx;
    mat[1] *= sy;
    mat[2] *= sz;

    return *this;
}

Mat3x4 Mat3x4::operator-() const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat3x4 dst;

    simd4f r0 = loadu_ps(mat[0]);
    simd4f r1 = loadu_ps(mat[1]);
    simd4f r2 = loadu_ps(mat[2]);

    store_ps(-r0, dst[0]);
    store_ps(-r1, dst[1]);
    store_ps(-r2, dst[2]);
#else
    Mat3x4 dst;

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
#endif
    return dst;
}

Mat3x4 Mat3x4::operator+(const Mat3x4 &a) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat3x4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd4f br0 = loadu_ps(a[0]);
    simd4f br1 = loadu_ps(a[1]);
    simd4f br2 = loadu_ps(a[2]);

    store_ps(ar0 + br0, dst[0]);
    store_ps(ar1 + br1, dst[1]);
    store_ps(ar2 + br2, dst[2]);
#else
    Mat3x4 dst;

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
#endif
    return dst;
}

Mat3x4 &Mat3x4::operator+=(const Mat3x4 &rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f ar01 = loadu_256ps(mat[0]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd8f br01 = loadu_256ps(rhs[0]);
    simd4f br2 = loadu_ps(rhs[2]);

    storeu_256ps(ar01 + br01, mat[0]);
    storeu_ps(ar2 + br2, mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd4f br0 = loadu_ps(rhs[0]);
    simd4f br1 = loadu_ps(rhs[1]);
    simd4f br2 = loadu_ps(rhs[2]);

    storeu_ps(ar0 + br0, mat[0]);
    storeu_ps(ar1 + br1, mat[1]);
    storeu_ps(ar2 + br2, mat[2]);
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
#endif
    return *this;
}

Mat3x4 Mat3x4::operator-(const Mat3x4 &a) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat3x4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd4f br0 = loadu_ps(a[0]);
    simd4f br1 = loadu_ps(a[1]);
    simd4f br2 = loadu_ps(a[2]);

    store_ps(ar0 - br0, dst[0]);
    store_ps(ar1 - br1, dst[1]);
    store_ps(ar2 - br2, dst[2]);
#else
    Mat3x4 dst;

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
#endif
    return dst;
}

Mat3x4 &Mat3x4::operator-=(const Mat3x4 &rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f ar01 = loadu_256ps(mat[0]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd8f br01 = loadu_256ps(rhs[0]);
    simd4f br2 = loadu_ps(rhs[2]);

    storeu_256ps(ar01 - br01, mat[0]);
    storeu_ps(ar2 - br2, mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd4f br0 = loadu_ps(rhs[0]);
    simd4f br1 = loadu_ps(rhs[1]);
    simd4f br2 = loadu_ps(rhs[2]);

    storeu_ps(ar0 - br0, mat[0]);
    storeu_ps(ar1 - br1, mat[1]);
    storeu_ps(ar2 - br2, mat[2]);
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
#endif
    return *this;
}

Mat3x4 Mat3x4::operator*(float rhs) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat3x4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd4f c = set1_ps(rhs);

    store_ps(ar0 * c, dst[0]);
    store_ps(ar1 * c, dst[1]);
    store_ps(ar2 * c, dst[2]);
#else
    Mat3x4 dst;

    dst[0][0] = mat[0][0] * rhs;
    dst[0][1] = mat[0][1] * rhs;
    dst[0][2] = mat[0][2] * rhs;
    dst[0][3] = mat[0][3] * rhs,

    dst[1][0] = mat[1][1] * rhs;
    dst[1][1] = mat[1][1] * rhs;
    dst[1][2] = mat[1][2] * rhs;
    dst[1][3] = mat[1][3] * rhs;

    dst[2][0] = mat[2][0] * rhs;
    dst[2][1] = mat[2][1] * rhs;
    dst[2][2] = mat[2][2] * rhs;
    dst[2][3] = mat[2][3] * rhs;
#endif
    return dst;
}

Mat3x4 &Mat3x4::operator*=(float rhs) {
#if defined(ENABLE_SIMD8_INTRIN)
    simd8f ar01 = loadu_256ps(mat[0]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd8f b = set1_256ps(rhs);

    storeu_256ps(ar01 * b, mat[0]);
    storeu_ps(ar2 * extract_256ps<0>(b), mat[2]);
#elif defined(ENABLE_SIMD4_INTRIN)
    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd4f b = set1_ps(rhs);

    storeu_ps(ar0 * b, mat[0]);
    storeu_ps(ar1 * b, mat[1]);
    storeu_ps(ar2 * b, mat[2]);
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
#endif
    return *this;
}

Vec4 Mat3x4::operator*(const Vec4 &vec) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Vec4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd4f v = loadu_ps(vec);

    simd4f x = ar0 * v;
    simd4f y = ar1 * v;
    simd4f z = ar2 * v;
    simd4f w = (v & simd4i(0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF));
    simd4f tmp1 = hadd_ps(x, y); // x0+x1, x2+x3, y0+y1, y2+y3
    simd4f tmp2 = hadd_ps(z, w); // z0+z1, z2+z3, w0+w1, w2+w3
    simd4f result = hadd_ps(tmp1, tmp2); // x0+x1+x2+x3, y0+y1+y2+y3, z0+z1+z2+z3, w0+w1+w2+w3

    store_ps(result, dst);
#else
    Vec4 dst;

    dst[0] = mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z + mat[0].w * vec.w;
    dst[1] = mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z + mat[1].w * vec.w;
    dst[2] = mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z + mat[2].w * vec.w;
    dst[3] = vec.w;
#endif
    return dst;
}

Vec3 Mat3x4::operator*(const Vec3 &vec) const {
    Vec3 dst;
    dst[0] = mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z + mat[0].w;
    dst[1] = mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z + mat[1].w;
    dst[2] = mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z + mat[2].w;
    return dst;
}

Vec4 Mat3x4::TransposedMulVec(const Vec4 &vec) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Vec4 dst;

    simd4f ac0 = load_ps(mat[0]);
    simd4f ac1 = load_ps(mat[1]);
    simd4f ac2 = load_ps(mat[2]);

    simd4f v = loadu_ps(vec);

    simd4f result = ac0 * shuffle_ps<0, 0, 0, 0>(v);
    result = madd_ps(ac1, shuffle_ps<1, 1, 1, 1>(v), result);
    result = madd_ps(ac2, shuffle_ps<2, 2, 2, 2>(v), result);
    result = simd4f(0, 0, 0, w_ps(v)) + result;

    store_ps(result, dst);
#else
    Vec4 dst;

    dst[0] = mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z;
    dst[1] = mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z;
    dst[2] = mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z;
    dst[3] = mat[0].w * vec.x + mat[1].w * vec.y + mat[2].w * vec.z + vec.w;
#endif
    return dst;
}

Vec3 Mat3x4::TransposedMulVec(const Vec3 &vec) const {
    Vec3 dst;
    dst[0] = mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z;
    dst[1] = mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z;
    dst[2] = mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z;
    return dst;
}

Mat3x4 Mat3x4::operator*(const Mat3x4 &a) const {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat3x4 dst;

    simd4f ar0 = loadu_ps(mat[0]);
    simd4f ar1 = loadu_ps(mat[1]);
    simd4f ar2 = loadu_ps(mat[2]);

    simd4f br0 = loadu_ps(a.mat[0]);
    simd4f br1 = loadu_ps(a.mat[1]);
    simd4f br2 = loadu_ps(a.mat[2]);

    store_ps(lincomb3x4(ar0, br0, br1, br2), dst.mat[0]);
    store_ps(lincomb3x4(ar1, br0, br1, br2), dst.mat[1]);
    store_ps(lincomb3x4(ar2, br0, br1, br2), dst.mat[2]);
#else
    Mat3x4 dst;

    dst.mat[0][0] = mat[0][0] * a.mat[0][0] + mat[0][1] * a.mat[1][0] + mat[0][2] * a.mat[2][0];
    dst.mat[0][1] = mat[0][0] * a.mat[0][1] + mat[0][1] * a.mat[1][1] + mat[0][2] * a.mat[2][1];
    dst.mat[0][2] = mat[0][0] * a.mat[0][2] + mat[0][1] * a.mat[1][2] + mat[0][2] * a.mat[2][2];
    dst.mat[0][3] = mat[0][0] * a.mat[0][3] + mat[0][1] * a.mat[1][3] + mat[0][2] * a.mat[2][3] + mat[0][3];

    dst.mat[1][0] = mat[1][0] * a.mat[0][0] + mat[1][1] * a.mat[1][0] + mat[1][2] * a.mat[2][0];
    dst.mat[1][1] = mat[1][0] * a.mat[0][1] + mat[1][1] * a.mat[1][1] + mat[1][2] * a.mat[2][1];
    dst.mat[1][2] = mat[1][0] * a.mat[0][2] + mat[1][1] * a.mat[1][2] + mat[1][2] * a.mat[2][2];
    dst.mat[1][3] = mat[1][0] * a.mat[0][3] + mat[1][1] * a.mat[1][3] + mat[1][2] * a.mat[2][3] + mat[1][3];

    dst.mat[2][0] = mat[2][0] * a.mat[0][0] + mat[2][1] * a.mat[1][0] + mat[2][2] * a.mat[2][0];
    dst.mat[2][1] = mat[2][0] * a.mat[0][1] + mat[2][1] * a.mat[1][1] + mat[2][2] * a.mat[2][1];
    dst.mat[2][2] = mat[2][0] * a.mat[0][2] + mat[2][1] * a.mat[1][2] + mat[2][2] * a.mat[2][2];
    dst.mat[2][3] = mat[2][0] * a.mat[0][3] + mat[2][1] * a.mat[1][3] + mat[2][2] * a.mat[2][3] + mat[2][3];
#endif
    return dst;
}

void Mat3x4::SetTRS(const Vec3 &translation, const Mat3 &rotation, const Vec3 &scale) {
    // T * R * S
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
}

void Mat3x4::SetTQS(const Vec3 &translation, const Quat &rotation, const Vec3 &scale) {
    SetTRS(translation, rotation.ToMat3(), scale);
}

void Mat3x4::GetTRS(Vec3 &translation, Mat3 &rotation, Vec3 &scale) const {
    translation.x = mat[0][3];
    translation.y = mat[1][3];
    translation.z = mat[2][3];

    rotation = ToMat3();

    scale.x = rotation[0].Length();
    scale.y = rotation[1].Length();
    scale.z = rotation[2].Length();
    
    rotation[0] *= 1.f / scale.x;
    rotation[1] *= 1.f / scale.y;
    rotation[2] *= 1.f / scale.z;
}

void Mat3x4::GetTQS(Vec3 &translation, Quat &rotation, Vec3 &scale) const {
    Mat3 r;
    GetTRS(translation, r, scale);
    rotation = r.ToQuat();
}

// Euclidean inverse (TRS)^{-1} = S^{-1} R^T (-T)
void Mat3x4::InverseSelf() {
#if 0
    *this = Mat3x4(ToMat4().Inverse());
#else
    float tmp[3];

    // get squared inverse scale factor
    float inv_sx2 = 1.0f / (mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0]);
    float inv_sy2 = 1.0f / (mat[0][1] * mat[0][1] + mat[1][1] * mat[1][1] + mat[2][1] * mat[2][1]);
    float inv_sz2 = 1.0f / (mat[0][2] * mat[0][2] + mat[1][2] * mat[1][2] + mat[2][2] * mat[2][2]);

    // negate inverse rotated translation part
    tmp[0] = mat[0][0] * mat[0][3] + mat[1][0] * mat[1][3] + mat[2][0] * mat[2][3];
    tmp[1] = mat[0][1] * mat[0][3] + mat[1][1] * mat[1][3] + mat[2][1] * mat[2][3];
    tmp[2] = mat[0][2] * mat[0][3] + mat[1][2] * mat[1][3] + mat[2][2] * mat[2][3];
    mat[0][3] = -tmp[0] * inv_sx2;
    mat[1][3] = -tmp[1] * inv_sy2;
    mat[2][3] = -tmp[2] * inv_sz2;

    // transpose rotation part
    mat[0][0] *= inv_sx2;
    tmp[0] = mat[0][1] * inv_sy2;
    mat[0][1] = mat[1][0] * inv_sx2;
    mat[1][0] = tmp[0];
    mat[1][1] *= inv_sy2;
    tmp[1] = mat[0][2] * inv_sz2;
    mat[0][2] = mat[2][0] * inv_sx2;
    mat[2][0] = tmp[1];
    tmp[2] = mat[1][2] * inv_sz2;
    mat[1][2] = mat[2][1] * inv_sy2;
    mat[2][1] = tmp[2];
    mat[2][2] *= inv_sz2;
#endif
}

Mat3x4 &Mat3x4::TransformSelf(const Mat3x4 &a) {
    float dst[3];

    dst[0] = mat[0][0] * a.mat[0][0] + mat[1][0] * a.mat[0][1] + mat[2][0] * a.mat[0][2];
    dst[1] = mat[0][0] * a.mat[1][0] + mat[1][0] * a.mat[1][1] + mat[2][0] * a.mat[1][2];
    dst[2] = mat[0][0] * a.mat[2][0] + mat[1][0] * a.mat[2][1] + mat[2][0] * a.mat[2][2];
    mat[0][0] = dst[0];
    mat[1][0] = dst[1];
    mat[2][0] = dst[2];

    dst[0] = mat[0][1] * a.mat[0][0] + mat[1][1] * a.mat[0][1] + mat[2][1] * a.mat[0][2];
    dst[1] = mat[0][1] * a.mat[1][0] + mat[1][1] * a.mat[1][1] + mat[2][1] * a.mat[1][2];
    dst[2] = mat[0][1] * a.mat[2][0] + mat[1][1] * a.mat[2][1] + mat[2][1] * a.mat[2][2];
    mat[0][1] = dst[0];
    mat[1][1] = dst[1];
    mat[2][1] = dst[2];

    dst[0] = mat[0][2] * a.mat[0][0] + mat[1][2] * a.mat[0][1] + mat[2][2] * a.mat[0][2];
    dst[1] = mat[0][2] * a.mat[1][0] + mat[1][2] * a.mat[1][1] + mat[2][2] * a.mat[1][2];
    dst[2] = mat[0][2] * a.mat[2][0] + mat[1][2] * a.mat[2][1] + mat[2][2] * a.mat[2][2];
    mat[0][2] = dst[0];
    mat[1][2] = dst[1];
    mat[2][2] = dst[2];

    dst[0] = mat[0][3] * a.mat[0][0] + mat[1][3] * a.mat[0][1] + mat[2][3] * a.mat[0][2];
    dst[1] = mat[0][3] * a.mat[1][0] + mat[1][3] * a.mat[1][1] + mat[2][3] * a.mat[1][2];
    dst[2] = mat[0][3] * a.mat[2][0] + mat[1][3] * a.mat[2][1] + mat[2][3] * a.mat[2][2];
    mat[0][3] = dst[0];
    mat[1][3] = dst[1];
    mat[2][3] = dst[2];

    mat[0][3] += a.mat[0][3];
    mat[1][3] += a.mat[1][3];
    mat[2][3] += a.mat[2][3];

    return *this;
}

Mat3x4 &Mat3x4::UntransformSelf(const Mat3x4 &a) {
    float dst[3];

    mat[0][3] -= a.mat[0][3];
    mat[1][3] -= a.mat[1][3];
    mat[2][3] -= a.mat[2][3];

    dst[0] = mat[0][0] * a.mat[0][0] + mat[1][0] * a.mat[1][0] + mat[2][0] * a.mat[2][0];
    dst[1] = mat[0][0] * a.mat[0][1] + mat[1][0] * a.mat[1][1] + mat[2][0] * a.mat[2][1];
    dst[2] = mat[0][0] * a.mat[0][2] + mat[1][0] * a.mat[1][2] + mat[2][0] * a.mat[2][2];
    mat[0][0] = dst[0];
    mat[1][0] = dst[1];
    mat[2][0] = dst[2];

    dst[0] = mat[0][1] * a.mat[0][0] + mat[1][1] * a.mat[1][0] + mat[2][1] * a.mat[2][0];
    dst[1] = mat[0][1] * a.mat[0][1] + mat[1][1] * a.mat[1][1] + mat[2][1] * a.mat[2][1];
    dst[2] = mat[0][1] * a.mat[0][2] + mat[1][1] * a.mat[1][2] + mat[2][1] * a.mat[2][2];
    mat[0][1] = dst[0];
    mat[1][1] = dst[1];
    mat[2][1] = dst[2];

    dst[0] = mat[0][2] * a.mat[0][0] + mat[1][2] * a.mat[1][0] + mat[2][2] * a.mat[2][0];
    dst[1] = mat[0][2] * a.mat[0][1] + mat[1][2] * a.mat[1][1] + mat[2][2] * a.mat[2][1];
    dst[2] = mat[0][2] * a.mat[0][2] + mat[1][2] * a.mat[1][2] + mat[2][2] * a.mat[2][2];
    mat[0][2] = dst[0];
    mat[1][2] = dst[1];
    mat[2][2] = dst[2];

    dst[0] = mat[0][3] * a.mat[0][0] + mat[1][3] * a.mat[1][0] + mat[2][3] * a.mat[2][0];
    dst[1] = mat[0][3] * a.mat[0][1] + mat[1][3] * a.mat[1][1] + mat[2][3] * a.mat[2][1];
    dst[2] = mat[0][3] * a.mat[0][2] + mat[1][3] * a.mat[1][2] + mat[2][3] * a.mat[2][2];
    mat[0][3] = dst[0];
    mat[1][3] = dst[1];
    mat[2][3] = dst[2];

    return *this;
}

Mat3x4 Mat3x4::FromString(const char *str) {
    Mat3x4 m;
    sscanf(str, "%f %f %f %f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[0].w, &m[1].x, &m[1].y, &m[1].z, &m[1].w, &m[2].x, &m[2].y, &m[2].z, &m[2].w);
    return m;
}

BE_NAMESPACE_END
