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

const OBB OBB::zero(Vec3::zero, Vec3::zero, Mat3::identity);

/*
            4---{4}---5
 +         /|        /|
 Z      {7} {8}   {5} |
 -     /    |    /    {9}
      7--{6}----6     |
      |     |   |     |
    {11}    0---|-{0}-1
      |    /    |    /       -
      | {3}  {10} {1}       Y
      |/        |/         +
      3---{2}---2

        - X +

  plane bits:
  0 = min x
  1 = max x
  2 = min y
  3 = max y
  4 = min z
  5 = max z

*/

static int obbPlaneBitsSilVerts[64][7] = {
    { 0, 0, 0, 0, 0, 0, 0 }, // 000000 = 0
    { 4, 7, 4, 0, 3, 0, 0 }, // 000001 = 1
    { 4, 5, 6, 2, 1, 0, 0 }, // 000010 = 2
    { 0, 0, 0, 0, 0, 0, 0 }, // 000011 = 3
    { 4, 4, 5, 1, 0, 0, 0 }, // 000100 = 4
    { 6, 3, 7, 4, 5, 1, 0 }, // 000101 = 5
    { 6, 4, 5, 6, 2, 1, 0 }, // 000110 = 6
    { 0, 0, 0, 0, 0, 0, 0 }, // 000111 = 7
    { 4, 6, 7, 3, 2, 0, 0 }, // 001000 = 8
    { 6, 6, 7, 4, 0, 3, 2 }, // 001001 = 9
    { 6, 5, 6, 7, 3, 2, 1 }, // 001010 = 10
    { 0, 0, 0, 0, 0, 0, 0 }, // 001011 = 11
    { 0, 0, 0, 0, 0, 0, 0 }, // 001100 = 12
    { 0, 0, 0, 0, 0, 0, 0 }, // 001101 = 13
    { 0, 0, 0, 0, 0, 0, 0 }, // 001110 = 14
    { 0, 0, 0, 0, 0, 0, 0 }, // 001111 = 15
    { 4, 0, 1, 2, 3, 0, 0 }, // 010000 = 16
    { 6, 0, 1, 2, 3, 7, 4 }, // 010001 = 17
    { 6, 3, 2, 6, 5, 1, 0 }, // 010010 = 18
    { 0, 0, 0, 0, 0, 0, 0 }, // 010011 = 19
    { 6, 1, 2, 3, 0, 4, 5 }, // 010100 = 20
    { 6, 1, 2, 3, 7, 4, 5 }, // 010101 = 21
    { 6, 2, 3, 0, 4, 5, 6 }, // 010110 = 22
    { 0, 0, 0, 0, 0, 0, 0 }, // 010111 = 23
    { 6, 0, 1, 2, 6, 7, 3 }, // 011000 = 24
    { 6, 0, 1, 2, 6, 7, 4 }, // 011001 = 25
    { 6, 0, 1, 5, 6, 7, 3 }, // 011010 = 26
    { 0, 0, 0, 0, 0, 0, 0 }, // 011011 = 27
    { 0, 0, 0, 0, 0, 0, 0 }, // 011100 = 28
    { 0, 0, 0, 0, 0, 0, 0 }, // 011101 = 29
    { 0, 0, 0, 0, 0, 0, 0 }, // 011110 = 30
    { 0, 0, 0, 0, 0, 0, 0 }, // 011111 = 31
    { 4, 7, 6, 5, 4, 0, 0 }, // 100000 = 32
    { 6, 7, 6, 5, 4, 0, 3 }, // 100001 = 33
    { 6, 5, 4, 7, 6, 2, 1 }, // 100010 = 34
    { 0, 0, 0, 0, 0, 0, 0 }, // 100011 = 35
    { 6, 4, 7, 6, 5, 1, 0 }, // 100100 = 36
    { 6, 3, 7, 6, 5, 1, 0 }, // 100101 = 37
    { 6, 4, 7, 6, 2, 1, 0 }, // 100110 = 38
    { 0, 0, 0, 0, 0, 0, 0 }, // 100111 = 39
    { 6, 6, 5, 4, 7, 3, 2 }, // 101000 = 40
    { 6, 6, 5, 4, 0, 3, 2 }, // 101001 = 41
    { 6, 5, 4, 7, 3, 2, 1 }, // 101010 = 42
    { 0, 0, 0, 0, 0, 0, 0 }, // 101011 = 43
    { 0, 0, 0, 0, 0, 0, 0 }, // 101100 = 44
    { 0, 0, 0, 0, 0, 0, 0 }, // 101101 = 45
    { 0, 0, 0, 0, 0, 0, 0 }, // 101110 = 46
    { 0, 0, 0, 0, 0, 0, 0 }, // 101111 = 47
    { 0, 0, 0, 0, 0, 0, 0 }, // 110000 = 48
    { 0, 0, 0, 0, 0, 0, 0 }, // 110001 = 49
    { 0, 0, 0, 0, 0, 0, 0 }, // 110010 = 50
    { 0, 0, 0, 0, 0, 0, 0 }, // 110011 = 51
    { 0, 0, 0, 0, 0, 0, 0 }, // 110100 = 52
    { 0, 0, 0, 0, 0, 0, 0 }, // 110101 = 53
    { 0, 0, 0, 0, 0, 0, 0 }, // 110110 = 54
    { 0, 0, 0, 0, 0, 0, 0 }, // 110111 = 55
    { 0, 0, 0, 0, 0, 0, 0 }, // 111000 = 56
    { 0, 0, 0, 0, 0, 0, 0 }, // 111001 = 57
    { 0, 0, 0, 0, 0, 0, 0 }, // 111010 = 58
    { 0, 0, 0, 0, 0, 0, 0 }, // 111011 = 59
    { 0, 0, 0, 0, 0, 0, 0 }, // 111100 = 60
    { 0, 0, 0, 0, 0, 0, 0 }, // 111101 = 61
    { 0, 0, 0, 0, 0, 0, 0 }, // 111110 = 62
    { 0, 0, 0, 0, 0, 0, 0 }, // 111111 = 63
};

