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

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

// NOTE: Math::Infinity is big number not real float INFINITY.
const AABB AABB::empty(Vec3::infinity, -Vec3::infinity);
const AABB AABB::zero(Vec3::zero, Vec3::zero);

float AABB::OuterRadius() const {
    float total = 0.0f;
    for (int i = 0; i < 3; i++) {
        float b0 = (float)Math::Fabs(b[0][i]);
        float b1 = (float)Math::Fabs(b[1][i]);
        float bmax = Max(b0, b1);
        total += bmax * bmax;
    }
    return Math::Sqrt(total);
}

float AABB::OuterRadius(const Vec3 &center) const {
    float total = 0.0f;
    for (int i = 0; i < 3; i++) {
        float b0 = (float)Math::Fabs(center[i] - b[0][i]);
        float b1 = (float)Math::Fabs(b[1][i] - center[i]);
        float bmax = Max(b0, b1);
        total += bmax * bmax;
    }
    return Math::Sqrt(total);
}

float AABB::InnerRadius() const {
    float bmin = FLT_MAX;
    Vec3 center = Center();
    for (int i = 0; i < 3; i++) {
        float b0 = (float)Math::Fabs(center[i] - b[0][i]);
        float b1 = (float)Math::Fabs(b[1][i] - center[i]);
        bmin = Min3(bmin, b0, b1);
    }
    return bmin;
}

int AABB::PlaneSide(const Plane &plane, const float epsilon) const {
    Vec3 center = (b[0] + b[1]) * 0.5f;

    float d1 = plane.Distance(center);
    float d2 = (b[1] - center).AbsDot(plane.normal);

    if (d1 - d2 > epsilon) {
        return Plane::Side::Front;
    }

    if (d1 + d2 < -epsilon) {
        return Plane::Side::Back;
    }

    return Plane::Side::Cross;
}

float AABB::PlaneDistance(const Plane &plane) const {
    Vec3 center = (b[0] + b[1]) * 0.5f;

    // d1 = center 와 평면의 거리
    float d1 = plane.Distance(center);
    // d2 = extent 벡터를 분리축(평면의 normal) 에 투영한 거리
    float d2 = (b[1] - center).AbsDot(plane.normal);

    if (d1 - d2 > 0.0f) {
        return d1 - d2;
    }

    if (d1 + d2 < 0.0f) {
        return d1 + d2;
    }

    return 0.0f;
}

void AABB::GetNearestVertex(const Vec3 &from, Vec3 &point) const {
    Vec3 localVec = from - Center();
    point.x = localVec.x > 0 ? b[1].x : b[0].x;
    point.y = localVec.y > 0 ? b[1].y : b[0].y;
    point.z = localVec.z > 0 ? b[1].z : b[0].z;
}

void AABB::GetNearestVertexFromDir(const Vec3 &dir, Vec3 &point) const {
    point.x = dir.x < 0 ? b[1].x : b[0].x;
    point.y = dir.y < 0 ? b[1].y : b[0].y;
    point.z = dir.z < 0 ? b[1].z : b[0].z;
}

void AABB::GetFarthestVertex(const Vec3 &from, Vec3 &point) const {
    Vec3 localVec = from - Center();
    point.x = localVec.x < 0 ? b[1].x : b[0].x;
    point.y = localVec.y < 0 ? b[1].y : b[0].y;
    point.z = localVec.z < 0 ? b[1].z : b[0].z;
}

void AABB::GetFarthestVertexFromDir(const Vec3 &dir, Vec3 &point) const {
    point.x = dir.x > 0 ? b[1].x : b[0].x;
    point.y = dir.y > 0 ? b[1].y : b[0].y;
    point.z = dir.z > 0 ? b[1].z : b[0].z;
}

void AABB::GetClosestPoint(const Vec3 &from, Vec3 &point) const {
    point = from;

    Clamp(point.x, b[0].x, b[1].x);
    Clamp(point.y, b[0].y, b[1].y);
    Clamp(point.z, b[0].z, b[1].z);
}

