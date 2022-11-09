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

ALIGN_AS16 const Mat3 Mat3::zero(Vec3(0, 0, 0), Vec3(0, 0, 0), Vec3(0, 0, 0));
ALIGN_AS16 const Mat3 Mat3::identity(Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1));

float Mat3::Determinant() const {
    float det2_12_01 = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];
    float det2_12_02 = mat[1][0] * mat[2][2] - mat[1][2] * mat[2][0];
    float det2_12_12 = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];

    return mat[0][0] * det2_12_12 - mat[0][1] * det2_12_02 + mat[0][2] * det2_12_01;
}

bool Mat3::InverseSelf() {
#if 1
    // 18+3+9 = 30 multiplications
    //          1 division
    Mat3 inverse;
    double det, invDet;

    inverse[0][0] = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];
    inverse[1][0] = mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2];
    inverse[2][0] = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];

    det = mat[0][0] * inverse[0][0] + mat[0][1] * inverse[1][0] + mat[0][2] * inverse[2][0];

    if (Math::Fabs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    invDet = 1.0f / det;

    inverse[0][1] = mat[0][2] * mat[2][1] - mat[0][1] * mat[2][2];
    inverse[0][2] = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
    inverse[1][1] = mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0];
    inverse[1][2] = mat[0][2] * mat[1][0] - mat[0][0] * mat[1][2];
    inverse[2][1] = mat[0][1] * mat[2][0] - mat[0][0] * mat[2][1];
    inverse[2][2] = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

    mat[0][0] = inverse[0][0] * invDet;
    mat[0][1] = inverse[0][1] * invDet;
    mat[0][2] = inverse[0][2] * invDet;

    mat[1][0] = inverse[1][0] * invDet;
    mat[1][1] = inverse[1][1] * invDet;
    mat[1][2] = inverse[1][2] * invDet;

    mat[2][0] = inverse[2][0] * invDet;
    mat[2][1] = inverse[2][1] * invDet;
    mat[2][2] = inverse[2][2] * invDet;

    return true;
#elif 0
    // 3*10 = 30 multiplications
    //        3 divisions
    float *mat = reinterpret_cast<float *>(this);
    float s;
    double d, di;

    di = mat[0];
    s = di;
    mat[0] = d = 1.0f / di;
    mat[1] *= d;
    mat[2] *= d;
    d = -d;
    mat[3] *= d;
    mat[6] *= d;
    d = mat[3] * di;
    mat[4] += mat[1] * d;
    mat[5] += mat[2] * d;
    d = mat[6] * di;
    mat[7] += mat[1] * d;
    mat[8] += mat[2] * d;
    di = mat[4];
    s *= di;
    mat[4] = d = 1.0f / di;
    mat[3] *= d;
    mat[5] *= d;
    d = -d;
    mat[1] *= d;
    mat[7] *= d;
    d = mat[1] * di;
    mat[0] += mat[3] * d;
    mat[2] += mat[5] * d;
    d = mat[7] * di;
    mat[6] += mat[3] * d;
    mat[8] += mat[5] * d;
    di = mat[8];
    s *= di;
    mat[8] = d = 1.0f / di;
    mat[6] *= d;
    mat[7] *= d;
    d = -d;
    mat[2] *= d;
    mat[5] *= d;
    d = mat[2] * di;
    mat[0] += mat[6] * d;
    mat[1] += mat[7] * d;
    d = mat[5] * di;
    mat[3] += mat[6] * d;
    mat[4] += mat[7] * d;

    return (s != 0.0f && !FLOAT_IS_NAN(s));
