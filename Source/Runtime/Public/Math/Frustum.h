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

    Frustum

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Vec3;
class Mat3;
class Plane;
class AABB;
class Sphere;
class OBB;
class Ray;

/// A perspective viewing frustum.
class BE_API Frustum {
public:
    Frustum();
    
                    /// Returns origin.
    const Vec3 &    GetOrigin() const { return origin; }
                    /// Returns axis.
    const Mat3 &    GetAxis() const { return axis; }
                    /// Returns center of frustum.
    Vec3            GetCenter() const;
                    /// Returns distance of near plane from frustum origin.
    float           GetNearDistance() const { return dNear; }
                    /// Returns distance of far plane from frustum origin.
    float           GetFarDistance() const { return dFar; }
                    /// Returns half the width at the far plane.
    float           GetLeft() const { return dLeft; }
                    /// Returns half the height at the far plane.
    float           GetUp() const { return dUp; }

    void            SetOrigin(const Vec3 &origin) { this->origin = origin; }
    void            SetAxis(const Mat3 &axis) { this->axis = axis; }
    void            SetSize(float dNear, float dFar, float dLeft, float dUp);

                    /// Moves near plane.
    void            MoveNearDistance(float dNear);
                    /// Moves far plane.
    void            MoveFarDistance(float dFar);

                    /// Tests if far plane distance is farther than near
    bool            IsValid() const { return dFar > dNear; }

                    /// Returns expanded frustum.
    Frustum         Expand(float d) const &;
    Frustum &&      Expand(float d) && { ExpandSelf(d); return std::move(*this); }
                    /// Expands this frustum.
    Frustum &       ExpandSelf(float d);

                    /// Returns translated frustum.
    Frustum         Translate(const Vec3 &translation) const &;
    Frustum &&      Translate(const Vec3 &translation) && { TranslateSelf(translation); return std::move(*this); }
                    /// Translates this frustum.
    Frustum &       TranslateSelf(const Vec3 &translation);

                    /// Returns rotated frustum.
    Frustum         Rotate(const Mat3 &rotation) const &;
    Frustum &&      Rotate(const Mat3 &rotation) && { RotateSelf(rotation); return std::move(*this); }
                    /// Rotates this frustum.
    Frustum &       RotateSelf(const Mat3 &rotation);

                    /// Returns this frustum is in which side of the plane.
    int             PlaneSide(const Plane &plane, const float epsilon = ON_EPSILON) const;
                    /// Returns distance between frustum and plane.
    float           PlaneDistance(const Plane &plane) const;

                    /// Fast culling but might not cull everything outside the frustum.
                    /// Tests if any of the planes of the frustum can be used as a separating plane.
    bool            CullPoint(const Vec3 &point) const;
    bool            CullAABB(const AABB &bounds) const;
    bool            CullOBB(const OBB &box) const;
    bool            CullSphere(const Sphere &sphere) const;
    bool            CullFrustum(const Frustum &frustum) const;

                    /// Tests if this frustum contain the given point.
    bool            IsContainPoint(const Vec3 &point) const;
                    /// Tests if this frustum intersect with the given AABB.
    bool            IsIntersectAABB(const AABB &bounds) const;
                    /// Tests if this frustum intersect with the given OBB.
    bool            IsIntersectOBB(const OBB &box) const;
                    /// Tests if this frustum intersect with the given sphere.
    bool            IsIntersectSphere(const Sphere &sphere) const;
                    /// Tests if this frustum intersect with the given frustum.
    bool            IsIntersectFrustum(const Frustum &frustum) const;
                    /// Tests if this frustum intersect with the given line segment.
    bool            IsIntersectLine(const Vec3 &p1, const Vec3 &p2) const;

                    /// Returns true if the ray intersects the bounds.
                    /// The ray can intersect the bounds in both directions from the start point.
                    /// If start is inside the frustum then hitDistMin < 0 and hitDistMax > 0.
    bool            IntersectRay(const Ray &ray, float *hitDistMin, float *hitDistMax) const;
    float           IntersectRay(const Ray &ray) const;

