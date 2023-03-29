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

    4D Vector

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/// A vector of form (x, y, z, w).
class BE_API Vec4 {
public:
    /// Specifies the number of elements in this vector.
    enum { Size = 4 };

    /// The default constructor does not initialize any members of this class.
    Vec4() = default;
    /// Constructs a Vec4 with the value (x, y, z, w).
    constexpr Vec4(float x, float y, float z, float w);
    /// Constructs a Vec4 with the value (xyz[0], xyz[1], xyz[2], w).
    constexpr Vec4(const Vec3 &xyz, float w);
    /// Constructs a Vec4 with the value (xy[0], xy[1], zw[0], zw[1]).
    constexpr Vec4(const Vec2 &xy, const Vec2 &zw);
    /// Constructs a Vec4 from a C array, to the value (data[0], data[1], data[2], data[3]).
    explicit constexpr Vec4(const float data[4]);
    /// Constructs a Vec4 from a single value (s, s, s, s)
    explicit constexpr Vec4(float s);

                        /// Casts this Vec4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&x; }
    float *             Ptr() { return (float *)&x; }
                        /// Casts this Vec4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&x; }
                        operator float *() { return (float *)&x; }

                        /// Accesses an element of this vector.
    float &             At(int index) { return (*this)[index]; }
                        /// Accesses an element of this vector using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Unary operator + allows this structure to be used in an expression '+v'.
    const Vec4 &        operator+() const { return *this; }

                        /// Performs an unary negation of this vector.
    Vec4                Negate() const & { return Vec4(-x, -y, -z, -w); }
    Vec4 &&             Negate() && { x = -x; y = -y; z = -z; w = -w; return std::move(*this); }
                        /// Performs an unary negation of this vector.
                        /// This function is identical to the member function Negate().
    Vec4                operator-() const & { return Vec4(-x, -y, -z, -w); }
    Vec4 &&             operator-() && { x = -x; y = -y; z = -z; w = -w; return std::move(*this); }

                        /// return Vec4(|x|, |y|, |z|, |w|)
    Vec4                Abs() const & { return Vec4(Math::Fabs(x), Math::Fabs(y), Math::Fabs(z), Math::Fabs(w)); }
    Vec4 &&             Abs() && { x = Math::Fabs(x); y = Math::Fabs(y); z = Math::Fabs(z); w = Math::Fabs(w); return std::move(*this); }

