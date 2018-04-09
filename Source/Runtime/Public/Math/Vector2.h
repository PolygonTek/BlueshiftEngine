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

    2D Vector

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/// A vector of form (x, y).
class BE_API Vec2 {
public:
    /// Specifies the number of elements in this vector.
    enum { Size = 2 };

    /// The default constructor does not initialize any members of this class.
    Vec2() = default;
    /// Constructs a Vec2 with the value (x, y).
    Vec2(float x, float y);
    /// Constructs a Vec2 from a C array, to the value (data[0], data[1]).
    explicit Vec2(const float data[2]);
    /// Constructs a Vec2 from a single value (s, s)
    explicit Vec2(float s);
    
                        /// Casts this Vec2 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&x; }
    float *             Ptr() { return (float *)&x; }
                        /// Casts this Vec2 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&x; }
                        operator float *() { return (float *)&x; }

                        /// Accesses an element of this vector.
    float &             At(int index) { return (*this)[index]; }
                        /// Accesses an element of this vector using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Performs an unary negation of this vector.
    Vec2                Negate() const { return Vec2(-x, -y); }
                        /// Performs an unary negation of this vector.
                        /// This function is identical to the member function Negate().
    Vec2                operator-() const { return Vec2(-x, -y); }

                        /// Returns Vec2(|x|, |y|)
    Vec2                Abs() const { return Vec2(Math::Fabs(x), Math::Fabs(y)); }

                        /// Unary operator + allows this structure to be used in an expression '+v'.
    Vec2                operator+() const { return *this; }

                        /// Adds a vector to this vector.
    Vec2                Add(const Vec2 &v) const { return *this + v; }
                        /// Adds a vector to this vector.
                        /// This function is identical to the member function Add().
    Vec2                operator+(const Vec2 &rhs) const { return Vec2(x + rhs.x, y + rhs.y); }

                        /// Adds the vector (s, s, s, s) to this vector.
    Vec2                AddScalar(float s) const { return *this + s; }
                        /// Adds the vector (s, s, s, s) to this vector.
                        /// This function is identical to the member function AddScalar().
    Vec2                operator+(float rhs) const { return Vec2(x + rhs, y + rhs); }
                        /// Adds the vector v to vector (s, s, s, s).
    friend Vec2         operator+(float lhs, const Vec2 &rhs) { return Vec2(lhs + rhs.x, lhs + rhs.y); }

                        /// Subtracts a vector from this vector.
    Vec2                Sub(const Vec2 &v) const { return *this - v; }
                        /// Subtracts the given vector from this vector.
                        /// This function is identical to the member function Sub()
    Vec2                operator-(const Vec2 &rhs) const { return Vec2(x - rhs.x, y - rhs.y); }

                        /// Subtracts the vector (s, s, s, s) from this vector.
    Vec2                SubScalar(float s) const { return *this - s; }
                        /// Subtracts the vector (s, s, s, s) from this vector.
                        /// This function is identical to the member function SubScalar()
    Vec2                operator-(float rhs) const { return Vec2(x - rhs, y - rhs); }
                        /// Subtracts the vector v from vector (s, s, s, s).
    friend Vec2         operator-(float lhs, const Vec2 &rhs) { return Vec2(lhs - rhs.x, lhs - rhs.y); }

                        /// Multiplies this vector by a scalar.
    Vec2                Mul(float s) const { return *this * s; }
                        /// Multiplies this vector by a scalar.
                        /// This function is identical to the member function Mul().
    Vec2                operator*(float rhs) const { return Vec2(x * rhs, y * rhs); }
                        /// Multiplies vector v by a scalar.
    friend Vec2         operator*(float lhs, const Vec2 &rhs) { return Vec2(lhs * rhs.x, lhs * rhs.y); }

                        /// Multiplies this vector by a vector, element-wise.
    Vec2                MulComp(const Vec2 &v) const { return *this * v; }
                        /// Multiplies this vector by a vector, element-wise.
                        /// This function is identical to the member function MulComp().
    Vec2                operator*(const Vec2 &rhs) const { return Vec2(x * rhs.x, y * rhs.y); }

                        /// Divides this vector by a scalar.
    Vec2                Div(float s) const { return *this / s; }
                        /// Divides this vector by a scalar.
                        /// This function is identical to the member function Div().
    Vec2                operator/(float rhs) const { float inv = 1.f / rhs; return Vec2(x * inv, y * inv); }