                    // returns true if the projection origin is far enough away from the bounding volume to create a valid frustum
                    /// Creates a frustum which contains the projection of the AABB.
    bool            FromProjection(const AABB &bounds, const Vec3 &projectionOrigin, const float dFar);
                    /// Creates a frustum which contains the projection of the OBB.
    bool            FromProjection(const OBB &box, const Vec3 &projectionOrigin, const float dFar);
                    /// Creates a frustum which contains the projection of the sphere.
    bool            FromProjection(const Sphere &sphere, const Vec3 &projectionOrigin, const float dFar);

                    // moves the far plane so it extends just beyond the bounding volume
                    /// Returns false if no part of the AABB extends beyond the near plane.
    bool            ConstrainToAABB(const AABB &bounds);
                    /// Returns false if no part of the OBB extends beyond the near plane.
    bool            ConstrainToOBB(const OBB &box);
                    /// Returns false if no part of the sphere extends beyond the near plane.
    bool            ConstrainToSphere(const Sphere &sphere);
                    /// Returns false if no part of the frustum extends beyond the near plane.
    bool            ConstrainToFrustum(const Frustum &frustum);

                    /// Converts to OBB.
    OBB             ToOBB() const;
                    /// Converts to minimum sphere enclosing frustum.
    Sphere          ToMinimumSphere() const;
                    /// Calculates planes point outwards.
    void            ToPlanes(Plane planes[6]) const;
                    /// Calculates 8 corners of the frustum.
    void            ToPoints(Vec3 points[8]) const;

                    /// Calculates minimum / maximum value by projecting frustum onto the given axis.
    void            ProjectOnAxis(const Vec3 &axis, float &min, float &max) const;
                    /// Calculates minimum / maximum values by projecting frustum onto the given axis.
    void            ProjectOnAxis(const Mat3 &axis, AABB &minmaxs) const;

                    // Calculates the bounds for the projection in this frustum.
    bool            ProjectionBounds(const AABB &bounds, AABB &projectionBounds) const;
    bool            ProjectionBounds(const OBB &box, AABB &projectionBounds) const;
    bool            ProjectionBounds(const Sphere &sphere, AABB &projectionBounds) const;
    bool            ProjectionBounds(const Frustum &frustum, AABB &projectionBounds) const;

                    /// Calculates the bounds for the projection in this frustum of the given frustum clipped to the given box.
    bool            ClippedProjectionBounds(const Frustum &frustum, const OBB &clipBox, AABB &projectionBounds) const;

private:
    bool            CullLocalOBB(const Vec3 &localOrigin, const Vec3 &extents, const Mat3 &localAxis) const;
    bool            CullLocalFrustum(const Frustum &localFrustum, const Vec3 indexPoints[8], const Vec3 cornerVecs[4]) const;
    bool            CullLocalWinding(const Vec3 *points, const int numPoints, int *pointCull) const;
    bool            BoundsCullLocalFrustum(const AABB &bounds, const Frustum &localFrustum, const Vec3 indexPoints[8], const Vec3 cornerVecs[4]) const;
    bool            LocalLineIntersection(const Vec3 &start, const Vec3 &end) const;
    bool            LocalRayIntersection(const Vec3 &start, const Vec3 &dir, float &scale1, float &scale2) const;
    bool            LocalFrustumIntersectsFrustum(const Vec3 points[8], const bool testFirstSide) const;
    bool            LocalFrustumIntersectsBounds(const Vec3 points[8], const AABB &bounds) const;
    void            ToClippedPoints(const float fractions[4], Vec3 points[8]) const;
    void            ToIndexPoints(Vec3 indexPoints[8]) const;
    void            ToIndexPointsAndCornerVecs(Vec3 indexPoints[8], Vec3 cornerVecs[4]) const;
    void            ProjectOnAxis(const Vec3 indexPoints[8], const Vec3 cornerVecs[4], const Vec3 &dir, float &min, float &max) const;
    void            AddLocalLineToProjectionBoundsSetCull(const Vec3 &start, const Vec3 &end, int &startCull, int &endCull, AABB &bounds) const;
    void            AddLocalLineToProjectionBoundsUseCull(const Vec3 &start, const Vec3 &end, int startCull, int endCull, AABB &bounds) const;
    bool            AddLocalCapsToProjectionBounds(const Vec3 endPoints[4], const int endPointCull[4], const Vec3 &point, int pointCull, int pointClip, AABB &projectionBounds) const;
    bool            AABBRayIntersection(const AABB &bounds, const Vec3 &start, const Vec3 &dir, float &scale1, float &scale2) const;
    void            ClipFrustumToOBB(const OBB &box, float clipFractions[4], int clipPlanes[4]) const;
    bool            ClipLine(const Vec3 localPoints[8], const Vec3 points[8], int startIndex, int endIndex, Vec3 &start, Vec3 &end, int &startClip, int &endClip) const;

