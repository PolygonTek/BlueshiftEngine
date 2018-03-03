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

/// Infinite straight line in three-dimensional space.
class BE_API Ray {
public:
    /// The default constructor does not initialize any members of this class.
    Ray() {}
    Ray(const Vec3 &origin, const Vec3 &direction);

    const Vec3          GetOrigin() const { return origin; }
    const Vec3          GetDirection() const { return direction; }
    const Vec3          GetDistancePoint(float distance) const;

    void                Set(const Vec3 &origin, const Vec3 &direction);

    Vec3                origin;
    Vec3                direction;
};

BE_INLINE Ray::Ray(const Vec3 &origin, const Vec3 &direction) :
    origin(origin), direction(direction) {
}

BE_INLINE void Ray::Set(const Vec3 &origin, const Vec3 &direction) {
    this->origin = origin;
    this->direction = direction;
}

BE_INLINE const Vec3 Ray::GetDistancePoint(float distance) const {
    return origin + direction * distance;
}

BE_NAMESPACE_END