bool OBB::AddPoint(const Vec3 &p) {
    // 비어 있을 경우
    if (extents[0] < 0.0f) {
        extents.SetFromScalar(0);
        center = p;
        axis.SetIdentity();
        return true;
    }

    // local axis 로 변환해서 AABB 로 다룬다.
    AABB aabb1;	
    aabb1[0][0] = aabb1[1][0] = center.Dot(axis[0]);
    aabb1[0][1] = aabb1[1][1] = center.Dot(axis[1]);
    aabb1[0][2] = aabb1[1][2] = center.Dot(axis[2]);
    aabb1[0] -= extents;
    aabb1[1] += extents;
    // p 를 local axis 로 변환해서 AABB 에 추가
    if (!aabb1.AddPoint(Vec3(p.Dot(axis[0]), p.Dot(axis[1]), p.Dot(axis[2])))) {
        // p 가 이미 포함되어 있다.
        return false;
    }

    // center 에서 p 방향으로 oriented 된 OBB 를 상정한다.
    Mat3 axis2;
    // 첫번째 축은 p 방향으로 고른다.
    axis2[0] = p - center;
    axis2[0].Normalize();
    // 두번째 축은 OBB 의 각 축에 투영해보고 가장 independant 한 벡터를 하나 고른다.
    axis2[1] = axis[Min3Index(axis2[0].Dot(axis[0]), axis2[0].Dot(axis[1]), axis2[0].Dot(axis[2]))];
    // 첫번째 축에 투영해서 orthonormal 하게 만든다.
    axis2[1] = axis2[1] - (axis2[1].Dot(axis2[0])) * axis2[0];
    axis2[1].Normalize();
    // 세번째 축은 외적으로 얻는다.
    axis2[2].SetFromCross(axis2[0], axis2[1]);
    
    AABB aabb2;
    // axis2 에 투영해서 AABB 를 만든다.
    AxisProjection(axis2, aabb2);
    // p 를 local axis 로 변환해서 AABB 에 추가
    aabb2.AddPoint(Vec3(p.Dot(axis2[0]), p.Dot(axis2[1]), p.Dot(axis2[2])));

    // aabb1 과 aabb2 중 작은 volume 의 aabb 를 기반으로 OBB 를 만든다.
    if (aabb1.Volume() < aabb2.Volume()) {
        center = (aabb1[0] + aabb1[1]) * 0.5f;
        extents = aabb1[1] - center;
        center *= axis;
    } else {
        center = (aabb2[0] + aabb2[1]) * 0.5f;
        extents = aabb2[1] - center;
        center *= axis2;
        axis = axis2;
    }
    return true;
}