#else
    // 4*2+4*4 = 24 multiplications
    //     2*1 =  2 divisions
    Mat2 r0;
    double r1[2], r2[2], r3;
    double det, invDet;
    float *mat = reinterpret_cast<float *>(this);

    // r0 = m0.Inverse();   // 2x2
    det = mat[0*3+0] * mat[1*3+1] - mat[0*3+1] * mat[1*3+0];

    if (Math::Fabs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    invDet = 1.0f / det;

    r0[0][0] =  mat[1*3+1] * invDet;
    r0[0][1] = -mat[0*3+1] * invDet;
    r0[1][0] = -mat[1*3+0] * invDet;
    r0[1][1] =  mat[0*3+0] * invDet;

    // r1 = r0 * m1;        // 2x1 = 2x2 * 2x1
    r1[0] = r0[0][0] * mat[0*3+2] + r0[0][1] * mat[1*3+2];
    r1[1] = r0[1][0] * mat[0*3+2] + r0[1][1] * mat[1*3+2];

    // r2 = m2 * r1;        // 1x1 = 1x2 * 2x1
    r2[0] = mat[2*3+0] * r1[0] + mat[2*3+1] * r1[1];

    // r3 = r2 - m3;        // 1x1 = 1x1 - 1x1
    r3 = r2[0] - mat[2*3+2];

    // r3.InverseSelf();
    if (Math::Fabs(r3) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    r3 = 1.0f / r3;

    // r2 = m2 * r0;        // 1x2 = 1x2 * 2x2
    r2[0] = mat[2*3+0] * r0[0][0] + mat[2*3+1] * r0[1][0];
    r2[1] = mat[2*3+0] * r0[0][1] + mat[2*3+1] * r0[1][1];

    // m2 = r3 * r2;        // 1x2 = 1x1 * 1x2
    mat[2*3+0] = r3 * r2[0];
    mat[2*3+1] = r3 * r2[1];

    // m0 = r0 - r1 * m2;   // 2x2 - 2x1 * 1x2
    mat[0*3+0] = r0[0][0] - r1[0] * mat[2*3+0];
    mat[0*3+1] = r0[0][1] - r1[0] * mat[2*3+1];
    mat[1*3+0] = r0[1][0] - r1[1] * mat[2*3+0];
    mat[1*3+1] = r0[1][1] - r1[1] * mat[2*3+1];

    // m1 = r1 * r3;        // 2x1 = 2x1 * 1x1
    mat[0*3+2] = r1[0] * r3;
    mat[1*3+2] = r1[1] * r3;

    // m3 = -r3;
    mat[2*3+2] = -r3;

    return true;
#endif
}

//------------------------------------------------
//  
//      | 1     0      0 |
// Rx = | 0  cosx  -sinx |
//      | 0  sinx   cosx |
//
//------------------------------------------------
Mat3 Mat3::FromRotationX(float ex) {
    float s, c;
    Math::SinCos(ex, s, c);

    Mat3 m;
    m[0][0] = 1.0f;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;

    m[1][0] = 0.0f;
    m[1][1] = c;
    m[1][2] = s;

    m[2][0] = 0.0f;
    m[2][1] = -s;
    m[2][2] = c;

    return m;
}

//------------------------------------------------
//  
//      |  cosy  0  siny |
// Ry = |    0   1     0 |
//      | -siny  0  cosy |
//
//------------------------------------------------
Mat3 Mat3::FromRotationY(float ey) {
    float s, c;
    Math::SinCos(ey, s, c);

    Mat3 m;
    m[0][0] = c;
    m[0][1] = 0.0f;
    m[0][2] = -s;

    m[1][0] = 0.0f;
    m[1][1] = 1.0f;
    m[1][2] = 0.0f;

    m[2][0] = s;
    m[2][1] = 0.0f;
    m[2][2] = c;

    return m;
}

//------------------------------------------------
//
//      | cosz  -sinz  0 |
// Rz = | sinz   cosz  0 |
//      |    0      0  1 |
//
//------------------------------------------------
Mat3 Mat3::FromRotationZ(float ez) {
    float s, c;
    Math::SinCos(ez, s, c);

    Mat3 m;
    m[0][0] = c;
    m[0][1] = s;
    m[0][2] = 0.0f;

    m[1][0] = -s;
    m[1][1] = c;
    m[1][2] = 0.0f;

    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = 1.0f;

    return m;
}

//----------------------------------------------------------------------------
//
//            | 1     0      0 | |  cosy  0   siny | | cosz  -sinz  0 |
// Rx Ry Rz = | 0  cosx  -sinx | |     0  1      0 | | sinz   cosz  0 |
//            | 0  sinx   cosx | | -siny  0   cosy | |    0      0  1 |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationXYZ(float ex, float ey, float ez) {
    float cx, sx, cy, sy, cz, sz;

    Math::SinCos(ex, sx, cx);
    Math::SinCos(ey, sy, cy);
    Math::SinCos(ez, sz, cz);

    float cxcz = cx * cz;
    float cxsz = cx * sz;
    float sxcz = sx * cz;
    float sxsz = sx * sz;

    Mat3 m;
    m[0][0] = cy * cz;
    m[0][1] = sxcz * sy + cxsz;
    m[0][2] = -cxcz * sy + sxsz;

    m[1][0] = -cy * sz;
    m[1][1] = cxcz - sxsz * sy;
    m[1][2] = sxcz + cxsz * sy;

    m[2][0] = sy;
    m[2][1] = -cy * sx;
    m[2][2] = cx * cy;

    return m;
}

//----------------------------------------------------------------------------
//
//            | 1     0      0 | | cosz  -sinz  0 | |  cosy  0   siny |
// Rx Rz Ry = | 0  cosx  -sinx | | sinz   cosz  0 | |     0  1      0 |
//            | 0  sinx   cosx | |    0      0  1 | | -siny  0   cosy |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationXZY(float ex, float ez, float ey) {
    float cx, sx, cy, sy, cz, sz;

    Math::SinCos(ex, sx, cx);
    Math::SinCos(ey, sy, cy);
    Math::SinCos(ez, sz, cz);

    float cxcy = cx * cy;
    float cxsy = cx * sy;
    float sxcy = sx * cy;
    float sxsy = sx * sy;

    Mat3 m;
    m[0][0] = cy * cz;
    m[0][1] = sxsy + cxcy * sz;
    m[0][2] = -cxsy + sxcy * sz;

    m[1][0] = -sz;
    m[1][1] = cx * cz;
    m[1][2] = cz * sx;

    m[2][0] = cz * sy;
    m[2][1] = -sxcy + cxsy * sz;
    m[2][2] = cxcy + sxsy * sz;

    return m;
}

//----------------------------------------------------------------------------
//
//            |  cosy  0   siny | | 1     0      0 | | cosz  -sinz  0 |
// Ry Rx Rz = |     0  1      0 | | 0  cosx  -sinx | | sinz   cosz  0 |
//            | -siny  0   cosy | | 0  sinx   cosx | |    0      0  1 |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationYXZ(float ey, float ex, float ez) {
    float cx, sx, cy, sy, cz, sz;

    Math::SinCos(ex, sx, cx);
    Math::SinCos(ey, sy, cy);
    Math::SinCos(ez, sz, cz);

    float cycz = cy * cz;
    float cysz = cy * sz;
    float sycz = sy * cz;
    float sysz = sy * sz;

    Mat3 m;
    m[0][0] = cycz + sx * sysz;
    m[0][1] = cx * sz;
    m[0][2] = -sycz + cysz * sx;

    m[1][0] = sycz * sx - cysz;
    m[1][1] = cx * cz;
    m[1][2] = cycz * sx + sysz;

    m[2][0] = cx * sy;
    m[2][1] = -sx;
    m[2][2] = cx * cy;

    return m;
}

