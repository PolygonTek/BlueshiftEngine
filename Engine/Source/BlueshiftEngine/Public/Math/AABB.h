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

/// A 3D axis-aligned bounding box.
class BE_API AABB {
public:
    /// The default constructor does not initialize any members of this class.
    AABB() {}
    /// Constructs this AABB by specifying the minimum and maximum extending corners of the box.
    AABB(const Vec3 &mins, const Vec3 &maxs);
    explicit AABB(const Vec3 &point);

                        /// Resets mins and maxs values. If you invoke this method, IsCleared() shall returns true.
    void                Clear();
                        /// Sets to zero sized AABB
    void                SetZero();

                        /// Returns true if this AABB is cleared
    bool                IsCleared() const;
                        /// Returns center point of this AABB
    Vec3                Center() const { return Vec3((b[1][0] + b[0][0]) * 0.5f, (b[1][1] + b[0][1]) * 0.5f, (b[1][2] + b[0][2]) * 0.5f); }
                        /// Returns extents
    Vec3                Extents() const { return (b[1] - b[0]) * 0.5f; }
                        /// Radius of circumscribed sphere that center in origin about the AABB
    float               OuterRadius() const;
                        /// Radius of circumscribed sphere that given center about the AABB
    float               OuterRadius(const Vec3 &center) const;
                        /// Radius of inscribed sphere inside the AABB
    float               InnerRadius() const;
                        /// Returns area of six sides
    float               Area() const;
                        /// Returns volume of the AABB
    float               Volume() const;

                        /// Accesses an element of this AABB using array notation.
                        /// 0 means mins, 1 means maxs.
    const Vec3 &        operator[](const int index) const;
    Vec3 &              operator[](const int index);

                        /// Returns translated AABB
    AABB                operator+(const Vec3 &rhs) const { return AABB(b[0] + rhs, b[1] + rhs); }
                        /// Returns translated AABB
    AABB                operator-(const Vec3 &rhs) const { return AABB(b[0] - rhs, b[1] - rhs); }
                        /// Translates AABB
    AABB &              operator+=(const Vec3 &rhs);
                        /// Translates AABB
    AABB &              operator-=(const Vec3 &rhs);
                        /// Returns scaled AABB
    AABB                operator*(const float &rhs) const;
                        /// Scales AABB
    AABB &              operator*=(const float &rhs);
                        /// Returns scaled AABB
    AABB                operator*(const Vec3 &rhs) const;
                        /// Scales AABB
    AABB &              operator*=(const Vec3 &rhs);
                        /// Returns rotated AABB
    AABB                operator*(const Mat3 &rhs) const;
                        /// Rotates AABB
    AABB &              operator*=(const Mat3 &rhs);
                        /// Returns added AABB
    AABB                operator+(const AABB &rhs) const;
                        /// Adds AABB
    AABB &              operator+=(const AABB &rhs);

                        /// Exact compare, no epsilon
    bool                Equals(const AABB &a) const { return (b[0].Equals(a.b[0]) && b[1].Equals(a.b[1])); }
                        /// Compare with epsilon
    bool                Equals(const AABB &a, const float epsilon) const { return (b[0].Equals(a.b[0], epsilon) && b[1].Equals(a.b[1], epsilon)); }
                        /// Exact compare, no epsilon
    bool                operator==(const AABB &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const AABB &rhs) const { return !Equals(rhs); }

                        // 점을 추가, AABB 가 확장됐다면 true 를 리턴
    bool                AddPoint(const Vec3 &v);
                        // AABB 를 추가, AABB 가 확장됐다면 true 를 리턴
    bool                AddAABB(const AABB &a);

                        // 교차된 AABB 리턴
    AABB                Intersect(const AABB &a) const;
                        // AABB 를 교차
    AABB &              IntersectSelf(const AABB &a);
                        // d 만큼 확장된 AABB 리턴
    AABB                Expand(const float d) const { return AABB(Vec3(b[0][0] - d, b[0][1] - d, b[0][2] - d), Vec3(b[1][0] + d, b[1][1] + d, b[1][2] + d)); }
                        // d 만큼 AABB 확장
    AABB &              ExpandSelf(const float d);
                        // 이동된 AABB 리턴
    AABB                Translate(const Vec3 &translation) const { return AABB(b[0] + translation, b[1] + translation); }
                        // AABB 이동
    AABB &              TranslateSelf(const Vec3 &translation);
                        // 회전된 AABB 리턴
    AABB                Rotate(const Mat3 &rotation) const;
                        // AABB 회전
    AABB &              RotateSelf(const Mat3 &rotation);