                        /// Adds a vector to this vector.
    Vec4                Add(const Vec4 &v) const & { return *this + v; }
    Vec4 &&             Add(const Vec4 &v) && { x += v.x; y += v.y; z += v.z; w += v.w; return std::move(*this); }
                        /// Adds a vector to this vector.
                        /// This function is identical to the member function Add().
    Vec4                operator+(const Vec4 &rhs) const & { return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
    Vec4 &&             operator+(const Vec4 &rhs) && { x += rhs.x; y += rhs.y; z += rhs.z; z += rhs.z; return std::move(*this); }

                        /// Adds the vector (s, s, s, s) to this vector.
    Vec4                AddScalar(float s) const & { return *this + s; }
    Vec4 &&             AddScalar(float s) && { x += s; y += s; z += s; w += s; return std::move(*this); }
                        /// Adds the vector (s, s, s, s) to this vector.
                        /// This function is identical to the member function AddScalar().
    Vec4                operator+(float rhs) const & { return Vec4(x + rhs, y + rhs, z + rhs, w + rhs); }
    Vec4 &&             operator+(float rhs) && { x += rhs; y += rhs; z += rhs; w += rhs; return std::move(*this); }
                        /// Adds the vector v to vector (s, s, s, s).
    friend Vec4         operator+(float lhs, const Vec4 &rhs) { return Vec4(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w); }
    friend Vec4 &&      operator+(float lhs, Vec4 &&rhs) { rhs.x += lhs; rhs.y += lhs; rhs.z += lhs; rhs.w += lhs; return std::move(rhs); }

                        /// Subtracts a vector from this vector.
    Vec4                Sub(const Vec4 &v) const & { return *this - v; }
    Vec4 &&             Sub(const Vec4 &v) && { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return std::move(*this); }
                        /// Subtracts the given vector from this vector.
                        /// This function is identical to the member function Sub()
    Vec4                operator-(const Vec4 &rhs) const & { return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
    Vec4 &&             operator-(const Vec4 &rhs) && { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return std::move(*this); }

                        /// Subtracts the vector (s, s, s, s) from this vector.
    Vec4                SubScalar(float s) const & { return *this - s; }
    Vec4 &&             SubScalar(float s) && { x -= s; y -= s; z -= s; w -= s; return std::move(*this); }
                        /// Subtracts the vector (s, s, s, s) from this vector.
                        /// This function is identical to the member function SubScalar()
    Vec4                operator-(float rhs) const & { return Vec4(x - rhs, y - rhs, z - rhs, w - rhs); }
    Vec4 &&             operator-(float rhs) && { x -= rhs; y -= rhs; z -= rhs; w -= rhs; return std::move(*this); }
                        /// Subtracts the vector v from vector (s, s, s, s).
    friend Vec4         operator-(float lhs, const Vec4 &rhs) { return Vec4(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w); }
    friend Vec4 &&      operator-(float lhs, Vec4 &&rhs) { rhs.x = lhs - rhs.x; rhs.y = lhs - rhs.y; rhs.z = lhs - rhs.z; rhs.z = lhs - rhs.z; return std::move(rhs); }

                        /// Multiplies this vector by a scalar.
    Vec4                Mul(float s) const & { return *this * s; }
    Vec4 &&             Mul(float s) && { x *= s; y *= s; z *= s; w *= s; return std::move(*this); }
                        /// Multiplies this vector by a scalar.
                        /// This function is identical to the member function Mul().
    Vec4                operator*(float rhs) const & { return Vec4(x * rhs, y * rhs, z * rhs, w * rhs); }
    Vec4 &&             operator*(float rhs) && { x *= rhs; y *= rhs; z *= rhs; w *= rhs; return std::move(*this); }
                        /// Multiplies vector v by a scalar.
    friend Vec4         operator*(float lhs, const Vec4 &rhs) { return Vec4(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w); }
    friend Vec4 &&      operator*(float lhs, Vec4 &&rhs) { rhs.x *= lhs; rhs.y *= lhs; rhs.z *= lhs; rhs.w *= lhs; return std::move(rhs); }

                        /// Multiplies this vector by a vector, element-wise.
    Vec4                MulComp(const Vec4 &v) const & { return *this * v; }
    Vec4 &&             MulComp(const Vec4 &v) && { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return std::move(*this); }
                        /// Multiplies this vector by a vector, element-wise.
                        /// This function is identical to the member function MulComp().
    Vec4                operator*(const Vec4 &rhs) const & { return Vec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }
    Vec4 &&             operator*(const Vec4 &rhs) && { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return std::move(*this); }

                        /// Divides this vector by a scalar.
    Vec4                Div(float s) const & { return *this / s; }
    Vec4 &&             Div(float s) && { float inv = 1.f / s; x *= inv; y *= inv; z *= inv; w *= inv; return std::move(*this); }
                        /// Divides this vector by a scalar.
                        /// This function is identical to the member function Div().
    Vec4                operator/(float rhs) const & { float inv = 1.f / rhs; return Vec4(x * inv, y * inv, z * inv, w * inv); }
    Vec4 &&             operator/(float rhs) && { float inv = 1.f / rhs; x *= inv; y *= inv; z *= inv; w *= inv; return std::move(*this); }

                        /// Divides this vector by a vector, element-wise.
    Vec4                DivComp(const Vec4 &v) const & { return *this / v; }
    Vec4 &&             DivComp(const Vec4 &v) && { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return std::move(*this); }
                        /// This function is identical to the member function DivComp().
                        /// Divides this vector by a vector, element-wise.
    Vec4                operator/(const Vec4 &rhs) const & { return Vec4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); }
    Vec4 &&             operator/(const Vec4 &rhs) && { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return std::move(*this); }
                        /// Divides vector (s, s, s, s) by a vector, element-wise.
    friend Vec4         operator/(float lhs, const Vec4 &rhs) { return Vec4(lhs / rhs.x, lhs / rhs.y, lhs / rhs.z, lhs / rhs.w); }
    friend Vec4 &&      operator/(float lhs, Vec4 &&rhs) { rhs.x = lhs / rhs.x; rhs.y = lhs / rhs.y; rhs.z = lhs / rhs.z; rhs.w = lhs / rhs.w; return std::move(rhs); }

                        /// Assign from another vector.
    Vec4 &              operator=(const Vec4 &rhs);

                        /// Adds a vector to this vector, in-place.
    Vec4 &              AddSelf(const Vec4 &v) { *this += v; return *this; }
                        /// Adds a vector to this vector, in-place.
                        /// This function is identical to the member function AddSelf().
    Vec4 &              operator+=(const Vec4 &rhs);

                        /// Subtracts a vector from this vector, in-place.
    Vec4 &              SubSelf(const Vec4 &v) { *this -= v; return *this; }
                        /// Subtracts a vector from this vector, in-place.
                        /// This function is identical to the member function SubSelf().
    Vec4 &              operator-=(const Vec4 &rhs);

                        /// Multiplies this vector by a scalar, in-place.
    Vec4 &              MulSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this vector by a scalar, in-place.
                        /// This function is identical to the member function MulSelf().
    Vec4 &              operator*=(float rhs);

                        /// Multiplies this vector by a vector, element-wise, in-place.
    Vec4 &              MulCompSelf(const Vec4 &v) { *this *= v; return *this; }
                        /// Multiplies this vector by a vector, element-wise, in-place.
                        /// This function is identical to the member function MulCompSelf().
    Vec4 &              operator*=(const Vec4 &rhs);

                        /// Divides this vector by a scalar, in-place.
    Vec4 &              DivSelf(float s) { *this /= s; return *this; }
                        /// Divides this vector by a scalar, in-place.
                        /// This function is identical to the member function DivSelf().
    Vec4 &              operator/=(float rhs);

                        /// Divides this vector by a vector, element-wise, in-place.
    Vec4 &              DivCompSelf(const Vec4 &v) { *this /= v; return *this; }
                        /// Divides this vector by a vector, element-wise, in-place.
                        /// This function is identical to the member function DivCompSelf().
    Vec4 &              operator/=(const Vec4 &rhs);

                        /// Exact compare, no epsilon
    bool                Equals(const Vec4 &v) const;
                        /// Compare with epsilon
    bool                Equals(const Vec4 &v, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Vec4 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Vec4 &rhs) const { return !Equals(rhs); }

                        /// Tests if this is the zero vector, up to the given epsilon.
    bool                IsZero(const float epsilon = 0.0f) const;

                        /// Sets all element of this vector
    void                Set(float x, float y, float z, float w);

                        /// Sets this Vec4 to (s, s, s, s).
    void                SetFromScalar(float s) { x = y = z = w = s; }
                        /// Returns Vec4(s, s, s, s).
    static Vec4         FromScalar(float s) { return Vec4(s, s, s, s); }

                        /// Get the minimum component of a vector
    float               MinComponent() const { return Min(Min(x, y), Min(z, w)); }
                        /// Get the maximum component of a vector
    float               MaxComponent() const { return Max(Max(x, y), Max(z, w)); }
                        /// Get the minimum absolute component of a vector
    float               MinAbsComponent() const { return Min(Min(Math::Fabs(x), Math::Fabs(y)), Min(Math::Fabs(z), Math::Fabs(w))); }
                        /// Get the maximum absolute component of a vector
    float               MaxAbsComponent() const { return Max(Max(Math::Fabs(x), Math::Fabs(y)), Max(Math::Fabs(z), Math::Fabs(w))); }

                        /// Computes the length of this vector.
    float               Length() const;

                        /// Computes the squared length of this vector.
    float               LengthSqr() const;

                        /// Normalizes this vector.
                        /// @return Length of this vector
    float               Normalize();
                        /// Normalizes this vector fast but approximately.
                        /// @return Length of this vector
    float               NormalizeFast();

                        /// Returns a normalized copy of this vector.
    Vec4                Normalized() const &;
    Vec4 &&             Normalized() && { Normalize(); return std::move(*this); }
                        /// Returns a fast but approximately normalized copy of this vector.
    Vec4                NormalizedFast() const &;
    Vec4 &&             NormalizedFast() && { NormalizeFast(); return std::move(*this); }

                        /// Clamp min <= this[i] <= max for each element.
    void                Clamp(const Vec4 &min, const Vec4 &max);

                        /// Computes the dot product of this and the given vector.
    float               Dot(const Vec4 &a) const;
                        /// Computes the absolute dot product of this and the given vector.
    float               AbsDot(const Vec4 &a) const;

                        /// Sets from linear interpolation between vector v1 and the vector v2.
    void                SetFromLerp(const Vec4 &v1, const Vec4 &v2, const float t);
                        /// Returns linear interpolation between the vector v1 and the vector v2.
    static Vec4         FromLerp(const Vec4 &v1, const Vec4 &v2, const float t);

                        /// Return hash value for HashIndex.
    int                 ToHash() const { return ((int)x) * 31 * 31 * 31 + ((int)y) * 31 * 31 + ((int)z) * 31 + ((int)w); }

                        /// Returns "x y z".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "x y z" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Vec4         FromString(const char *str);

                        /// Casts this Vec4 to a Vec2.
    const Vec2 &        ToVec2() const;
    Vec2 &              ToVec2();

                        /// Casts this Vec4 to a Vec3.
    const Vec3 &        ToVec3() const;
    Vec3 &              ToVec3();

                        /// Casts this Vec3 to a Color3.
    const Color3 &      ToColor3() const;
    Color3 &            ToColor3();

                        /// Casts this Vec3 to a Color4.
    const Color4 &      ToColor4() const;
    Color4 &            ToColor4();

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return Size; }