//----------------------------------------------------------------------------
//
//            |  cosy  0   siny | | cosz  -sinz  0 | | 1     0      0 | 
// Ry Rz Rx = |     0  1      0 | | sinz   cosz  0 | | 0  cosx  -sinx | 
//            | -siny  0   cosy | |    0      0  1 | | 0  sinx   cosx | 
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationYZX(float ey, float ez, float ex) {
    float cx, sx, cy, sy, cz, sz;

    Math::SinCos(ex, sx, cx);
    Math::SinCos(ey, sy, cy);
    Math::SinCos(ez, sz, cz);

    float cxcy = cx * cy;
    float cxsy = cx * sy;
    float sxcy = sx * cy;
    float sxsy = sx * sy;

    Mat3 m;
    m[0][0] = cy * cz;
    m[0][1] = sz;
    m[0][2] = -cxsy;

    m[1][0] = sxsy - cxcy * sz;
    m[1][1] = cx * cz;
    m[1][2] = sxcy + cxsy * sz;

    m[2][0] = cxsy + sxcy * sz;
    m[2][1] = -cz * sx;
    m[2][2] = cxcy - sxsy * sz;

    return m;
}

//----------------------------------------------------------------------------
//
//            | cosz  -sinz  0 | | 1     0      0 | |  cosy  0   siny | 
// Rz Rx Ry = | sinz   cosz  0 | | 0  cosx  -sinx | |     0  1      0 | 
//            |    0      0  1 | | 0  sinx   cosx | | -siny  0   cosy | 
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationZXY(float ez, float ex, float ey) {
    float cx, sx, cy, sy, cz, sz;

    Math::SinCos(ex, sx, cx);
    Math::SinCos(ey, sy, cy);
    Math::SinCos(ez, sz, cz);

    float cycz = cy * cz;
    float cysz = cy * sz;
    float sycz = sy * cz;
    float sysz = sy * sz;

    Mat3 m;
    m[0][0] = cycz - sx * sysz;
    m[0][1] = sycz * sx + cysz;
    m[0][2] = -cx * sy;

    m[1][0] = -cx * sz;
    m[1][1] = cx * cz;
    m[1][2] = sx;

    m[2][0] = sycz + cysz * sx;
    m[2][1] = -cycz * sx + sysz;
    m[2][2] = cx * cy;

    return m;
}

//----------------------------------------------------------------------------
//
//            | cosz  -sinz  0 | |  cosy  0   siny | | 1     0      0 |
// Rz Ry Rx = | sinz   cosz  0 | |     0  1      0 | | 0  cosx  -sinx |
//            |    0      0  1 | | -siny  0   cosy | | 0  sinx   cosx |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationZYX(float ez, float ey, float ex) {
    float cx, sx, cy, sy, cz, sz;

    Math::SinCos(ex, sx, cx);
    Math::SinCos(ey, sy, cy);
    Math::SinCos(ez, sz, cz);

    float cxcz = cx * cz;
    float cxsz = cx * sz;
    float sxcz = sx * cz;
    float sxsz = sx * sz;

    Mat3 m;
    m[0][0] = cy * cz;
    m[0][1] = cy * sz;
    m[0][2] = -sy;

    m[1][0] = sxcz * sy - cxsz;
    m[1][1] = sxsz * sy + cxcz;
    m[1][2] = sx * cy;

    m[2][0] = cxcz * sy + sxsz;
    m[2][1] = cxsz * sy - sxcz;
    m[2][2] = cx * cy;

    return m;
}

//----------------------------------------------------------------------------
//
//              | 1      0       0 | |  cosy  0   siny | | 1      0       0 |
// Rx2 Ry Rx1 = | 0  cosx2  -sinx2 | |     0  1      0 | | 0  cosx1  -sinx1 |
//              | 0  sinx2   cosx2 | | -siny  0   cosy | | 0  sinx1   cosx1 |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationXYX(float ex2, float ey, float ex1) {
    float cx2, sx2, cy, sy, cx1, sx1;

    Math::SinCos(ex2, sx2, cx2);
    Math::SinCos(ey, sy, cy);
    Math::SinCos(ex1, sx1, cx1);

    float cx1cx2 = cx1 * cx2;
    float cx1sx2 = cx1 * sx2;
    float sx1cx2 = sx1 * cx2;
    float sx1sx2 = sx1 * sx2;

    Mat3 m;
    m[0][0] = cy; 
    m[0][1] = sy * sx2;
    m[0][2] = -sy * cx2;

    m[1][0] = sy * sx1;
    m[1][1] = cx1cx2 - cy * sx1sx2;
    m[1][2] = cx1sx2 + cy * sx1cx2;

    m[2][0] = sy * cx1;
    m[2][1] = -cy * cx1sx2 - sx1cx2;
    m[2][2] = cy * cx1cx2 - sx1sx2;

    return m;
}

//----------------------------------------------------------------------------
//
//              | 1      0       0 | | cosz  -sinz  0 | | 1      0       0 |
// Rx2 Rz Rx1 = | 0  cosx2  -sinx2 | | sinz   cosz  0 | | 0  cosx1  -sinx1 |
//              | 0  sinx2   cosx2 | |    0      0  1 | | 0  sinx1   cosx1 |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationXZX(float ex2, float ez, float ex1) {
    float cx2, sx2, cz, sz, cx1, sx1;

    Math::SinCos(ex2, sx2, cx2);
    Math::SinCos(ez, sz, cz);
    Math::SinCos(ex1, sx1, cx1);

    float cx1cx2 = cx1 * cx2;
    float cx1sx2 = cx1 * sx2;
    float sx1cx2 = sx1 * cx2;
    float sx1sx2 = sx1 * sx2;

    Mat3 m;
    m[0][0] = cz;
    m[0][1] = sz * cx2;
    m[0][2] = sz * sx2;

    m[1][0] = -sz * cx1;
    m[1][1] = cz * cx1cx2 - sx1sx2;
    m[1][2] = cz * cx1sx2 + sx1cx2;

    m[2][0] = sz * sx1;
    m[2][1] = -cx1sx2 - cz * sx1cx2;
    m[2][2] = cx1cx2 - cz * sx1sx2;

    return m;
}