bool OBB::AddOBB(const OBB &a) {
    // a 가 비어있을 경우
    if (a.extents[0] < 0.0f) {
        return false;
    }

    // this 가 비어있을 경우
    if (extents[0] < 0.0f) {
        center = a.center;
        extents = a.extents;
        axis = a.axis;
        return true;
    }

    Mat3 ax[4];
    AABB aabbs[4];
    AABB b;

    // this 의 local axis 로 변환해서 AABB 로 다룬다.
    ax[0] = axis;
    aabbs[0][0][0] = aabbs[0][1][0] = center.Dot(ax[0][0]);
    aabbs[0][0][1] = aabbs[0][1][1] = center.Dot(ax[0][1]);
    aabbs[0][0][2] = aabbs[0][1][2] = center.Dot(ax[0][2]);
    aabbs[0][0] -= extents;
    aabbs[0][1] += extents;
    // a 를 this.axis 에 투영해서 local AABB 를 만든다.
    a.AxisProjection(ax[0], b);
    if (!aabbs[0].AddAABB(b)) {
        // a 는 이미 포함되어 있다.
        return false;
    }

    // a 의 local axis 로 변환해서 AABB 로 다룬다.
    ax[1] = a.axis;
    aabbs[1][0][0] = aabbs[1][1][0] = a.center.Dot(ax[1][0]);
    aabbs[1][0][1] = aabbs[1][1][1] = a.center.Dot(ax[1][1]);
    aabbs[1][0][2] = aabbs[1][1][2] = a.center.Dot(ax[1][2]);
    aabbs[1][0] -= a.extents;
    aabbs[1][1] += a.extents;
    // this 를 a.axis 에 투영해서 local AABB 를 만든다.
    AxisProjection(ax[1], b);
    if (!aabbs[1].AddAABB(b)) {
        // this OBB 는 이미 a 에 포함되어 있다.
        center = a.center;
        extents = a.extents;
        axis = a.axis;
        return true;
    }

    // center 사이 벡터 방향으로 oriented 된 OBB 를 2개 상정한다.
    Vec3 dir = a.center - center;
    dir.Normalize();
    for (int i = 2; i < 4; i++) {
        // 첫번째 축은 OBB 의 center 사이 벡터 방향으로 고른다.
        ax[i][0] = dir;
        // 두번째 축은 OBB 의 각 축에 투영해보고 가장 independant 한 벡터를 하나 고른다.
        // ax[0] 와 ax[1] 은 각각 this.axis, a.axis
        ax[i][1] = ax[i-2][Min3Index(dir * ax[i-2][0], dir * ax[i-2][1], dir * ax[i-2][2])];
        // 첫번째 축에 투영해서 orthonormal 하게 만든다.
        ax[i][1] = ax[i][1] - (ax[i][1] * dir) * dir;
        ax[i][1].Normalize();
        // 세번째 축은 외적으로 얻는다.
        ax[i][2].SetFromCross(dir, ax[i][1]);

        // this 와 a 를 각각 ax[i] 에 투영해서 AABB 를 만들고 합친다.
        AxisProjection(ax[i], aabbs[i]);
        a.AxisProjection(ax[i], b);
        aabbs[i].AddAABB(b);
    }

    // 가장 작은 volume 의 aabb 를 구한다.
    float bestv = Math::Infinity;
    int besti = 0;
    for (int i = 0; i < 4; i++) {
        float v = aabbs[i].Volume();
        if (v < bestv && v > 0.0f) {
            bestv = v;
            besti = i;
        }
    }

    // 가장 작은 volume 의 aabb 를 기반으로 OBB 를 만든다.
    center = (aabbs[besti][0] + aabbs[besti][1]) * 0.5f;
    extents = aabbs[besti][1] - center;
    center *= ax[besti];
    axis = ax[besti];

    return false;
}

int OBB::PlaneSide(const Plane &plane, const float epsilon) const {
    float d1, d2;

    d1 = plane.Distance(center);
    d2 = Math::Fabs(extents[0] * axis[0].Dot(plane.Normal())) +
         Math::Fabs(extents[1] * axis[1].Dot(plane.Normal())) +
         Math::Fabs(extents[2] * axis[2].Dot(plane.Normal()));

    if (d1 - d2 > epsilon) {
        return Plane::Side::Front;
    }

    if (d1 + d2 < -epsilon) {
        return Plane::Side::Back;
    }

    return Plane::Side::Cross;
}