float AABB::DistanceSqr(const Vec3 &p) const {
    float dsq = 0.0f;
    float d;

    for (int i = 0; i < 3; i++) {
        if (p[i] < b[0][i]) {
            d = b[0][i] - p[i];
            dsq += d * d;
        }
        else if (p[i] > b[1][i]) {
            d = p[i] - b[1][i];
            dsq += d * d;
        }
    }

    return dsq;
}

float AABB::Distance(const Vec3 &p) const {
    float dsq = DistanceSqr(p);
    return Math::Sqrt(dsq);
}

bool AABB::IsIntersectLine(const Vec3 &start, const Vec3 &end) const {
    Vec3 e = b[1] - b[0];
    Vec3 d = end - start;
    Vec3 m = start + end - b[0] - b[1];

    // X 축 투영 거리 비교
    float adx = Math::Fabs(d.x);
    if (Math::Fabs(m.x) > e.x + adx) {
        return false;
    }

    // Y 축 투영 거리 비교
    float ady = Math::Fabs(d.y);
    if (Math::Fabs(m.y) > e.y + ady) {
        return false;
    }
    
    // Z 축 투영 거리 비교
    float adz = Math::Fabs(d.z);
    if (Math::Fabs(m.z) > e.z + adz) {
        return false;
    }

    adx += 0.00001f;
    ady += 0.00001f;
    adz += 0.00001f;

    // 선분의 방향과 AABB 의 축들과의 외적한 축들에 대해서 검사
    //Vec3 cross = m.Cross(d);

    if (Math::Fabs(m.y * d.z - m.z * d.y) > e.y * adz + e.z * ady) {
        return false;
    }

    if (Math::Fabs(m.z * d.x - m.x * d.z) > e.x * adz + e.z * adx) {
        return false;
    }
    
    if (Math::Fabs(m.x * d.y - m.y * d.x) > e.x * ady + e.y * adx) {
        return false;
    }

    return true;
}

bool AABB::IsIntersectPlane(const Plane &plane) const {
    Vec3 center = (b[0] + b[1]) * 0.5f;
    Vec3 extents = b[1] - center;

    // Compute the projection interval radius of the AABB onto L(t) = aabb.center + t * plane.normal;
    float r = extents[0] * Math::Abs(plane.normal[0]) + extents[1] * Math::Abs(plane.normal[1]) + extents[2] * Math::Abs(plane.normal[2]);
    // Compute the distance of the box center from plane.
    float s = plane.Distance(center);

    return Math::Abs(s) <= r;
}

bool AABB::IsIntersectOBB(const OBB &obb, float epsilon) const {
    return obb.IsIntersectAABB(*this, epsilon);
}

bool AABB::IntersectRay(const Ray &ray, float *hitDistMin, float *hitDistMax) const {
    float tmin = -FLT_MAX;
    float tmax = FLT_MAX;

    // For all three slabs.
    for (int i = 0; i < 3; i++) {
        if (Math::Fabs(ray.dir[i]) < 0.000001f) {
            // Ray is parallel to slab. No hit if origin not within slab.
            if (ray.origin[i] < b[0][i] || ray.origin[i] > b[1][i]) {
                return false;
            }
        } else {
            // Compute intersection ray.dir value of ray with near and far plane of slab.
            float ood = 1.0f / ray.dir[i];
            float t1 = (b[0][i] - ray.origin[i]) * ood;
            float t2 = (b[1][i] - ray.origin[i]) * ood;

            // Make t1 be intersection with near plane, t2 with far plane.
            if (t1 > t2) {
                Swap(t1, t2);
            }

            // Compute the intersection of slab intersection intervals.
            tmin = Max(tmin, t1);
            tmax = Min(tmax, t2);

            // Exit with no collision as soon as slab intersection becomes empty.
            if (tmin > tmax) {
                return false;
            }
        }
    }

    if (hitDistMin) {
        *hitDistMin = tmin;
    }
    if (hitDistMax) {
        *hitDistMax = tmax;
    }
    return true;
}

float AABB::IntersectRay(const Ray &ray) const {
    float hitDistMin;

    if (IntersectRay(ray, &hitDistMin, nullptr)) {
        return hitDistMin;
    }
    return FLT_MAX;
}