                        /// Divides this vector by a vector, element-wise.
    Vec2                DivComp(const Vec2 &v) const { return *this / v; }
                        /// This function is identical to the member function DivComp().
                        /// Divides this vector by a vector, element-wise.
    Vec2                operator/(const Vec2 &rhs) const { return Vec2(x / rhs.x, y / rhs.y); }
                        /// Divides vector (s, s, s, s) by a vector, element-wise.
    friend Vec2         operator/(float lhs, const Vec2 &rhs) { return Vec2(lhs / rhs.x, lhs / rhs.y); }


                        /// Assign from another vector.
    Vec2 &              operator=(const Vec2 &rhs);

                        /// Adds a vector to this vector, in-place.
    Vec2 &              AddSelf(const Vec2 &v) { *this += v; return *this; }
                        /// Adds a vector to this vector, in-place.
                        /// This function is identical to the member function AddSelf().
    Vec2 &              operator+=(const Vec2 &rhs);

                        /// Subtracts a vector from this vector, in-place.
    Vec2 &              SubSelf(const Vec2 &v) { *this -= v; return *this; }
                        /// Subtracts a vector from this vector, in-place.
                        /// This function is identical to the member function SubSelf().
    Vec2 &              operator-=(const Vec2 &rhs);

                        /// Multiplies this vector by a scalar, in-place.
    Vec2 &              MulSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this vector by a scalar, in-place.
                        /// This function is identical to the member function MulSelf().
    Vec2 &              operator*=(float rhs);

                        /// Multiplies this vector by a vector, element-wise, in-place.
    Vec2 &              MulCompSelf(const Vec2 &v) { *this *= v; return *this; }
                        /// Multiplies this vector by a vector, element-wise, in-place.
                        /// This function is identical to the member function MulCompSelf().
    Vec2 &              operator*=(const Vec2 &rhs);

                        /// Divides this vector by a scalar, in-place.
    Vec2 &              DivSelf(float s) { *this /= s; return *this; }
                        /// Divides this vector by a scalar, in-place.
                        /// This function is identical to the member function DivSelf().
    Vec2 &              operator/=(float rhs);

                        /// Divides this vector by a vector, element-wise, in-place.
    Vec2 &              DivCompSelf(const Vec2 &v) { *this /= v; return *this; }
                        /// Divides this vector by a vector, element-wise, in-place.
                        /// This function is identical to the member function DivCompSelf().
    Vec2 &              operator/=(const Vec2 &rhs);

                        /// Exact compare, no epsilon.
    bool                Equals(const Vec2 &a) const;
                        /// Compare with epsilon.
    bool                Equals(const Vec2 &a, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Vec2 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Vec2 &rhs) const { return !Equals(rhs); }

                        /// Tests if this is the zero vector, up to the given epsilon.
    bool                IsZero(const float epsilon = 0.0f) const;

                        /// Sets all element of this vector.
    void                Set(float x, float y);

                        /// Sets this Vec2 to (s, s).
    void                SetFromScalar(float s) { x = y = s; }
                        /// Returns Vec2(s, s).
    static Vec2         FromScalar(float s) { return Vec2(s, s); }

                        /// Get the minimum component of a vector.
    float               MinComponent() const { return Min(x, y); }
                        /// Get the maximum component of a vector.
    float               MaxComponent() const { return Max(x, y); }
                        /// Get the minimum component index of a vector.
    int                 MinComponentIndex() const { return MinIndex(x, y); }
                        /// Get the maximum component index of a vector.
    int                 MaxComponentIndex() const { return MaxIndex(x, y); }

                        /// Computes the length of this vector.
    float               Length() const;

                        /// Computes the squared length of this vector.
    float               LengthSqr() const;

                        /// Computes the distance between this point and the given vector.
    float               Distance(const Vec2 &v) const;

                        /// Computes the squared distance between this point and the given vector.
    float               DistanceSqr(const Vec2 &v) const;

                        /// Normalizes this vector.
                        /// @return Length of this vector.
    float               Normalize();
                        /// Normalizes this vector fast but approximately.
                        /// @return Length of this vector.
    float               NormalizeFast();

                        /// Scales this vector so that its new length is as given.
    Vec2 &              ScaleToLength(float length);

                        /// Clamp min <= this[i] <= max for each element.
    void                Clamp(const Vec2 &min, const Vec2 &max);

                        /// Computes the dot product of this and the given vector.
    float               Dot(const Vec2 &a) const;
                        /// Computes the absolute dot product of this and the given vector.
    float               AbsDot(const Vec2 &a) const;

