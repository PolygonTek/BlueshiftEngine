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

BE_NAMESPACE_BEGIN

ALIGN_AS16 const Quat Quat::zero(0.0f, 0.0f, 0.0f, 0.0f);
ALIGN_AS16 const Quat Quat::identity(0.0f, 0.0f, 0.0f, 1.0f);

Quat Quat::operator*(const Quat &a) const & {
#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f signx = simd4i(0, 0x80000000, 0, 0x80000000); // [+ - + -]
    const simd4f signy = simd4i(0, 0, 0x80000000, 0x80000000); // [+ + - -]
    const simd4f signz = simd4i(0x80000000, 0, 0, 0x80000000); // [- + + -]

    simd4f q = loadu_ps(*this);
    simd4f qx = signx ^ shuffle_ps<0, 0, 0, 0>(q);
    simd4f qy = signy ^ shuffle_ps<1, 1, 1, 1>(q);
    simd4f qz = signz ^ shuffle_ps<2, 2, 2, 2>(q);
    simd4f qw = shuffle_ps<3, 3, 3, 3>(q);

    simd4f a4 = loadu_ps(a);
    simd4f a3 = shuffle_ps<1, 0, 3, 2>(a4);
    simd4f a2 = shuffle_ps<2, 3, 0, 1>(a4);
    simd4f a1 = shuffle_ps<3, 2, 1, 0>(a4);

    simd4f r = qx * a1;
    r = madd_ps(qy, a2, r);
    r = madd_ps(qz, a3, r);
    r = madd_ps(qw, a4, r);

    ALIGN_AS16 Quat dst;
    store_ps(r, dst);
    return dst;
#else
    return Quat(
        x * a.w + y * a.z - z * a.y + w * a.x,
        -x * a.z + y * a.w + z * a.x + w * a.y,
        x * a.y - y * a.x + z * a.w + w * a.z,
        -x * a.x - y * a.y - z * a.z + w * a.w);
#endif
}

Quat Quat::operator/(const Quat &a) const & {
#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f signx = simd4i(0, 0, 0x80000000, 0); // [+ + - +]
    const simd4f signy = simd4i(0x80000000, 0, 0, 0); // [- + + +]
    const simd4f signz = simd4i(0, 0x80000000, 0, 0); // [+ - + +]
    const simd4f signw = simd4i(0x80000000, 0x80000000, 0x80000000, 0); // [- - - +]

    simd4f q = loadu_ps(*this);
    simd4f qx = signx ^ shuffle_ps<0, 0, 0, 0>(q);
    simd4f qy = signy ^ shuffle_ps<1, 1, 1, 1>(q);
    simd4f qz = signz ^ shuffle_ps<2, 2, 2, 2>(q);
    simd4f qw = signw ^ shuffle_ps<3, 3, 3, 3>(q);

    simd4f a4 = loadu_ps(a);
    simd4f a3 = shuffle_ps<1, 0, 3, 2>(a4);
    simd4f a2 = shuffle_ps<2, 3, 0, 1>(a4);
    simd4f a1 = shuffle_ps<3, 2, 1, 0>(a4);

    simd4f r = qx * a1;
    r = madd_ps(qy, a2, r);
    r = madd_ps(qz, a3, r);
    r = madd_ps(qw, a4, r);

    ALIGN_AS16 Quat dst;
    store_ps(r, dst);
    return dst;
#else
    return Quat(
        x * a.w - y * a.z + z * a.y - w * a.x,
        x * a.z + y * a.w - z * a.x - w * a.y,
        -x * a.y + y * a.x + z * a.w - w * a.z,
        x * a.x + y * a.y + z * a.z + w * a.w);
#endif
}

Quat Quat::FromString(const char *str) {
    Quat q;
    int count = sscanf(str, "%f %f %f %f", &q.x, &q.y, &q.z, &q.w);
    assert(count == q.GetDimension());
    return q;
}

