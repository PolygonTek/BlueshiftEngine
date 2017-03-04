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

Angles Rotation::ToAngles() const {
    return ToMat3().ToAngles();
}

Quat Rotation::ToQuat() const {
    // take half-angle
    float a = DEG2RAD(angle) * 0.5f;
    float s, c;
    Math::SinCos(a, s, c);

    Quat q;
    q.x = vec.x * s;
    q.y = vec.y * s;
    q.z = vec.z * s;
    q.w = c;

    return q;
}

//------------------------------------------------------------------------------
// ref. Essential Mathematics for Games & Interactive Applications 2nd Edition p.148
//
// N(x, y, z) = 단위 축벡터
//
//     |  0  -z   y |          | -(y^2 + z^2)           x*y           x*z |
// S = |  z   0  -x |    S^2 = |          x*y  -(x^2 + z^2)           y*z |
//     | -y   x   0 |          |          x*z           y*z  -(x^2 + y^2) |
//
// R = I + sin(theta)*S + (1-cos(theta))*S^2
//
//     |   c + (1-c)*x^2  (1-c)*x*y - s*z  (1-c)*x*z + s*y |
// R = | (1-c)*x*y + s*z    c + (1-c)*y^2  (1-c)*y*z - s*x |
//     | (1-c)*x*z - s*y  (1-c)*y*z + s*x    c + (1-c)*z^2 |
//  
//------------------------------------------------------------------------------
const Mat3 &Rotation::ToMat3() const {
#if 1
    // 12 multiplications
    if (axisValid) {
        return axis;
    }

    float s, c;
    Math::SinCos(DEG2RAD(angle), s, c);
    float t = 1.0f - c;

    float x = vec.x;
    float y = vec.y;
    float z = vec.z;

    float tx = t * x;
    float ty = t * y;
    float tz = t * z;

    float sx = s * x;
    float sy = s * y;
    float sz = s * z;

    float txx = tx * x;
    float txy = tx * y;
    float txz = tx * z;

    float tyy = ty * y;
    float tyz = ty * z;
    float tzz = tz * z;
    
    axis[0][0] = txx + c;
    axis[0][1] = txy + sz;
    axis[0][2] = txz - sy;

    axis[1][0] = txy - sz;
    axis[1][1] = tyy + c;
    axis[1][2] = tyz + sx;

    axis[2][0] = txz + sy;
    axis[2][1] = tyz - sx;
    axis[2][2] = tzz + c;
#else
    // 13 multiplications
    if (axisValid) {
        return axis;
    }

    float a = DEG2RAD(angle) * 0.5f;
    float c, s;
    Math::SinCos(a, s, c);

    float x = vec[0] * s;
    float y = vec[1] * s;
    float z = vec[2] * s;

    float x2 = x + x;
    float y2 = y + y;
    float z2 = z + z;

    float xx = x * x2;
    float xy = x * y2;
    float xz = x * z2;

    float yy = y * y2;
    float yz = y * z2;
    float zz = z * z2;

    float wx = c * x2;
    float wy = c * y2;
    float wz = c * z2;

    axis[0][0] = 1.0f - (yy + zz);
    axis[0][1] = xy - wz;
    axis[0][2] = xz + wy;

    axis[1][0] = xy + wz;
    axis[1][1] = 1.0f - (xx + zz);
    axis[1][2] = yz - wx;

    axis[2][0] = xz - wy;
    axis[2][1] = yz + wx;
    axis[2][2] = 1.0f - (xx + yy);
#endif
    axisValid = true;
    return axis;
}

Mat4 Rotation::ToMat4() const {
    return ToMat3().ToMat4();
}

void Rotation::Normalize180() {
    angle -= floor(angle / 360.0f) * 360.0f;
    if (angle > 180.0f) {
        angle -= 360.0f;
    } else if (angle < -180.0f) {
        angle += 360.0f;
    }
}

void Rotation::Normalize360() {
    angle -= floor(angle / 360.0f) * 360.0f;
    if (angle > 360.0f) {
        angle -= 360.0f;
    } else if (angle < 0.0f) {
        angle += 360.0f;
    }
}

BE_NAMESPACE_END