float OBB::PlaneDistance(const Plane &plane) const {
    float d1, d2;

    d1 = plane.Distance(center);
    d2 = Math::Fabs(extents[0] * axis[0].Dot(plane.Normal())) +
         Math::Fabs(extents[1] * axis[1].Dot(plane.Normal())) +
         Math::Fabs(extents[2] * axis[2].Dot(plane.Normal()));

    if (d1 - d2 > 0.0f) {
        return d1 - d2;
    }

    if (d1 + d2 < 0.0f) {
        return d1 + d2;
    }

    return 0.0f;
}

void OBB::GetNearestVertex(const Vec3 &from, Vec3 &point) const {
    Vec3 vec = from - center;
    
    point.x = vec.Dot(axis[0]) > 0 ? extents.x : -extents.x;
    point.y = vec.Dot(axis[1]) > 0 ? extents.y : -extents.y;
    point.z = vec.Dot(axis[2]) > 0 ? extents.z : -extents.z;

    point = center + point * axis;
}

void OBB::GetFarthestVertex(const Vec3 &from, Vec3 &point) const {
    Vec3 vec = from - center;
    
    point.x = vec.Dot(axis[0]) < 0 ? extents.x : -extents.x;
    point.y = vec.Dot(axis[1]) < 0 ? extents.y : -extents.y;
    point.z = vec.Dot(axis[2]) < 0 ? extents.z : -extents.z;

    point = center + point * axis;
}

void OBB::GetClosestPoint(const Vec3 &from, Vec3 &point) const {
    Vec3 vec = from - center;

    point = center;

    for (int i = 0; i < 3; i++) {
        float d = vec.Dot(axis[i]);
        if (d > extents[i]) {
            d = extents[i];
        } else if (d < -extents[i]) {
            d = -extents[i];
        }
        
        point += d * axis[i];
    }
}

float OBB::DistanceSqr(const Vec3 &p) const {
    Vec3 vec = p - center;

    float dsq = 0.0f;

    for (int i = 0; i < 3; i++) {
        float d = vec.Dot(axis[i]);
        if (d > extents[i]) {
            d = d - extents[i];
            dsq += d * d;
        } else if (d < -extents[i]) {
            d = d + extents[i];
            dsq += d * d;
        }		
    }

    return dsq;
}

float OBB::Distance(const Vec3 &p) const {
    float dsq = DistanceSqr(p);
    return Math::Sqrt(dsq);
}

