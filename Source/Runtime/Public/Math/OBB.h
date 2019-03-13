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

    OBB (Oriented Bounding Box)

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Vec3;
class Mat3;
class Rotation;
class Plane;
class AABB;
class Frustum;
class Ray;

/// A 3D oriented bounding box.
class BE_API OBB {
public:
    /// The default constructor does not initialize any members of this class.
    OBB() {}
    constexpr OBB(const Vec3 &center, const Vec3 &extents, const Mat3 &axis);
    OBB(const AABB &aabb, const Vec3 &origin, const Mat3 &axis);
    OBB(const AABB &aabb, const Mat3x4 &transform);
    explicit OBB(const Vec3 &point);
    explicit OBB(const AABB &aabb);

                        /// Returns true if this OBB is inside out.
    bool                IsCleared() const;
                        /// Resets this OBB. If you invoke this method, IsCleared() shall returns true.
    void                Clear();

                        /// Sets center of this OBB.
    void                SetCenter(const Vec3 &center) { this->center = center; }
                        /// Sets extents of this OBB.
    void                SetExtents(const Vec3 &extents) { this->extents = extents; }
                        /// Sets axis of this OBB.
    void                SetAxis(const Mat3 &axis) { this->axis = axis; }

                        /// Returns center of the OBB.
    const Vec3 &        Center() const { return center; }
                        /// Returns extents of the OBB.
    const Vec3 &        Extents() const { return extents; }
                        /// Returns the axis of the OBB.
    const Mat3 &        Axis() const { return axis; }

                        /// Sets to zero sized OBB.
    void                SetZero();

                        /// Returns the volume of the OBB.
    float               Volume() const;

                        /// Exact compare, no epsilon
    bool                Equals(const OBB &a) const;
                        /// Compare with epsilon
    bool                Equals(const OBB &a, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const OBB &a) const { return Equals(a); }
                        /// Exact compare, no epsilon
    bool                operator!=(const OBB &a) const { return !Equals(a); }

                        /// Returns translated OBB.
    OBB                 operator+(const Vec3 &t) const;
                        /// Translates OBB.
    OBB &               operator+=(const Vec3 &t);

                        /// Returns rotated OBB.
    OBB                 operator*(const Mat3 &r) const;
                        /// Rotates OBB.
    OBB &               operator*=(const Mat3 &r);

                        /// Returns added OBB.
    OBB                 operator+(const OBB &a) const;
                        /// Adds OBB.
    OBB &               operator+=(const OBB &a);

                        /// Adds a point. Returns true if this OBB is expanded.
    bool                AddPoint(const Vec3 &v);
                        /// Adds a OBB. Returns true if this OBB is expanded.
    bool                AddOBB(const OBB &a);

                        /// Returns expanded OBB.
    OBB                 Expand(const float d) const;
                        /// Expands this OBB.
    OBB &               ExpandSelf(const float d);
                        /// Returns translated OBB.
    OBB                 Translate(const Vec3 &translation) const;
                        /// Translates this OBB.
    OBB &               TranslateSelf(const Vec3 &translation);
                        /// Returns rotated OBB.
    OBB                 Rotate(const Mat3 &rotation) const;
                        /// Rotates this OBB.
    OBB &               RotateSelf(const Mat3 &rotation);

                        /// Returns this OBB is in which side of the plane.
    int                 PlaneSide(const Plane &plane, const float epsilon = ON_EPSILON) const;
                        /// Returns distance between OBB and plane.
    float               PlaneDistance(const Plane &plane) const;

                        /// Calculates nearest point among 8 vertices of OBB with the given point.
    void                GetNearestVertex(const Vec3 &from, Vec3 &point) const;
                        /// Calculates farthest point among 8 vertices of OBBB with the given point.
    void                GetFarthestVertex(const Vec3 &from, Vec3 &point) const;

                        /// Calculates closest point with the given point.
    void                GetClosestPoint(const Vec3 &from, Vec3 &point) const;