    ALIGN_AS32 static const Vec4 origin;    ///< (0, 0, 0, 0)
    ALIGN_AS32 static const Vec4 zero;      ///< (0, 0, 0, 0)
    ALIGN_AS32 static const Vec4 one;       ///< (1, 1, 1, 1)
    ALIGN_AS32 static const Vec4 unitX;     ///< (1, 0, 0, 0)
    ALIGN_AS32 static const Vec4 unitY;     ///< (0, 1, 0, 0)
    ALIGN_AS32 static const Vec4 unitZ;     ///< (0, 0, 1, 0)
    ALIGN_AS32 static const Vec4 unitW;     ///< (0, 0, 0, 1)
    ALIGN_AS32 static const Vec4 infinity;  ///< (1e30f, 1e30f, 1e30f, 1e30f)

    float               x;          ///< The x components.
    float               y;          ///< The y components.
    float               z;          ///< The z components.
    float               w;          ///< The w components.
};

BE_INLINE constexpr Vec4::Vec4(float inX, float inY, float inZ, float inW) :
    x(inX), y(inY), z(inZ), w(inW) {
}

BE_INLINE constexpr Vec4::Vec4(const Vec3 &inXyz, float inW) :
    x(inXyz.x), y(inXyz.y), z(inXyz.z), w(inW) {
}