Quat &Quat::SetFromAngleAxis(float angle, const Vec3 &axis) {
    float s;
    Math::SinCos(angle * 0.5f, s, w);
    x = axis.x * s;
    y = axis.y * s;
    z = axis.z * s;
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
        if (len < VECTOR_EPSILON) {
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

float Quat::AngleBetween(const Quat &target) const {
    Quat delta = target / *this;
    delta.Normalize();
    return delta.Angle();
}

Vec3 Quat::AxisBetween(const Quat &target) const {
    Quat delta = target / *this;
    delta.Normalize();
    return delta.Axis();
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
    float scale0, scale1;
    if ((1.0f - cosom) > 1e-6f) { //Math::FLT_EPSILON
#if 0   // standard case (slerp)
        float omega = Math::ACos(cosom);
        float sinom = Math::Sin(omega);
        scale0 = Math::Sin((1.0f - t) * omega) * sinom;
        scale1 = Math::Sin(t * omega) * sinom;
#else
        scale0 = 1.0f - cosom * cosom;
        float sinom = Math::InvSqrt(scale0);
        float omega = Math::ATan2Fast(scale0 * sinom, cosom);
        scale0 = Math::SinFast((1.0f - t) * omega) * sinom;
        scale1 = Math::SinFast(t * omega) * sinom;
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

#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f fromQuat = loadu_ps(from.Ptr());
    const simd4f toQuat = loadu_ps(to.Ptr());
    const float cosom = x_ps(dot4_ps(fromQuat, toQuat));
    const float scale0 = 1.0f - t;
    const float scale1 = cosom >= 0.0f ? t : -t;

    simd4f interpolated = madd_ps(toQuat, set1_ps(scale1), fromQuat * set1_ps(scale0));
    interpolated *= SIMD_4::F4_one / sqrt_ps(dot4_ps(interpolated, interpolated));
    storeu_ps(interpolated, Ptr());
#else
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
#endif

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
#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f firstSignBit = simd4i(0x80000000, 0x00000000, 0x00000000, 0x00000000);
    const simd4f firstPosHalf = set_ps(0.5f, 0.0f, 0.0f, 0.0f);
    const simd4f firstNegHalf = set_ps(-0.5f, 0.0f, 0.0f, 0.0f);
    const simd4f quat2MatMad1 = set_ps(-1.0f, -1.0f, 1.0f, -1.0f);
    const simd4f quat2MatMad2 = set_ps(-1.0f, 1.0f, -1.0f, -1.0f);
    const simd4f quat2MatMad3 = set_ps(1.0f, -1.0f, -1.0f, 1.0f);

    const simd4f q = loadu_ps(Ptr());
    const simd4f d = q + q;

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

    const simd4f row0 = madd_ps(mb, quat2MatMad1, ma);
    const simd4f row1 = madd_ps(mb, quat2MatMad2, md);
    const simd4f row2 = madd_ps(me, quat2MatMad3, md);

    simd4f temp = shuffle_ps<0, 0, 1, 1>(row0, row1);
    const simd4f column0 = shuffle_ps<0, 2, 2, 2>(temp, row2);
    temp = shuffle_ps<1, 1, 0, 0>(row0, row1);
    const simd4f column1 = shuffle_ps<0, 2, 3, 3>(temp, row2);
    temp = shuffle_ps<2, 2, 3, 3>(row0, row1);
    const simd4f column2 = shuffle_ps<0, 2, 0, 0>(temp, row2);

    Mat3 mat;
    storeu_ps(column0, mat.Ptr() + 0);
    storeu_ps(column1, mat.Ptr() + 3);
    mat[2].Set(x_ps(column2), y_ps(column2), z_ps(column2));
    return mat;
#else
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
#endif
}

Mat4 Quat::ToMat4() const {
    return ToMat3().ToMat4();
}

CQuat Quat::ToCQuat() const {
    if (w < 0.0f) {
        return CQuat(-x, -y, -z);
    }
    return CQuat(x, y, z);
}

Vec3 Quat::ToAngularVelocity() const {
    return Vec3(x, y, z).Normalized() * (2.0f * Math::ACos(w));
}

BE_NAMESPACE_END
