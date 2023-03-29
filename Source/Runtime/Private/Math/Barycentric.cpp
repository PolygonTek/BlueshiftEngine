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

// Affinely independent points p0, p1, ..., p{n-1} can determine a simplex (such as a point, line segment, triangle, tetrahedron, ...).
// Any given point p can be represented by affine combination of p0, p2, ..., p{n-1}
// p = c0 * p0 + c1 * p1 + ... + c{n-1} * p{n-1}, s.t. c0 + c1 + ... c{n-1} = 1
// p = (1 - c1 - ... - c{n-1}) * p0 + c1 * p1 + ... + c{n-1} * p{n-1}
// p = p0 + c1 * (p1 - p0) + ... + c{n-1} * (p{n-1} - p0)
// p - p0 = [p1 - p0 ... p{n-1} - p0] * [c1 c2 ... c{n-1}]^T
// So, barycentric (affine) coordinates can be calculated as a linear system.

Vec2 Barycentric::LineSegment1D(float p0, float p1, float p) {
    float v1 = p1 - p0;
    if (Math::Fabs(v1) < VECTOR_EPSILON) {
        return Vec2::zero;
    }
    float c1 = (p - p0) / v1;

    return Vec2(1.0f - c1, c1);
}

Vec2 Barycentric::LineSegment2D(const Vec2 &p0, const Vec2 &p1, const Vec2 &p) {
    Vec2 v1 = p1 - p0;
    float c1;

    if (Math::Fabs(v1.x) >= Math::Fabs(v1.y)) {
        c1 = (p.x - p0.x) / v1.x;
    } else {
        c1 = (p.y - p0.y) / v1.y;
    }
    return Vec2(1.0f - c1, c1);
}

Vec3 Barycentric::Triangle2D(const Vec2 &p0, const Vec2 &p1, const Vec2 &p2, const Vec2 &p) {
    Vec2 v1 = p1 - p0;
    Vec2 v2 = p2 - p0;
    float det = v1[0] * v2[1] - v1[0] * v2[1];

    if (Math::Fabs(det) < VECTOR_EPSILON) {
        return Vec3::zero;
    }
    float invDet = 1.0f / det;

    Vec2 vp = p - p0;
    float c1 = (v2[1] * vp[0] - v2[0] * vp[1]) * invDet;
    float c2 = (v1[0] * vp[1] - v1[1] * vp[0]) * invDet;
    return Vec3(1.0f - c1 - c2, c1, c2);
}

// 2D parallelogram area
static BE_FORCE_INLINE float ParaArea2D(float x1, float y1, float x2, float y2, float x3, float y3) {
    return (x1 - x2) * (y2 - y3) - (x2 - x3) * (y1 - y2);
}

// Christer Ericson's Real-Time Collision Detection, pp. 51-52.
Vec3 Barycentric::Triangle3D(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2, const Vec3 &p) {
    // Unnormalized triangle normal.
    Vec3 m = (p1 - p0).Cross(p2 - p0);
#if 0
    float inv_denom = 1.0f / m.LengthSqr();
    float u = (p1 - p).Cross(p2 - p).LengthSqr() * inv_denom;
    float v = (p2 - p).Cross(p0 - p).LengthSqr() * inv_denom;
    float w = 1.0f - u - v;
#else
    // Absolute components for determining projection plane.
    float amx = Math::Fabs(m.x);
    float amy = Math::Fabs(m.y);
    float amz = Math::Fabs(m.z);

    // Nominators and one-over-denominator for u and v ratios.
    float nu, nv, ood;

    // We can speed up this operation by projecting the points and triangles to one of the
    // xy, xz, or yz planes and treating it as a 2D problem. To improve accuracy, we'll choose
    // the one that provide maximum area for the projection of the triangle.
    if (amx >= amy && amx >= amz) {
        // Project to the yz plane.
        nu = ParaArea2D(p.y, p.z, p1.y, p1.z, p2.y, p2.z); // 2 * area of (p, p1, p2) in yz-plane.
        nv = ParaArea2D(p.y, p.z, p2.y, p2.z, p0.y, p0.z); // 2 * area of (p, p2, p0) in yz-plane.
        ood = 1.0f / m.x; // 1 / (2 * area of (p0, p1, p2) in yz plane)
    } else if (amy >= amz) {
        // Note: The book has a redundant 'if (y >= x)' comparison
        // y is largest, project to the xz-plane.
        nu = ParaArea2D(p.x, p.z, p1.x, p1.z, p2.x, p2.z); // 2 * area of (p, p1, p2) in xz-plane.
        nv = ParaArea2D(p.x, p.z, p2.x, p2.z, p0.x, p0.z); // 2 * area of (p, p2, p0) in xz-plane.
        ood = 1.0f / -m.y; // 1 / (2 * area of (p0, p1, p2) in xz plane)
    } else {
        // z is largest, project to the xy-plane.
        nu = ParaArea2D(p.x, p.y, p1.x, p1.y, p2.x, p2.y); // 2 * area of (p, p1, p2) in xy-plane.
        nv = ParaArea2D(p.x, p.y, p2.x, p2.y, p0.x, p0.y); // 2 * area of (p, p2, p0) in xy-plane.
        ood = 1.0f / m.z; // 1 / (2 * area of (p0, p1, p2) in xy plane)
    }

    float u = nu * ood;
    float v = nv * ood;
    float w = 1.0f - u - v;
#endif
    return Vec3(u, v, w);
}

Vec4 Barycentric::Tetrahedron3D(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, const Vec3 &p) {
#if 0
    Vec3 v1 = p1 - p0;
    Vec3 v2 = p2 - p0;
    Vec3 v3 = p3 - p0;

    float inv_denom = 1.0f / v1.Cross(v2).Dot(v3);
    float u = (p1 - p).Cross(p2 - p).Dot(p3 - p) * inv_denom;
    float v = (p2 - p).Cross(p3 - p).Dot(p0 - p) * inv_denom;
    float w = (p3 - p).Cross(p0 - p).Dot(p1 - p) * inv_denom;

    return Vec4(u, v, w, 1.0f - u - v - w);

#else
    Mat3 matInv(p1 - p0, p2 - p0, p3 - p0);
    if (!matInv.InverseSelf()) {
        return Vec4::zero;
    }
    Vec3 c = matInv * (p - p0);

    return Vec4(1.0f - c[0] - c[1] - c[2], c[0], c[1], c[2]);
#endif
}

BE_NAMESPACE_END