void AABB::SetFromPoints(const Vec3 *points, int numPoints) {
    b[0][0] = Math::Infinity;
    b[0][1] = Math::Infinity;
    b[0][2] = Math::Infinity;

    b[1][0] = -Math::Infinity;
    b[1][1] = -Math::Infinity;
    b[1][2] = -Math::Infinity;

    for (int i = 0; i < numPoints; i++)	{
        for (int j = 0; j < 3; j++)	{
            if (points[i][j] < b[0][j]) {
                b[0][j] = points[i][j];
            }

            if (points[i][j] > b[1][j]) {
                b[1][j] = points[i][j];
            }
        }
    }
}

void AABB::SetFromPointTranslation(const Vec3 &point, const Vec3 &translation) {
    for (int i = 0; i < 3; i++) {
        if (translation[i] < 0.0f) {
            b[0][i] = point[i] + translation[i];
            b[1][i] = point[i];
        } else {
            b[0][i] = point[i];
            b[1][i] = point[i] + translation[i];
        }
    }
}

void AABB::SetFromAABBTranslation(const AABB &aabb, const Vec3 &translation) {
    for (int i = 0; i < 3; i++) {
        if (translation[i] < 0.0f) {
            b[0][i] = aabb[0][i] + translation[i];
            b[1][i] = aabb[1][i];
        } else {
            b[0][i] = aabb[0][i];
            b[1][i] = aabb[1][i] + translation[i];
        }
    }
}

void AABB::SetFromTransformedAABB(const AABB &aabb, const Vec3 &origin, const Mat3 &axis) {
    Vec3 center = (aabb[0] + aabb[1]) * 0.5f;
    Vec3 extents = aabb[1] - center;

    Vec3 rotatedExtents;
    for (int i = 0; i < 3; i++) {
        rotatedExtents[i] = 
            Math::Fabs(extents[0] * axis[0][i]) + 
            Math::Fabs(extents[1] * axis[1][i]) + 
            Math::Fabs(extents[2] * axis[2][i]);
    }

    center = axis * center + origin;
    b[0] = center - rotatedExtents;
    b[1] = center + rotatedExtents;
}

void AABB::SetFromTransformedAABBFast(const AABB &aabb, const Mat3x4 &transform) {
    Vec3 center = (aabb[0] + aabb[1]) * 0.5f;
    Vec3 extents = aabb[1] - center;

    Vec3 rotatedExtents;
    for (int i = 0; i < 3; i++) {
        rotatedExtents[i] =
            Math::Fabs(extents[0] * transform[i][0]) +
            Math::Fabs(extents[1] * transform[i][1]) +
            Math::Fabs(extents[2] * transform[i][2]);
    }

    center = transform.TransformPos(center);
    b[0] = center - rotatedExtents;
    b[1] = center + rotatedExtents;
}

void AABB::SetFromTransformedAABB(const AABB &aabb, const Mat3x4 &transform) {
    Vec3 points[8];
    aabb.ToPoints(points);
    transform.BatchTransformPos(points, COUNT_OF(points));

    Clear();

    for (int i = 0; i < 8; i++) {
        AddPoint(points[i]);
    }
}

void AABB::ProjectOnAxis(const Vec3 &axis, float &min, float &max) const {
    Vec3 center = (b[0] + b[1]) * 0.5f;
    Vec3 extents = b[1] - center;

    float d1 = axis.Dot(center);
    float d2 = extents.AbsDot(axis);

    min = d1 - d2;
    max = d1 + d2;
}

void AABB::ProjectOnAxis(const Vec3 &transformOrigin, const Mat3 &transformAxis, const Vec3 &axis, float &min, float &max) const {
    Vec3 center = (b[0] + b[1]) * 0.5f;
    Vec3 extents = b[1] - center;

    center = transformAxis * center + transformOrigin;

    float d1 = axis.Dot(center);
    float d2 = Math::Fabs(extents[0] * transformAxis[0].Dot(axis)) +
               Math::Fabs(extents[1] * transformAxis[1].Dot(axis)) +
               Math::Fabs(extents[2] * transformAxis[2].Dot(axis));

    min = d1 - d2;
    max = d1 + d2;
}