bool OBB::IsIntersectOBB(const OBB &b, float epsilon) const {
    float c[3][3];
    float ac[3][3];
    float tproj[3];		// axis[i] * t
    float d, e0, e1;	// distance between centers and projected extents

    // vector between centers
    Vec3 t = b.center - center;
    
    // Ax 축으로 비교 (axis[0])
    c[0][0] = axis[0].Dot(b.axis[0]);
    c[0][1] = axis[0].Dot(b.axis[1]);
    c[0][2] = axis[0].Dot(b.axis[2]);
    ac[0][0] = Math::Fabs(c[0][0]) + epsilon;
    ac[0][1] = Math::Fabs(c[0][1]) + epsilon;
    ac[0][2] = Math::Fabs(c[0][2]) + epsilon;
    tproj[0] = axis[0].Dot(t);

    d = Math::Fabs(tproj[0]);
    e0 = extents[0];
    e1 = b.extents[0] * ac[0][0] + b.extents[1] * ac[0][1] + b.extents[2] * ac[0][2];
    if (d > e0 + e1) {
        return false;
    }

    // Ay 축으로 비교 (axis[1])
    c[1][0] = axis[1].Dot(b.axis[0]);
    c[1][1] = axis[1].Dot(b.axis[1]);
    c[1][2] = axis[1].Dot(b.axis[2]);
    ac[1][0] = Math::Fabs(c[1][0]) + epsilon;
    ac[1][1] = Math::Fabs(c[1][1]) + epsilon;
    ac[1][2] = Math::Fabs(c[1][2]) + epsilon;
    tproj[1] = axis[1].Dot(t);

    d = Math::Fabs(tproj[1]);
    e0 = extents[1];
    e1 = b.extents[0] * ac[1][0] + b.extents[1] * ac[1][1] + b.extents[2] * ac[1][2];
    if (d > e0 + e1) {
        return false;
    }

    // Az 축으로 비교 (axis[2])
    c[2][0] = axis[2].Dot(b.axis[0]);
    c[2][1] = axis[2].Dot(b.axis[1]);
    c[2][2] = axis[2].Dot(b.axis[2]);
    ac[2][0] = Math::Fabs(c[2][0]) + epsilon;
    ac[2][1] = Math::Fabs(c[2][1]) + epsilon;
    ac[2][2] = Math::Fabs(c[2][2]) + epsilon;
    tproj[2] = axis[2].Dot(t);

    d = Math::Fabs(tproj[2]);
    e0 = extents[2];
    e1 = b.extents[0] * ac[2][0] + b.extents[1] * ac[2][1] + b.extents[2] * ac[2][2];
    if (d > e0 + e1) {
        return false;
    }

    // Bx 축으로 비교 (b.axis[0])
    d = Math::Fabs(b.axis[0].Dot(t));
    e0 = extents[0] * ac[0][0] + extents[1] * ac[1][0] + extents[2] * ac[2][0];
    e1 = b.extents[0];
    if (d > e0 + e1) {
        return false;
    }

    // By 축으로 비교 (b.axis[1])
    d = Math::Fabs(b.axis[1].Dot(t));
    e0 = extents[0] * ac[0][1] + extents[1] * ac[1][1] + extents[2] * ac[2][1];
    e1 = b.extents[1];
    if (d > e0 + e1) {
        return false;
    }
    
    // Bz 축으로 비교 (b.axis[2])
    d = Math::Fabs(b.axis[2].Dot(t));
    e0 = extents[0] * ac[0][2] + extents[1] * ac[1][2] + extents[2] * ac[2][2];
    e1 = b.extents[2];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Ax, Bx) 축으로 비교
    d = Math::Fabs(tproj[2] * c[1][0] - tproj[1] * c[2][0]);
    e0 = extents[1] * ac[2][0] + extents[2] * ac[1][0];
    e1 = b.extents[1] * ac[0][2] + b.extents[2] * ac[0][1];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Ax, By) 축으로 비교
    d = Math::Fabs(tproj[2] * c[1][1] - tproj[1] * c[2][1]);
    e0 = extents[1] * ac[2][1] + extents[2] * ac[1][1];
    e1 = b.extents[0] * ac[0][2] + b.extents[2] * ac[0][0];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Ax, Bz) 축으로 비교
    d = Math::Fabs(tproj[2] * c[1][2] - tproj[1] * c[2][2]);
    e0 = extents[1] * ac[2][2] + extents[2] * ac[1][2];
    e1 = b.extents[0] * ac[0][1] + b.extents[1] * ac[0][0];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Ay, Bx) 축으로 비교
    d = Math::Fabs(tproj[0] * c[2][0] - tproj[2] * c[0][0]);
    e0 = extents[0] * ac[2][0] + extents[2] * ac[0][0];
    e1 = b.extents[1] * ac[1][2] + b.extents[2] * ac[1][1];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Ay, By) 축으로 비교
    d = Math::Fabs(tproj[0] * c[2][1] - tproj[2] * c[0][1]);
    e0 = extents[0] * ac[2][1] + extents[2] * ac[0][1];
    e1 = b.extents[0] * ac[1][2] + b.extents[2] * ac[1][0];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Ay, Bz) 축으로 비교
    d = Math::Fabs(tproj[0] * c[2][2] - tproj[2] * c[0][2]);
    e0 = extents[0] * ac[2][2] + extents[2] * ac[0][2];
    e1 = b.extents[0] * ac[1][1] + b.extents[1] * ac[1][0];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Az, Bx) 축으로 비교
    d = Math::Fabs(tproj[1] * c[0][0] - tproj[0] * c[1][0]);
    e0 = extents[0] * ac[1][0] + extents[1] * ac[0][0];
    e1 = b.extents[1] * ac[2][2] + b.extents[2] * ac[2][1];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Az, By) 축으로 비교
    d = Math::Fabs(tproj[1] * c[0][1] - tproj[0] * c[1][1]);
    e0 = extents[0] * ac[1][1] + extents[1] * ac[0][1];
    e1 = b.extents[0] * ac[2][2] + b.extents[2] * ac[2][0];
    if (d > e0 + e1) {
        return false;
    }

    // cross(Az, Bz) 축으로 비교
    d = Math::Fabs(tproj[1] * c[0][2] - tproj[0] * c[1][2]);
    e0 = extents[0] * ac[1][2] + extents[1] * ac[0][2];
    e1 = b.extents[0] * ac[2][1] + b.extents[1] * ac[2][0];
    if (d > e0 + e1) {
        return false;
    }

    return true;
}

