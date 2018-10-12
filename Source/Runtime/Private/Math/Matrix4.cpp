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

const Mat4 Mat4::zero(Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0));
const Mat4 Mat4::identity(Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0), Vec4(0, 0, 0, 1));

Mat4 &Mat4::operator=(const Mat3x4 &rhs) {
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

    return *this;
}

Mat4 Mat4::operator*(const Mat3x4 &a) const {
    Mat4 dst;

    dst[0][0] = mat[0][0] * a[0][0] + mat[0][1] * a[1][0] + mat[0][2] * a[2][0];
    dst[0][1] = mat[0][0] * a[0][1] + mat[0][1] * a[1][1] + mat[0][2] * a[2][1];
    dst[0][2] = mat[0][0] * a[0][2] + mat[0][1] * a[1][2] + mat[0][2] * a[2][2];
    dst[0][3] = mat[0][0] * a[0][3] + mat[0][1] * a[1][3] + mat[0][2] * a[2][3] + mat[0][3];

    dst[1][0] = mat[1][0] * a[0][0] + mat[1][1] * a[1][0] + mat[1][2] * a[2][0];
    dst[1][1] = mat[1][0] * a[0][1] + mat[1][1] * a[1][1] + mat[1][2] * a[2][1];
    dst[1][2] = mat[1][0] * a[0][2] + mat[1][1] * a[1][2] + mat[1][2] * a[2][2];
    dst[1][3] = mat[1][0] * a[0][3] + mat[1][1] * a[1][3] + mat[1][2] * a[2][3] + mat[1][3];

    dst[2][0] = mat[2][0] * a[0][0] + mat[2][1] * a[1][0] + mat[2][2] * a[2][0];
    dst[2][1] = mat[2][0] * a[0][1] + mat[2][1] * a[1][1] + mat[2][2] * a[2][1];
    dst[2][2] = mat[2][0] * a[0][2] + mat[2][1] * a[1][2] + mat[2][2] * a[2][2];
    dst[2][3] = mat[2][0] * a[0][3] + mat[2][1] * a[1][3] + mat[2][2] * a[2][3] + mat[2][3];

    dst[3][0] = mat[3][0] * a[0][0] + mat[3][1] * a[1][0] + mat[3][2] * a[2][0];
    dst[3][1] = mat[3][0] * a[0][1] + mat[3][1] * a[1][1] + mat[3][2] * a[2][1];
    dst[3][2] = mat[3][0] * a[0][2] + mat[3][1] * a[1][2] + mat[3][2] * a[2][2];
    dst[3][3] = mat[3][0] * a[0][3] + mat[3][1] * a[1][3] + mat[3][2] * a[2][3] + mat[3][3];

    return dst;
}

Mat4 Mat4::TransposedMul(const Mat3x4 &a) const {
    Mat4 dst;

    dst[0][0] = mat[0][0] * a[0][0] + mat[0][1] * a[0][1] + mat[0][2] * a[0][2] + mat[0][3] * a[0][3];
    dst[0][1] = mat[0][0] * a[1][0] + mat[0][1] * a[1][1] + mat[0][2] * a[1][2] + mat[1][3] * a[1][3];
    dst[0][2] = mat[0][0] * a[2][0] + mat[0][1] * a[2][1] + mat[0][2] * a[2][2] + mat[2][3] * a[2][3];
    dst[0][3] = mat[0][3];

    dst[1][0] = mat[1][0] * a[0][0] + mat[1][1] * a[0][1] + mat[1][2] * a[0][2] + mat[1][3] * a[0][3];
    dst[1][1] = mat[1][0] * a[1][0] + mat[1][1] * a[1][1] + mat[1][2] * a[1][2] + mat[1][3] * a[1][3];
    dst[1][2] = mat[1][0] * a[2][0] + mat[1][1] * a[2][1] + mat[1][2] * a[2][2] + mat[1][3] * a[2][3];
    dst[1][3] = mat[1][3];

    dst[2][0] = mat[2][0] * a[0][0] + mat[2][1] * a[0][1] + mat[2][2] * a[0][2] + mat[2][3] * a[0][3];
    dst[2][1] = mat[2][0] * a[1][0] + mat[2][1] * a[1][1] + mat[2][2] * a[1][2] + mat[2][3] * a[1][3];
    dst[2][2] = mat[2][0] * a[2][0] + mat[2][1] * a[2][1] + mat[2][2] * a[2][2] + mat[2][3] * a[2][3];
    dst[2][3] = mat[2][3];

    dst[3][0] = mat[3][0] * a[0][0] + mat[3][1] * a[0][1] + mat[3][2] * a[0][2] + mat[3][3] * a[0][3];
    dst[3][1] = mat[3][0] * a[1][0] + mat[3][1] * a[1][1] + mat[3][2] * a[1][2] + mat[3][3] * a[1][3];
    dst[3][2] = mat[3][0] * a[2][0] + mat[3][1] * a[2][1] + mat[3][2] * a[2][2] + mat[3][3] * a[2][3];
    dst[3][3] = mat[3][3];
    
    return dst;
}