//       +Z
//
//       4------7
//      /|     /|
//     / |    / |
//    5------6  |
//    |  0---|--3  +Y
//    | /    | /
//    |/     |/
//    1------2
//
// +X
//
void AABB::ToPoints(Vec3 points[8]) const {
    for (int i = 0; i < 8; i++) {
        points[i][0] = b[(i ^ (i >> 1)) & 1][0];
        points[i][1] = b[     (i >> 1)  & 1][1];
        points[i][2] = b[     (i >> 2)  & 1][2];
    }
}

// Christer Ericson's Real-Time Collision Detection, pp. 179
bool AABB::IsIntersectTriangle(const Vec3 &_v0, const Vec3 &_v1, const Vec3 &_v2) const {
    float p0, p1, p2, r;

    Vec3 c = (b[0] + b[1]) * 0.5f; // AABB center
    Vec3 e = (b[1] - b[0]) * 0.5f; // AABB extent

    // Get the relative coordinates of a triangle with respect to the center of AABB.
    Vec3 v0 = _v0 - c;
    Vec3 v1 = _v1 - c;
    Vec3 v2 = _v2 - c;

    // Get the triangle edge vectors.
    Vec3 f0 = v1 - v0;
    Vec3 f1 = v2 - v1;
    Vec3 f2 = v0 - v2;

    // Compare in axis a00.
    // a00 = cross(x, f0) = (0, -f0.z, f0.y)
    // r = e.x * abs(dot(x, a00)) + e.y * abs(dot(y, a00)) + e.z * abs(dot(z, a00))
    r = e.y * Math::Fabs(f0.z) + e.z * Math::Fabs(f0.y);
    // dot(a00, v0) = dot(a00, v1)
    p0 = -v0.y * v1.z + v0.z * v1.y;
    // dot(a00, v2)
    p2 = -v2.y * (v1.z - v0.z) + v2.z * (v1.y - v0.y);
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p2), Min(p0, p2)) > r) {
        return false;
    }

    // Compare in axis a01.
    // a01 = cross(x, f1) = (0, -f1.z, f1.y)
    // r = e.x * abs(dot(x, a01)) + e.y * abs(dot(y, a01)) + e.z * abs(dot(z, a01))
    r = e.y * Math::Fabs(f1.z) + e.z * Math::Fabs(f1.y);
    // dot(a01, v0)
    p0 = -v0.y * (v2.z - v1.z) + v0.z * (v2.y - v1.y);
    // dot(a01, v1) = dot(a01, v2)
    p1 = -v1.y * v2.z + v1.z * v2.y;
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p1), Min(p0, p1)) > r) {
        return false;
    }

    // Compare in axis a02.
    // a02 = cross(x, f2) = (0, -f2.z, f2.y)
    // r = e.x * abs(dot(x, a02)) + e.y * abs(dot(y, a02)) + e.z * abs(dot(z, a02))
    r = e.y * Math::Fabs(f2.z) + e.z * Math::Fabs(f2.y);
    // dot(a02, v0) = dot(a02, v2)
    p0 = -v0.z * v2.y + v0.y * v2.z;
    // dot(a02, v1)
    p1 = -v1.y * (v0.z - v2.z) + v1.z * (v0.y - v2.y);
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p1), Min(p0, p1)) > r) {
        return false;
    }

    // Compare in axis a10.
    // a10 = cross(y, f0) = (f0.z, 0, -f0.x)
    // r = e.x * abs(dot(x, a10)) + e.y * abs(dot(y, a10)) + e.z * abs(dot(z, a10))
    r = e.x * Math::Fabs(f0.z) + e.z * Math::Fabs(f0.x);
    // dot(a10, v0) = dot(v10, v1)
    p0 = v0.x * v1.z - v0.z * v1.x;
    // dot(a10, v2)
    p2 = v2.x * (v1.z - v0.z) - v2.z * (v1.x - v0.x);
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p2), Min(p0, p2)) > r) {
        return false;
    }

    // Compare in axis a11.
    // a11 = cross(y, f1) = (f1.z, 0, -f1.x)
    // r = e.x * abs(dot(x, a11)) + e.y * abs(dot(y, a11)) + e.z * abs(dot(z, a11))
    r = e.x * Math::Fabs(f1.z) + e.z * Math::Fabs(f1.x);
    // dot(a11, v0)
    p0 = v0.x * (v2.z - v1.z) - v0.z * (v2.x - v1.x);
    // dot(a11, v1) = dot(a11, v2)
    p1 = v1.x * v2.z - v1.z * v2.x;
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p1), Min(p0, p1)) > r) {
        return false;
    }

    // Compare in axis a12.
    // a12 = cross(y, f2) = (f2.z, 0, -f2.x)
    // r = e.x * abs(dot(x, a12)) + e.y * abs(dot(y, a12)) + e.z * abs(dot(z, a12))
    r = e.x * Math::Fabs(f2.z) + e.z * Math::Fabs(f2.x);
    // dot(a12, v0) = dot(a12, v2)
    p0 = -v0.x * v2.z + v0.z * v2.x;
    // dot(a12, v1)
    p1 = v1.x * (v0.z - v2.z) - v1.z * (v0.x - v2.x);
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p1), Min(p0, p1)) > r) {
        return false;
    }

    // Compare in axis a20.
    // a20 = cross(z, f0) = (-f0.y, f0.x, 0)
    // r = e.x * abs(dot(x, a20)) + e.y * abs(dot(y, a20)) + e.z * abs(dot(z, a20))
    r = e.x * Math::Fabs(f0.y) + e.y * Math::Fabs(f0.x);
    // dot(a20, v0) = dot(v20, v1)
    p0 = -v0.x * v1.y + v0.y * v1.x;
    // dot(a20, v2)
    p2 = -v2.x * (v1.y - v0.y) + v2.y * (v1.x - v0.x);
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p2), Min(p0, p2)) > r) {
        return false;
    }

    // Compare in axis a21.
    // a21 = cross(z, f1) = (-f1.y, f1.x, 0)
    // r = e.x * abs(dot(x, a21)) + e.y * abs(dot(y, a21)) + e.z * abs(dot(z, a21))
    r = e.x * Math::Fabs(f1.y) + e.y * Math::Fabs(f1.x);
    // dot(a21, v0)
    p0 = -v0.x * (v2.y - v1.y) + v0.y * (v2.x - v1.x);
    // dot(a21, v1) = dot(a21, v2)
    p1 = v2.x * v1.y - v2.y * v1.x;
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p1), Min(p0, p1)) > r) {
        return false;
    }

    // Compare in axis a22.
    // a22 = cross(z, f2) = (-f2.y, f2.x, 0)
    // r = e.x * abs(dot(x, a22)) + e.y * abs(dot(y, a22)) + e.z * abs(dot(z, a22))
    r = e.x * Math::Fabs(f2.y) + e.y * Math::Fabs(f2.x);
    // dot(a22, v0) = dot(a22, v2)
    p0 = v0.x * v2.y - v0.y * v2.x;
    // dot(a22, v1)
    p1 = -v1.x * (v0.y - v2.y) + v1.y * (v0.x - v2.x);
    // projection intervals are disjoint if Min(p0, p1, p2) > r or -Max(p0, p1, p2) > r
    if (Max(-Max(p0, p1), Min(p0, p1)) > r) {
        return false;
    }

    // Compare in x axis of AABB.
    if (Max3(v0.x, v1.x, v2.x) < -e.x || Min3(v0.x, v1.x, v2.x) > e.x) {
        return false;
    }

    // Compare in y axis of AABB.
    if (Max3(v0.y, v1.y, v2.y) < -e.y || Min3(v0.y, v1.y, v2.y) > e.y) {
        return false;
    }

    // Compare in z axis of AABB.
    if (Max3(v0.z, v1.z, v2.z) < -e.z || Min3(v0.z, v1.z, v2.z) > e.z) {
        return false;
    }

    // Compare in the normal axis of the triangle.
    Vec3 n = f0.Cross(f1).Normalized();
    Plane p = Plane(n, -n.Dot(_v0));
    return PlaneSide(p, 0.0f) == Plane::Side::Cross ? true : false;
}

BE_NAMESPACE_END