bool OBB::IsIntersectSphere(const Sphere &sphere) const {
    Vec3 p;
    GetClosestPoint(sphere.Origin(), p);
    return p.DistanceSqr(sphere.Origin()) < sphere.Radius() * sphere.Radius();
}

bool OBB::LineIntersection(const Vec3 &start, const Vec3 &end) const {    
    Vec3 lineDir = 0.5f * (end - start);
    Vec3 lineCenter = start + lineDir;
    Vec3 dir = lineCenter - center;
    float ld[3];

    // X 축 투영 거리 비교
    ld[0] = Math::Fabs(lineDir.Dot(axis[0]));
    if (Math::Fabs(dir.Dot(axis[0])) > extents[0] + ld[0]) {
        return false;
    }

    // Y 축 투영 거리 비교
    ld[1] = Math::Fabs(lineDir.Dot(axis[1]));
    if (Math::Fabs(dir.Dot(axis[1])) > extents[1] + ld[1]) {
        return false;
    }

    // Z 축 투영 거리 비교
    ld[2] = Math::Fabs(lineDir.Dot(axis[2]));
    if (Math::Fabs(dir.Dot(axis[2])) > extents[2] + ld[2]) {
        return false;
    }

    // 선분의 방향과 AABB 의 축들과의 외적한 축들에 대해서 검사
    Vec3 cross = lineDir.Cross(dir);

    if (Math::Fabs(cross.Dot(axis[0])) > extents[1] * ld[2] + extents[2] * ld[1]) {
        return false;
    }

    if (Math::Fabs(cross.Dot(axis[1])) > extents[0] * ld[2] + extents[2] * ld[0]) {
        return false;
    }

    if (Math::Fabs(cross.Dot(axis[2])) > extents[0] * ld[1] + extents[1] * ld[0]) {
        return false;
    }

    return true;
}