    Vec3            origin;     ///< Frustum origin
    Mat3            axis;       ///< Frustum orientation
    float           dNear;      ///< Distance of near plane, dNear >= 0.0f
    float           dFar;       ///< Distance of far plane, dFar > dNear
    float           dLeft;      ///< Half the width at the far plane
    float           dUp;        ///< Half the height at the far plane
    float           invFar;     ///< 1.0f / dFar
};

BE_INLINE Frustum::Frustum() :
    dNear(0.0f), dFar(0.0f) {
}

BE_INLINE void Frustum::SetSize(float dNear, float dFar, float dLeft, float dUp) {
    assert(dNear >= 0.0f && dFar > dNear && dLeft > 0.0f && dUp > 0.0f);
    this->dNear = dNear;
    this->dFar = dFar;
    this->dLeft = dLeft;
    this->dUp = dUp;
    this->invFar = 1.0f / dFar;
}

BE_INLINE void Frustum::MoveNearDistance(float dNear) {
    assert(dNear >= 0.0f);
    this->dNear = dNear;
}

BE_INLINE void Frustum::MoveFarDistance(float dFar) {
    assert(dFar > this->dNear);
    float scale = dFar / this->dFar;
    this->dFar = dFar;
    this->dLeft *= scale;
    this->dUp *= scale;
    this->invFar = 1.0f / dFar;
}

BE_INLINE Vec3 Frustum::GetCenter() const {
    return (origin + axis[0] * ((dFar - dNear) * 0.5f));
}

BE_INLINE Frustum Frustum::Expand(float d) const & {
    Frustum f = *this;
    f.origin -= d * f.axis[0];
    f.dFar += 2.0f * d;
    f.dLeft = f.dFar * dLeft * invFar;
    f.dUp = f.dFar * dUp * invFar;
    f.invFar = 1.0f / dFar;
    return f;
}

BE_INLINE Frustum &Frustum::ExpandSelf(float d) {
    origin -= d * axis[0];
    dFar += 2.0f * d;
    dLeft = dFar * dLeft * invFar;
    dUp = dFar * dUp * invFar;
    invFar = 1.0f / dFar;
    return *this;
}

BE_INLINE Frustum Frustum::Translate(const Vec3 &translation) const & {
    Frustum f = *this;
    f.origin += translation;
    return f;
}

BE_INLINE Frustum &Frustum::TranslateSelf(const Vec3 &translation) {
    origin += translation;
    return *this;
}

BE_INLINE Frustum Frustum::Rotate(const Mat3 &rotation) const & {
    Frustum f = *this;
    f.axis *= rotation;
    return f;
}

BE_INLINE Frustum &Frustum::RotateSelf(const Mat3 &rotation) {
    axis *= rotation;
    return *this;
}

BE_NAMESPACE_END