//----------------------------------------------------------------------------
//
//              |  cosy2  0   siny2 | | 1     0      0 | |  cosy1  0   siny1 |
// Ry2 Rx Ry1 = |      0  1       0 | | 0  cosx  -sinx | |      0  1       0 |
//              | -siny2  0   cosy2 | | 0  sinx   cosx | | -siny1  0   cosy1 |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationYXY(float ey2, float ex, float ey1) {
    float cy2, sy2, cx, sx, cy1, sy1;

    Math::SinCos(ey2, sy2, cy2);
    Math::SinCos(ex, sx, cx);
    Math::SinCos(ey1, sy1, cy1);

    float cy1cy2 = cy1 * cy2;
    float cy1sy2 = cy1 * sy2;
    float sy1cy2 = sy1 * cy2;
    float sy1sy2 = sy1 * sy2;

    Mat3 m;
    m[0][0] = cy1cy2 - cx * sy1sy2;
    m[0][1] = sx * sy1;
    m[0][2] = -cy1sy2 - cx * sy1cy2;

    m[1][0] = sx * sy2;
    m[1][1] = cx;
    m[1][2] = sx * cy2;

    m[2][0] = cx * cy1sy2 + sy1cy2;
    m[2][1] = -sx * cy1;
    m[2][2] = cx * cy1cy2 - sy1sy2;

    return m;
}

//----------------------------------------------------------------------------
//
//              |  cosy2  0   siny2 | | cosz  -sinz  0 | |  cosy1  0   siny1 |
// Ry2 Rz Ry1 = |      0  1       0 | | sinz   cosz  0 | |      0  1       0 |
//              | -siny2  0   cosy2 | |    0      0  1 | | -siny1  0   cosy1 |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationYZY(float ey2, float ez, float ey1) {
    float cy2, sy2, cz, sz, cy1, sy1;

    Math::SinCos(ey2, sy2, cy2);
    Math::SinCos(ez, sz, cz);
    Math::SinCos(ey1, sy1, cy1);

    float cy1cy2 = cy1 * cy2;
    float cy1sy2 = cy1 * sy2;
    float sy1cy2 = sy1 * cy2;
    float sy1sy2 = sy1 * sy2;

    Mat3 m;
    m[0][0] = cz * cy1cy2 - sy1sy2;
    m[0][1] = sz * cy1;
    m[0][2] = -cz * cy1sy2 - sy1cy2;

    m[1][0] = -sz * cy2;
    m[1][1] = cz;
    m[1][2] = sz * sy2;

    m[2][0] = cy1sy2 + cz * sy1cy2;
    m[2][1] = sz * sy1;
    m[2][2] = cy1cy2 - cz * sy1sy2;

    return m;
}

//----------------------------------------------------------------------------
//
//              | cosz2  -sinz2  0 | | 1     0      0 | | cosz1  -sinz1  0 |
// Rz2 Rx Rz1 = | sinz2   cosz2  0 | | 0  cosx  -sinx | | sinz1   cosz1  0 |
//              |     0       0  1 | | 0  sinx   cosx | |     0       0  1 |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationZXZ(float ez2, float ex, float ez1) {
    float cz2, sz2, cx, sx, cz1, sz1;

    Math::SinCos(ez2, sz2, cz2);
    Math::SinCos(ex, sx, cx);
    Math::SinCos(ez1, sz1, cz1);

    float cz1cz2 = cz1 * cz2;
    float cz1sz2 = cz1 * sz2;
    float sz1cz2 = sz1 * cz2;
    float sz1sz2 = sz1 * sz2;

    Mat3 m;
    m[0][0] = cz1cz2 - cx * sz1sz2; 
    m[0][1] = cz1sz2 + cx * sz1cz2;
    m[0][2] = sx * sz1;

    m[1][0] = -cx * cz1sz2 - sz1cz2; 
    m[1][1] = cx * cz1cz2 - sz1sz2;
    m[1][2] = sx * cz1;

    m[2][0] = sx * sz2;
    m[2][1] = -sx * cz2;
    m[2][2] = cx;

    return m;
}

//----------------------------------------------------------------------------
//
//              | cosz2  -sinz2  0 | |  cosy  0   siny | | cosz1  -sinz1  0 |
// Rz2 Ry Rz1 = | sinz2   cosz2  0 | |     0  1      0 | | sinz1   cosz1  0 |
//              |     0       0  1 | | -siny  0   cosy | |     0       0  1 |
//
//----------------------------------------------------------------------------
Mat3 Mat3::FromRotationZYZ(float ez2, float ey, float ez1) {
    float cz2, sz2, cy, sy, cz1, sz1;

    Math::SinCos(ez2, sz2, cz2);
    Math::SinCos(ey, sy, cy);
    Math::SinCos(ez1, sz1, cz1);

    float cz1cz2 = cz1 * cz2;
    float cz1sz2 = cz1 * sz2;
    float sz1cz2 = sz1 * cz2;
    float sz1sz2 = sz1 * sz2;

    Mat3 m;
    m[0][0] = cy * cz1cz2 - sz1sz2;
    m[0][1] = cy * cz1sz2 + sz1cz2;
    m[0][2] = -sy * cz1;

    m[1][0] = -cz1sz2 - cy * sz1cz2;
    m[1][1] = cz1cz2 - cy * sz1sz2;
    m[1][2] = sy * sz1;

    m[2][0] = sy * cz2;
    m[2][1] = sy * sz2;
    m[2][2] = cy;

    return m;
}