                        /// Calculates the silhouette of the OBB
    int                 GetProjectionSilhouetteVerts(const Vec3 &projectionOrigin, Vec3 silVerts[6]) const;
    int                 GetParallelProjectionSilhouetteVerts(const Vec3 &projectionDir, Vec3 silVerts[6]) const;

                        /// Returns squared distance between this OBB and the given point.
    float               DistanceSqr(const Vec3 &p) const;
                        /// Returns distance between this OBB and the given point.
    float               Distance(const Vec3 &p) const;

                        /// Tests if this OBB contain the given point.
    bool                IsContainPoint(const Vec3 &p) const;
                        /// Tests if this OBB intersect with the given OBB.
    bool                IsIntersectOBB(const OBB &b, float epsilon = 1e-3f) const;
                        /// Tests if this OBB intersect with the given sphere.
    bool                IsIntersectSphere(const Sphere &sphere) const;
                        /// Tests if this OBB intersect with the given line segment.
    bool                IsIntersectLine(const Vec3 &p1, const Vec3 &p2) const;

                        /// Intersects a ray with this OBB.
                        /// Returns false if there is no intersection.
    bool                IntersectRay(const Ray &ray, float *hitDistMin = nullptr, float *hitDistMax = nullptr) const;

                        /// Sets OBB with the given points using PCA (Principal Component Analysis).
    void                SetFromPoints(const Vec3 *points, int numPoints);

                        /// Calculates minimum / maximum value by projecting OBB onto the given axis.
    void                ProjectOnAxis(const Vec3 &axis, float &min, float &max) const;

                        /// Calculates minimum / maximum values by projecting OBB onto the given axis.
    void                ProjectOnAxis(const Mat3 &axis, AABB &minmaxs) const;

                        /// Calculates local bounds [-1, 1] by projecting sphere in this OBB volume.
                        /// Returns false if the sphere is completely outside this OBB.
    bool                ProjectionBounds(const Sphere &sphere, AABB &localBounds) const;

                        /// Calculates local bounds [-1, 1] by projecting OBB in this OBB volume.
                        /// Returns false if the OBB is completely outside this OBB.
    bool                ProjectionBounds(const OBB &obb, AABB &localBounds) const;

                        /// Calculates local bounds [-1, 1] by projecting frustum in this OBB volume.
                        /// Returns false if the frustum is completely outside this OBB.
    bool                ProjectionBounds(const Frustum &frustum, AABB &localBounds) const;

                        /// Calcuates 8 vertices of OBB.
    void                ToPoints(Vec3 points[8]) const;

                        /// Converts to AABB.
    AABB                ToAABB() const;

                        /// Converts to surrounding sphere.
    Sphere              ToSphere() const;

    static const OBB    zero;

private:
    Vec3                center;
    Vec3                extents;
    Mat3                axis;
};

BE_INLINE constexpr OBB::OBB(const Vec3 &inCenter, const Vec3 &inExtents, const Mat3 &inAxis) :
    center(inCenter), extents(inExtents), axis(inAxis) {
}

BE_INLINE OBB::OBB(const Vec3 &point) {
    this->center = point;
    this->extents.SetFromScalar(0);
    this->axis.SetIdentity();
}

BE_INLINE OBB::OBB(const AABB &aabb) {
    this->center = (aabb[0] + aabb[1]) * 0.5f;
    this->extents = aabb[1] - this->center;
    this->axis.SetIdentity();
}

BE_INLINE OBB::OBB(const AABB &aabb, const Vec3 &origin, const Mat3 &axis) {
    this->center = (aabb[0] + aabb[1]) * 0.5f;
    this->extents = aabb[1] - this->center;
    this->center = origin + axis * this->center;
    this->axis = axis;
}

BE_INLINE OBB::OBB(const AABB &aabb, const Mat3x4 &transform) {
    this->center = (aabb[0] + aabb[1]) * 0.5f;
    this->extents = aabb[1] - this->center;
    this->center = transform * this->center;
    this->axis = transform.ToMat3().OrthoNormalize();
}

BE_INLINE OBB OBB::operator+(const Vec3 &t) const {
    return OBB(center + t, extents, axis);
}

