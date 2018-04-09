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

    3D Vector

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/// A vector of form (x, y, z).
class BE_API Vec3 {
public:
    /// Specifies the number of elements in this vector.
    enum { Size = 3 };

    /// The default constructor does not initialize any members of this class.
    Vec3() = default;
    /// Constructs a Vec3 with the value (x, y, z).
    Vec3(float x, float y, float z);
    /// Constructs a Vec3 from a C array, to the value (data[0], data[1], data[2]).
    explicit Vec3(const float data[3]);
    /// Constructs a Vec3 from a single value (s, s, s)
    explicit Vec3(float s);

                        /// Casts this Vec3 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&x; }
    float *             Ptr() { return (float *)&x; }
                        /// Casts this Vec3 to a C array.
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
                        /// This function is identical to the member function Negate().
    Vec3                operator-() const { return Vec3(-x, -y, -z); }
                        /// Performs an unary negation of this vector.
    Vec3                Negate() const { return Vec3(-x, -y, -z); }

                        /// Returns Vec3(|x|, |y|, |z|)
    Vec3                Abs() const { return Vec3(Math::Fabs(x), Math::Fabs(y), Math::Fabs(z)); }

                        /// Unary operator + allows this structure to be used in an expression '+v'.
    Vec3                operator+() const { return *this; }

                        /// Adds a vector to this vector.
    Vec3                Add(const Vec3 &v) const { return *this + v; }
                        /// Adds a vector to this vector.
                        /// This function is identical to the member function Add().
    Vec3                operator+(const Vec3 &rhs) const { return Vec3(x + rhs.x, y + rhs.y, z + rhs.z); }

                        /// Adds the vector (s, s, s, s) to this vector.
    Vec3                AddScalar(float s) const { return *this + s; }
                        /// Adds the vector (s, s, s, s) to this vector.
                        /// This function is identical to the member function AddScalar().
    Vec3                operator+(float rhs) const { return Vec3(x + rhs, y + rhs, z + rhs); }
                        /// Adds the vector v to vector (s, s, s, s).
    friend Vec3         operator+(float lhs, const Vec3 &rhs) { return Vec3(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z); }

                        /// Subtracts a vector from this vector.
    Vec3                Sub(const Vec3 &v) const { return *this - v; }
                        /// Subtracts the given vector from this vector.
                        /// This function is identical to the member function Sub()
    Vec3                operator-(const Vec3 &rhs) const { return Vec3(x - rhs.x, y - rhs.y, z - rhs.z); }

                        /// Subtracts the vector (s, s, s, s) from this vector.
    Vec3                SubScalar(float s) const { return *this - s; }
                        /// Subtracts the vector (s, s, s, s) from this vector.
                        /// This function is identical to the member function SubScalar()
    Vec3                operator-(float rhs) const { return Vec3(x - rhs, y - rhs, z - rhs); }
                        /// Subtracts the vector v from vector (s, s, s, s).
    friend Vec3         operator-(float lhs, const Vec3 &rhs) { return Vec3(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z); }

                        /// Multiplies this vector by a scalar.
    Vec3                Mul(float s) const { return *this * s; }
                        /// Multiplies this vector by a scalar.
                        /// This function is identical to the member function Mul().
    Vec3                operator*(float rhs) const { return Vec3(x * rhs, y * rhs, z * rhs); }
                        /// Multiplies vector v by a scalar.
    friend Vec3         operator*(float lhs, const Vec3 &rhs) { return Vec3(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z); }

                        /// Multiplies this vector by a vector, element-wise.
    Vec3                MulComp(const Vec3 &v) const { return *this * v; }
                        /// Multiplies this vector by a vector, element-wise.
                        /// This function is identical to the member function MulComp().
    Vec3                operator*(const Vec3 &rhs) const { return Vec3(x * rhs.x, y * rhs.y, z * rhs.z); }

                        /// Divides this vector by a scalar.
    Vec3                Div(float s) const { return *this / s; }
                        /// Divides this vector by a scalar.
                        /// This function is identical to the member function Div().
    Vec3                operator/(float rhs) const { float inv = 1.f / rhs; return Vec3(x * inv, y * inv, z * inv); }