BE_INLINE constexpr Vec4::Vec4(const Vec2 &inXy, const Vec2 &inZw) :
    x(inXy.x), y(inXy.y), z(inZw.x), w(inZw.y) {
}

BE_INLINE constexpr Vec4::Vec4(const float data[4]) :
    x(data[0]), y(data[1]), z(data[2]), w(data[3]) {
}

BE_INLINE constexpr Vec4::Vec4(float c) :
    x(c), y(c), z(c), w(c) {
}

BE_INLINE void Vec4::Set(float x, float y, float z, float w) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

BE_INLINE float Vec4::operator[](int index) const {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE float &Vec4::operator[](int index) {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE Vec4 &Vec4::operator=(const Vec4 &rhs) {
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
    w = rhs.w;
    return *this;
}

BE_INLINE Vec4 &Vec4::operator+=(const Vec4 &rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    w += rhs.w;
    return *this;
}

BE_INLINE Vec4 &Vec4::operator-=(const Vec4 &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    w -= rhs.w;
    return *this;
}

BE_INLINE Vec4 &Vec4::operator*=(float rhs) {
    x *= rhs;
    y *= rhs;
    z *= rhs;
    w *= rhs;
    return *this;
}

BE_INLINE Vec4 &Vec4::operator*=(const Vec4 &rhs) {
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    w *= rhs.w;
    return *this;
};

BE_INLINE Vec4 &Vec4::operator/=(float rhs) {
    float inv = 1.0f / rhs;
    x *= inv;
    y *= inv;
    z *= inv;
    w *= inv;
    return *this;
}

BE_INLINE Vec4 &Vec4::operator/=(const Vec4 &rhs) {
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    w /= rhs.w;
    return *this;
}

BE_INLINE bool Vec4::Equals(const Vec4 &a) const {
    return x != a.x || y != a.y || z != a.z || w != a.w ? false : true;
}

BE_INLINE bool Vec4::Equals(const Vec4 &a, const float epsilon) const {
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

BE_INLINE float Vec4::Length() const {
    return (float)Math::Sqrt(x * x + y * y + z * z + w * w);
}

BE_INLINE float Vec4::LengthSqr() const {
    return x * x + y * y + z * z + w * w;
}

BE_INLINE float Vec4::Normalize() {
    float sqrLength = x * x + y * y + z * z + w * w;
    float invLength = Math::InvSqrt(sqrLength);

    x *= invLength;
    y *= invLength;
    z *= invLength;
    w *= invLength;

    return invLength * sqrLength;
}

BE_INLINE float Vec4::NormalizeFast() {
    float sqrLength = x * x + y * y + z * z + w * w;
    float invLength = Math::RSqrt(sqrLength);

    x *= invLength;
    y *= invLength;
    z *= invLength;
    w *= invLength;

    return invLength * sqrLength;
}

BE_INLINE Vec4 Vec4::Normalized() const & {
    Vec4 n = *this;
    n.Normalize();
    return n;
}

BE_INLINE Vec4 Vec4::NormalizedFast() const & {
    Vec4 n = *this;
    n.NormalizeFast();
    return n;
}

BE_INLINE void Vec4::Clamp(const Vec4 &min, const Vec4 &max) {
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

    if (z < min.z) {
        z = min.z;
    } else if (z > max.z) {
        z = max.z;
    }
}

BE_INLINE float Vec4::Dot(const Vec4 &a) const {
    return x * a.x + y * a.y + z * a.z + w * a.w;
}

BE_INLINE float Vec4::AbsDot(const Vec4 &a) const {
    return Math::Fabs(x * a.x) + Math::Fabs(y * a.y) + Math::Fabs(z * a.z) + Math::Fabs(w * a.w);
}

BE_INLINE void Vec4::SetFromLerp(const Vec4 &v1, const Vec4 &v2, const float t) {
    if (t <= 0.0f) {
        (*this) = v1;
    } else if (t >= 1.0f) {
        (*this) = v2;
    } else {
        (*this) = v1 + t * (v2 - v1);
    }
}

BE_INLINE Vec4 Vec4::FromLerp(const Vec4 &v1, const Vec4 &v2, const float t) {
    Vec4 v;
    v.SetFromLerp(v1, v2, t);
    return v;
}

BE_INLINE bool Vec4::IsZero(float epsilon) const { 
    return Equals(Vec4::zero, epsilon);
}

BE_INLINE const Vec2 &Vec4::ToVec2() const {
    return *reinterpret_cast<const Vec2 *>(this);
}

BE_INLINE Vec2 &Vec4::ToVec2() {
    return *reinterpret_cast<Vec2 *>(this);
}

BE_INLINE const Vec3 &Vec4::ToVec3() const {
    return *reinterpret_cast<const Vec3 *>(this);
}

BE_INLINE Vec3 &Vec4::ToVec3() {
    return *reinterpret_cast<Vec3 *>(this);
}

BE_INLINE const Color3 &Vec4::ToColor3() const {
    return *reinterpret_cast<const Color3 *>(this);
}

BE_INLINE Color3 &Vec4::ToColor3() {
    return *reinterpret_cast<Color3 *>(this);
}

BE_INLINE const Color4 &Vec4::ToColor4() const {
    return *reinterpret_cast<const Color4 *>(this);
}

BE_INLINE Color4 &Vec4::ToColor4() {
    return *reinterpret_cast<Color4 *>(this);
}

BE_INLINE const char *Vec4::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
