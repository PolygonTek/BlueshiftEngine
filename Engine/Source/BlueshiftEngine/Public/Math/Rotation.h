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

BE_NAMESPACE_BEGIN

class Angles;
class Quat;
class Mat3;

/// Describes a complete rotation in degrees about an abritray axis.
/// A local rotation matrix is stored for fast rotation of multiple points.
class BE_API Rotation {
    friend class Angles;
    friend class Quat;
    friend class Mat3;

public:
    /// The default constructor does not initialize any members of this class.
    Rotation() {}
    Rotation(const Vec3 &rotOrigin, const Vec3 &rotVec, const float rotAngle);
    /// Assignment operator
    Rotation &operator=(const Rotation &rhs);

    void                Set(const Vec3 &rotOrigin, const Vec3 &rotVec, const float rotAngle);
    void                SetOrigin(const Vec3 &rotOrigin);
                        // has to be normalized
    void                SetVec(const Vec3 &rotVec);
                        // has to be normalized
    void                SetVec(const float x, const float y, const float z);
    void                SetAngle(const float rotAngle);
    
    const Vec3 &        GetOrigin() const { return origin; }
    const Vec3 &        GetVec() const { return vec; }
    float               GetAngle() const { return angle; }

                        /// Flips rotation
    Rotation            operator-() const { return Rotation(origin, vec, -angle); }

                        /// Scale rotation
    Rotation &          operator*=(const float rhs);
                        /// Rotate vector
    friend Vec3 &       operator*=(Vec3 &lhs, const Rotation &rhs);
                        /// Scale rotation
    Rotation &          operator/=(const float rhs);

                        /// Scale rotation
    Rotation            operator*(const float rhs) const { return Rotation(origin, vec, angle * rhs); }
                        /// Rotate vector
    Vec3                operator*(const Vec3 &rhs) const;
                        /// Scale rotation
    friend Rotation     operator*(const float lhs, const Rotation &rhs) { return rhs * lhs; }
                        /// Rotate vector
    friend Vec3         operator*(const Vec3 &lhs, const Rotation &rhs) { return rhs * lhs; }
                        /// Scale rotation
    Rotation            operator/(const float rhs) const;

    void                Scale(const float s);
    void                ReCalculateMatrix();

    Vec3                RotatePoint(const Vec3 &point) const;

    void                Normalize180();
    void                Normalize360();

    Angles              ToAngles() const;
    Quat                ToQuat() const;
    const Mat3 &        ToMat3() const;
    Mat4                ToMat4() const;

private:
    Vec3                origin;         ///< Origin of rotation
    Vec3                vec;            ///< Normalized vector to rotate around
    float               angle;          ///< Angle of rotation in degrees
    mutable Mat3        axis;           ///< Rotation axis
    mutable bool        axisValid;      ///< True if rotation axis is valid
};

BE_INLINE Rotation::Rotation(const Vec3 &rotOrigin, const Vec3 &rotVec, const float rotAngle) : 
    origin(rotOrigin), vec(rotVec), angle(rotAngle) {
    vec.Normalize();
    axisValid = false;
}

BE_INLINE Rotation &Rotation::operator=(const Rotation &rhs) {
    origin = rhs.origin;
    vec = rhs.vec;
    angle = rhs.angle;
    axis = rhs.axis;
    axisValid = rhs.axisValid;
    return *this;
}

BE_INLINE void Rotation::Set(const Vec3 &rotOrigin, const Vec3 &rotVec, const float rotAngle) {
    origin = rotOrigin;
    vec = rotVec;
    vec.Normalize();
    angle = rotAngle;
    axisValid = false;
}

BE_INLINE void Rotation::SetOrigin(const Vec3 &rotOrigin) {
    origin = rotOrigin;
}

BE_INLINE void Rotation::SetVec(const Vec3 &rotVec) {
    vec = rotVec;
    vec.Normalize();
    axisValid = false;
}

BE_INLINE void Rotation::SetVec(float x, float y, float z) {
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
    vec.Normalize();
    axisValid = false;
}

BE_INLINE void Rotation::SetAngle(const float rotAngle) {
    angle = rotAngle;
    axisValid = false;
}

BE_INLINE void Rotation::Scale(const float s) {
    angle *= s;
    axisValid = false;
}

BE_INLINE void Rotation::ReCalculateMatrix() {
    axisValid = false;
    ToMat3();
}

BE_INLINE Rotation Rotation::operator/(const float rhs) const {
    assert(rhs != 0.0f);
    return Rotation(origin, vec, angle / rhs);
}

BE_INLINE Rotation &Rotation::operator*=(const float rhs) {
    angle *= rhs;
    axisValid = false;
    return *this;
}

BE_INLINE Rotation &Rotation::operator/=(const float rhs) {
    assert(rhs != 0.0f);
    angle /= rhs;
    axisValid = false;
    return *this;
}

BE_INLINE Vec3 Rotation::operator*(const Vec3 &rhs) const {
    if (!axisValid) {
        ToMat3();
    }

    return (axis * (rhs - origin) + origin);
}

BE_INLINE Vec3 &operator*=(Vec3 &v, const Rotation &r) {
    v = r * v;
    return v;
}

BE_INLINE Vec3 Rotation::RotatePoint(const Vec3 &point) const {
    if (!axisValid) {
        ToMat3();
    }

    return axis * (point - origin) + origin;
}

BE_NAMESPACE_END