Mat4 Mat4::Transpose() const {
    Mat4 transpose;
    
    for (int i = 0; i < 4; i++ ) {
        for (int j = 0; j < 4; j++ ) {
            transpose[i][j] = mat[j][i];
        }
    }
    return transpose;
}

Mat4 &Mat4::TransposeSelf() {
    float temp;
    
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            temp = mat[i][j];
            mat[i][j] = mat[j][i];
            mat[j][i] = temp;
        }
    }
    return *this;
}

float Mat4::Determinant() const {
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

    return (- det3_201_123 * mat[3][0] + det3_201_023 * mat[3][1] - det3_201_013 * mat[3][2] + det3_201_012 * mat[3][3]);
}

bool Mat4::InverseSelf() {
#if 1
    // 84+4+16 = 104 multiplications
    //             1 division
    double det, invDet;

    // 2x2 sub-determinants required to calculate 4x4 determinant
    float det2_01_01 = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
    float det2_01_02 = mat[0][0] * mat[1][2] - mat[0][2] * mat[1][0];
    float det2_01_03 = mat[0][0] * mat[1][3] - mat[0][3] * mat[1][0];
    float det2_01_12 = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
    float det2_01_13 = mat[0][1] * mat[1][3] - mat[0][3] * mat[1][1];
    float det2_01_23 = mat[0][2] * mat[1][3] - mat[0][3] * mat[1][2];

    // 3x3 sub-determinants required to calculate 4x4 determinant
    float det3_201_012 = mat[2][0] * det2_01_12 - mat[2][1] * det2_01_02 + mat[2][2] * det2_01_01;
    float det3_201_013 = mat[2][0] * det2_01_13 - mat[2][1] * det2_01_03 + mat[2][3] * det2_01_01;
    float det3_201_023 = mat[2][0] * det2_01_23 - mat[2][2] * det2_01_03 + mat[2][3] * det2_01_02;
    float det3_201_123 = mat[2][1] * det2_01_23 - mat[2][2] * det2_01_13 + mat[2][3] * det2_01_12;

    det = (- det3_201_123 * mat[3][0] + det3_201_023 * mat[3][1] - det3_201_013 * mat[3][2] + det3_201_012 * mat[3][3]);

    if (Math::Fabs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    invDet = 1.0f / det;

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

    mat[0][0] = - det3_213_123 * invDet;
    mat[1][0] = + det3_213_023 * invDet;
    mat[2][0] = - det3_213_013 * invDet;
    mat[3][0] = + det3_213_012 * invDet;

    mat[0][1] = + det3_203_123 * invDet;
    mat[1][1] = - det3_203_023 * invDet;
    mat[2][1] = + det3_203_013 * invDet;
    mat[3][1] = - det3_203_012 * invDet;

    mat[0][2] = + det3_301_123 * invDet;
    mat[1][2] = - det3_301_023 * invDet;
    mat[2][2] = + det3_301_013 * invDet;
    mat[3][2] = - det3_301_012 * invDet;

    mat[0][3] = - det3_201_123 * invDet;
    mat[1][3] = + det3_201_023 * invDet;
    mat[2][3] = - det3_201_013 * invDet;
    mat[3][3] = + det3_201_012 * invDet;

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

    if (Math::Fabs(det) < MATRIX_INVERSE_EPSILON) {
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

    if (Math::Fabs(det) < MATRIX_INVERSE_EPSILON) {
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
}

bool Mat4::AffineInverseSelf() {
    Mat4    invMat;
    Vec3    t;

    // The bottom row vector of the matrix should always be [ 0 0 0 1 ]
    if (mat[3][0] != 0.0f || mat[3][1] != 0.0f || mat[3][2] != 0.0f || mat[3][3] != 1.0f) {
        return false;
    }

    // The translation components of the original matrix
    t.x = mat[0][3];
    t.y = mat[1][3];
    t.z = mat[2][3];

    // The rotational part of the matrix should be inverted
    Mat3 r = ToMat3();
    r.InverseSelf();
    mat[0][0] = r.mat[0][0];
    mat[1][0] = r.mat[0][1];
    mat[2][0] = r.mat[0][2];

    mat[0][1] = r.mat[1][0];
    mat[1][1] = r.mat[1][1];
    mat[2][1] = r.mat[1][2];

    mat[0][2] = r.mat[2][0];
    mat[1][2] = r.mat[2][1];
    mat[2][2] = r.mat[2][2];

    // -(Rt * T)
    mat[0][3] = -(mat[0].x * t.x + mat[0].y * t.y + mat[0].z * t.z);
    mat[1][3] = -(mat[1].x * t.x + mat[1].y * t.y + mat[1].z * t.z);
    mat[2][3] = -(mat[2].x * t.x + mat[2].y * t.y + mat[2].z * t.z);

    return true;
}

bool Mat4::EuclideanInverseSelf() {
    Mat4    invMat;
    Vec3    t;
    float   temp;
    
    // The bottom row vector of the matrix should always be [ 0 0 0 1 ]
    if (mat[3][0] != 0.0f || mat[3][1] != 0.0f || mat[3][2] != 0.0f || mat[3][3] != 1.0f) {
        return false;
    }

    // The translation components of the original matrix
    t.x = mat[0][3];
    t.y = mat[1][3];
    t.z = mat[2][3];

    // The rotational part of the matrix is simply the transpose of the original matrix
    temp = mat[0][1];
    mat[0][1] = mat[1][0];
    mat[1][0] = temp;
    temp = mat[0][2];
    mat[0][2] = mat[2][0];
    mat[2][0] = temp;
    temp = mat[1][2];
    mat[1][2] = mat[2][1];
    mat[2][1] = temp;

    // -(Rt * T)
    mat[0][3] = -(mat[0].x * t.x + mat[0].y * t.y + mat[0].z * t.z);
    mat[1][3] = -(mat[1].x * t.x + mat[1].y * t.y + mat[1].z * t.z);
    mat[2][3] = -(mat[2].x * t.x + mat[2].y * t.y + mat[2].z * t.z);

    return true;
}

// Doolittle Algorithm
// Doolittle uses unit diagonals for the lower triangle
bool Mat4::DecompLU() {
    float sum;

    for (int i = 0; i < Rows; i++) {
        // Compute a line of U
        for (int j = i; j < Cols; j++) {
            sum = 0;
            for (int k = 0; k < i; k++) {
                sum += mat[i][k] * mat[k][j];
            }
            mat[i][j] = mat[i][j] - sum; // not dividing by diagonals
        }
        // Compute a line of L
        for (int j = 0; j < i; j++) {
            sum = 0;
            for (int k = 0; k < j; k++) {
                sum += mat[i][k] * mat[k][j];
            }
            mat[i][j] = (mat[i][j] - sum) / mat[j][j];
        }        
    }
    return true;
}

Vec4 Mat4::SolveLU(const Vec4 &b) const {
    Vec4 x, y;
    float sum;

    // Solve Ly = b
    for (int i = 0; i < Rows; i++) {
        sum = 0;
        for (int j = 0; j < i; j++) {
            sum += mat[i][j] * y[j];
        }
        y[i] = b[i] - sum; // not dividing by diagonals
    }
    // Solve Ux = y
    for (int i = Rows - 1; i >= 0; i--) {
        sum = 0;
        for (int j = i; j < Cols; j++) {
            sum += mat[i][j] * x[j];
        }
        x[i] = (y[i] - sum) / mat[i][i];
    }
    return x;
}

//---------------------------------------------------
//
//        |  1  0  0  tx | | m00  m01  m02  m03 |
// T M  = |  0  1  0  ty | | m10  m11  m12  m13 |
//        |  0  0  1  tz | | m20  m21  m22  m23 |
//        |  0  0  0   1 | | m30  m31  m32  m33 |
//
//---------------------------------------------------

Mat4 &Mat4::Translate(float tx, float ty, float tz) {
    mat[0][3] += mat[3][3] * tx;
    mat[1][3] += mat[3][3] * ty;
    mat[2][3] += mat[3][3] * tz;

    return *this;
}

//---------------------------------------------------
//
//       | m00  m01  m02  m03 | |  1  0  0  tx |
// M T = | m10  m11  m12  m13 | |  0  1  0  ty |
//       | m20  m21  m22  m23 | |  0  0  1  tz |
//       | m30  m31  m32  m33 | |  0  0  0   1 |
//
//---------------------------------------------------

Mat4 &Mat4::TranslateRight(float tx, float ty, float tz) {
    mat[0][3] += mat[0][0] * tx + mat[0][1] * ty + mat[0][2] * tz;
    mat[1][3] += mat[1][0] * tx + mat[1][1] * ty + mat[1][2] * tz;
    mat[2][3] += mat[2][0] * tx + mat[2][1] * ty + mat[2][2] * tz;
    mat[3][3] += mat[3][0] * tx + mat[3][1] * ty + mat[3][2] * tz;

    return *this;
}

//---------------------------------------------------
//
//       | sx   0   0  0 | | m00  m01  m02  m03 |
// S M = |  0  sy   0  0 | | m10  m11  m12  m13 |
//       |  0   0  sz  0 | | m20  m21  m22  m23 |
//       |  0   0   0  1 | | m30  m31  m32  m33 |
//
//---------------------------------------------------

Mat4 &Mat4::Scale(float sx, float sy, float sz) {
    mat[0] *= sx;
    mat[1] *= sy;
    mat[2] *= sz;

    return *this;
}

void Mat4::SetFrustum(float left, float right, float bottom, float top, float znear, float zfar) {
    float nudge = 0.999f;   // prevent artifacts with infinite far plane
    
    // check for division by 0
    if (left == right || top == bottom || znear == zfar) {
        return;
    }

    mat[0][0] = (2 * znear) / (right - left);
    mat[0][1] = 0.f;
    mat[0][2] = (right + left) / (right - left);
    mat[0][3] = 0.f;
        
    mat[1][0] = 0.f;
    mat[1][1] = (2 * znear) / (top - bottom);
    mat[1][2] = (top + bottom) / (top - bottom);
    mat[1][3] = 0.f;
    
    mat[2][0] = 0.f;
    mat[2][1] = 0.f;
    if (zfar != -1) {
        mat[2][2] = -(zfar + znear) / (zfar - znear);
    } else { // if zfar == -1, use an infinite far plane
        mat[2][2] = -nudge;
    }

    if (zfar != -1) {
        mat[2][3] = -(2 * zfar * znear) / (zfar - znear);
    } else { // if zfar == -1, use an infinite far plane
        mat[2][3] = -2 * znear * nudge;
    }
    
    mat[3][0] = 0.f;
    mat[3][1] = 0.f;
    mat[3][2] = -1;
    mat[3][3] = 0.f;
}

void Mat4::SetPerspective(float fovy, float aspect, float znear, float zfar) {
    float top       = znear * Math::Tan(DEG2RAD(fovy / (aspect * 2)));
    float bottom    = -top;
    float left      = bottom * aspect;
    float right     = top * aspect;
    
    SetFrustum(left, right, bottom, top, znear, zfar);
}

void Mat4::SetOrtho(float left, float right, float bottom, float top, float znear, float zfar) {
    mat[0][0] = 2.f / (right-left);
    mat[0][1] = 0.f;
    mat[0][2] = 0.f;
    mat[0][3] = -(right+left) / (right-left);

    mat[1][0] = 0.f;
    mat[1][1] = 2.f / (top-bottom);
    mat[1][2] = 0.f;
    mat[1][3] = -(top+bottom) / (top-bottom);

    mat[2][0] = 0.f;
    mat[2][1] = 0.f;
    mat[2][2] = -2.f / (zfar-znear);
    mat[2][3] = -(zfar+znear) / (zfar-znear);

    mat[3][0] = 0.f;
    mat[3][1] = 0.f;
    mat[3][2] = 0.f;
    mat[3][3] = 1.f;
}

//--------------------------------------------------------------------------------------------
//
// * 한점을 주어진 평면으로 반사(reflection)시키는 행렬을 만든다
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
    Vec3 normal = plane.Normal();
    float dist = plane.Dist();

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
}

void Mat4::SetTQS(const Vec3 &translation, const Quat &rotation, const Vec3 &scale) {
    SetTRS(translation, rotation.ToMat3(), scale);
}

void Mat4::GetTRS(Vec3 &translation, Mat3 &rotation, Vec3 &scale) const {
    ToMat3x4().GetTRS(translation, rotation, scale);
}

void Mat4::GetTQS(Vec3 &translation, Quat &rotation, Vec3 &scale) const {
    Mat3 r;
    GetTRS(translation, r, scale);
    rotation = r.ToQuat();
}

Mat4 Mat4::FromString(const char *str) {
    Mat4 m;
    sscanf(str, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[0].w, &m[1].x, &m[1].y, &m[1].z, &m[1].w, &m[2].x, &m[2].y, &m[2].z, &m[2].w, &m[3].x, &m[3].y, &m[3].z, &m[3].w);
    return m;
}

BE_NAMESPACE_END