                        // AABB 가 평면의 어느쪽에 존재하는지 판단
                        // Plane::Side::Front or Plane::Side::Back or Plane::Side::Cross
    int                 PlaneSide(const Plane &plane, const float epsilon = ON_EPSILON) const;
                        // AABB 와 평면 사이의 거리
    float               PlaneDistance(const Plane &plane) const;
    
                        // 8 개의 꼭지점중에서 from 과 가장 가까운 점 구하기
    void                GetNearestVertex(const Vec3 &from, Vec3 &point) const;	
                        // 8 개의 꼭지점중에서 dir 방향으로 가장 가까운 점 구하기
    void                GetNearestVertexFromDir(const Vec3 &dir, Vec3 &point) const;
                        // 8 개의 꼭지점중에서 from 과 가장 먼 점 구하기
    void                GetFarthestVertex(const Vec3 &from, Vec3 &point) const;
                        // 8 개의 꼭지점중에서 dir 방향으로 가장 먼 점 구하기
    void                GetFarthestVertexFromDir(const Vec3 &dir, Vec3 &point) const;	

                        // AABB 점들의 집합 중에서 from 과 가장 가까운 점 구하기
    void                GetClosestPoint(const Vec3 &from, Vec3 &point) const;
    
                        // point 와 AABB 사이 거리의 제곱
    float               DistanceSqr(const Vec3 &p) const;
                        // point 와 AABB 사이 거리
    float               Distance(const Vec3 &p) const;

                        // point 포함 여부 리턴
    bool                IsContainPoint(const Vec3 &p) const;
                        // AABB 포함 여부 리턴
    bool                IsContainAABB(const AABB &a) const;
                        // AABB 교차 여부 리턴
    bool                IsIntersectAABB(const AABB &a, bool ignoreBorders = false) const;
                        // sphere 교차 여부 리턴
    bool                IsIntersectSphere(const Sphere &s) const;
                        // 삼각형 교차 여부 리턴
    bool                IsIntersectTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2) const;

                        // Line segment vs AABB intersection
    bool                LineIntersection(const Vec3 &start, const Vec3 &end) const;
                        // Ray vs AABB intersection
                        // intersection point is start + dir * scale
    float               RayIntersection(const Vec3 &start, const Vec3 &dir) const;
    
                        // point 집합을 포함하는 AABB
    void                SetFromPoints(const Vec3 *points, const int numPoints);
                        // 이동하는 point 를 감싸는 AABB
    void                SetFromPointTranslation(const Vec3 &point, const Vec3 &translation);
                        // 이동하는 AABB 를 감싸는 AABB
    void                SetFromAABBTranslation(const AABB &aabb, const Vec3 &origin, const Mat3 &axis, const Vec3 &translation);
                        // transformed AABB (OBB) 를 포함하는 AABB
    void                SetFromTransformedAABB(const AABB &aabb, const Vec3 &origin, const Mat3 &axis);

                        // AABB 를 dir 축으로 투영했을 때 min, max 값
    void                AxisProjection(const Vec3 &dir, float &min, float &max) const;
                        // transformed AABB (OBB) 를 dir 축으로 투영했을 때 min, max 값
    void                AxisProjection(const Vec3 &origin, const Mat3 &axis, const Vec3 &dir, float &min, float &max) const;

    void                ToPoints(Vec3 points[8]) const;
    Sphere              ToSphere() const;
    OBB                 ToOBB() const;

    static const AABB   zero;   ///< (0 0 0) (0 0 0)

    Vec3                b[2];   ///< minimum/maximum value for each axis
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
    if (DistanceSqr(s.origin) > s.radius * s.radius) {
        return false;
    }
    return true;
}

BE_INLINE Sphere AABB::ToSphere() const {
    Sphere sphere;
    sphere.origin = (b[0] + b[1]) * 0.5f;
    sphere.radius = (b[1] - sphere.origin).Length();
    return sphere;
}

BE_NAMESPACE_END