                        /// Computes z-component of 3D vector cross product of this (x, y, 0) and the given vector (a.x, a.y, 0).
    float               Cross(const Vec2 &a) const;

                        /// Returns the angle between this vector and the specified vector, in radians.
    float               AngleBetween(const Vec2 &other) const;
                        /// Returns the angle between this vector and the specified normalized vector, in radians.
    float               AngleBetweenNorm(const Vec2 &other) const;

                        /// Sets from linear interpolation between vector v1 and the vector v2.
    void                SetFromLerp(const Vec2 &v1, const Vec2 &v2, float l);
                        /// Returns linear interpolation between the vector v1 and the vector v2.
    static Vec2         FromLerp(const Vec2 &v1, const Vec2 &v2, const float t);

                        /// Sets this vector on unit circle has uniform distribution with the given random variable u [0, 1].
    static Vec2         FromUniformSampleCircle(float u);

                        /// Sets this vector on unit disk (filled circle) has uniform distribution with the given random variable u1, u2 [0, 1].
    static Vec2         FromUniformSampleDisk(float u1, float u2);

                        /// Sets this vector on unit disk (filled circle) has uniform distribution with the given random variable u1, u2 [0, 1].
                        /// Less distorted mapping than FromUniformSampleDisk()
    static Vec2         FromConcentricSampleDisk(float u1, float u2);

                        /// Returns "x y z".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "x y z" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Vec2         FromString(const char *str);

                        /// Converts to the polar coordinates.
    float               ToPolar(float &theta) const;

                        /// Sets from the polar coordinates.
    void                SetFromPolar(float radius, float theta);

                        /// Returns radian [-PI, PI] as a direction vector.
    float               ToAngle() const;

                        /// Sets from the radian as a direction.
    void                SetFromAngle(float theta);

                        /// Sets this vector from the radian as a direction
    static Vec2         FromAngle(float theta);

                        /// Returns dimension of this type.
    int                 GetDimension() const { return Size; }

                        /// Compute 2D barycentric coordinates from the point based on 2 simplex vector.
    static const Vec2   Compute2DBarycentricCoords(const float s1, const float s2, const float p);

    static const Vec2   origin;     ///< (0, 0)
    static const Vec2   zero;       ///< (0, 0)
    static const Vec2   one;        ///< (1, 1)
    static const Vec2   unitX;      ///< (1, 0)
    static const Vec2   unitY;      ///< (0, 1)

    float               x;          ///< The x components.
    float               y;          ///< The y components.
};

BE_INLINE Vec2::Vec2(float x, float y) {
    this->x = x;
    this->y = y;
}

BE_INLINE Vec2::Vec2(const float data[2]) {
    this->x = data[0];
    this->y = data[1];
}

BE_INLINE Vec2::Vec2(float c) {
    this->x = c;
    this->y = c;
}

BE_INLINE void Vec2::Set(float x, float y) {
    this->x = x;
    this->y = y;
}

BE_INLINE bool Vec2::IsZero(const float epsilon) const {
    return Equals(Vec2::zero, epsilon);
}

