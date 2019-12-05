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
class Ray;

/// A 3D sphere.
class BE_API Sphere {
public:
    /// The default constructor does not initialize any members of this class.
    Sphere() = default;
    /// Constructs a sphere with a given position and radius.
    constexpr Sphere(const Vec3 &center, float radius);

                    /// Returns center position of this sphere.
    const Vec3 &    Center() const { return center; }

                    /// Returns radius of this sphere.
    float           Radius() const { return radius; }

                    /// Returns diameter of this sphere.
    float           Diameter() const { return radius * 2; }

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

                    /// Sets to zero sized sphere.
    void            SetZero();

                    /// Sets center position of this sphere.
    void            SetCenter(const Vec3 &center) { this->center = center; }

                    /// Sets radius of this sphere.
    void            SetRadius(float radius) { this->radius = radius; }

                    /// Computes the surface area of this sphere.
    float           Area() const { return 4.0f * Math::Pi * radius * radius; }
                    /// Computes the volume of this sphere.
    float           Volume() const { return 4.0f * Math::Pi * radius * radius * radius / 3.0f; }

                    /// Tests if this sphere contain the given point.
    bool            IsContainPoint(const Vec3 &p) const;

                    /// Tests if this sphere intersect with the given sphere.
    bool            IsIntersectSphere(const Sphere &sphere) const;
                    /// Tests if this sphere intersect with the given AABB.
    bool            IsIntersectAABB(const AABB &aabb) const;
                    /// Tests if this sphere intersect with the given line segment.
    bool            IsIntersectLine(const Vec3 &p1, const Vec3 &p2) const;

                    /// Intersects a ray with this sphere.
                    /// Returns false if there is no intersection.
    bool            IntersectRay(const Ray &ray, float *hitDistMin = nullptr, float *hitDistMax = nullptr) const;
    float           IntersectRay(const Ray &ray) const;

                    /// Calculates minimum / maximum value by projecting sphere onto the given axis.
    void            ProjectOnAxis(const Vec3 &axis, float &min, float &max) const;

                    /// Converts to surrounding AABB.
    AABB            ToAABB() const;

                    /// Returns dimension of this type.
    constexpr int   GetDimension() const { return 4; }

    Vec3            center;     ///< The center position of this sphere.
    float           radius;     ///< The radius of this sphere.
};

BE_INLINE constexpr Sphere::Sphere(const Vec3 &inOrigin, float inRadius) :
    center(inOrigin), radius(inRadius) {
}

BE_INLINE void Sphere::Clear() {
    center.SetFromScalar(0);
    radius = -1.0f;
}

BE_INLINE void Sphere::SetZero() {
    center.SetFromScalar(0);
    radius = 0.0f;
}

BE_INLINE bool Sphere::Equals(const Sphere &a) const {
    return (center.Equals(a.center) && radius == a.radius);
}

BE_INLINE bool Sphere::Equals(const Sphere &a, const float epsilon) const {
    return (center.Equals(a.center, epsilon) && Math::Fabs(radius - a.radius) <= epsilon);
}

BE_INLINE bool Sphere::IsContainPoint(const Vec3 &p) const {
    if ((p - center).LengthSqr() > radius * radius) {
        return false;
    }
    return true;
}

BE_INLINE bool Sphere::IsIntersectSphere(const Sphere &s) const {
    float r = s.radius + radius;
    if ((s.center - center).LengthSqr() > r * r) {
        return false;
    }
    return true;
}

BE_INLINE void Sphere::ProjectOnAxis(const Vec3 &dir, float &min, float &max) const {
    float d = dir.Dot(center);
    min = d - radius;
    max = d + radius;
}

BE_NAMESPACE_END
