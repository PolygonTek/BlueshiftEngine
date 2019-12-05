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

    AABB (Axis Aligned Bounding Box)

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Vec3;
class Mat3;
class Plane;
class Sphere;
class OBB;
class Ray;

/// A 3D axis-aligned bounding box.
class BE_API AABB {
public:
    /// The default constructor does not initialize any members of this class.
    AABB() {}
    /// Constructs this AABB by specifying the minimum and maximum extending corners of the box.
    AABB(const Vec3 &mins, const Vec3 &maxs);
    /// Constructs this AABB by single point.
    explicit AABB(const Vec3 &point);

                        /// Returns true if this AABB is inside out.
    bool                IsCleared() const;

                        /// Resets mins and maxs values. If you invoke this method, IsCleared() shall returns true.
    void                Clear();

                        /// Sets to zero sized AABB.
    void                SetZero();

                        /// Returns center point of this AABB.
    Vec3                Center() const { return Vec3((b[1][0] + b[0][0]) * 0.5f, (b[1][1] + b[0][1]) * 0.5f, (b[1][2] + b[0][2]) * 0.5f); }
                        /// Returns extents.
    Vec3                Extents() const { return (b[1] - b[0]) * 0.5f; }
                        /// Returns sizes.
    Vec3                Sizes() const { return b[1] - b[0]; }

                        /// Radius of circumscribed sphere that center in origin about the AABB.
    float               OuterRadius() const;
                        /// Radius of circumscribed sphere that given center about the AABB.
    float               OuterRadius(const Vec3 &center) const;
                        /// Radius of inscribed sphere inside the AABB.
    float               InnerRadius() const;

                        /// Returns area of six sides.
    float               Area() const;
                        /// Returns volume of the AABB.
    float               Volume() const;

                        /// Accesses an element of this AABB using array notation.
                        /// 0 means mins, 1 means maxs.
    const Vec3 &        operator[](const int index) const;
    Vec3 &              operator[](const int index);

                        /// Exact compare, no epsilon.
    bool                Equals(const AABB &a) const { return (b[0].Equals(a.b[0]) && b[1].Equals(a.b[1])); }
                        /// Compare with epsilon.
    bool                Equals(const AABB &a, const float epsilon) const { return (b[0].Equals(a.b[0], epsilon) && b[1].Equals(a.b[1], epsilon)); }
                        /// Exact compare, no epsilon.
    bool                operator==(const AABB &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const AABB &rhs) const { return !Equals(rhs); }

                        /// Returns translated AABB.
    AABB                operator+(const Vec3 &rhs) const { return AABB(b[0] + rhs, b[1] + rhs); }
                        /// Returns translated AABB.
    AABB                operator-(const Vec3 &rhs) const { return AABB(b[0] - rhs, b[1] - rhs); }
                        /// Translates AABB.
    AABB &              operator+=(const Vec3 &rhs);
                        /// Translates AABB.
    AABB &              operator-=(const Vec3 &rhs);

                        /// Returns scaled AABB.
    AABB                operator*(const float &rhs) const;
                        /// Scales AABB.
    AABB &              operator*=(const float &rhs);
                        /// Returns scaled AABB.
    AABB                operator*(const Vec3 &rhs) const;
                        /// Scales AABB.
    AABB &              operator*=(const Vec3 &rhs);

                        /// Returns rotated AABB
    AABB                operator*(const Mat3 &rhs) const;
                        /// Rotates AABB.
    AABB &              operator*=(const Mat3 &rhs);

                        /// Returns added AABB.
    AABB                operator+(const AABB &rhs) const;
                        /// Adds AABB.
    AABB &              operator+=(const AABB &rhs);

                        /// Adds a point. Returns true if this AABB is expanded.
    bool                AddPoint(const Vec3 &v);
                        /// Adds a AABB. Returns true if this AABB is expanded.
    bool                AddAABB(const AABB &a);

                        /// Returns intersecting AABB with given AABB.
    AABB                Intersect(const AABB &a) const;
                        /// Intersects this AABB with the given AABB.
    AABB &              IntersectSelf(const AABB &a);
                        /// Returns expanded AABB.
    AABB                Expand(const float d) const { return AABB(Vec3(b[0][0] - d, b[0][1] - d, b[0][2] - d), Vec3(b[1][0] + d, b[1][1] + d, b[1][2] + d)); }
                        /// Expands this AABB.
    AABB &              ExpandSelf(const float d);
                        /// Returns translated AABB.
    AABB                Translate(const Vec3 &translation) const { return AABB(b[0] + translation, b[1] + translation); }
                        /// Translates this AABB.
    AABB &              TranslateSelf(const Vec3 &translation);
                        /// Returns rotated AABB.
    AABB                Rotate(const Mat3 &rotation) const;
                        /// Rotates this AABB.
    AABB &              RotateSelf(const Mat3 &rotation);