void Mat3::ToRotationXYZ(float &ex, float &ey, float &ez) const {
    if (mat[2][0] < 1.f - 1e-6f) {
        if (mat[2][0] > -1.f + 1e-6f) {
            ey = asin(mat[2][0]);
            ex = atan2(-mat[2][1], mat[2][2]);
            ez = atan2(-mat[1][0], mat[0][0]);
        } else {
            // Not a unique solution: z - x = atan2(mat[0][1], mat[1][1]);
            ey = -Math::HalfPi;
            ex = -atan2(mat[0][1], mat[1][1]);
            ez = 0.f;
        }
    } else {
        // Not a unique solution: z + x = atan2(mat[0][1], mat[1][1]);
        ey = Math::HalfPi;
        ex = atan2(mat[0][1], mat[1][1]);
        ez = 0.f;
    }
}

void Mat3::ToRotationXZY(float &ex, float &ez, float &ey) const {
    if (mat[1][0] < 1.f - 1e-6f) {
        if (mat[1][0] > -1.f + 1e-6f) {
            ez = asin(-mat[1][0]);
            ex = atan2(mat[1][2], mat[1][1]);
            ey = atan2(mat[2][0], mat[0][0]);
        } else {
            // Not a unique solution: y - x = atan2(-mat[0][2], mat[2][2]);
            ez = Math::HalfPi;
            ex = atan2(-mat[0][2], mat[2][2]);
            ey = 0;
        }
    } else {
        // Not a unique solution: y + x = atan2(-mat[0][2], mat[2][2]);
        ez = -Math::Pi / 2.f;
        ex = atan2(-mat[0][2], mat[2][2]);
        ey = 0;
    }
}

void Mat3::ToRotationYXZ(float &ey, float &ex, float &ez) const {
    if (mat[2][1] < 1.f - 1e-6f) {
        if (mat[2][1] > -1.f + 1e-6f) {
            ex = asin(-mat[2][1]);
            ey = atan2(mat[2][0], mat[2][2]);
            ez = atan2(mat[0][1], mat[1][1]);
        } else {
            // Not a unique solution.
            ex = Math::HalfPi;
            ey = -atan2(-mat[1][0], mat[0][0]);
            ez = 0;
        }
    } else {
        // Not a unique solution.
        ex = -Math::HalfPi;
        ey = atan2(-mat[1][0], mat[0][0]);
        ez = 0;
    }
}

void Mat3::ToRotationYZX(float &ey, float &ez, float &ex) const {
    if (mat[0][1] < 1.f - 1e-6f) {
        if (mat[0][1] > -1.f + 1e-6f) {
            ez = asin(mat[0][1]);
            ey = atan2(mat[0][2], mat[0][0]);
            ex = atan2(mat[2][1], mat[1][1]);
        } else {
            // Not a unique solution.
            ez = -Math::HalfPi;
            ey = -atan2(mat[1][2], mat[2][2]);
            ex = 0;
        }
    } else {
        // Not a unique solution.
        ez = Math::HalfPi;
        ey = atan2(-mat[1][2], mat[2][2]);
        ex = 0;
    }
}

void Mat3::ToRotationZXY(float &ez, float &ex, float &ey) const {
    if (mat[1][2] < 1.f - 1e-6f) {
        if (mat[1][2] > -1.f + 1e-6f) {
            ex = asin(mat[1][2]);
            ez = atan2(-mat[1][0], mat[1][1]);
            ey = atan2(-mat[0][2], mat[2][2]);
        } else {
            // Not a unique solution.
            ex = -Math::HalfPi;
            ez = -atan2(-mat[2][0], mat[0][0]);
            ey = 0;
        }
    } else {
        // Not a unique solution.
        ex = Math::HalfPi;
        ez = atan2(mat[2][0], mat[0][0]);
        ey = 0;
    }
}

void Mat3::ToRotationZYX(float &ez, float &ey, float &ex) const {
    if (mat[0][2] < 1.f - 1e-6f) {
        if (mat[0][2] > -1.f + 1e-6f) {
            ey = asin(-mat[0][2]);
            ez = atan2(mat[0][1], mat[0][0]);
            ex = atan2(mat[1][2], mat[2][2]);
        } else {
            // Not a unique solution.
            ey = Math::HalfPi;
            ez = -atan2(-mat[2][1], mat[1][1]);
            ex = 0;
        }
    } else {
        // Not a unique solution.
        ey = -Math::HalfPi;
        ez = atan2(-mat[2][1], mat[1][1]);
        ex = 0;
    }
}

void Mat3::ToRotationXYX(float &ex2, float &ey, float &ex1) const {
    if (mat[0][0] < 1.f - 1e-6f) {
        if (mat[0][0] > -1.f + 1e-6f) {
            ey = acos(mat[0][0]);
            ex2 = atan2(mat[0][1], -mat[0][2]);
            ex1 = atan2(mat[1][0], mat[2][0]);
        } else {
            // Not a unique solution.
            ey = Math::Pi;
            ex2 = -atan2(-mat[2][1], mat[1][1]);
            ex1 = 0;
        }
    } else {
        // Not a unique solution.
        ey = 0;
        ex2 = atan2(-mat[2][1], mat[1][1]);
        ex1 = 0;
    }
}

void Mat3::ToRotationXZX(float &ex2, float &ez, float &ex1) const {
    if (mat[0][0] < 1.f - 1e-6f) {
        if (mat[0][0] > -1.f + 1e-6f) {
            ez = acos(mat[0][0]);
            ex2 = atan2(mat[0][2], mat[0][1]);
            ex1 = atan2(mat[2][0], -mat[1][0]);
        } else {
            // Not a unique solution.
            ez = Math::Pi;
            ex2 = -atan2(-mat[1][2], mat[2][2]);
            ex1 = 0;
        }
    } else {
        // Not a unique solution.
        ez = 0;
        ex2 = atan2(mat[1][2], mat[2][2]);
        ex1 = 0;
    }
}