BE_INLINE float Vec2::operator[](int index) const {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE float &Vec2::operator[](int index) {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE Vec2 &Vec2::operator=(const Vec2 &rhs) {
    x = rhs.x;
    y = rhs.y;
    return *this;
}

BE_INLINE Vec2 &Vec2::operator+=(const Vec2 &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

BE_INLINE Vec2 &Vec2::operator-=(const Vec2 &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

BE_INLINE Vec2 &Vec2::operator*=(float rhs) {
    x *= rhs;
    y *= rhs;
    return *this;
}

BE_INLINE Vec2 &Vec2::operator*=(const Vec2 &rhs) {
    x *= rhs.x;
    y *= rhs.y;
    return *this;
};

BE_INLINE Vec2 &Vec2::operator/=(float rhs) {
    float inv = 1.0f / rhs;
    x *= inv;
    y *= inv;
    return *this;
}

BE_INLINE Vec2 &Vec2::operator/=(const Vec2 &rhs) {
    x /= rhs.x;
    y /= rhs.y;
    return *this;
};

BE_INLINE bool Vec2::Equals(const Vec2 &a) const {
    return x != a.x || y != a.y ? false : true;
}

BE_INLINE bool Vec2::Equals(const Vec2 &a, const float epsilon) const {
    if (Math::Fabs(x - a.x) > epsilon) {
        return false;
    }
    if (Math::Fabs(y - a.y) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE float Vec2::Length() const {
    return (float)Math::Sqrt(x * x + y * y);
}

BE_INLINE float Vec2::LengthSqr() const {
    return x * x + y * y;
}

BE_INLINE float Vec2::Distance(const Vec2 &a) const {
    Vec2 v = *this - a;
    return v.Length();
}

BE_INLINE float Vec2::DistanceSqr(const Vec2 &a) const {
    Vec2 v = *this - a;
    return v.LengthSqr();
}

BE_INLINE float Vec2::Normalize() {
    float sqrLength = x * x + y * y;
    float invLength = Math::InvSqrt(sqrLength);

    x *= invLength;
    y *= invLength;

    return invLength * sqrLength;
}

BE_INLINE float Vec2::NormalizeFast() {
    float sqrLength = x * x + y * y;
    float invLength = Math::RSqrt(sqrLength);

    x *= invLength;
    y *= invLength;

    return invLength * sqrLength;
}

BE_INLINE Vec2 &Vec2::ScaleToLength(float length) {
    if (!length) {
        SetFromScalar(0);
    } else {
        float length2 = LengthSqr();
        float ilength = length * Math::InvSqrt(length2);
        x *= ilength;
        y *= ilength;
    }

    return *this;
}

BE_INLINE void Vec2::Clamp(const Vec2 &min, const Vec2 &max) {
    if (x < min.x) {
        x = min.x;
    } else if (x > max.x) {
        x = max.x;
    }

    if (y < min.y) {
        y = min.y;
    } else if (y > max.y) {
        y = max.y;
    }
}

BE_INLINE float Vec2::Dot(const Vec2 &a) const {
    return x * a.x + y * a.y;
}

BE_INLINE float Vec2::AbsDot(const Vec2 &a) const {
    return Math::Fabs(x * a.x) + Math::Fabs(y * a.y);
}

BE_INLINE float Vec2::Cross(const Vec2 &a) const {
    return x * a.y - y * a.x;
}

BE_INLINE float Vec2::AngleBetween(const Vec2 &other) const {
    float cosa = Dot(other) / Math::Sqrt(LengthSqr() * other.LengthSqr());
    if (cosa >= 1.f) {
        return 0.f;
    } else if (cosa <= -1.f) {
        return Math::Pi;
    } else {
        return Math::ACos(cosa);
    }
}

BE_INLINE float Vec2::AngleBetweenNorm(const Vec2 &other) const {
    float cosa = Dot(other);
    if (cosa >= 1.f) {
        return 0.f;
    } else if (cosa <= -1.f) {
        return Math::Pi;
    } else {
        return Math::ACos(cosa);
    }
}

BE_INLINE void Vec2::SetFromLerp(const Vec2 &v1, const Vec2 &v2, const float l) {
    if (l <= 0.0f) {
        (*this) = v1;
    } else if (l >= 1.0f) {
        (*this) = v2;
    } else {
        (*this) = v1 + l * (v2 - v1);
    }
}

BE_INLINE Vec2 Vec2::FromLerp(const Vec2 &v1, const Vec2 &v2, const float t) {
    Vec2 v;
    v.SetFromLerp(v1, v2, t);
    return v;
}

BE_INLINE Vec2 Vec2::FromUniformSampleCircle(float u) {
    float s, c;
    Math::SinCos(Math::TwoPi * u, s, c);
    return Vec2(c, s);
}

BE_INLINE Vec2 Vec2::FromAngle(float theta) {
    Vec2 v;
    v.SetFromAngle(theta);
    return v;
}

BE_INLINE const char *Vec2::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Size, precision);
}

BE_INLINE void Vec2::SetFromPolar(float radius, float theta) {
    float sinTheta, cosTheta;
    Math::SinCos(theta, sinTheta, cosTheta);

    x = radius * cosTheta;
    y = radius * sinTheta;
}

BE_INLINE float Vec2::ToPolar(float &theta) const {
    float radius = Math::Sqrt(x * x + y * y);
    theta = Math::ATan(y, x);
    return radius;
}

BE_INLINE void Vec2::SetFromAngle(float theta) {
    float sinTheta, cosTheta;
    Math::SinCos(theta, sinTheta, cosTheta);

    x = cosTheta;
    y = sinTheta;
}

BE_INLINE float Vec2::ToAngle() const {
    return Math::ATan(y, x);
}

BE_NAMESPACE_END
