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

// point = u*a + v*b + w*c
Vec3 Triangle::ComputeBarycentricCoordsFromPoint(const Vec3 &point) const {
    return Barycentric::Triangle3D(a, b, c, point);
}

bool Triangle::IsContainPoint(const Vec3 &point, float thicknessSq) const {
    Vec3 normal = UnnormalizedNormalCCW();
    float lengthSqr = normal.LengthSqr();
    float d = normal.Dot(b - point);
    if (d * d > thicknessSq * lengthSqr) {
        return false;
    }

    Vec3 br = ComputeBarycentricCoordsFromPoint(point);
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