void Mat3::ToRotationYXY(float &ey2, float &ex, float &ey1) const {
    if (mat[1][1] < 1.f - 1e-6f) {
        if (mat[1][1] > -1.f + 1e-6f) {
            ex = acos(mat[1][1]);
            ey2 = atan2(mat[1][0], mat[1][2]);
            ey1 = atan2(mat[0][1], -mat[2][1]);
        } else {
            // Not a unique solution.
            ex = Math::Pi;
            ey2 = -atan2(mat[2][0], mat[0][0]);
            ey1 = 0;
        }
    } else {
        // Not a unique solution.
        ex = 0;
        ey2 = atan2(mat[0][2], mat[0][0]);
        ey1 = 0;
    }
}

void Mat3::ToRotationYZY(float &ey2, float &ez, float &ey1) const {
    if (mat[1][1] < 1.f - 1e-6f) {
        if (mat[1][1] > -1.f + 1e-6f) {
            ez = acos(mat[1][1]);
            ey2 = atan2(mat[1][2], -mat[1][0]);
            ey1 = atan2(mat[2][1], mat[0][1]);
        } else {
            // Not a unique solution.
            ez = Math::Pi;
            ey2 = -atan2(-mat[0][2], mat[2][2]);
            ey1 = 0;
        }
    } else {
        // Not a unique solution.
        ez = 0;
        ey2 = atan2(-mat[0][2], mat[2][2]);
        ey1 = 0;
    }
}

void Mat3::ToRotationZXZ(float &ez2, float &ex, float &ez1) const {
    if (mat[2][2] < 1.f - 1e-6f) {
        if (mat[2][2] > -1.f + 1e-6f) {
            ex = acos(mat[2][2]);
            ez2 = atan2(mat[2][0], -mat[2][1]);
            ez1 = atan2(mat[0][2], mat[1][2]);
        } else {
            // Not a unique solution.
            ex = Math::Pi;
            ez2 = -atan2(-mat[1][0], mat[0][0]);
            ez1 = 0;
        }
    } else {
        // Not a unique solution.
        ex = 0;
        ez2 = atan2(-mat[1][0], mat[0][0]);
        ez1 = 0;
    }
}

void Mat3::ToRotationZYZ(float &ez2, float &ey, float &ez1) const {
    if (mat[2][2] < 1.f - 1e-6f) {
        if (mat[2][2] > -1.f + 1e-6f) {
            ey = acos(mat[2][2]);
            ez2 = atan2(mat[2][1], mat[2][0]);
            ez1 = atan2(mat[1][2], -mat[0][2]);
        } else {
            // Not a unique solution.
            ey = Math::Pi;
            ez2 = -atan2(-mat[0][1], mat[1][1]);
            ez1 = 0;
        }
    } else {
        // Not a unique solution.
        ey = 0;
        ez2 = atan2(mat[0][1], mat[1][1]);
        ez1 = 0;
    }
}

//------------------------------------------------
//  
//        | 1     0      0 | | m00  m10  m20 |
// Rx M = | 0  cosx  -sinx | | m01  m11  m21 |
//        | 0  sinx   cosx | | m02  m12  m22 |
//
//------------------------------------------------
Mat3 &Mat3::RotateX(float ex) {
    float s, c;
    Math::SinCos(ex, s, c);

    float tmp[6];
    tmp[0] = c * mat[0][1] - s * mat[0][2];
    tmp[1] = c * mat[0][2] + s * mat[0][1];
    tmp[2] = c * mat[1][1] - s * mat[1][2];
    tmp[3] = c * mat[1][2] + s * mat[1][1];
    tmp[4] = c * mat[2][1] - s * mat[2][2];
    tmp[5] = c * mat[2][2] + s * mat[2][1];
    
    mat[0][1] = tmp[0];
    mat[0][2] = tmp[1];
    mat[1][1] = tmp[2];
    mat[1][2] = tmp[3];
    mat[2][1] = tmp[4];
    mat[2][2] = tmp[5];

    return *this;
}

//------------------------------------------------
//  
//        |  cosy  0  siny | | m00  m10  m20 |
// Ry M = |     0  1     0 | | m01  m11  m21 |
//        | -siny  0  cosy | | m02  m12  m22 |
//
//------------------------------------------------
Mat3 &Mat3::RotateY(float ey) {
    float s, c;
    Math::SinCos(ey, s, c);
    
    float tmp[6];
    tmp[0] = c * mat[0][0] + s * mat[0][2];
    tmp[1] = c * mat[0][2] - s * mat[0][0];
    tmp[2] = c * mat[1][0] + s * mat[1][2];
    tmp[3] = c * mat[1][2] - s * mat[1][0];
    tmp[4] = c * mat[2][0] + s * mat[2][2];
    tmp[5] = c * mat[2][2] - s * mat[2][0];
    
    mat[0][0] = tmp[0];
    mat[0][2] = tmp[1];
    mat[1][0] = tmp[2];
    mat[1][2] = tmp[3];
    mat[2][0] = tmp[4];
    mat[2][2] = tmp[5];

    return *this;
}

//------------------------------------------------
//
//        | cosz  -sinz  0 | | m00  m10  m20 |
// Rz M = | sinz   cosz  0 | | m01  m11  m21 |
//        |    0      0  1 | | m02  m12  m22 |
//
//------------------------------------------------
Mat3 &Mat3::RotateZ(float ez) {
    float s, c;
    Math::SinCos(ez, s, c);
    
    float tmp[6];
    tmp[0] = c * mat[0][0] - s * mat[0][1];
    tmp[1] = c * mat[0][1] + s * mat[0][0];
    tmp[2] = c * mat[1][0] - s * mat[1][1];
    tmp[3] = c * mat[1][1] + s * mat[1][0];
    tmp[4] = c * mat[2][0] - s * mat[2][1];
    tmp[5] = c * mat[2][1] + s * mat[2][0];

    mat[0][0] = tmp[0];
    mat[0][1] = tmp[1];
    mat[1][0] = tmp[2];
    mat[1][1] = tmp[3];
    mat[2][0] = tmp[4];
    mat[2][1] = tmp[5];

    return *this;
}

