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

    Sphere

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class AABB;

/// A 3D sphere.
class BE_API Sphere {
public:
    /// The default constructor does not initialize any members of this class.
    Sphere() = default;
    /// Constructs a sphere with a given position and radius.
    Sphere(const Vec3 &origin, float radius);

    const Vec3 &    Origin() const { return origin; }
    float           Radius() const { return radius; }

                    /// Exact compare, no epsilon
    bool            Equals(const Sphere &a) const;
                    /// Compare with epsilon
    bool            Equals(const Sphere &a, const float epsilon) const;
                    /// Exact compare, no epsilon
    bool            operator==(const Sphere &rhs) const { return Equals(rhs); }
                    /// Exact compare, no epsilon
    bool            operator!=(const Sphere &rhs) const { return !Equals(rhs); }

                    /// Clear to undefined state.
    void            Clear();

    void            SetZero();
    void            SetOrigin(const Vec3 &origin) { this->origin = origin; }
    void            SetRadius(float radius) { this->radius = radius; }

                    /// Computes the surface area of this sphere.
    float           Area() const { return 4.0f * Math::Pi * radius * radius; }
                    /// Computes the volume of this sphere.
    float           Volume() const { return 4.0f * Math::Pi * radius * radius * radius / 3.0f; }

                    // includes touching
    bool            IsContainPoint(const Vec3 &p) const;
    bool            IsIntersectSphere(const Sphere &sphere) const;
    bool            IsIntersectAABB(const AABB &aabb) const;

    bool            LineIntersection(const Vec3 &start, const Vec3 &end) const;
    float           RayIntersection(const Vec3 &start, const Vec3 &dir) const;

                    // 구를 dir 축으로 투영했을 때 min, max 값
    void            AxisProjection(const Vec3 &dir, float &min, float &max) const;

    AABB            ToAABB() const;

                    /// Returns dimension of this type
    int             GetDimension() const { return 4; }

    Vec3            origin;     ///< The center point of this sphere.
    float           radius;     ///< The radius of this sphere.
};

BE_INLINE Sphere::Sphere(const Vec3 &origin, float radius) :
    origin(origin), radius(radius) {
}

BE_INLINE void Sphere::Clear() {
    origin.SetFromScalar(0);
    radius = -1.0f;
}

BE_INLINE void Sphere::SetZero() {
    origin.SetFromScalar(0);
    radius = 0.0f;
}

BE_INLINE bool Sphere::Equals(const Sphere &a) const {
    return (origin.Equals(a.origin) && radius == a.radius);
}

BE_INLINE bool Sphere::Equals(const Sphere &a, const float epsilon) const {
    return (origin.Equals(a.origin, epsilon) && Math::Fabs(radius - a.radius) <= epsilon);
}

BE_INLINE bool Sphere::IsContainPoint(const Vec3 &p) const {
    if ((p - origin).LengthSqr() > radius * radius) {
        return false;
    }
    return true;
}

BE_INLINE bool Sphere::IsIntersectSphere(const Sphere &s) const {
    float r = s.radius + radius;
    if ((s.origin - origin).LengthSqr() > r * r) {
        return false;
    }
    return true;
}

BE_INLINE void Sphere::AxisProjection(const Vec3 &dir, float &min, float &max) const {
    float d = dir.Dot(origin);
    min = d - radius;
    max = d + radius;
}

BE_NAMESPACE_END