float OBB::RayIntersection(const Vec3 &start, const Vec3 &dir) const {
    float tmin = 0.0f;
    float tmax = FLT_MAX;

    for (int i = 0; i < 3; i++) {
        float ld = dir.Dot(axis[i]);
        float ls = (start - center).Dot(axis[i]);

        if (Math::Fabs(ld) < FLT_EPSILON) {
            if (ls < -extents[i] || ls > extents[i]) {
                return FLT_MAX;
            }
        } else {
            float ood = 1.0f / ld;
            float t1 = (-extents[i] - ls) * ood;
            float t2 = (+extents[i] - ls) * ood;
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

//       +Z.axis
//
//       4------7
//      /|     /|
//     / |    / |
//    5------6  |       
//    |  0---|--3  +Y.axis
//    | /    | /
//    |/     |/  
//    1------2
//
// +X.axis
//
void OBB::ToPoints(Vec3 points[8]) const {
    Mat3 ax;
    Vec3 temp[4];

    ax[0] = extents[0] * axis[0];
    ax[1] = extents[1] * axis[1];
    ax[2] = extents[2] * axis[2];
    temp[0] = center - ax[0];
    temp[1] = center + ax[0];
    temp[2] = ax[1] - ax[2];
    temp[3] = ax[1] + ax[2];
    points[0] = temp[0] - temp[3];
    points[1] = temp[1] - temp[3];
    points[2] = temp[1] + temp[2];
    points[3] = temp[0] + temp[2];
    points[4] = temp[0] - temp[2];
    points[5] = temp[1] - temp[2];
    points[6] = temp[1] + temp[3];
    points[7] = temp[0] + temp[3];
}

AABB OBB::ToAABB() const {
    AABB aabb;
    AxisProjection(Mat3::identity, aabb);
    return aabb;
}

int OBB::GetProjectionSilhouetteVerts(const Vec3 &projectionOrigin, Vec3 silVerts[6]) const {
    Vec3 points[8];
    ToPoints(points);

    Vec3 dir1 = points[0] - projectionOrigin;
    Vec3 dir2 = points[6] - projectionOrigin;
    float f = dir1.Dot(axis[0]);
    int planeBits = IEEE_FLT_SIGNBITNOTSET(f);
    f = dir2.Dot(axis[0]);
    planeBits |= IEEE_FLT_SIGNBITSET(f) << 1;
    f = dir1.Dot(axis[1]);
    planeBits |= IEEE_FLT_SIGNBITNOTSET(f) << 2;
    f = dir2.Dot(axis[1]);
    planeBits |= IEEE_FLT_SIGNBITSET(f) << 3;
    f = dir1.Dot(axis[2]);
    planeBits |= IEEE_FLT_SIGNBITNOTSET(f) << 4;
    f = dir2.Dot(axis[2]);
    planeBits |= IEEE_FLT_SIGNBITSET(f) << 5;

    int *index = obbPlaneBitsSilVerts[planeBits];
    for (int i = 0; i < index[0]; i++) {
        silVerts[i] = points[index[i+1]];
    }

    return index[0];
}

int OBB::GetParallelProjectionSilhouetteVerts(const Vec3 &projectionDir, Vec3 silVerts[6]) const {
    Vec3 points[8];
    ToPoints(points);

    int planeBits = 0;
    float f = projectionDir.Dot(axis[0]);
    if (IEEE_FLT_NOTZERO(f)) {
        planeBits = 1 << IEEE_FLT_SIGNBITSET(f);
    }
    
    f = projectionDir.Dot(axis[1]);
    if (IEEE_FLT_NOTZERO(f)) {
        planeBits |= 4 << IEEE_FLT_SIGNBITSET(f);
    }
    
    f = projectionDir.Dot(axis[2]);
    if (IEEE_FLT_NOTZERO(f)) {
        planeBits |= 16 << IEEE_FLT_SIGNBITSET(f);
    }
    
    int *index = obbPlaneBitsSilVerts[planeBits];
    for (int i = 0; i < index[0]; i++) {
        silVerts[i] = points[index[i+1]];
    }
    
    return index[0];
}
/*
void OBB::SetFromPoints(const Vec3 *points, const int numPoints) {
    // compute mean of points
    center = points[0];
    for (int i = 1; i < numPoints; i++) {
        center += points[i];
    }
    float invNumPoints = 1.0f / numPoints;
    center *= invNumPoints;

    // compute covariances of points
    float covXX = 0.0f; 
    float covXY = 0.0f;
    float covXZ = 0.0f;
    float covYY = 0.0f; 
    float covYZ = 0.0f;
    float covZZ = 0.0f;

    for (int i = 0; i < numPoints; i++) {
        Vec3 diff = points[i] - center;
        covXX += diff.x * diff.x;
        covXY += diff.x * diff.y;
        covXZ += diff.x * diff.z;
        covYY += diff.y * diff.y;
        covYZ += diff.y * diff.z;
        covZZ += diff.z * diff.z;
    }
    covXX *= invNumPoints;
    covXY *= invNumPoints;
    covXZ *= invNumPoints;
    covYY *= invNumPoints;
    covYZ *= invNumPoints;
    covZZ *= invNumPoints;

    // compute eigenvectors for covariance matrix
    MatX eigenVectors;
    VecX eigenValues;
    eigenVectors.SetData(3, 3, MATX_ALLOCA(3 * 3));	
    eigenValues.SetData(3, VECX_ALLOCA(3));

    eigenVectors[0][0] = covXX;
    eigenVectors[0][1] = covXY;
    eigenVectors[0][2] = covXZ;
    eigenVectors[1][0] = covXY;
    eigenVectors[1][1] = covYY;
    eigenVectors[1][2] = covYZ;
    eigenVectors[2][0] = covXZ;
    eigenVectors[2][1] = covYZ;
    eigenVectors[2][2] = covZZ;
    eigenVectors.Eigen_SolveSymmetric(eigenValues);
    eigenVectors.Eigen_SortIncreasing(eigenValues);

    axis[0] = eigenVectors[0];
    axis[1] = eigenVectors[1];
    axis[2] = eigenVectors[2];

    extents[0] = eigenValues[0];
    extents[1] = eigenValues[0];
    extents[2] = eigenValues[0];

    // refine by calculating the aabb of the points projected onto the axis and adjusting the center and extents
    AABB aabb;
    aabb.Clear();
    for (int i = 0; i < numPoints; i++)	{
        aabb.AddPoint(Vec3(points[i].Dot(axis[0]), points[i].Dot(axis[1]), points[i].Dot(axis[2])));
    }
    center = (aabb[0] + aabb[1]) * 0.5f;
    extents = aabb[1] - center;
    center *= axis;
}*/

bool OBB::ProjectionBounds(const Sphere &sphere, AABB &projectionBounds) const {
    float min, max;
    float centerProj;

    sphere.AxisProjection(axis[0], min, max);
    centerProj = center.Dot(axis[0]);

    projectionBounds[0][0] = (min - centerProj) / extents[0];
    Clamp(projectionBounds[0][0], -1.0f, +1.0f);

    projectionBounds[1][0] = (max - centerProj) / extents[0];
    Clamp(projectionBounds[1][0], -1.0f, +1.0f);

    sphere.AxisProjection(axis[1], min, max);
    centerProj = center.Dot(axis[1]);

    projectionBounds[0][1] = (min - centerProj) / extents[1];
    Clamp(projectionBounds[0][1], -1.0f, +1.0f);

    projectionBounds[1][1] = (max - centerProj) / extents[1];
    Clamp(projectionBounds[1][1], -1.0f, +1.0f);

    sphere.AxisProjection(axis[2], min, max);
    centerProj = center.Dot(axis[2]);

    projectionBounds[0][2] = (min - centerProj) / extents[2];
    Clamp(projectionBounds[0][2], -1.0f, +1.0f);

    projectionBounds[1][2] = (max - centerProj) / extents[2];
    Clamp(projectionBounds[1][2], -1.0f, +1.0f);

    return true;
}

bool OBB::ProjectionBounds(const OBB &box, AABB &projectionBounds) const {
    float min, max;
    float centerProj;

    box.AxisProjection(axis[0], min, max);
    centerProj = center.Dot(axis[0]);

    projectionBounds[0][0] = (min - centerProj) / extents[0];
    Clamp(projectionBounds[0][0], -1.0f, +1.0f);

    projectionBounds[1][0] = (max - centerProj) / extents[0];
    Clamp(projectionBounds[1][0], -1.0f, +1.0f);

    box.AxisProjection(axis[1], min, max);
    centerProj = center.Dot(axis[1]);

    projectionBounds[0][1] = (min - centerProj) / extents[1];
    Clamp(projectionBounds[0][1], -1.0f, +1.0f);

    projectionBounds[1][1] = (max - centerProj) / extents[1];
    Clamp(projectionBounds[1][1], -1.0f, +1.0f);

    box.AxisProjection(axis[2], min, max);
    centerProj = center.Dot(axis[2]);

    projectionBounds[0][2] = (min - centerProj) / extents[2];
    Clamp(projectionBounds[0][2], -1.0f, +1.0f);

    projectionBounds[1][2] = (max - centerProj) / extents[2];
    Clamp(projectionBounds[1][2], -1.0f, +1.0f);

    return true;
}

bool OBB::ProjectionBounds(const Frustum &frustum, AABB &projectionBounds) const {
    float min, max;
    float centerProj;

    frustum.AxisProjection(axis[0], min, max);
    centerProj = center.Dot(axis[0]);

    projectionBounds[0][0] = (min - centerProj) / extents[0];
    Clamp(projectionBounds[0][0], -1.0f, +1.0f);

    projectionBounds[1][0] = (max - centerProj) / extents[0];
    Clamp(projectionBounds[1][0], -1.0f, +1.0f);
    
    frustum.AxisProjection(axis[1], min, max);
    centerProj = center.Dot(axis[1]);

    projectionBounds[0][1] = (min - centerProj) / extents[1];
    Clamp(projectionBounds[0][1], -1.0f, +1.0f);

    projectionBounds[1][1] = (max - centerProj) / extents[1];
    Clamp(projectionBounds[1][1], -1.0f, +1.0f);

    frustum.AxisProjection(axis[2], min, max);
    centerProj = center.Dot(axis[2]);

    projectionBounds[0][2] = (min - centerProj) / extents[2];
    Clamp(projectionBounds[0][2], -1.0f, +1.0f);

    projectionBounds[1][2] = (max - centerProj) / extents[2];
    Clamp(projectionBounds[1][2], -1.0f, +1.0f);

    return true;
}

BE_NAMESPACE_END