                        /// Divides this vector by a vector, element-wise.
    Vec3                DivComp(const Vec3 &v) const { return *this / v; }
                        /// This function is identical to the member function DivComp().
                        /// Divides this vector by a vector, element-wise.
    Vec3                operator/(const Vec3 &rhs) const { return Vec3(x / rhs.x, y / rhs.y, z / rhs.z); }
                        /// Divides vector (s, s, s, s) by a vector, element-wise.
    friend Vec3         operator/(float lhs, const Vec3 &rhs) { return Vec3(lhs / rhs.x, lhs / rhs.y, lhs / rhs.z); }
    
                        /// Assign from another vector.
    Vec3 &              operator=(const Vec3 &rhs);

                        /// Adds a vector to this vector, in-place.
    Vec3 &              AddSelf(const Vec3 &v) { *this += v; return *this; }
                        /// Adds a vector to this vector, in-place.
                        /// This function is identical to the member function AddSelf().
    Vec3 &              operator+=(const Vec3 &rhs);

                        /// Subtracts a vector from this vector, in-place.
    Vec3 &              SubSelf(const Vec3 &v) { *this -= v; return *this; }
                        /// Subtracts a vector from this vector, in-place.
                        /// This function is identical to the member function SubSelf().
    Vec3 &              operator-=(const Vec3 &rhs);

                        /// Multiplies this vector by a scalar, in-place.
    Vec3 &              MulSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this vector by a scalar, in-place.
                        /// This function is identical to the member function MulSelf().
    Vec3 &              operator*=(float rhs);

                        /// Multiplies this vector by a vector, element-wise, in-place.
    Vec3 &              MulCompSelf(const Vec3 &v) { *this *= v; return *this; }
                        /// Multiplies this vector by a vector, element-wise, in-place.
                        /// This function is identical to the member function MulCompSelf().
    Vec3 &              operator*=(const Vec3 &rhs);

                        /// Divides this vector by a scalar, in-place.
    Vec3 &              DivSelf(float s) { *this /= s; return *this; }
                        /// Divides this vector by a scalar, in-place.
                        /// This function is identical to the member function DivSelf().
    Vec3 &              operator/=(float rhs);

                        /// Divides this vector by a vector, element-wise, in-place.
    Vec3 &              DivCompSelf(const Vec3 &v) { *this /= v; return *this; }
                        /// Divides this vector by a vector, element-wise, in-place.
                        /// This function is identical to the member function DivCompSelf().
    Vec3 &              operator/=(const Vec3 &rhs);

                        /// Exact compare, no epsilon
    bool                Equals(const Vec3 &v) const;
                        /// Compare with epsilon
    bool                Equals(const Vec3 &v, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Vec3 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Vec3 &rhs) const { return !Equals(rhs); }

                        /// Tests if this is the zero vector, up to the given epsilon.
    bool                IsZero(const float epsilon = 0.0f) const;

                        /// Sets all element of this vector
    void                Set(float x, float y, float z);

                        /// Sets this Vec3 to (s, s, s).
    void                SetFromScalar(float s) { x = y = z = s; }
                        /// Returns Vec3(s, s, s).
    static Vec3         FromScalar(float s) { return Vec3(s, s, s); }

                        /// Get the minimum component of a vector
    float               MinComponent() const { return Min3(x, y, z); }
                        /// Get the maximum component of a vector
    float               MaxComponent() const { return Max3(x, y, z); }
                        /// Get the minimum component index of a vector
    int                 MinComponentIndex() const { return Min3Index(x, y, z); }
                        /// Get the maximum component index of a vector
    int                 MaxComponentIndex() const { return Max3Index(x, y, z); }

                        /// Computes the length of this vector.
    float               Length() const;

                        /// Computes the squared length of this vector.
    float               LengthSqr() const;

                        /// Computes the distance between this point and the given vector.
    float               Distance(const Vec3 &a) const;

