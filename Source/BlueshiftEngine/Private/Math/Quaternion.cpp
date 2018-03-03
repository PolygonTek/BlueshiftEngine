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

const Quat Quat::identity(0.0f, 0.0f, 0.0f, 1.0f);

Quat Quat::FromString(const char *str) {
    Quat q;
    sscanf(str, "%f %f %f %f", &q.x, &q.y, &q.z, &q.w);
    return q;
}

Quat &Quat::SetFromAngleAxis(float angle, const Vec3 &axis) {
    Rotation rot(Vec3::origin, axis, angle);
    *this = rot.ToQuat();
    return *this;
}

Quat &Quat::SetFromTwoVectors(const Vec3 &from, const Vec3 &to) {
    float c = from.Dot(to);

    // check if parallel
    if (c > 0.99999f) {
        x = y = z = 0.0f;
        w = 1.0f;
    } else if (c < -0.99999f) { // check if opposite
        // check if we can use cross product of from vec with [1, 0, 0]
        Vec3 t(0.0f, from.x, -from.y);

        float len = t.Length();
        if (len < Math::FloatEpsilon) {
            // nope! we need cross product of from vec with [0, 1, 0]
            t.Set(-from.z, 0.0f, from.x);
        }

        t.Normalize();
        
        x = t.x;
        y = t.y;
        z = t.z;
        w = 0.0;
    } else {
        // ... else we can just cross two vecs
        float m = Math::Sqrt(2.0f + 2.0f * c);
        Vec3 t = from.Cross(to) / m;
        
        x = t.x;
        y = t.y;
        z = t.z;
        w = m * 0.5f;
    }

    return *this;
}

// Spherical linear interpolation between two quaternions.
Quat &Quat::SetFromSlerp(const Quat &from, const Quat &to, float t) {
    if (t <= 0.0f) {
        *this = from;
        return *this;
    }

    if (t >= 1.0f) {
        *this = to;
        return *this;
    }

    if (from == to) {
        *this = to;
        return *this;
    }

    // calc cosine
    float cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;

    // adjust signs (if necessary)
    Quat temp;
    if (cosom < 0.0f) {
        temp = -to;
        cosom = -cosom;
    } else {
        temp = to;
    }

    // calculate coefficients
    float omega, sinom, scale0, scale1;
    if ((1.0f - cosom) > 1e-6f) { //Math::FLT_EPSILON
#if 0	// standard case (slerp)
        omega = Math::ACos(cosom);
        sinom = Math::Sin(omega);
        scale0 = Math::Sin((1.0f - t) * omega) * sinom;
        scale1 = Math::Sin(t * omega) * sinom;
#else
        scale0 = 1.0f - cosom * cosom;
        sinom = Math::InvSqrt(scale0);
        omega = Math::ATan16(scale0 * sinom, cosom);
        scale0 = Math::Sin16((1.0f - t) * omega) * sinom;
        scale1 = Math::Sin16(t * omega) * sinom;
#endif
    } else {
        // "from" and "to" quaternions are very close 
        //  ... so we can do a linear interpolation		
        scale0 = 1.0f - t;
        scale1 = t;
    }

    // calculate final values
    *this = (scale0 * from) + (scale1 * temp);
    return *this;
}

// Approximation of spherical linear interpolation between two quaternions.
// The interpolation traces out the exact same curve as Slerp but does not maintain a constant speed across the arc.
Quat &Quat::SetFromSlerpFast(const Quat &from, const Quat &to, float t) {
    if (t <= 0.0f) {
        *this = from;
        return *this;
    }

    if (t >= 1.0f) {
        *this = to;
        return *this;
    }

    if (from == to) {
        *this = to;
        return *this;
    }

    float cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;

    float scale0 = 1.0f - t;
    float scale1 = (cosom >= 0.0f) ? t : -t;

    x = scale0 * from.x + scale1 * to.x;
    y = scale0 * from.y + scale1 * to.y;
    z = scale0 * from.z + scale1 * to.z;
    w = scale0 * from.w + scale1 * to.w;

    float s = Math::InvSqrt(x * x + y * y + z * z + w * w);

    x *= s;
    y *= s;
    z *= s;
    w *= s;

    return *this;
}

Angles Quat::ToAngles() const {
    return ToMat3().ToAngles();
}
 
Rotation Quat::ToRotation() const {
    Vec3 vec;
    float angle = Math::ACos(w);
    if (angle == 0.0f) {
        vec.Set(0.0f, 0.0f, 1.0f);
    } else {
        vec.Set(x, y, z);
        vec.Normalize();
        vec.FixDegenerateNormal();
        angle = RAD2DEG(angle * 2.0f);
    }

    return Rotation(Vec3::origin, vec, angle);
}

Mat3 Quat::ToMat3() const {
    float x2 = x + x;
    float y2 = y + y;
    float z2 = z + z;

    float xx2 = x * x2;
    float xy2 = x * y2;
    float xz2 = x * z2;

    float yy2 = y * y2;
    float yz2 = y * z2;
    float zz2 = z * z2;

    float wx2 = w * x2;
    float wy2 = w * y2;
    float wz2 = w * z2;

    Mat3 mat;
    mat[0][0] = 1.0f - yy2 - zz2;
    mat[0][1] = xy2 + wz2;
    mat[0][2] = xz2 - wy2;

    mat[1][0] = xy2 - wz2;
    mat[1][1] = 1.0f - xx2 - zz2;
    mat[1][2] = yz2 + wx2;

    mat[2][0] = xz2 + wy2;
    mat[2][1] = yz2 - wx2;
    mat[2][2] = 1.0f - xx2 - yy2;

    return mat;
}

Mat4 Quat::ToMat4() const {
    return ToMat3().ToMat4();
}

Vec3 Quat::ToAngularVelocity() const {
    Vec3 vec(x, y, z);
    vec.Normalize();
    return 2.0f * vec * Math::ACos(w);
}

BE_NAMESPACE_END
