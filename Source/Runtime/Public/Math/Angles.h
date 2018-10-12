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

/// Euler angles - roll, pitch, yaw
class BE_API Angles {
public:
    /// Specifies the number of elements in this class.
    enum { Size = 3 };

    /// Angles indexes
    enum Index {
        Roll,
        Pitch,
        Yaw
    };

    /// The default constructor does not initialize any members of this class.
    Angles() = default;
    /// Constructs a Angles with the value (roll, pitch, yaw).
    Angles(float roll, float pitch, float yaw);
    /// Copy constructor
    explicit Angles(const Vec3 &v);
    /// Assignment operator
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

    Angles &            operator+=(const Angles &rhs);
    Angles &            operator-=(const Angles &rhs);
    Angles &            operator*=(const float rhs);
    Angles &            operator/=(const float rhs);

                        /// Negate angles, in general not the inverse rotation
    Angles              operator-() const { return Angles(-roll , -pitch, -yaw); }

    Angles              operator+(const Angles &rhs) const { return Angles(roll + rhs.roll, pitch + rhs.pitch, yaw + rhs.yaw); }
    Angles              operator-(const Angles &rhs) const { return Angles(roll - rhs.roll, pitch - rhs.pitch, yaw - rhs.yaw); }
    Angles              operator*(const float rhs) const { return Angles(roll * rhs, pitch * rhs, yaw * rhs); }
    
    friend Angles       operator*(const float lhs, const Angles &rhs) { return Angles(lhs * rhs.roll, lhs * rhs.pitch, lhs * rhs.yaw); }
    Angles              operator/(const float rhs) const;
    
                        /// Exact compare, no epsilon
    bool                Equals(const Angles &a) const { return ((a.roll == roll) && (a.pitch == pitch) && (a.yaw == yaw)); }
                        /// Compare with epsilon
    bool                Equals(const Angles &a, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Angles &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Angles &rhs) const { return !Equals(rhs); }

    float               GetRoll() const { return roll; }
    float               GetPitch() const { return pitch; }
    float               GetYaw() const { return yaw; }

    void                Set(float roll, float pitch, float yaw);
    void                SetRoll(float roll) { this->roll = roll; }
    void                SetPitch(float pitch) { this->pitch = pitch; }
    void                SetYaw(float yaw) { this->yaw = yaw; }
    Angles &            SetZero();

                        /// Normalize to the range [0 <= angle < 360]
    Angles &            Normalize360();
                        /// Normalize to the range [-180 < angle <= 180]
    Angles &            Normalize180();

    void                Clamp(const Angles &min, const Angles &max);

                        /// Sets from linear interpolation between the angles a1 and the angles a2.
    void                SetFromLerp(const Angles &a1, const Angles &a2, const float t);
                        /// Returns linear interpolation between the angles a1 and the angles a2.
    static Angles       FromLerp(const Angles &a1, const Angles &a2, const float t);

    void                ToVectors(Vec3 *forward, Vec3 *right = nullptr, Vec3 *up = nullptr) const;
    Vec3                ToForward() const;
    Vec3                ToRight() const;
    Vec3                ToUp() const;

    Rotation            ToRotation() const;
    Quat                ToQuat() const;
    Mat3                ToMat3() const;
    Mat4                ToMat4() const;

                        /// Returns "yaw pitch roll".
    const char *        ToString() const { return ToString(2); }
                        /// Returns "yaw pitch roll" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Angles       FromString(const char *str);

                        /// Returns dimension of this type
    int                 GetDimension() const { return Size; }

    static const Angles zero;

    float               roll;       ///< Angle of rotation around forward axis in degrees
    float               pitch;      ///< Angle of rotation around left axis in degrees
    float               yaw;        ///< Angle of rotation around up axis in degrees
};

BE_INLINE Angles::Angles(float roll, float pitch, float yaw) {
    this->roll = roll;
    this->pitch = pitch;
    this->yaw = yaw;
}

BE_INLINE Angles::Angles(const Vec3 &v) {
    this->roll = v[0];
    this->pitch = v[1];
    this->yaw = v[2];
}

BE_INLINE Angles &Angles::operator=(const Angles &a) {
    roll = a.roll;
    pitch = a.pitch;
    yaw = a.yaw;
    return *this;
}

BE_INLINE void Angles::Set(float roll, float pitch, float yaw) {
    this->roll = roll;
    this->pitch = pitch;
    this->yaw = yaw;
}

BE_INLINE Angles &Angles::SetZero() {
    roll = pitch = pitch = 0.0f;
    return *this;
}

BE_INLINE const char *Angles::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Size, precision);
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
    roll += a.roll;
    pitch += a.pitch;
    yaw += a.yaw;
    return *this;
}

BE_INLINE Angles &Angles::operator-=(const Angles &a) {
    roll -= a.roll;
    pitch -= a.pitch;
    yaw -= a.yaw;
    return *this;
}

BE_INLINE Angles &Angles::operator*=(float a) {
    roll *= a;
    pitch *= a;
    yaw *= a;
    return *this;
}

BE_INLINE Angles Angles::operator/(const float a) const {
    float invA = 1.0f / a;
    return Angles(roll * invA, pitch * invA, yaw * invA);
}

BE_INLINE Angles &Angles::operator/=(float a) {
    float invA = 1.0f / a;
    roll *= invA;
    pitch *= invA;
    yaw *= invA;
    return *this;
}

BE_INLINE bool Angles::Equals(const Angles &a, const float epsilon) const {
    if (Math::Fabs(roll - a.roll) > epsilon) {
        return false;
    }
    if (Math::Fabs(pitch - a.pitch) > epsilon) {
        return false;
    }
    if (Math::Fabs(yaw - a.yaw) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE void Angles::Clamp(const Angles &min, const Angles &max) {
    BE1::Clamp(roll, min.roll, max.roll);
    BE1::Clamp(pitch, min.pitch, max.pitch);
    BE1::Clamp(yaw, min.yaw, max.yaw);
}

BE_INLINE void Angles::SetFromLerp(const Angles &a1, const Angles &a2, const float t) {
    if (t <= 0.0f) {
        (*this) = a1;
    } else if (t >= 1.0f) {
        (*this) = a2;
    } else {
        roll = a1.roll + Math::AngleDelta(a2.roll, a1.roll) * t;
        pitch = a1.pitch + Math::AngleDelta(a2.pitch, a1.pitch) * t;
        yaw = a1.yaw + Math::AngleDelta(a2.yaw, a1.yaw) * t;
    }
}

BE_INLINE Angles Angles::FromLerp(const Angles &a1, const Angles &a2, const float t) {
    Angles a;
    a.SetFromLerp(a1, a2, t);
    return a;
}

BE_NAMESPACE_END
