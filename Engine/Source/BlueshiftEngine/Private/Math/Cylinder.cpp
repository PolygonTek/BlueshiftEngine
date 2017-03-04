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

float Cylinder::RayIntersection(const Vec3 &start, const Vec3 &n) const {
    Vec3 d = p[1] - p[0];
    Vec3 m = start - p[0];
    float md = m.Dot(d);
    float nd = n.Dot(d);
    float dd = d.Dot(d);
    float scale;

    // Test if ray fully outside either endcap of cylinder
    if (md < 0.0f && nd < 0.0f) {
        // ray outside p[0] side of cylinder
        return FLT_MAX;
    }
    if (md > dd && nd >= 0.0f) {
        // ray outside p[1] side of cylinder
        return FLT_MAX;
    }

    float nn = n.Dot(n);
    float mn = m.Dot(n);
    float a = dd * nn - nd * nd;
    float k = m.Dot(m) - radius * radius;
    float c = dd * k - md * md;
    // Test if ray runs parallel to cylinder axis
    if (Math::Fabs(a) < FLT_EPSILON) {
        if (c > 0) {
            // 'a' and thus the ray lie outside cylinder
            return FLT_MAX;
        }
        
        // Now known that ray intersects cylinder; figure out how it intersects
        if (md < 0) {
            // Intersect ray against p[0] endcap
            scale = -mn / nn;
        } else if (md > dd) {
            // Intersect ray against p[1] endcap
            scale = (nd - mn) / nn;
        } else {
            // 'a' lies inside cylinder
            scale = 0.0f;
        }

        return scale;
    }

    float b = dd * mn - nd * md;
    float discr = b * b - a * c;
    if (discr < 0) {
        // No real roots; no intersection
        return FLT_MAX;
    }

    float s = Math::Sqrt(discr);
    float t0 = (-b - s) / a;
    float t1 = (-b + s) / a;
    if (t0 < 0 && t1 < 0) {
        return FLT_MAX;
    }

    if (md + t0 * nd < 0) { // Test if intersection outside cylinder on p[0] side	
        scale = -md / nd;
        // Keep intersection if Dot(L(t) - p[0], L(t) - p[0]) <= r^2
        if (k + scale * (2 * mn + scale * nn) <= 0.0f) {
            return scale;
        } else {
            return FLT_MAX;
        }
    } else if (md + t0 * nd > dd) { // Test if intersection outside cylinder on p[1] side	
        scale = (dd - md) / nd;
        // Keep intersection if Dot(L(t) - p[1], L(t) - p[1]) <= r^2
        if (k + dd - 2 * md + scale * (2 * (mn - nd) + scale * nn) <= 0.0f) {
            return scale;
        } else {
            return FLT_MAX;
        }
    }

    // Ray intersects cylinder between the endcaps
    scale = Max(t0, 0.0f);
    return scale;
}

bool Cylinder::LineIntersection(const Vec3 &start, const Vec3 &end) const {
    Vec3 d = p[1] - p[0];
    Vec3 m = start - p[0];
    Vec3 n = end - start;
    float md = m.Dot(d);
    float nd = n.Dot(d);
    float dd = d.Dot(d);

    // Test if segment fully outside either endcap of cylinder
    if (md < 0.0f && md + nd < 0.0f) {
        // Segment outside p[0] side of cylinder
        return false;
    }
    if (md > dd && md + nd > dd) {
        // Segment outside p[1] side of cylinder
        return false;
    }

    float nn = n.Dot(n);
    float mn = m.Dot(n);
    float a = dd * nn - nd * nd;
    float k = m.Dot(m) - radius * radius;
    float c = dd * k - md * md;
    // Test if segment runs parallel to cylinder axis
    if (Math::Fabs(a) < FLT_EPSILON) {
        if (c > 0) {
            // 'a' and thus the segment lie outside cylinder
            return false;
        }

        return true;
    }

    float b = dd * mn - nd * md;
    float discr = b * b - a * c;
    if (discr < 0) {
        // No real roots; no intersection
        return false;
    }

    float s = Math::Sqrt(discr);
    float t0 = (-b - s) / a;
    if (t0 > 1.0) {
        return false;
    }

    float t1 = (-b + s) / a;
    if (t0 < 0 && t1 < 0) {
        return false;
    }

    if (md + t0 * nd < 0) { // Test if intersection outside cylinder on p[0] side	
        float t = -md / nd;
        // Keep intersection if Dot(L(t) - p[0], L(t) - p[0]) <= r^2
        return k + t * (2 * mn + t * nn) <= 0.0f;
    } else if (md + t0 * nd > dd) { // Test if intersection outside cylinder on p[1] side
        float t = (dd - md) / nd;
        // Keep intersection if Dot(L(t) - p[1], L(t) - p[1]) <= r^2
        return k + dd - 2 * md + t * (2 * (mn - nd) + t * nn) <= 0.0f;
    }

    if (t0 <= 0) {
        return true;
    }

    return t0 <= 1.0f;
}

Sphere Cylinder::ToSphere() const {
    Vec3 center = (p[0] + p[1]) * 0.5f;
    float sphereRadius = Math::Sqrt(radius * radius + (center - p[0]).LengthSqr());
    return Sphere(center, sphereRadius);
}

OBB Cylinder::ToOBB() const {
    Vec3 center = (p[0] + p[1]) * 0.5f;
    Vec3 dir = p[1] - p[0];
    float l = dir.Normalize();
    
    Vec3 left, up;	
    dir.OrthogonalBasis(left, up);
    
    return OBB(center, Vec3(l, radius, radius), Mat3(dir, left, up));
}

BE_NAMESPACE_END