                        /// Computes the squared distance between this point and the given vector.
    float               DistanceSqr(const Vec3 &a) const;

                        /// Fix degenerate axial cases
    bool                FixDegenerateNormal();
                        /// Change tiny numbers to zero
    bool                FixDenormals();

                        /// Normalizes this vector.
                        /// @return Length of this vector
    float               Normalize();
                        /// Normalizes this vector fast but approximately.
                        /// @return Length of this vector
    float               NormalizeFast();

                        /// Scales this vector so that its new length is as given.
    Vec3 &              ScaleToLength(float length);
    
                        /// Clamp min <= this[i] <= max for each element.
    void                Clamp(const Vec3 &min, const Vec3 &max);

                        /// Computes the dot product of this and the given vector.
    float               Dot(const Vec3 &a) const;
                        /// Computes the absolute dot product of this and the given vector.
    float               AbsDot(const Vec3 &a) const;

                        /// Computes the cross product of this and the given vector.
    Vec3                Cross(const Vec3 &a) const;
                        /// Sets this vector from the cross product of two vectors.
    Vec3 &              SetFromCross(const Vec3 &a, const Vec3 &b);
                        /// Returns the cross product of two vectors.
    static Vec3         FromCross(const Vec3 &a, const Vec3 &b);

                        /// Returns this vector reflected about a plane with the given normal.
    Vec3                Reflect(const Vec3 &normal) const;
                        /// Returns this vector reflected about a plane with the given normal, in-place.
    Vec3 &              ReflectSelf(const Vec3 &normal);

                        /// Refracts this vector about a plane with the given normal.
    Vec3                Refract(const Vec3 &normal, float eta) const;
                        /// Refracts this vector about a plane with the given normal, in-place.
    Vec3 &              RefractSelf(const Vec3 &normal, float eta);

                        /// Returns the angle between this vector and the specified vector, in radians.
    float               AngleBetween(const Vec3 &other) const;
                        /// Returns the angle between this vector and the specified normalized vector, in radians.
    float               AngleBetweenNorm(const Vec3 &other) const;

                        /// Returns this vector slided about a plane with the given normal.
    Vec3                Slide(const Vec3 &normal, float backoffScale = 1.001f) const;

                        /// Sets from linear interpolation between the vector v1 and the vector v2.
    void                SetFromLerp(const Vec3 &v1, const Vec3 &v2, const float t);
                        /// Sets from spherical linear interpolation between the vector v1 and the vector v2.
    void                SetFromSLerp(const Vec3 &v1, const Vec3 &v2, const float t);
                        /// Returns linear interpolation between the vector v1 and the vector v2.
    static Vec3         FromLerp(const Vec3 &v1, const Vec3 &v2, const float t);
                        /// Returns spherical linear interpolation between the vector v1 and the vector v2.
    static Vec3         FromSLerp(const Vec3 &v1, const Vec3 &v2, const float t);

                        /// Returns vector on the unit sphere has uniform distribution with the given random variables u1, u2 [0, 1].
    static Vec3         FromUniformSampleSphere(float u1, float u2);

                        /// Returns vector on the unit hemisphere has uniform distribution with the given random variables u1, u2 [0, 1].
    static Vec3         FromUniformSampleHemisphere(float u1, float u2);

                        /// Returns vector on the unit hemisphere has cosine weighted distribution with the given random variables u1, u2 [0, 1].
    static Vec3         FromCosineSampleHemisphere(float u1, float u2);

                        /// Returns vector on the unit hemisphere has powered cosine weighted distribution with the given random variables u1, u2 [0, 1].
    static Vec3         FromPowerCosineSampleHemisphere(float u1, float u2, float power);

                        /// Compute yaw angle in degree looking from the viewpoint of 2D view in x-y plane.
    float               ComputeYaw() const;
                        /// Compute pitch angle in degree looking from the viewpoint of 2D view in x-y plane.
    float               ComputePitch() const;

                        /// Returns "x y z".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "x y z" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Vec3         FromString(const char *str);
        