BE_INLINE OBB &OBB::operator+=(const Vec3 &t) {
    center += t;
    return *this;
}

BE_INLINE OBB OBB::operator*(const Mat3 &r) const {
    return OBB(center * r, extents, axis * r);
}

BE_INLINE OBB &OBB::operator*=(const Mat3 &r) {
    center *= r;
    axis *= r;
    return *this;
}

BE_INLINE OBB OBB::operator+(const OBB &a) const {
    OBB newBox;
    newBox = *this;
    newBox.AddOBB(a);
    return newBox;
}

BE_INLINE OBB &OBB::operator+=(const OBB &a) {
    OBB::AddOBB(a);
    return *this;
}

BE_INLINE bool OBB::Equals(const OBB &a) const {
    return (center.Equals(a.center) && extents.Equals(a.extents) && axis.Equals(a.axis));
}

BE_INLINE bool OBB::Equals(const OBB &a, const float epsilon) const {
    return (center.Equals(a.center, epsilon) && extents.Equals(a.extents, epsilon) && axis.Equals(a.axis, epsilon));
}

BE_INLINE void OBB::Clear() {
    center.SetFromScalar(0);
    extents[0] = extents[1] = extents[2] = -Math::Infinity;
    axis.SetIdentity();
}

BE_INLINE void OBB::SetZero() {
    center.SetFromScalar(0);
    extents.SetFromScalar(0);
    axis.SetIdentity();
}

BE_INLINE float OBB::Volume() const {
    return (extents * 2.0f).LengthSqr();
}

BE_INLINE bool OBB::IsCleared() const {
    return extents[0] < 0.0f;
}

BE_INLINE OBB OBB::Expand(const float d) const {
    return OBB(center, extents + Vec3(d, d, d), axis);
}

BE_INLINE OBB &OBB::ExpandSelf(const float d) {
    extents[0] += d;
    extents[1] += d;
    extents[2] += d;
    return *this;
}

BE_INLINE OBB OBB::Translate(const Vec3 &translation) const {
    return OBB(center + translation, extents, axis);
}

BE_INLINE OBB &OBB::TranslateSelf(const Vec3 &translation) {
    center += translation;
    return *this;
}

BE_INLINE OBB OBB::Rotate(const Mat3 &rotation) const {
    return OBB(center * rotation, extents, axis * rotation);
}

BE_INLINE OBB &OBB::RotateSelf(const Mat3 &rotation) {
    center *= rotation;
    axis *= rotation;
    return *this;
}

BE_INLINE bool OBB::IsContainPoint(const Vec3 &p) const {
    Vec3 lp = p - center;
    if (Math::Fabs(lp.Dot(axis[0])) > extents[0] || 
        Math::Fabs(lp.Dot(axis[1])) > extents[1] || 
        Math::Fabs(lp.Dot(axis[2])) > extents[2]) {
        return false;
    }
    return true;
}

BE_INLINE void OBB::ProjectOnAxis(const Vec3 &axis, float &min, float &max) const {
    float d1 = axis.Dot(center);
    float d2 = Math::Fabs(extents[0] * this->axis[0].Dot(axis)) +
               Math::Fabs(extents[1] * this->axis[1].Dot(axis)) +
               Math::Fabs(extents[2] * this->axis[2].Dot(axis));
    min = d1 - d2;
    max = d1 + d2;
}

BE_INLINE void OBB::ProjectOnAxis(const Mat3 &axis, AABB &minmaxs) const {
    for (int i = 0; i < 3; i++) {
        float d1 = axis[i].Dot(center);
        float d2 = Math::Fabs(extents[0] * this->axis[0].Dot(axis[i])) +
                   Math::Fabs(extents[1] * this->axis[1].Dot(axis[i])) +
                   Math::Fabs(extents[2] * this->axis[2].Dot(axis[i]));
        minmaxs[0][i] = d1 - d2;
        minmaxs[1][i] = d1 + d2;
    }
}

BE_INLINE Sphere OBB::ToSphere() const {
    return Sphere(center, extents.Length());
}

BE_NAMESPACE_END
