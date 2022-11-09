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

ALIGN_AS16 const Angles Angles::zero(0.0f, 0.0f, 0.0f);

Angles &Angles::Normalize360() {
    for (int i = 0; i < Size; i++) {
        if (((*this)[i] >= 360.0f) || ((*this)[i] < 0.0f)) {
            (*this)[i] -= Math::Floor((*this)[i] / 360.0f) * 360.0f;

            if ((*this)[i] >= 360.0f) {
                (*this)[i] -= 360.0f;
            }            
            if ((*this)[i] < 0.0f) {
                (*this)[i] += 360.0f;
            }
        }
    }
    return *this;
}

Angles &Angles::Normalize180() {
    // Angle in the range [0, 360)
    Normalize360();
    
    // Shift to (-180, 180]
    if (x > 180.0f) {
        x -= 360.0f;
    }
    if (y > 180.0f) {
        y -= 360.0f;
    }
    if (z > 180.0f) {
        z -= 360.0f;
    }
    return *this;
}

Angles Angles::FromString(const char *str) {
    Angles a;
    int count = sscanf(str, "%f %f %f", &a.x, &a.y, &a.z);
    assert(count == GetDimension());
    return a;
}

//-------------------------------------------------------------------------------------------
//
// * Euler Angles to Quaternion
//
// x 축 회전 사원수 qx = ( sin(x/2), 0, 0, cos(x/2) )
// y 축 회전 사원수 qy = ( 0, sin(y/2), 0, cos(y/2) )
// z 축 회전 사원수 qz = ( 0, 0, sin(z/2), cos(z/2) )
//  
// qz * qy * qx = { sin(x/2) * cos(y/2) * cos(z/2) - cos(x/2) * sin(y/2) * sin(z/2) } * i +
//                { cos(x/2) * sin(y/2) * cos(z/2) + sin(x/2) * cos(y/2) * sin(z/2) } * j + 
//                { cos(x/2) * cos(y/2) * sin(z/2) - sin(x/2) * sin(y/2) * cos(z/2) } * k + 
//                { cos(x/2) * cos(y/2) * cos(z/2) + sin(x/2) * sin(y/2) * sin(z/2) }
//      
//-------------------------------------------------------------------------------------------
Rotation Angles::ToRotation() const {
    if (y == 0.0f) {
        if (z == 0.0f) {
            return Rotation(Vec3::origin, Vec3(1.0f, 0.0f, 0.0f), x);
        }
        if (x == 0.0f) {
            return Rotation(Vec3::origin, Vec3(0.0f, 0.0f, 1.0f), z);
        }
    } else if (z == 0.0f && x == 0.0f) {
        return Rotation(Vec3::origin, Vec3(0.0f, 1.0f, 0.0f), y);
    }

    float sx, cx, sy, cy, sz, cz;
    const float DegreeToRadianHalf = Math::MulDegreeToRadian * 0.5f;
    Math::SinCos(DegreeToRadianHalf * x, sx, cx);
    Math::SinCos(DegreeToRadianHalf * y, sy, cy);
    Math::SinCos(DegreeToRadianHalf * z, sz, cz);

    float sxcy = sx * cy;
    float cxcy = cx * cy;
    float sxsy = sx * sy;
    float cxsy = cx * sy;

    Vec3 vec;
    vec.x = sxcy * cz - cxsy * sz;
    vec.y = cxsy * cz + sxcy * sz;
    vec.z = cxcy * sz - sxsy * cz;
    float w = cxcy * cz + sxsy * sz;

    float angle = Math::ACos(w);

    if (angle == 0.0f) {
        vec.Set(0.0f, 0.0f, 1.0f);
    } else {
        //vec *= (1.0f / sin(angle));
        vec.Normalize();
        vec.FixDegenerateNormal();
        angle = RAD2DEG(angle) * 2.0f;
    }

    return Rotation(Vec3::origin, vec, angle);
}

Quat Angles::ToQuat() const {
    float sx, cx, sy, cy, sz, cz;
    const float DegreeToRadianHalf = Math::MulDegreeToRadian * 0.5f;
    Math::SinCos(DegreeToRadianHalf * x, sx, cx);
    Math::SinCos(DegreeToRadianHalf * y, sy, cy);
    Math::SinCos(DegreeToRadianHalf * z, sz, cz);

    float sxcy = sx * cy;
    float cxcy = cx * cy;
    float sxsy = sx * sy;
    float cxsy = cx * sy;

    Quat q;
    q.x = sxcy * cz - cxsy * sz;
    q.y = cxsy * cz + sxcy * sz;
    q.z = cxcy * sz - sxsy * cz;
    q.w = cxcy * cz + sxsy * sz;
    return q;
}

//---------------------------------------------------------------------------------
//
// * Concatenated Rotation Matrix From Euler Angles
//
// R = Rz * Ry * Rx
//
//     | cosz  -sinz  0 |   |  cosy  0  siny |   | 1     0      0 |
//   = | sinz   cosz  0 | * |     0  1     0 | * | 0  cosx  -sinx |
//     |    0      0  1 |   | -siny  0  cosy |   | 0  sinx   cosx |
//
//     | cosz*cosy  -sinz*cosx + sinx*siny*cosz   sinx*sinz + siny*cosz*cosx |
//   = | sinz*cosy   cosx*cosz + sinx*siny*sinz  -cosz*sinx + cosx*siny*sinz |
//     |     -siny                    sinx*cosy                    cosx*cosy |
//
//---------------------------------------------------------------------------------
void Angles::ToVectors(Vec3* xAxis, Vec3* yAxis, Vec3* zAxis) const {
    assert(xAxis || yAxis || zAxis);

    float sx, sy, sz, cx, cy, cz;
    Math::SinCos(DEG2RAD(x), sx, cx);
    Math::SinCos(DEG2RAD(y), sy, cy);
    Math::SinCos(DEG2RAD(z), sz, cz);

    if (xAxis) {
        xAxis->Set(cy * cz, cy * sz, -sy);
    }
    if (yAxis) {
        float sxsy = sx * sy;
        yAxis->Set(sxsy * cz - cx * sz, sxsy * sz + cx * cz, sx * cy);
    }
    if (zAxis) {
        float cxsy = cx * sy;
        zAxis->Set(cxsy * cz + sx * sz, cxsy * sz - sx * cz, cx * cy);
    }
}

Vec3 Angles::ToXAxis() const {
    float sy, sz, cy, cz;
    Math::SinCos(DEG2RAD(y), sy, cy);
    Math::SinCos(DEG2RAD(z), sz, cz);

    return Vec3(cy * cz, cy * sz, -sy);
}

Vec3 Angles::ToYAxis() const {
    Vec3 yAxis;
    ToVectors(nullptr, &yAxis, nullptr);
    return yAxis;
}

Vec3 Angles::ToZAxis() const {
    Vec3 zAxis;
    ToVectors(nullptr, nullptr, &zAxis);
    return zAxis;
}

Mat3 Angles::ToMat3() const {
    return Mat3::FromRotationZYX(DEG2RAD(z), DEG2RAD(y), DEG2RAD(x));
}

Mat4 Angles::ToMat4() const {
    return ToMat3().ToMat4();
}

BE_NAMESPACE_END
