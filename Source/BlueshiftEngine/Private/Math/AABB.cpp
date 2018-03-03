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
    float d2 = Math::Fabs((b[1][0] - center[0]) * plane.Normal()[0]) + 
        Math::Fabs((b[1][1] - center[1]) * plane.Normal()[1]) + 
        Math::Fabs((b[1][2] - center[2]) * plane.Normal()[2]);

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
    float d2 = Math::Fabs((b[1][0] - center[0]) * plane.Normal()[0]) + 
        Math::Fabs((b[1][1] - center[1]) * plane.Normal()[1]) +
        Math::Fabs((b[1][2] - center[2]) * plane.Normal()[2]);

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

    for (int i = 0; i < 3; i++) {
        if (from[i] < b[0][i]) {
            point[i] = b[0][i];
        } else if (from[i] > b[1][i]) {
            point[i] = b[1][i];
        }
    }
}

float AABB::DistanceSqr(const Vec3 &p) const {
    float dsq = 0.0f;
    float d;

    for (int i = 0; i < 3; i++)	{
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

bool AABB::LineIntersection(const Vec3 &start, const Vec3 &end) const {
    Vec3 center = (b[0] + b[1]) * 0.5f;
    Vec3 extents = b[1] - center;
    Vec3 lineDir = 0.5f * (end - start);
    Vec3 lineCenter = start + lineDir;
    Vec3 dir = lineCenter - center;
    float ld[3];

    // X 축 투영 거리 비교
    ld[0] = Math::Fabs(lineDir[0]);
    if (Math::Fabs(dir[0]) > extents[0] + ld[0]) {
        return false;
    }

    // Y 축 투영 거리 비교
    ld[1] = Math::Fabs(lineDir[1]);
    if (Math::Fabs(dir[1]) > extents[1] + ld[1]) {
        return false;
    }
    
    // Z 축 투영 거리 비교
    ld[2] = Math::Fabs(lineDir[2]);
    if (Math::Fabs(dir[2]) > extents[2] + ld[2]) {
        return false;
    }

    // 선분의 방향과 AABB 의 축들과의 외적한 축들에 대해서 검사
    Vec3 cross = lineDir.Cross(dir);

    if (Math::Fabs(cross[0]) > extents[1] * ld[2] + extents[2] * ld[1]) {
        return false;
    }

    if (Math::Fabs(cross[1]) > extents[0] * ld[2] + extents[2] * ld[0]) {
        return false;
    }
    
    if (Math::Fabs(cross[2]) > extents[0] * ld[1] + extents[1] * ld[0]) {
        return false;
    }

    return true;
}

float AABB::RayIntersection(const Vec3 &start, const Vec3 &dir) const {
    float tmin = 0.0f;
    float tmax = FLT_MAX;

    for (int i = 0; i < 3; i++) {
        if (Math::Fabs(dir[i]) < FLT_EPSILON) {
            if (start[i] < b[0][i] || start[i] > b[1][i]) {
                return FLT_MAX;
            }
        } else {
            float ood = 1.0f / dir[i];
            float t1 = (b[0][i] - start[i]) * ood;
            float t2 = (b[1][i] - start[i]) * ood;
            if (t1 > t2) {
                Swap(t1, t2);
            }
            tmin = Max(tmin, t1);
            tmax = Min(tmax, t2);
            if (tmin > tmax) {
                return FLT_MAX;
            }
        }
    }

    return tmin;
}

void AABB::SetFromPoints(const Vec3 *points, const int numPoints) {
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
        }
        else {
            b[0][i] = point[i];
            b[1][i] = point[i] + translation[i];
        }
    }
}

void AABB::SetFromAABBTranslation(const AABB &aabb, const Vec3 &origin, const Mat3 &axis, const Vec3 &translation) {
    if (!axis.IsIdentity()) {
        SetFromTransformedAABB(aabb, origin, axis);
    } else {
        b[0] = aabb[0] + origin;
        b[1] = aabb[1] + origin;
    }

    // 이동 성분만큼 확장
    for (int i = 0; i < 3; i++) {
        if (translation[i] < 0.0f) {
            b[0][i] += translation[i];
        } else {
            b[1][i] += translation[i];
        }
    }
}