Mat3 &Mat3::Rotate(const Vec3 &axis, float angle) {
    *this = Rotation(Vec3::origin, axis, RAD2DEG(angle)).ToMat3() * *this;

    return *this;
}

//------------------------------------------------
//
//        | sx   0   0 | | m00  m10  m20 |
// S M  = |  0  sy   0 | | m01  m11  m21 |
//        |  0   0  sz | | m02  m12  m22 |
//
//------------------------------------------------

Mat3 &Mat3::Scale(const Vec3 &scale) {
    mat[0][0] = mat[0][0] * scale.x;
    mat[0][1] = mat[0][1] * scale.y;
    mat[0][2] = mat[0][2] * scale.z;

    mat[1][0] = mat[1][0] * scale.x;
    mat[1][1] = mat[1][1] * scale.y;
    mat[1][2] = mat[1][2] * scale.z;

    mat[2][0] = mat[2][0] * scale.x;
    mat[2][1] = mat[2][1] * scale.y;
    mat[2][2] = mat[2][2] * scale.z;

    return *this;
}

//--------------------------------------------------------------------------------------------
//
// * Rotation Matrix to Euler Angles
//
// V = Rz * Ry * Rx
//    
//     | cosz*cosy  -sinz*cosx + sinx*siny*cosz   sinx*sinz + siny*cosz*cosx |
// V = | sinz*cosy   cosx*cosz + sinx*siny*sinz  -cosz*sinx + cosx*siny*sinz |
//     |     -siny                    sinx*cosy                    cosx*cosy |
// 
// Case 1: cosy != 0
//
// f02       = -siny
// f01 / f00 = sinz*cosy / cosz*cosy = sinz / cosz = tany
// f12 / f22 = sinx*cosy / cosx*cosy = sinx / cosx = tanr
//
// angleY = -asin( f02 )
// angleZ = atan( f01 / f00 )
// angleX = atan( f12 / f22 )
//
// Case 2: cosy == 0
//
// Case 2.a: siny == 1
//
//     |  0  -sinz*cosx + sinx*cosz   sinx*sinz + cosz*cosx |
// V = |  0   cosx*cosz + sinx*sinz  -cosz*sinx + cosx*sinz |
//     | -1                       0                       0 |
//
//     |  0  -sin(z - x)  cos(z - x) |
// V = |  0   cos(z - x)  sin(z - x) |
//     | -1            0           0 |
//
// f10 / f11 = -sin(z - x) / cos(z - x) = -tan(z - x)
//
// angleZ - angleX = -atan( f10 / f11 )
// angleY          = -asin( f02 )
//  
// Case 2.b: siny == -1
//
//     |  0  -sinz*cosx - sinx*cosz   sinx*sinz - cosz*cosx |
// V = |  0   cosx*cosz - sinx*sinz  -cosz*sinx - cosx*sinz |
//     |  1                       0                       0 |
//
//     |  0  -sin(z + x)   cos(z + x) |
// V = |  0   cos(z + x)  -sin(z + x) |
//     |  1            0            0 |
//  
// f10 / f11 = -sin(z + x) / cos(z + x) = -tan(z + x)
//
// angleZ + angleX = -atan( f10 / f11 )
// angleY          = -asin( f02 )
//
// The DOF of the z and y is the same.
// Locking the x to zero will in all cases ..
//
// angleY  = -asin( f02 )
// angleZ  = -atan( f10 / f11 )
// angleX  = 0
//
//--------------------------------------------------------------------------------------------
Angles Mat3::ToAngles() const {
    Angles angles;
    ToRotationZYX(angles[2], angles[1], angles[0]);

    angles[0] = RAD2DEG(angles[0]);
    angles[1] = RAD2DEG(angles[1]);
    angles[2] = RAD2DEG(angles[2]);

    return angles;
}

