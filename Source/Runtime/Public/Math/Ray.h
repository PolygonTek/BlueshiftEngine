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

    Ray

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

/// A ray in 3D space is a line that starts from an origin point and extends to infinity in one direction.
class BE_API Ray {
public:
    /// The default constructor does not initialize any members of this class.
    Ray() {}
    /// Constructs a new ray by explicitly specifying the member variables.
    constexpr Ray(const Vec3 &origin, const Vec3 &dir);

                        /// Casts this Ray to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&origin; }
    float *             Ptr() { return (float *)&origin; }
                        /// Casts this Ray to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&origin; }
                        operator float *() { return (float *)&origin; }

    const Vec3          GetOrigin() const { return origin; }
    const Vec3          GetDirection() const { return dir; }

    void                Set(const Vec3 &origin, const Vec3 &dir);

                        /// Gets a point along the ray at the given distance.
    const Vec3          GetPoint(float distance) const { return origin + dir * distance; }

                        /// Translates this Ray in world space.
    void                Translate(const Vec3 &offset) { origin += offset; }

                        /// Returns "x y z dx dy dz".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "x y z dx dy dz" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Ray          FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 6; }

    Vec3                origin;     ///< The origin of this ray
    Vec3                dir;        ///< The normalized direction of this ray
};

BE_INLINE constexpr Ray::Ray(const Vec3 &inOrigin, const Vec3 &inDir) :
    origin(inOrigin), dir(inDir) {
}

BE_INLINE void Ray::Set(const Vec3 &origin, const Vec3 &dir) {
    this->origin = origin;
    this->dir = dir;
}

BE_NAMESPACE_END
