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

    Compressed Quaternion

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class Vec3;
class Mat3;
class Mat4;
class Angles;
class Rotation;

class BE_API CQuat {
public:
    /// The default constructor does not initialize any members of this class.
    CQuat() = default;
    CQuat(float x, float y, float z);

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

                        /// Exact compare, no epsilon
    bool                Equals(const CQuat &a) const;
                        /// Compare with epsilon
    bool                Equals(const CQuat &a, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const CQuat &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const CQuat &rhs) const { return !Equals(rhs); }

    void                Set(float x, float y, float z);

    Angles              ToAngles() const;
    Rotation            ToRotation() const;
    Mat3                ToMat3() const;
    Mat4                ToMat4() const;
    Quat                ToQuat() const;
                        /// Returns "x y z".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "x y z" with the given precision.
    const char *        ToString(int precision) const;

                        /// Change tiny numbers to zero
    bool                FixDenormals(float epsilon = Math::FloatEpsilon);

                        /// Returns dimension of this type
    int                 GetDimension() const { return 3; }

    float               x;      ///< The factor of i.
    float               y;      ///< The factor of j.
    float               z;      ///< The factor of k.
};

BE_INLINE CQuat::CQuat(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

BE_INLINE void CQuat::Set(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

BE_INLINE float CQuat::operator[](int index) const {
    assert((index >= 0) && (index < 3));
    return (&x)[index];
}

BE_INLINE float& CQuat::operator[](int index) {
    assert((index >= 0) && (index < 3));
    return (&x)[index];
}

BE_INLINE bool CQuat::Equals(const CQuat &a) const {
    return ((x == a.x) && (y == a.y) && (z == a.z));
}

BE_INLINE bool CQuat::Equals(const CQuat &a, const float epsilon) const {
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

BE_INLINE CQuat::operator const float *() const {
    return &x;
}

BE_INLINE CQuat::operator float *() {
    return &x;
}

BE_INLINE Quat CQuat::ToQuat() const {
    // take the absolute value because floating point rounding may cause the dot of x, y, z to be larger than 1
    return Quat(x, y, z, sqrt(fabs(1.0f - (x * x + y * y + z * z))));
}

BE_INLINE const char *CQuat::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), 3, precision);
}

BE_INLINE bool CQuat::FixDenormals(float epsilon) {
    bool denormal = false;
    if (fabs(x) < epsilon) {
        x = 0.0f;
        denormal = true;
    }
    if (fabs(y) < epsilon) {
        y = 0.0f;
        denormal = true;
    }
    if (fabs(z) < epsilon) {
        z = 0.0f;
        denormal = true;
    }
    return denormal;
}

BE_NAMESPACE_END