                        /// Returns this AABB is in which side of the plane.
    int                 PlaneSide(const Plane &plane, const float epsilon = ON_EPSILON) const;
                        /// Returns distance between AABB and plane.
    float               PlaneDistance(const Plane &plane) const;
    
                        /// Calculates nearest point among 8 vertices of AABB with the given point.
    void                GetNearestVertex(const Vec3 &from, Vec3 &point) const;
                        /// Calculates nearest point among 8 vertices of AABB with the given direction.
    void                GetNearestVertexFromDir(const Vec3 &dir, Vec3 &point) const;
                        /// Calculates farthest point among 8 vertices of AABB with the given point.
    void                GetFarthestVertex(const Vec3 &from, Vec3 &point) const;
                        /// Calculates farthest point among 8 vertices of AABB with the given direction.
    void                GetFarthestVertexFromDir(const Vec3 &dir, Vec3 &point) const;

                        /// Calculates closest point with the given point.
    void                GetClosestPoint(const Vec3 &from, Vec3 &point) const;
    
                        /// Returns squared distance between this AABB and the given point.
    float               DistanceSqr(const Vec3 &p) const;
                        /// Returns distance between this AABB and the given point.
    float               Distance(const Vec3 &p) const;

                        /// Tests if this AABB contain the given point.
    bool                IsContainPoint(const Vec3 &p) const;
                        /// Tests if this AABB contain the given AABB.
    bool                IsContainAABB(const AABB &a) const;

                        /// Tests if this AABB intersect with the given AABB.
    bool                IsIntersectAABB(const AABB &a, bool ignoreBorders = false) const;
                        /// Tests if this AABB intersect with the given sphere.
    bool                IsIntersectSphere(const Sphere &s) const;
                        /// Tests if this AABB intersect with the given triangle.
    bool                IsIntersectTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2) const;
                        /// Tests if this AABB intersect with the given line segment.
    bool                IsIntersectLine(const Vec3 &p1, const Vec3 &p2) const;

                        /// Intersects a ray with this AABB.
                        /// Returns false if there is no intersection.
    bool                IntersectRay(const Ray &ray, float *hitDistMin = nullptr, float *hitDistMax = nullptr) const;
    float               IntersectRay(const Ray &ray) const;
    
                        /// Sets AABB enclosing all points.
    void                SetFromPoints(const Vec3 *points, const int numPoints);

                        /// Sets AABB to enclose the moving point.
    void                SetFromPointTranslation(const Vec3 &point, const Vec3 &translation);

                        /// Sets AABB to enclose the moving AABB.
    void                SetFromAABBTranslation(const AABB &aabb, const Vec3 &translation);

                        /// Sets AABB to enclose transformed AABB.
    void                SetFromTransformedAABB(const AABB &aabb, const Vec3 &origin, const Mat3 &axis);

                        /// Sets AABB to enclose transformed AABB.
    void                SetFromTransformedAABB(const AABB &aabb, const Mat3x4 &transform);

                        /// Sets AABB to enclose transformed AABB fast.
                        /// This is not mathematically correct for non-uniform scaled transform matrix.
    void                SetFromTransformedAABBFast(const AABB &aabb, const Mat3x4 &transform);

                        /// Calculates minimum / maximum value by projecting AABB onto the given axis.
    void                ProjectOnAxis(const Vec3 &axis, float &min, float &max) const;

                        /// Calculates minimum / maximum value by projecting transformed AABB onto the given axis.
    void                ProjectOnAxis(const Vec3 &transformOrigin, const Mat3 &transformAxis, const Vec3 &axis, float &min, float &max) const;

                        /// Calcuates 8 vertices of AABB.
    void                ToPoints(Vec3 points[8]) const;

                        /// Converts to surrounding sphere.
    Sphere              ToSphere() const;

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 6; }

    static const AABB   empty;  ///< (Infinity, Infinity, Infinity) (-Infinity, -Infinity, -Infinity)
    static const AABB   zero;   ///< (0 0 0) (0 0 0)

    Vec3                b[2];   ///< Minimum/Maximum values for each axis.
};

BE_INLINE AABB::AABB(const Vec3 &mins, const Vec3 &maxs) {
    b[0] = mins;
    b[1] = maxs;
}

