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

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class Vec3;
class Mat3;
class Mat4;
class Quat;
class Rotation;

/// Euler angles
class BE_API Angles {
public:
    /// Specifies the number of elements in this class.
    enum { Size = 3 };

    /// Angles indexes
    struct Index {
        enum Enum {
            Roll,       ///< Rotation angle around x-axis
            Pitch,      ///< Rotation angle around y-axis
            Yaw         ///< Rotation angle around z-axis
        };
    };

    /// The default constructor does not initialize any members of this class.
    Angles() = default;
    /// Constructs a Angles with the value (x, y, z).
    explicit constexpr Angles(float x, float y, float z);
    /// Copy constructor.
    explicit constexpr Angles(const Vec3 &v);
    /// Assignment operator.
    Angles &operator=(const Angles &rhs);

                        /// Casts this Angles to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)this; }
    float *             Ptr() { return (float *)this; }
                        /// Casts this Angles to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)this; }
                        operator float *() { return (float *)this; }

                        /// Accesses an element.
    float &             At(int index) { return (*this)[index]; }
                        /// Accesses an element using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Adds a angles to this angles, in-place.
    Angles &            operator+=(const Angles &rhs);

                        /// Subtracts a angles to this angles, in-place.
    Angles &            operator-=(const Angles &rhs);

                        /// Multiplies this angles by a scalar, in-place.
    Angles &            operator*=(const float rhs);

                        /// Divides this angles by a scalar, in-place.
    Angles &            operator/=(const float rhs);

                        /// Negate angles, in general not the inverse rotation.
    Angles              operator-() const { return Angles(-x , -y, -z); }

                        /// Adds a angles from this angles.
    Angles              operator+(const Angles &rhs) const { return Angles(x + rhs.x, y + rhs.y, z + rhs.z); }

                        /// Subtracts a angles from this angles.
    Angles              operator-(const Angles &rhs) const { return Angles(x - rhs.x, y - rhs.y, z - rhs.z); }

                        /// Multiplies this angles by a scalar.
    Angles              operator*(const float rhs) const { return Angles(x * rhs, y * rhs, z * rhs); }
    
                        /// Multiplies angles by a scalar.
    friend Angles       operator*(const float lhs, const Angles &rhs) { return Angles(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z); }

                        /// Divides angles by a scalar.
    Angles              operator/(const float rhs) const;
    
                        /// Exact compare, no epsilon.
    bool                Equals(const Angles &a) const { return ((a.x == x) && (a.y == y) && (a.z == z)); }
                        /// Compare with epsilon.
    bool                Equals(const Angles &a, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Angles &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Angles &rhs) const { return !Equals(rhs); }

                        /// Returns roll.
    constexpr float     GetRoll() const { return x; }
                        /// Returns pitch.
    constexpr float     GetPitch() const { return y; }
                        /// Returns yaw.
    constexpr float     GetYaw() const { return z; }

    void                SetRoll(float roll) { this->x = roll; }
    void                SetPitch(float pitch) { this->y = pitch; }
    void                SetYaw(float yaw) { this->z = yaw; }

    void                Set(float x, float y, float z);

    Angles &            SetZero();

                        /// Normalize to the range [0 <= angle < 360]
    Angles &            Normalize360();
                        /// Normalize to the range [-180 < angle <= 180]
    Angles &            Normalize180();

                        /// Clamp Euler angles between min and max.
    void                Clamp(const Angles &min, const Angles &max);

                        /// Sets from linear interpolation between the angles a1 and the angles a2.
    void                SetFromLerp(const Angles &a1, const Angles &a2, const float t);
                        /// Returns linear interpolation between the angles a1 and the angles a2.
    static Angles       FromLerp(const Angles &a1, const Angles &a2, const float t);

