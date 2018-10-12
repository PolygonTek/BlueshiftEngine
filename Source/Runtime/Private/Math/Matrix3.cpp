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

const Mat3 Mat3::zero(Vec3(0, 0, 0), Vec3(0, 0, 0), Vec3(0, 0, 0));
const Mat3 Mat3::identity(Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1));

float Mat3::Determinant() const {
    float det2_12_01 = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];
    float det2_12_02 = mat[1][0] * mat[2][2] - mat[1][2] * mat[2][0];
    float det2_12_12 = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];

    return mat[0][0] * det2_12_12 - mat[0][1] * det2_12_02 + mat[0][2] * det2_12_01;
}

bool Mat3::InverseSelf() {
#if 1
    // 18+3+9 = 30 multiplications
    //			 1 division
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
    //		   3 divisions
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
    //	4*2+4*4 = 24 multiplications
    //		2*1 =  2 divisions
    Mat2 r0;
    float r1[2], r2[2], r3;
    float det, invDet;
    float *mat = reinterpret_cast<float *>(this);

    // r0 = m0.Inverse();	// 2x2
    det = mat[0*3+0] * mat[1*3+1] - mat[0*3+1] * mat[1*3+0];

    if (Math::Fabs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    invDet = 1.0f / det;

    r0[0][0] =   mat[1*3+1] * invDet;
    r0[0][1] = - mat[0*3+1] * invDet;
    r0[1][0] = - mat[1*3+0] * invDet;
    r0[1][1] =   mat[0*3+0] * invDet;

    // r1 = r0 * m1;		// 2x1 = 2x2 * 2x1
    r1[0] = r0[0][0] * mat[0*3+2] + r0[0][1] * mat[1*3+2];
    r1[1] = r0[1][0] * mat[0*3+2] + r0[1][1] * mat[1*3+2];

    // r2 = m2 * r1;		// 1x1 = 1x2 * 2x1
    r2[0] = mat[2*3+0] * r1[0] + mat[2*3+1] * r1[1];

    // r3 = r2 - m3;		// 1x1 = 1x1 - 1x1
    r3 = r2[0] - mat[2*3+2];

    // r3.InverseSelf();
    if (Math::Fabs(r3) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    r3 = 1.0f / r3;

    // r2 = m2 * r0;		// 1x2 = 1x2 * 2x2
    r2[0] = mat[2*3+0] * r0[0][0] + mat[2*3+1] * r0[1][0];
    r2[1] = mat[2*3+0] * r0[0][1] + mat[2*3+1] * r0[1][1];

    // m2 = r3 * r2;		// 1x2 = 1x1 * 1x2
    mat[2*3+0] = r3 * r2[0];
    mat[2*3+1] = r3 * r2[1];

    // m0 = r0 - r1 * m2;	// 2x2 - 2x1 * 1x2
    mat[0*3+0] = r0[0][0] - r1[0] * mat[2*3+0];
    mat[0*3+1] = r0[0][1] - r1[0] * mat[2*3+1];
    mat[1*3+0] = r0[1][0] - r1[1] * mat[2*3+0];
    mat[1*3+1] = r0[1][1] - r1[1] * mat[2*3+1];

    // m1 = r1 * r3;		// 2x1 = 2x1 * 1x1
    mat[0*3+2] = r1[0] * r3;
    mat[1*3+2] = r1[1] * r3;

    // m3 = -r3;
    mat[2*3+2] = -r3;

    return true;
#endif
}

//------------------------------------------------
//  
//        | 1    0     0 | | m00  m10  m20 |
// Rx M = | 0  cos  -sin | | m01  m11  m21 |
//        | 0  sin   cos | | m02  m12  m22 |
//
//------------------------------------------------
Mat3 &Mat3::RotateX(const float degree) {
    float s, c;
    Math::SinCos(DEG2RAD(degree), s, c);

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
//        |  cos  0  sin | | m00  m10  m20 |
// Ry M = |    0  1    0 | | m01  m11  m21 |
//        | -sin  0  cos | | m02  m12  m22 |
//
//------------------------------------------------
Mat3 &Mat3::RotateY(const float degree) {
    float s, c;
    Math::SinCos(DEG2RAD(degree), s, c);
    
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
//        | cos  -sin  0 | | m00  m10  m20 |
// Rz M = | sin   cos  0 | | m01  m11  m21 |
//        |   0     0  1 | | m02  m12  m22 |
//
//------------------------------------------------
Mat3 &Mat3::RotateZ(const float degree) {
    float s, c;
    Math::SinCos(DEG2RAD(degree), s, c);
    
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

Mat3 &Mat3::Rotate(const Vec3 &axis, const float degree) {
    Rotation rot(Vec3::origin, axis, degree);
    *this = rot.ToMat3() * *this;

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
//   = R(yaw) * R(pitch) * R(roll)
//    
//     | cosy*cosp  -siny*cosr + sinr*sinp*cosy   sinr*siny + sinp*cosy*cosr |
// V = | siny*cosp   cosr*cosy + sinr*sinp*siny  -cosy*sinr + cosr*sinp*siny |
//     |     -sinp                    sinr*cosp                    cosr*cosp |
// 
// 1) cosp != 0 인 경우
//
// f02       = -sinp
// f01 / f00 = siny*cosp / cosy*cosp = siny / cosy = tany
// f12 / f22 = sinr*cosp / cosr*cosp = sinr / cosr = tanr	
//
// pitch = -asin( f02 )
// yaw   = atan( f01 / f00 )
// roll  = atan( f12 / f22 )
//
// 2) cosp == 0 인 경우
//
// i) sinp == 1 인 경우
//
//     |  0  -siny*cosr + sinr*cosy   sinr*siny + cosy*cosr |
// V = |  0   cosr*cosy + sinr*siny  -cosy*sinr + cosr*siny |
//     | -1                       0                       0 |
//
//     |  0  -sin(y - r)  cos(y - r) |
// V = |  0   cos(y - r)  sin(y - r) |
//     | -1            0           0 |
//
// f10 / f11 = -sin(y - r) / cos(y - r) = -tan(y - r)
//
// yaw-roll = -atan( f10 / f11 )
// pitch    = -asin( f02 )
//  
// ii) sinp == -1 인경우
//
//     |  0  -siny*cosr - sinr*cosy   sinr*siny - cosy*cosr |
// V = |  0   cosr*cosy - sinr*siny  -cosy*sinr - cosr*siny |
//     |  1                       0                       0 |
//
//     |  0  -sin(y + r)   cos(y + r) |
// V = |  0   cos(y + r)  -sin(y + r) |
//     |  1            0            0 |
//  
// f10 / f11 = -sin(y + r) / cos(y + r) = -tan(y + r)
//
// yaw+roll = -atan( f10 / f11 )
// pitch    = -asin( f02 )
//
// yaw 와 pitch 가 같은 자유도를 갖는다. roll 을 0 으로 고정시키면 i), ii) 모두의 경우에,..
//
// pitch    = -asin( f02 )
// yaw      = -atan( f10 / f11 )
// roll     = 0
//
//--------------------------------------------------------------------------------------------
Angles Mat3::ToAngles() const {
#if 0
    float sp = mat[0][2];
    // asin 의 NAN 값을 피하기위해 clamp
    Clamp(sp, -1.0f, 1.0f);

    double theta = -asin(sp);
    double cp = cos(theta);

    Angles angles;
    if (cp > 8192.0f * Math::FloatEpsilon) { //
        angles.pitch    = (float)RAD2DEG(theta);
        angles.yaw      = (float)RAD2DEG(atan2(mat[0][1], mat[0][0]));
        angles.roll     = (float)RAD2DEG(atan2(mat[1][2], mat[2][2]));
    } else { // not a unique solution
        angles.pitch    = (float)RAD2DEG(theta);
        angles.yaw      = (float)RAD2DEG(-atan2(mat[1][0], mat[1][1]));
        angles.roll     = (float)0.0f;
    } 

    return angles;
#else
    Angles angles;
    float s = Math::Sqrt(mat[0][0] * mat[0][0] + mat[0][1] * mat[0][1]);
    if (s > Math::FloatEpsilon) {
        angles.pitch = RAD2DEG(-Math::ATan(mat[0][2], s));
        angles.yaw = RAD2DEG(Math::ATan(mat[0][1], mat[0][0]));
        angles.roll = RAD2DEG(Math::ATan(mat[1][2], mat[2][2]));
    } else {
        angles.pitch = mat[0][2] < 0.0f ? 90.0f : -90.0f;
        angles.yaw = RAD2DEG(-Math::ATan(mat[1][0], mat[1][1]));
        angles.roll = 0.0f;
    }
    return angles;
#endif
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
// 회전행렬은 단위행렬이므로, 회전축은 아무거나 할수 있다
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
// (단순히 V 를 normalize 해도 된다)
//
// 3) theta == pi (trace(R) == -1)
//
//     | 2*Nx^2 - 1     2*Nx*Ny     2*Nx*Nz |
// R = |    2*Nx*Ny  2*Ny^2 - 1     2*Ny*Nz |
//     |    2*Nx*Nz     2*Ny*Nz  2*Nz^2 - 1 |
//
// 이때 3가지 방법이 있는데..
//     ___________________
// i) √R00 - R11 - R22 + 1 / 2 = Nx 
//     ___________________
// j) √R11 - R00 - R22 + 1 / 2 = Ny
//     ___________________
// k) √R22 - R00 - R11 + 1 / 2 = Nz
//
// 제곱근 안의 값이 양수가 나오게 하려면 R 의 대각성분중에 가장 큰성분을 찾아야 한다
//
// i, j, k 중 한가지 방법으로 Nx, Ny, Nz 중 하나를 구했으면, 나머지 값들은 쉽게 구할수 있다
//  
//-------------------------------------------------------------------------------------------
Rotation Mat3::ToRotation() const {
    static constexpr int next[3] = { 1, 2, 0 };
    Rotation r;
    float s, t;
    int i, j, k;

    // trace == 2c + 1
    float trace = mat[0][0] + mat[1][1] + mat[2][2];

#if 0 
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
            s = 1.0 / (r.vec[i] * 2.0f);
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
    // 일단 쿼터니언 값을 만든다
    if (trace > 0.0f) { // TODO: trace > -1.0f 가 아닌 이유는?? 3D Game Engine Design 참고할것
        t = trace + 1.0f;
        s = Math::InvSqrt(t) * 0.5f;

        r.angle = s * t;
        r.vec[0] = (mat[1][2] - mat[2][1]) * s;
        r.vec[1] = (mat[2][0] - mat[0][2]) * s;
        r.vec[2] = (mat[0][1] - mat[1][0]) * s;
    } else {
        i = 0;
        if (mat[1][1] > mat[0][0]) {
            i = 1;
        }
    
        if (mat[2][2] > mat[i][i]) {
            i = 2;
        }

        j = next[i];
        k = next[j];

        t = (mat[i][i] - (mat[j][j] + mat[k][k])) + 1.0f;
        s = Math::InvSqrt(t) * 0.5f;

        r.vec[i] = s * t;
        r.vec[j] = (mat[i][j] + mat[j][i]) * s;
        r.vec[k] = (mat[i][k] + mat[k][i]) * s;
        r.angle = (mat[j][k] - mat[k][j]) * s;
    }

    r.angle = Math::ACos(r.angle);

    // angle is zero, axis can be anything
    if (Math::Fabs(r.angle) < 1e-10f) {
        r.vec.Set(0.0f, 0.0f, 1.0f);
        r.angle = 0.0f;
    } else {
        // r.vec *= (1.0f / sin(angle));
        // 걍 노멀라이징 한다
        r.vec.Normalize();
        r.vec.FixDegenerateNormal();
        r.angle = RAD2DEG(r.angle) * 2.0f;
    }
#endif
    r.origin.SetFromScalar(0);
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
    sscanf(str, "%f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[1].x, &m[1].y, &m[1].z, &m[2].x, &m[2].y, &m[2].z);    
    return m;
}

BE_NAMESPACE_END
