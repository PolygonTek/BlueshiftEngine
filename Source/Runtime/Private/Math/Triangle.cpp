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

static float TriArea2D(float x1, float y1, float x2, float y2, float x3, float y3) {
    return (x1 - x2) * (y2 - y3) - (x2 - x3) * (y1 - y2);
}

// Christer Ericson's Real-Time Collision Detection, pp. 51-52.
Vec3 Triangle::BarycentricUVW(const Vec3 &point) const {
    // Unnormalized triangle normal.
    Vec3 m = (b - a).Cross(c - a);

    // Absolute components for determining projection plane.
    float x = Math::Fabs(m.x);
    float y = Math::Fabs(m.y);
    float z = Math::Fabs(m.z);

    // Nominators and one-over-denominator for u and v ratios.
    float nu, nv, ood;

    if (x >= y && x >= z) { 
        // Project to the yz plane.
        nu = TriArea2D(point.y, point.z, b.y, b.z, c.y, c.z); // Area of PBC in yz-plane.
        nv = TriArea2D(point.y, point.z, c.y, c.z, a.y, a.z); // Area of PCA in yz-plane.
        ood = 1.0f / m.x; // 1 / (2 * area of ABC in yz plane)
    } else if (y >= z) { 
        // Note: The book has a redundant 'if (y >= x)' comparison
        // y is largest, project to the xz-plane.
        nu = TriArea2D(point.x, point.z, b.x, b.z, c.x, c.z);
        nv = TriArea2D(point.x, point.z, c.x, c.z, a.x, a.z);
        ood = 1.0f / -m.y;
    } else { 
        // z is largest, project to the xy-plane.
        nu = TriArea2D(point.x, point.y, b.x, b.y, c.x, c.y);
        nv = TriArea2D(point.x, point.y, c.x, c.y, a.x, a.y);
        ood = 1.0f / m.z;
    }

    float u = nu * ood;
    float v = nv * ood;
    float w = 1.0f - u - v;

    return Vec3(u, v, w);
}

Vec2 Triangle::BarycentricUV(const Vec3 &point) const {
    Vec3 uvw = BarycentricUVW(point);
    return Vec2(uvw.y, uvw.z);
}

Vec3 Triangle::PointFromBarycentricUVW(const Vec3 &uvw) const {
    return uvw[0] * a + uvw[1] * b + uvw[2] * c;
}

Vec3 Triangle::PointFromBarycentricUV(const Vec2 &uv) const {
    // In case the triangle is far away from the origin but is small in size, the elements of 'a' will have large magnitudes,
    // and the elements of (b-a) and (c-a) will be much smaller quantities. Therefore be extra careful with the
    // parentheses and first sum the small floats together before adding it to the large one.
    return a + ((b - a) * uv[0] + (c - a) * uv[1]);
}

Vec3 Triangle::Centroid() const {
    return (a + b + c) * (1.f / 3.f);
}

float Triangle::Area() const {
    return 0.5f * (b - a).Cross(c - a).Length();
}

float Triangle::Perimeter() const {
    return a.Distance(b) + b.Distance(c) + c.Distance(a);
}

Plane Triangle::PlaneCCW() const {
    Plane plane;
    plane.SetFromPoints(a, b, c);
    return plane;
}

Plane Triangle::PlaneCW() const {
    Plane plane;
    plane.SetFromPoints(a, c, b);
    return plane;
}

Vec3 Triangle::UnnormalizedNormalCCW() const {
    return (b - a).Cross(c - a);
}

Vec3 Triangle::UnnormalizedNormalCW() const {
    return (c - a).Cross(b - a);
}

Vec3 Triangle::NormalCCW() const {
    Vec3 normal = UnnormalizedNormalCCW();
    normal.Normalize();
    return normal;
}

Vec3 Triangle::NormalCW() const {
    Vec3 normal = UnnormalizedNormalCW();
    normal.Normalize();
    return normal;
}

bool Triangle::IsContainPoint(const Vec3 &point, float thicknessSq) const {
    Vec3 normal = UnnormalizedNormalCCW();
    float lengthSqr = normal.LengthSqr();
    float d = normal.Dot(b - point);
    if (d * d > thicknessSq * lengthSqr) {
        return false;
    }

    Vec3 br = BarycentricUVW(point);
    // Allow for a small epsilon to properly account for points very near the edges of the triangle.
    return br.x >= -1e-3f && br.y >= -1e-3f && br.z >= -1e-3f; 
}

// Christer Ericson's Real-Time Collision Detection, pp. 191-192.
bool Triangle::IntersectRay(const Ray &ray, float *hitDist) const {
    // Find vectors for two edges sharing vertex a.
    Vec3 ab = b - a;
    Vec3 ac = c - a;

    // Get unnormalized triangle normal.
    Vec3 n = ab.Cross(ac);

    // Compute denominator d.
    float d = -ray.dir.Dot(n);

    // If d <= 0, ray is lies in plane of triangle, so exit early.
    if (Math::Fabs(d) < 1e-6f) {
        return false;
    }

    // Compute intersection t value of ray.dir with plane of triangle. 
    // A ray intersects iff 0 <= t.
    // Delay dividing by d until intersection has been found to pierce triangle.
    Vec3 ap = ray.origin - a;
    float t = ap.Dot(n);
    if (t < 0.0f) {
        return false;
    }

    // Compute barycentric coordinate components (v, w) and test if within bounds.
    Vec3 e = ap.Cross(ray.dir);
    float v = ac.Dot(e);
    if (v < 0.0f || v > d) {
        return false;
    }
    float w = -ab.Dot(e);
    if (w < 0.0f || v + w > d) {
        return false;
    }

    *hitDist = t / d;
    return true;
}

float Triangle::IntersectRay(const Ray &ray) const {
    float hitDist;

    if (IntersectRay(ray, &hitDist)) {
        return hitDist;
    }
    return FLT_MAX;
}

AABB Triangle::ToAABB() const {
    AABB aabb;
    aabb.b[0] = Min3(a, b, c);
    aabb.b[1] = Max3(a, b, c);
    return aabb;
}

Triangle Triangle::FromString(const char *str) {
    Triangle t;
    int count = sscanf(str, "%f %f %f %f %f %f %f %f %f", &t.a.x, &t.a.y, &t.a.z, &t.b.x, &t.b.y, &t.b.z, &t.c.x, &t.c.y, &t.c.z);
    assert(count == t.GetDimension());
    return t;
}

BE_NAMESPACE_END