                        /// Returns rotated three axes from concatenated rotation matrix R = Rz Ry Rx.
    void                ToVectors(Vec3 *xAxis, Vec3 *yAxis = nullptr, Vec3 *zAxis = nullptr) const;
                        /// Returns rotated x-axis from concatenated rotation matrix R = Rz Ry Rx.
    Vec3                ToXAxis() const;
                        /// Returns rotated y-axis from concatenated rotation matrix R = Rz Ry Rx.
    Vec3                ToYAxis() const;
                        /// Returns rotated z-axis from concatenated rotation matrix R = Rz Ry Rx.
    Vec3                ToZAxis() const;

    Rotation            ToRotation() const;
    Quat                ToQuat() const;
    Mat3                ToMat3() const;
    Mat4                ToMat4() const;

                        /// Returns "x y z".
    const char *        ToString() const { return ToString(2); }
                        /// Returns "x y z" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Angles       FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return Size; }

    static const Angles zero;

    float               x;      ///< Angle of rotation around x (forward) axis in degrees (roll)
    float               y;      ///< Angle of rotation around y (left) axis in degrees (pitch)
    float               z;      ///< Angle of rotation around z (up) axis in degrees (yaw)
};

BE_INLINE constexpr Angles::Angles(float inX, float inY, float inZ) :
    x(inX), y(inY), z(inZ) {
}

BE_INLINE constexpr Angles::Angles(const Vec3 &v) :
    x(v.x), y(v.y), z(v.z) {
}

BE_INLINE Angles &Angles::operator=(const Angles &a) {
    x = a.x;
    y = a.y;
    z = a.z;
    return *this;
}

BE_INLINE void Angles::Set(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

BE_INLINE Angles &Angles::SetZero() {
    x = y = z = 0.0f;
    return *this;
}

BE_INLINE const char *Angles::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_INLINE float Angles::operator[](int index) const {
    assert((index >= 0) && (index < Size));
    return ((float *)this)[index];
}

BE_INLINE float &Angles::operator[](int index) {
    assert((index >= 0) && (index < Size));
    return ((float *)this)[index];
}

BE_INLINE Angles &Angles::operator+=(const Angles &a) {
    x += a.x;
    y += a.y;
    z += a.z;
    return *this;
}

BE_INLINE Angles &Angles::operator-=(const Angles &a) {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    return *this;
}

BE_INLINE Angles &Angles::operator*=(float a) {
    x *= a;
    y *= a;
    z *= a;
    return *this;
}

BE_INLINE Angles Angles::operator/(const float a) const {
    float invA = 1.0f / a;
    return Angles(x * invA, y * invA, z * invA);
}

BE_INLINE Angles &Angles::operator/=(float a) {
    float invA = 1.0f / a;
    x *= invA;
    y *= invA;
    z *= invA;
    return *this;
}

BE_INLINE bool Angles::Equals(const Angles &a, const float epsilon) const {
    if (Math::Fabs(x - a.x) > epsilon) {
        return false;
    }
    if (Math::Fabs(y - a.y) > epsilon) {
        return false;
    }
    if (Math::Fabs(z - a.z) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE void Angles::Clamp(const Angles &min, const Angles &max) {
    BE1::Clamp(x, min.x, max.x);
    BE1::Clamp(y, min.y, max.y);
    BE1::Clamp(z, min.z, max.z);
}

BE_INLINE void Angles::SetFromLerp(const Angles &a1, const Angles &a2, const float t) {
    if (t <= 0.0f) {
        (*this) = a1;
    } else if (t >= 1.0f) {
        (*this) = a2;
    } else {
        x = a1.x + Math::AngleNormalize180(a2.x - a1.x) * t;
        y = a1.y + Math::AngleNormalize180(a2.y - a1.y) * t;
        z = a1.z + Math::AngleNormalize180(a2.z - a1.z) * t;
    }
}

BE_INLINE Angles Angles::FromLerp(const Angles &a1, const Angles &a2, const float t) {
    Angles a;
    a.SetFromLerp(a1, a2, t);
    return a;
}

BE_NAMESPACE_END