BE_INLINE AABB::AABB(const Vec3 &point) {
    b[0] = point;
    b[1] = point;
}

BE_INLINE const Vec3 &AABB::operator[](const int index) const {
    assert(index >= 0 && index < 2);
    return b[index];
}

BE_INLINE Vec3 &AABB::operator[](const int index) {
    assert(index >= 0 && index < 2);
    return b[index];
}

BE_INLINE AABB &AABB::operator+=(const Vec3 &rhs) {
    b[0] += rhs;
    b[1] += rhs;
    return *this;
}

BE_INLINE AABB &AABB::operator-=(const Vec3 &rhs) {
    b[0] -= rhs;
    b[1] -= rhs;
    return *this;
}

BE_INLINE AABB AABB::operator*(const float &rhs) const {
    AABB aabb = *this;
    aabb.b[0] *= rhs;
    aabb.b[1] *= rhs;
    return aabb;
}

BE_INLINE AABB &AABB::operator*=(const float &rhs) {
    this->b[0] *= rhs;
    this->b[1] *= rhs;
    return *this;
}

BE_INLINE AABB AABB::operator*(const Vec3 &rhs) const {
    AABB aabb = *this;
    aabb.b[0] *= rhs;
    aabb.b[1] *= rhs;
    return aabb;
}

BE_INLINE AABB &AABB::operator*=(const Vec3 &rhs) {
    this->b[0] *= rhs;
    this->b[1] *= rhs;
    return *this;
}

BE_INLINE AABB AABB::operator*(const Mat3 &rhs) const {
    AABB aabb;
    aabb.SetFromTransformedAABB(*this, Vec3::origin, rhs);
    return aabb;
}

BE_INLINE AABB &AABB::operator*=(const Mat3 &rhs) {
    this->SetFromTransformedAABB(*this, Vec3::origin, rhs);
    return *this;
}

BE_INLINE AABB AABB::operator+(const AABB &rhs) const {
    AABB newAABB;
    newAABB = *this;
    newAABB.AddAABB(rhs);
    return newAABB;
}

BE_INLINE AABB &AABB::operator+=(const AABB &rhs) {
    AABB::AddAABB(rhs);
    return *this;
}

BE_INLINE void AABB::Clear() {
    b[0][0] = b[0][1] = b[0][2] = Math::Infinity;
    b[1][0] = b[1][1] = b[1][2] = -Math::Infinity;
}

BE_INLINE void AABB::SetZero() {
    b[0][0] = b[0][1] = b[0][2] = b[1][0] = b[1][1] = b[1][2] = 0;
}

BE_INLINE float AABB::Area() const {
    float w = b[1][0] - b[0][0];
    float h = b[1][1] - b[0][1];
    float d = b[1][2] - b[0][2];
    return (w * h + w * d + h * d) * 2.0f;
}

BE_INLINE float AABB::Volume() const {
    if (b[0][0] >= b[1][0] || b[0][1] >= b[1][1] || b[0][2] >= b[1][2]) {
        return 0.0f;
    }    
    return ((b[1][0] - b[0][0]) * (b[1][1] - b[0][1]) * (b[1][2] - b[0][2]));
}

BE_INLINE bool AABB::IsCleared() const {
    return (b[0][0] > b[1][0] || b[0][1] > b[1][1] || b[0][2] > b[1][2]);
}

BE_INLINE bool AABB::AddPoint(const Vec3 &v) {
    bool expanded = false;
    if (v[0] < b[0][0]) {
        b[0][0] = v[0];
        expanded = true;
    }
    if (v[0] > b[1][0]) {
        b[1][0] = v[0];
        expanded = true;
    }
    if (v[1] < b[0][1]) {
        b[0][1] = v[1];
        expanded = true;
    }
    if (v[1] > b[1][1]) {
        b[1][1] = v[1];
        expanded = true;
    }
    if (v[2] < b[0][2]) {
        b[0][2] = v[2];
        expanded = true;
    }
    if (v[2] > b[1][2]) {
        b[1][2] = v[2];
        expanded = true;
    }
    return expanded;
}