                        /// Casts this Vec3 to a Vec2.
    const Vec2 &        ToVec2() const;
    Vec2 &              ToVec2();

                        /// Casts this Vec3 to a Color3.
    const Color3 &      ToColor3() const;
    Color3 &            ToColor3();

    Mat3                ToMat3() const;

    Angles              ToAngles() const;

                        /// Converts to the spherical coordinates.
                        /// @return Length of this vector
    float               ToSpherical(float &theta, float &phi) const;

                        /// Sets from the spherical coordinates.
    void                SetFromSpherical(float radius, float theta, float phi);

    void                NormalVectors(Vec3 &left, Vec3 &down) const;

                        /// This function computes two new vectors left and up which are both orthogonal to this vector and to each other.
                        /// That is, the set { this, left, up } is an orthogonal set. The vectors left and up that are outputted are also normalized.
                        /// @param left[out]    Receives vector left.
                        /// @param up[out]      Receives vector up.
    void                OrthogonalBasis(Vec3 &left, Vec3 &up) const;

                        /// Returns dimension of this type.
    int                 GetDimension() const { return Size; }

                        /// Compute 3D barycentric coordinates from the point based on 3 simplex vector.
    static const Vec3   Compute3DBarycentricCoords(const Vec2 &s1, const Vec2 &s2, const Vec2 &s3, const Vec2 &p);

    static float        ComputeBitangentSign(const Vec3 &n, const Vec3 &t0, const Vec3 &t1);

    static const Vec3   origin;     ///< (0, 0, 0)
    static const Vec3   zero;       ///< (0, 0, 0)
    static const Vec3   one;        ///< (1, 1, 1)
    static const Vec3   unitX;      ///< (1, 0, 0)
    static const Vec3   unitY;      ///< (0, 1, 0)
    static const Vec3   unitZ;      ///< (0, 0, 1)

    float               x;          ///< The x component.
    float               y;          ///< The y component.
    float               z;          ///< The z component.
};