void AABB::SetFromTransformedAABB(const AABB &aabb, const Vec3 &origin, const Mat3 &axis) {
    Vec3 center = (aabb[0] + aabb[1]) * 0.5f;
    Vec3 extents = aabb[1] - center;

    Vec3 rotatedExtents;
    for (int i = 0; i < 3; i++) {
        rotatedExtents[i] = Math::Fabs(extents[0] * axis[0][i]) + 
            Math::Fabs(extents[1] * axis[1][i]) + 
            Math::Fabs(extents[2] * axis[2][i]);
    }
    
    center = axis * center + origin;
    b[0] = center - rotatedExtents;
    b[1] = center + rotatedExtents;
}

void AABB::AxisProjection(const Vec3 &dir, float &min, float &max) const {
    Vec3 center = (b[0] + b[1]) * 0.5f;
    Vec3 extents = b[1] - center;

    float d1 = dir.Dot(center);
    float d2 = Math::Fabs(extents[0] * dir[0]) + 
        Math::Fabs(extents[1] * dir[1]) + 
        Math::Fabs(extents[2] * dir[2]);

    min = d1 - d2;
    max = d1 + d2;
}

void AABB::AxisProjection(const Vec3 &origin, const Mat3 &axis, const Vec3 &dir, float &min, float &max) const {	
    Vec3 center = (b[0] + b[1]) * 0.5f;
    Vec3 extents = b[1] - center;

    center = axis * center + origin;

    float d1 = dir.Dot(center);
    float d2 = Math::Fabs(extents[0] * axis[0].Dot(dir)) + 
               Math::Fabs(extents[1] * axis[1].Dot(dir)) + 
               Math::Fabs(extents[2] * axis[2].Dot(dir));

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
        points[i][0] = b[(i^(i>>1))&1][0];
        points[i][1] = b[(i>>1)&1][1];
        points[i][2] = b[(i>>2)&1][2];
    }
}

// REF: RTCD p.179 - Testing AABB Against Triangle
bool AABB::IsIntersectTriangle(const Vec3 &_v0, const Vec3 &_v1, const Vec3 &_v2) const {
    float p0, p1, p2, r;

    Vec3 c = (b[0] + b[1]) * 0.5f; // AABB center
    Vec3 e = (b[1] - b[0]) * 0.5f; // AABB extent

    // AABB center 를 원점으로 설정
    Vec3 v0 = _v0 - c;
    Vec3 v1 = _v1 - c;
    Vec3 v2 = _v2 - c;

    // triangle edge vectors
    Vec3 f0 = v1 - v0;
    Vec3 f1 = v2 - v1;
    Vec3 f2 = v0 - v2;

    // a00 축 비교 
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

    // a01 축 비교
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

    // a02 축 비교
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

    // a10 축 비교
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

    // a11 축 비교
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

    // a12 축 비교
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

    // a20 축 비교
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

    // a21 축 비교
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

    // a22 축 비교
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

    // AABB.x 축 비교
    if (Max3(v0.x, v1.x, v2.x) < -e.x || Min3(v0.x, v1.x, v2.x) > e.x) {
        return false;
    }

    // AABB.y 축 비교
    if (Max3(v0.y, v1.y, v2.y) < -e.y || Min3(v0.y, v1.y, v2.y) > e.y) {
        return false;
    }

    // AABB.z 축 비교
    if (Max3(v0.z, v1.z, v2.z) < -e.z || Min3(v0.z, v1.z, v2.z) > e.z) {
        return false;
    }

    // Triangle.normal 축 비교
    Vec3 n = f0.Cross(f1);
    n.Normalize();
    Plane p = Plane(n, -n.Dot(_v0));
    return PlaneSide(p, 0.0f) == Plane::Side::Cross ? true : false;
}

BE_NAMESPACE_END
