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

/// Euler angles - yaw, pitch, roll
class BE_API Angles {
public:
    /// Specifies the number of elements in this class.
    enum { Size = 3 };

    /// Angles indexes
    enum Index {
        Yaw,
        Pitch,
        Roll
    };

    /// The default constructor does not initialize any members of this class.
    Angles() = default;
    /// Constructs a Angles with the value (yaw, pitch, roll).
    Angles(float yaw, float pitch, float roll);
    /// Copy constructor
    explicit Angles(const Vec3 &v);
    /// Assignment operator
    Angles &operator=(const Angles &rhs);

                        /// Casts this Angles to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&yaw; }
    float *             Ptr() { return (float *)&yaw; }
                        /// Casts this Angles to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&yaw; }
                        operator float *() { return (float *)&yaw; }

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
    Angles              operator-() const { return Angles(-yaw, -pitch, -roll); }

    Angles              operator+(const Angles &rhs) const { return Angles(yaw + rhs.yaw, pitch + rhs.pitch, roll + rhs.roll); }
    Angles              operator-(const Angles &rhs) const { return Angles(yaw - rhs.yaw, pitch - rhs.pitch, roll - rhs.roll); }
    Angles              operator*(const float rhs) const { return Angles(yaw * rhs, pitch * rhs, roll * rhs); }
    
    friend Angles       operator*(const float lhs, const Angles &rhs) { return Angles(lhs * rhs.yaw, lhs * rhs.pitch, lhs * rhs.roll); }
    Angles              operator/(const float rhs) const;
    
                        /// Exact compare, no epsilon
    bool                Equals(const Angles &a) const { return ((a.yaw == yaw) && (a.pitch == pitch) && (a.roll == roll)); }
                        /// Compare with epsilon
    bool                Equals(const Angles &a, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Angles &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Angles &rhs) const { return !Equals(rhs); }

    void                Set(float yaw, float pitch, float roll);
    void                SetYaw(float yaw) { this->yaw = yaw; }
    void                SetPitch(float pitch) { this->pitch = pitch; }
    void                SetRoll(float roll) { this->roll = roll; }
    Angles &            SetZero();

                        /// Normalize to the range [0 <= angle < 360]
    Angles &            Normalize360();
                        /// Normalize to the range [-180 < angle <= 180]
    Angles &            Normalize180();

    void                Clamp(const Angles &min, const Angles &max);

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

    float               yaw;
    float               pitch;
    float               roll;
};

BE_INLINE Angles::Angles(float yaw, float pitch, float roll) {
    this->yaw = yaw;
    this->pitch = pitch;
    this->roll = roll;
}

BE_INLINE Angles::Angles(const Vec3 &v) {
    this->yaw = v[0];
    this->pitch = v[1];
    this->roll = v[2];
}

BE_INLINE Angles &Angles::operator=(const Angles &a) {
    yaw = a.yaw;
    pitch = a.pitch;
    roll = a.roll;
    return *this;
}

BE_INLINE void Angles::Set(float yaw, float pitch, float roll) {
    this->yaw = yaw;
    this->pitch = pitch;
    this->roll = roll;
}

BE_INLINE Angles &Angles::SetZero() {
    yaw = pitch = roll = 0.0f;
    return *this;
}

BE_INLINE const char *Angles::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Size, precision);
}

BE_INLINE float Angles::operator[](int index) const {
    assert((index >= 0) && (index < Size));
    return (&yaw)[index];
}

BE_INLINE float &Angles::operator[](int index) {
    assert((index >= 0) && (index < Size));
    return (&yaw)[index];
}

BE_INLINE Angles &Angles::operator+=(const Angles &a) {
    yaw += a.yaw;
    pitch += a.pitch;
    roll += a.roll;
    return *this;
}

BE_INLINE Angles &Angles::operator-=(const Angles &a) {
    yaw -= a.yaw;
    pitch -= a.pitch;
    roll -= a.roll;
    return *this;
}

BE_INLINE Angles &Angles::operator*=(float a) {
    yaw *= a;
    pitch *= a;
    roll *= a;
    return *this;
}

BE_INLINE Angles Angles::operator/(const float a) const {
    float inva = 1.0f / a;
    return Angles(yaw * inva, pitch * inva, roll * inva);
}

BE_INLINE Angles &Angles::operator/=(float a) {
    float inva = 1.0f / a;
    yaw *= inva;
    pitch *= inva;
    roll *= inva;
    return *this;
}

BE_INLINE bool Angles::Equals(const Angles &a, const float epsilon) const {
    if (Math::Fabs(yaw - a.yaw) > epsilon) {
        return false;
    }
    if (Math::Fabs(pitch - a.pitch) > epsilon) {
        return false;
    }            
    if (Math::Fabs(roll - a.roll) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE void Angles::Clamp(const Angles &min, const Angles &max) {
    BE1::Clamp(yaw,   min.yaw,   max.yaw);
    BE1::Clamp(pitch, min.pitch, max.pitch);
    BE1::Clamp(roll,  min.roll,  max.roll);
}

BE_NAMESPACE_END