//-------------------------------------------------------------------------------------------
//
// * Rotation Matrix to Axis-Angle
//
// N = rotation axis
// s = sin( theta )
// c = cos( theta )
//
//     |   0  -Nz   Ny |        | -(Ny^2 + Nz^2)           Nx*Ny           Nx*Nz |
// S = |  Nz    0  -Nx |  S^2 = |           Nx*y  -(Nx^2 + Nz^2)           Ny*Nz |
//     | -Ny   Nx    0 |        |           Nx*z           Ny*Nz  -(Nx^2 + Ny^2) |
//
// R = I + s*S + (1-c)*S^2
//
//     |     c + (1-c)*Nx^2  (1-c)*Nx*Ny - s*Nz  (1-c)*Nx*Nz + s*Ny |
// R = | (1-c)*Nx*Ny + s*Nz      c + (1-c)*Ny^2  (1-c)*Ny*Nz - s*Nx |
//     | (1-c)*Nx*Nz - s*Ny  (1-c)*Ny*Nz + s*Nx      c + (1-c)*Nz^2 |
//
// trace(R) = R00 + R11 + R22 = 3*c + (1-c) * (Nx^2 + Ny^2 + Nz^2) = 3*c + (1-c)
//          = 2*c + 1
//
// 1) theta == 0 (trace(R) == 3)
//
// Since the rotation matrix is ​​a unit matrix, the axis of rotation can be anything.
//
// 2) 0 < theta < pi (3 > trace(R) > -1)
//  
// theta = acos( (trace(R) - 1) / 2 )
//
// Vx = R21 - R12 = 2*s*Nx
// Vy = R02 - R20 = 2*s*Ny
// Vz = R10 - R01 = 2*s*Nz
//  
// Nx = Vx / (2*s) 
// Ny = Vy / (2*s)
// Nz = Vz / (2*s)
//
// (You can simply normalize V)
//
// 3) theta == pi (trace(R) == -1)
//
//     | 2*Nx^2 - 1     2*Nx*Ny     2*Nx*Nz |
// R = |    2*Nx*Ny  2*Ny^2 - 1     2*Ny*Nz |
//     |    2*Nx*Nz     2*Ny*Nz  2*Nz^2 - 1 |
//
// There are three ways to do this..
//     ___________________
// i) √R00 - R11 - R22 + 1 / 2 = Nx
//     ___________________
// j) √R11 - R00 - R22 + 1 / 2 = Ny
//     ___________________
// k) √R22 - R00 - R11 + 1 / 2 = Nz
//
// In order for the value in the square root to be positive, we need to find the largest of the diagonal components of R.
//
// If you have obtained one of Nx, Ny, or Nz by one of i, j, and k, you can easily find the rest of the values.
//  
//-------------------------------------------------------------------------------------------
Rotation Mat3::ToRotation() const {
    Rotation r;

#if 0
    static constexpr int next[3] = { 1, 2, 0 };
    int i, j, k;

    // trace == 2c + 1
    float trace = mat[0][0] + mat[1][1] + mat[2][2];

    // 쿼터니언 안 거치고 구현하기
    // theta == 0
    if (trace == 3) {
        r.vec.Set(0.0f, 0.0f, 1.0f);
        r.angle = 0.0f;
    } else {
        if (trace == -1) {
            i = 0;
            if (mat[1][1] > mat[0][0]) {
                i = 1;
            }
        
            if (mat[2][2] > mat[i][i]) {
                i = 2;
            }

            j = next[i];
            k = next[j];

            r.angle = 180.0f;
            r.vec[i] = (mat[i][i] - (mat[j][j] + mat[k][k])) * 0.5f;
            float s = 1.0 / (r.vec[i] * 2.0f);
            r.vec[j] = mat[j][i] * s;
            r.vec[k] = mat[k][i] * s;
        } else {
            r.angle = RAD2DEG(Math::ACos((trace - 1) * 0.5f));
            r.vec[0] = mat[1][2] - mat[2][1];
            r.vec[1] = mat[2][0] - mat[0][2];
            r.vec[2] = mat[0][1] - mat[1][0];
        }

        r.vec.Normalize();
        r.vec.FixDegenerateNormal();
    }
#else
    Quat q = ToQuat();
    r.vec.Set(q.x, q.y, q.z);
    r.angle = Math::ACos(q.w);

    // angle is zero, axis can be anything
    if (Math::Fabs(r.angle) < 1e-10f) {
        r.vec.Set(0.0f, 0.0f, 1.0f);
        r.angle = 0.0f;
    } else {
        // r.vec *= (1.0f / sin(angle));
        r.vec.Normalize();
        r.vec.FixDegenerateNormal();
        r.angle = RAD2DEG(r.angle) * 2.0f;
    }
#endif
    r.origin = Vec3::zero;
    r.axis = *this;
    r.axisValid = true;
    return r;
}

//-------------------------------------------------------------------------------------------
//
// * Rotation Matrix to Quaternion
//
// Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
// article "Quaternion Calculus and Fast Animation".
//
//-------------------------------------------------------------------------------------------
Quat Mat3::ToQuat() const {
    constexpr int next[3] = { 1, 2, 0 };
    float s, t;
    Quat q;

    float trace = mat[0][0] + mat[1][1] + mat[2][2];

    // check the diagonal
    if (trace > 0.0f) {
        t = trace + 1.0f;
        s = Math::InvSqrt(t) * 0.5f;

        q.w = s * t;
        q.x = (mat[1][2] - mat[2][1]) * s;
        q.y = (mat[2][0] - mat[0][2]) * s;
        q.z = (mat[0][1] - mat[1][0]) * s;
    } else { // diagonal is negative
        int i = 0;
        if (mat[1][1] > mat[0][0]) {
            i = 1;
        }    
        if (mat[2][2] > mat[i][i]) {
            i = 2;
        }

        int j = next[i];
        int k = next[j];

        t = (mat[i][i] - mat[j][j] - mat[k][k]) + 1.0f;
        s = Math::InvSqrt(t) * 0.5f;

        q[i] = s * t;
        q[j] = (mat[i][j] + mat[j][i]) * s;
        q[k] = (mat[i][k] + mat[k][i]) * s;
        q[3] = (mat[j][k] - mat[k][j]) * s;
    }

    return q;
}

bool Mat3::SetFromLookAt(const Vec3 &viewDir, const Vec3 &up) {
    mat[0] = viewDir;
    if (mat[0].Normalize() < VECTOR_EPSILON) {
        return false;
    }
    mat[1] = up.Cross(mat[0]);
    if (mat[1].Normalize() < VECTOR_EPSILON) {
        return false;
    }
    mat[2] = mat[0].Cross(mat[1]);
    if (Math::Fabs(mat[2].LengthSqr() - 1.0f) > VECTOR_EPSILON) {
        return false;
    }
    return true;
}

Mat3 Mat3::FromString(const char *str) {
    Mat3 m;
    int count = sscanf(str, "%f %f %f %f %f %f %f %f %f",
        &m[0].x, &m[0].y, &m[0].z, &m[1].x, &m[1].y, &m[1].z, &m[2].x, &m[2].y, &m[2].z);
    assert(count == GetDimension());
    return m;
}

BE_NAMESPACE_END