BE_INLINE bool AABB::AddAABB(const AABB &a) {
    bool expanded = false;
    if (a.b[0][0] < b[0][0]) {
        b[0][0] = a.b[0][0];
        expanded = true;
    }    
    if (a.b[0][1] < b[0][1]) {
        b[0][1] = a.b[0][1];
        expanded = true;
    }
    if (a.b[0][2] < b[0][2]) {
        b[0][2] = a.b[0][2];
        expanded = true;
    }    
    if (a.b[1][0] > b[1][0]) {
        b[1][0] = a.b[1][0];
        expanded = true;
    }    
    if (a.b[1][1] > b[1][1]) {
        b[1][1] = a.b[1][1];
        expanded = true;
    }    
    if (a.b[1][2] > b[1][2]) {
        b[1][2] = a.b[1][2];
        expanded = true;
    }    
    return expanded;
}

BE_INLINE AABB AABB::Intersect(const AABB &a) const {
    AABB n;
    n.b[0][0] = (a.b[0][0] > b[0][0]) ? a.b[0][0] : b[0][0];
    n.b[0][1] = (a.b[0][1] > b[0][1]) ? a.b[0][1] : b[0][1];
    n.b[0][2] = (a.b[0][2] > b[0][2]) ? a.b[0][2] : b[0][2];
    n.b[1][0] = (a.b[1][0] < b[1][0]) ? a.b[1][0] : b[1][0];
    n.b[1][1] = (a.b[1][1] < b[1][1]) ? a.b[1][1] : b[1][1];
    n.b[1][2] = (a.b[1][2] < b[1][2]) ? a.b[1][2] : b[1][2];
    return n;
}

BE_INLINE AABB &AABB::IntersectSelf(const AABB &a) {
    if (a.b[0][0] > b[0][0]) {
        b[0][0] = a.b[0][0];
    }
    if (a.b[0][1] > b[0][1]) {
        b[0][1] = a.b[0][1];
    }
    if (a.b[0][2] > b[0][2]) {
        b[0][2] = a.b[0][2];
    }
    if (a.b[1][0] < b[1][0]) {
        b[1][0] = a.b[1][0];
    }    
    if (a.b[1][1] < b[1][1]) {
        b[1][1] = a.b[1][1];
    }
    if (a.b[1][2] < b[1][2]) {
        b[1][2] = a.b[1][2];
    }
    return *this;
}

BE_INLINE AABB &AABB::ExpandSelf(const float d) {
    b[0][0] -= d;
    b[0][1] -= d;
    b[0][2] -= d;
    b[1][0] += d;
    b[1][1] += d;
    b[1][2] += d;
    return *this;
}

BE_INLINE AABB &AABB::TranslateSelf(const Vec3 &translation) {
    b[0] += translation;
    b[1] += translation;
    return *this;
}

BE_INLINE AABB AABB::Rotate(const Mat3 &rotation) const {
    AABB aabb;
    aabb.SetFromTransformedAABB(*this, Vec3::origin, rotation);
    return aabb;
}

BE_INLINE AABB &AABB::RotateSelf(const Mat3 &rotation) {
    SetFromTransformedAABB(*this, Vec3::origin, rotation);
    return *this;
}

BE_INLINE bool AABB::IsContainPoint(const Vec3 &p) const {
    if (p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2] || 
        p[0] > b[1][0] || p[1] > b[1][1] || p[2] > b[1][2]) {
        return false;
    }
    return true;
}

BE_INLINE bool AABB::IsContainAABB(const AABB &a) const {
    if (a.b[0][0] < b[0][0] || a.b[0][1] < b[0][1] || a.b[0][2] < b[0][2] ||
        a.b[1][0] > b[1][0] || a.b[1][1] > b[1][1] || a.b[1][2] > b[1][2]) {
        return false;
    }
    return true;
}

BE_INLINE bool AABB::IsIntersectAABB(const AABB &a, bool ignoreBorders) const {
    if (!ignoreBorders) {
        if (a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[1][2] < b[0][2] ||
            a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] || a.b[0][2] > b[1][2]) {
            return false;
        }
    } else {
        if (a.b[1][0] <= b[0][0] || a.b[1][1] <= b[0][1] || a.b[1][2] <= b[0][2] ||
            a.b[0][0] >= b[1][0] || a.b[0][1] >= b[1][1] || a.b[0][2] >= b[1][2]) {
            return false;
        }
    }
    return true;
}

BE_INLINE bool AABB::IsIntersectSphere(const Sphere &s) const {
    if (DistanceSqr(s.center) > s.radius * s.radius) {
        return false;
    }
    return true;
}

BE_INLINE Sphere AABB::ToSphere() const {
    Sphere sphere;
    sphere.center = (b[0] + b[1]) * 0.5f;
    sphere.radius = (b[1] - sphere.center).Length();
    return sphere;
}

BE_NAMESPACE_END
