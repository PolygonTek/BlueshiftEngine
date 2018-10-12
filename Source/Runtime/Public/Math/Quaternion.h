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

#pragma once

/*
-------------------------------------------------------------------------------

    Quaternion

    i^2 = j^2 = k^2 = i*j*k = -1
    i*j = -j*i = k
    j*k = -k*j = i
    k*i = -i*k = j

    qx = ax * sin(angle/2)
    qy = ay * sin(angle/2)
    qz = az * sin(angle/2)
    qw = cos(angle/2)

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class Vec3;
class Mat3;
class Mat4;
class Angles;
class Rotation;
class CQuat;

/// Rotation represented as a four-dimensional normalized vector.
class BE_API Quat {
public:
    /// The default constructor does not initialize any members of this class.
    Quat() = default;
    /// Constructs a Quat with the value (x, y, z, w).
    Quat(float x, float y, float z, float w);
    /// Constructs a Quat from a C array, to the value (data[0], data[1], data[2], data[3]).
    explicit Quat(const float data[4]);
    /// Assignment operator
    Quat &operator=(const Quat &rhs);
    
                        /// Casts this Quat to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&x; }
    float *             Ptr() { return (float *)&x; }
                        /// Casts this Quat to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const;
                        operator float *();

                        /// Accesses an element of this quaternion.
    float &             At(int index) { return (*this)[index]; }
                        /// Accesses an element of this quaternion using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Negates the quaternion.
                        /// @note Negating a quaternion will not produce the inverse rotation. Call Quat::Inverse() to generate the inverse rotation.
    Quat                operator-() const;

                        /// Adds two quaternions.
                        /// @note Adding two quaternions does not concatenate the two rotation operations. Use quaternion multiplication to achieve that.
    Quat                operator+(const Quat &rhs) const;
                        
                        /// Subtracts the quaternion from this quternion.
    Quat                operator-(const Quat &rhs) const;

                        /// Multiplies a quaternion by a scalar.
    Quat                operator*(float rhs) const;
    friend Quat         operator*(const float lhs, const Quat &rhs);

                        /// Multiplies two quaternions together.
                        /// The product q1 * q2 returns a quaternion that concatenates the two orientation rotations. The rotation
                        /// q2 is applied first before q1.
    Quat                operator*(const Quat &rhs) const;

                        /// Transforms the given vector by this Quaternion.
                        /// @note Technically, this function does not perform a simple multiplication of 'q * v',
                        /// but instead performs a conjugation operation 'q * v * q^{-1}'. This corresponds to transforming
                        /// the given vector by this Quaternion.
    Vec3                operator*(const Vec3 &rhs) const;
    friend Vec3         operator*(const Vec3 &lhs, const Quat &rhs);

                        /// Adds a quaternion to this quaternion, in-place.
    Quat &              operator+=(const Quat &rhs);

                        /// Subtracts a quaternion to this quaternion, in-place.
    Quat &              operator-=(const Quat &rhs);

                        /// Multiplies a quaternion by a scalar, in-place.
    Quat &              operator*=(float rhs);

                        /// Multiplies two quaternions, in-place.
    Quat &              operator*=(const Quat &rhs);

                        /// Exact compare, no epsilon
    bool                Equals(const Quat &a) const;
                        /// Compare with epsilon
    bool                Equals(const Quat &a, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Quat &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Quat &rhs) const { return !Equals(rhs); }

                        /// Sets all elements of this quaternion.
    void                Set(float x, float y, float z, float w);

                        /// Sets identity quaternion (0, 0, 0, 1)
    void                SetIdentity();

    float               CalcW() const;

                        // Compute square root of norm
    float               Length();

    float               LengthSqr();

                        /// Normalizes this quaternion in-place.
    Quat &              Normalize();

                        /// Inverts this quaternion.
    Quat                Inverse() const;
                        /// Inverts this quaternion, in-place.
    Quat &              InverseSelf();

                        /// Sets this quaternion by specifying the axis about which the rotation is performed, and the angle of rotation.
    Quat &              SetFromAngleAxis(float angle, const Vec3 &axis);
                        /// Returns the quaternion with the given angle and axis.
    static Quat         FromAngleAxis(float angle, const Vec3 &axis);
                        /// Sets this quaternion by specifying the Euler angles.
    Quat &              SetFromAngles(const Angles &angles);
                        /// Returns the quaternion with the given Euler angles.
    static Quat         FromAngles(const Angles &angles);

                        /// Compute a quaternion that rotates unit-length vector "from" to unit-length vector "to".
    Quat &              SetFromTwoVectors(const Vec3 &from, const Vec3 &to);
                        /// Returns the quaternion that rotates unit-length vector "from" to unit-length vector "to".
    static Quat         FromTwoVectors(const Vec3 &from, const Vec3 &to);

    Quat &              SetFromSlerp(const Quat &from, const Quat &to, float t);
    Quat &              SetFromSlerpFast(const Quat &from, const Quat &to, float t);

    static Quat         FromSlerp(const Quat &from, const Quat &to, float t);
    static Quat         FromSlerpFast(const Quat &from, const Quat &to, float t);

    Angles              ToAngles() const;
    Rotation            ToRotation() const;
    Mat3                ToMat3() const;
    Mat4                ToMat4() const;
    CQuat               ToCQuat() const;
    Vec3                ToAngularVelocity() const;
                        /// Returns "x y z w".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "x y z w" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Quat         FromString(const char *str);

                        /// Returns dimension of this type
    int                 GetDimension() const { return 4; }

    static const Quat   identity;
    
    float               x;          ///< The factor of i.
    float               y;          ///< The factor of j.
    float               z;          ///< The factor of k.
    float               w;          ///< The scalar part.
};

BE_INLINE Quat::Quat(float x, float y, float z, float w) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

BE_INLINE Quat::Quat(const float data[4]) {
    this->x = data[0];
    this->y = data[1];
    this->z = data[2];
    this->w = data[3];
}

BE_INLINE void Quat::Set(float x, float y, float z, float w) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

BE_INLINE Quat::operator const float *() const { 
    return (const float *)&x;
}

BE_INLINE Quat::operator float *() { 
    return (float *)&x;
}

BE_INLINE float Quat::operator[](int index) const {
    assert(index >= 0 && index < 4);
    return (&x)[index];
}

BE_INLINE float &Quat::operator[](int index) {
    assert(index >= 0 && index < 4);
    return (&x)[index];
}

BE_INLINE Quat Quat::operator-() const {
    return Quat(-x, -y, -z, -w);
}

BE_INLINE Quat Quat::operator+(const Quat &a) const {
    return Quat(x + a.x, y + a.y, z + a.z, w + a.w);
}

BE_INLINE Quat &Quat::operator=(const Quat &rhs) {
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
    w = rhs.w;
    return *this;
}

BE_INLINE Quat& Quat::operator+=(const Quat &a) {
    x += a.x;
    y += a.y;
    z += a.z;
    w += a.w;
    return *this;
}

BE_INLINE Quat Quat::operator-(const Quat &a) const {
    return Quat(x - a.x, y - a.y, z - a.z, w - a.w);
}

BE_INLINE Quat& Quat::operator-=(const Quat &a) {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    w -= a.w;
    return *this;
}

BE_INLINE Quat Quat::operator*(const Quat &a) const {
    return Quat(
        w * a.x + x * a.w + y * a.z - z * a.y,
        w * a.y + y * a.w + z * a.x - x * a.z,
        w * a.z + z * a.w + x * a.y - y * a.x,
        w * a.w - x * a.x - y * a.y - z * a.z);
}

BE_INLINE Vec3 Quat::operator*(const Vec3 &a) const {
    return ToMat3() * a;
}

BE_INLINE Quat Quat::operator*(float a) const {
    return Quat(x * a, y * a, z * a, w * a);
}

BE_INLINE Quat operator*(const float a, const Quat &b) {
    return b * a;
}

BE_INLINE Vec3 operator*(const Vec3 &a, const Quat &b) {
    return b * a;
}

BE_INLINE Quat& Quat::operator*=(const Quat &a) {
    *this = *this * a;
    return *this;
}

BE_INLINE Quat& Quat::operator*=(float a) {
    x *= a;
    y *= a;
    z *= a;
    w *= a;
    return *this;
}

BE_INLINE bool Quat::Equals(const Quat &a) const {
    return ((x == a.x) && (y == a.y) && (z == a.z) && (w == a.w));
}

BE_INLINE bool Quat::Equals(const Quat &a, const float epsilon) const {
    if (Math::Fabs(x - a.x) > epsilon) {
        return false;
    }
    if (Math::Fabs(y - a.y) > epsilon) {
        return false;
    }
    if (Math::Fabs(z - a.z) > epsilon) {
        return false;
    }
    if (Math::Fabs(w - a.w) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE float Quat::CalcW() const {
    // take the absolute value because floating point rounding may cause the dot of x, y, z to be larger than 1
    return sqrt(fabs(1.0f - (x * x + y * y + z * z)));
}

BE_INLINE float Quat::Length() {
    return Math::Sqrt(x * x + y * y + z * z + w * w);
}

BE_INLINE float Quat::LengthSqr() {
    return x * x + y * y + z * z + w * w;
}

BE_INLINE Quat &Quat::Normalize() {
    float len = Length();
    if (len) {
        float invlength = 1 / len;
        x *= invlength;
        y *= invlength;
        z *= invlength;
        w *= invlength;
    }
    return *this;
}

BE_INLINE void Quat::SetIdentity() {
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
    w = 1.0f;
}

BE_INLINE Quat Quat::FromAngleAxis(float angle, const Vec3 &axis) {
    Quat q;
    q.SetFromAngleAxis(angle, axis);
    return q;
}

BE_INLINE Quat Quat::FromAngles(const Angles &angles) {
    Quat q;
    q.SetFromAngles(angles);
    return q;
}

BE_INLINE Quat Quat::FromTwoVectors(const Vec3 &from, const Vec3 &to) { 
    Quat q;
    q.SetFromTwoVectors(from, to); 
    return q; 
}

BE_INLINE Quat Quat::FromSlerp(const Quat &from, const Quat &to, float t) {
    Quat q;
    q.SetFromSlerp(from, to, t);
    return q;
}

BE_INLINE Quat Quat::FromSlerpFast(const Quat &from, const Quat &to, float t) {
    Quat q;
    q.SetFromSlerpFast(from, to, t);
    return q;
}

BE_INLINE Quat Quat::Inverse() const {
    // 켤레 사원수 (conjugate quaternion)
    // q  = w + xi + yj + zk
    // q' = w - xi - yj - zk
    return Quat(-x, -y, -z, w);
}

BE_INLINE Quat &Quat::InverseSelf() {
    x = -x;
    y = -y;
    z = -z;
    return *this;
}

BE_INLINE const char *Quat::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), 4, precision);
}

BE_NAMESPACE_END