BE_INLINE Vec3::Vec3(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

BE_INLINE Vec3::Vec3(const float data[3]) {
    this->x = data[0];
    this->y = data[1];
    this->z = data[2];
}

BE_INLINE Vec3::Vec3(float c) {
    this->x = c;
    this->y = c;
    this->z = c;
}

BE_INLINE void Vec3::Set(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

BE_INLINE bool Vec3::IsZero(const float epsilon) const { 
    return Equals(Vec3::zero, epsilon);
}

BE_INLINE float Vec3::operator[](int index) const {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE float &Vec3::operator[](int index) {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE Vec3 &Vec3::operator=(const Vec3 &rhs) {
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
    return *this;
}

BE_INLINE Vec3 &Vec3::operator+=(const Vec3 &rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

BE_INLINE Vec3 &Vec3::operator-=(const Vec3 &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

BE_INLINE Vec3 &Vec3::operator*=(float rhs) {
    x *= rhs;
    y *= rhs;
    z *= rhs;
    return *this;
}

BE_INLINE Vec3 &Vec3::operator*=(const Vec3 &rhs) {
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    return *this;
};

BE_INLINE Vec3 &Vec3::operator/=(float rhs) {
    float inv = 1.0f / rhs;
    x *= inv;
    y *= inv;
    z *= inv;
    return *this;
}

BE_INLINE Vec3 &Vec3::operator/=(const Vec3 &rhs) {
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    return *this;
};

BE_INLINE bool Vec3::Equals(const Vec3 &a) const {
    return x != a.x || y != a.y || z != a.z ? false : true;
}

BE_INLINE bool Vec3::Equals(const Vec3 &a, const float epsilon) const {
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

BE_INLINE bool Vec3::FixDenormals() {
    bool denormal = false;

    if (fabs(x) < 1e-30f) {
        x = 0.0f;
        denormal = true;
    }
    if (fabs(y) < 1e-30f) {
        y = 0.0f;
        denormal = true;
    }
    if (fabs(z) < 1e-30f) {
        z = 0.0f;
        denormal = true;
    }

    return denormal;
}

BE_INLINE float Vec3::Length() const {
    return (float)Math::Sqrt(x * x + y * y + z * z);
}

BE_INLINE float Vec3::LengthSqr() const {
    return x * x + y * y + z * z;
}

BE_INLINE float Vec3::Distance(const Vec3 &a) const {
    Vec3 v = *this - a;
    return v.Length();
}

BE_INLINE float Vec3::DistanceSqr(const Vec3 &a) const {
    Vec3 v = *this - a;
    return v.LengthSqr();
}

BE_INLINE float Vec3::Normalize() {
    float sqrLength = x * x + y * y + z * z;
    float invLength = Math::InvSqrt(sqrLength);

    x *= invLength;
    y *= invLength;
    z *= invLength;

    return invLength * sqrLength;
}

BE_INLINE float Vec3::NormalizeFast() {
    float sqrLength = x * x + y * y + z * z;
    float invLength = Math::RSqrt(sqrLength);

    x *= invLength;
    y *= invLength;
    z *= invLength;

    return invLength * sqrLength;
}

BE_INLINE Vec3 &Vec3::ScaleToLength(float length) {
    if (!length) {
        SetFromScalar(0);
    } else {
        float length2 = LengthSqr();
        float ilength = length * Math::InvSqrt(length2);
        x *= ilength;
        y *= ilength;
        z *= ilength;
    }

    return *this;
}

BE_INLINE void Vec3::Clamp(const Vec3 &min, const Vec3 &max) {
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

BE_INLINE float Vec3::Dot(const Vec3 &a) const {
    return x * a.x + y * a.y + z * a.z;
}

BE_INLINE float Vec3::AbsDot(const Vec3 &a) const {
    return Math::Fabs(x * a.x) + Math::Fabs(y * a.y) + Math::Fabs(z * a.z);
}

BE_INLINE Vec3 Vec3::Cross(const Vec3 &a) const {
    return Vec3(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x);
}

BE_INLINE Vec3 &Vec3::SetFromCross(const Vec3 &a, const Vec3 &b) {
    x = a.y * b.z - a.z * b.y;
    y = a.z * b.x - a.x * b.z;
    z = a.x * b.y - a.y * b.x;
    return *this;
}

BE_INLINE Vec3 Vec3::FromCross(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

BE_INLINE Vec3 Vec3::Reflect(const Vec3 &normal) const {
    Vec3 v(x, y, z);
    return v.ReflectSelf(normal);
}

BE_INLINE Vec3 &Vec3::ReflectSelf(const Vec3 &normal) {
    float d = (normal.Dot(*this)) * 2.f;
    x = normal.x * d - x;
    y = normal.y * d - y;
    z = normal.z * d - z;
    return *this;
}

BE_INLINE Vec3 Vec3::Refract(const Vec3 &normal, float eta) const {
    Vec3 v(x, y, z);
    return v.RefractSelf(normal, eta);
}

BE_INLINE Vec3 &Vec3::RefractSelf(const Vec3 &normal, float eta) {
    float cos_in = normal.Dot(*this);
    float cos_tn2 = 1.0f - eta * eta * (1.0f -  cos_in * cos_in);
    if (cos_tn2 < 0.0f) {
        this->SetFromScalar(0);
    } else {
        *this = (eta * cos_in - Math::Sqrt(cos_tn2)) * normal - eta * (*this);
    }
    return *this;
}

BE_INLINE float Vec3::AngleBetween(const Vec3 &other) const {
    float cosa = Dot(other) / Math::Sqrt(LengthSqr() * other.LengthSqr());
    if (cosa >= 1.f) {
        return 0.f;
    } else if (cosa <= -1.f) {
        return Math::Pi;
    } else {
        return Math::ACos(cosa);
    }
}

BE_INLINE float Vec3::AngleBetweenNorm(const Vec3 &other) const {
    float cosa = Dot(other);
    if (cosa >= 1.f) {
        return 0.f;
    } else if (cosa <= -1.f) {
        return Math::Pi;
    } else {
        return Math::ACos(cosa);
    }
}

BE_INLINE void Vec3::SetFromLerp(const Vec3 &v1, const Vec3 &v2, const float t) {
    if (t <= 0.0f) {
        (*this) = v1;
    } else if (t >= 1.0f) {
        (*this) = v2;
    } else {
        (*this) = v1 + t * (v2 - v1);
    }
}

BE_INLINE Vec3 Vec3::FromLerp(const Vec3 &v1, const Vec3 &v2, const float t) {
    Vec3 v;
    v.SetFromLerp(v1, v2, t);
    return v;
}

BE_INLINE Vec3 Vec3::FromSLerp(const Vec3 &v1, const Vec3 &v2, const float t) {
    Vec3 v;
    v.SetFromSLerp(v1, v2, t);
    return v;
}

BE_INLINE float Vec3::ComputeBitangentSign(const Vec3 &n, const Vec3 &t0, const Vec3 &t1) {
    Vec3 biTangent = FromCross(n, t0);
    return (biTangent.Dot(t1)) > 0.0f ? 1.0f : -1.0f;
}

BE_INLINE const Vec2 &Vec3::ToVec2() const {
    return *reinterpret_cast<const Vec2 *>(this);
}

BE_INLINE Vec2 &Vec3::ToVec2() {
    return *reinterpret_cast<Vec2 *>(this);
}

BE_INLINE const Color3 &Vec3::ToColor3() const {
    return *reinterpret_cast<const Color3 *>(this);
}

BE_INLINE Color3 &Vec3::ToColor3() {
    return *reinterpret_cast<Color3 *>(this);
}

BE_INLINE const char *Vec3::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Size, precision);
}

BE_INLINE void Vec3::SetFromSpherical(float radius, float theta, float phi) {
    float sinTheta, cosTheta;
    float sinPhi, cosPhi;

    Math::SinCos(theta, sinTheta, cosTheta);
    Math::SinCos(phi, sinPhi, cosPhi);

    x = radius * sinTheta * cosPhi;
    y = radius * sinTheta * sinPhi;
    z = radius * cosTheta;
}

BE_INLINE float Vec3::ToSpherical(float &theta, float &phi) const {
    float radius = Math::Sqrt(x * x + y * y + z * z);
    theta = Math::ACos(z / radius);
    phi = Math::ATan(y, x);
    return radius;
}
    
BE_INLINE void Vec3::NormalVectors(Vec3 &left, Vec3 &down) const {
    float d = x * x + y * y;
    if (!d) {
        left[0] = 1;
        left[1] = 0;
        left[2] = 0;
    } else {
        d = Math::InvSqrt(d);
        left[0] = -y * d;
        left[1] = x * d;
        left[2] = 0;
    }
    down = left.Cross(*this);
}

BE_INLINE void Vec3::OrthogonalBasis(Vec3 &left, Vec3 &up) const {
    float l, s;

    // Suppose this vector to forward vector
    // If length of the vector that projected to x-y plane are greater than 0.7, compute basis vectors with z-rotation
    if (Math::Fabs(z) < 0.7f) {
        l = x * x + y * y;
        s = Math::InvSqrt(l);
        left[0] = -y * s;
        left[1] = x * s;
        left[2] = 0;
        // Cross(forward, left)
        up[0] = -z * left[1];
        up[1] = z * left[0];
        up[2] = l * s; 
    } else {
        // Else compute basis vectors with x-rotation
        l = y * y + z * z;
        s = Math::InvSqrt(l);
        up[0] = 0;
        up[1] = z * s;
        up[2] = -y * s;
        // Cross(up, forward)
        left[0] = l * s;
        left[1] = x * up[2];
        left[2] = -x * up[1];
    }
}

BE_INLINE Vec3 Vec3::Slide(const Vec3 &normal, float backoffScale) const {
    float backoff = Dot(normal) * backoffScale;
    Vec3 slideVector = *this - normal * backoff;
    return slideVector;
}

BE_NAMESPACE_END
