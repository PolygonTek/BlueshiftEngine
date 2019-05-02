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

bool Sphere::IsIntersectAABB(const AABB &aabb) const {
    return aabb.IsIntersectSphere(*this);
}
    
bool Sphere::IntersectRay(const Ray &ray, float *hitDistMin, float *hitDistMax) const {
    Vec3 m = center - ray.origin;
    float b = m.Dot(ray.dir); // * 2 생략
    float c = m.Dot(m) - radius * radius;
    
    if (b < 0 && c > 0) {
        return false;
    }

    // determinant = (b^2 - 4ac) / 4
    float discr = b * b - c; 
    // No intersection occured if determinant is less than 0.
    if (discr < 0.0f) {
        return false;
    }

    if (hitDistMin || hitDistMax) {
        float q = Math::Sqrt(discr);

        if (hitDistMin) {
            *hitDistMin = b - q;
        }
        if (hitDistMax) {
            *hitDistMax = b + q;
        }
    }
    return true;
}

float Sphere::IntersectRay(const Ray &ray) const {
    float hitDistMin;

    if (IntersectRay(ray, &hitDistMin)) {
        return hitDistMin;
    }
    return FLT_MAX;
}

bool Sphere::IsIntersectLine(const Vec3 &start, const Vec3 &end) const {
    Vec3 m = start - center;
    float c = m.Dot(m) - radius * radius;
    // If there is definitely at least one real root, there must be an intersection	
    if (c <= 0) {
        return true;
    }

    Vec3 d = end - start;
    float l = d.Length();
    float b = m.Dot(d) / l;
    // Exit if line's origin outside sphere (c > 0) and line pointing away from sphere (b > 0)	
    if (b > 0) {
        return false;
    }
    
    float discr = b * b - c;
    // A negative discriminant corresponds to ray missing sphere
    if (discr < 0) {
        return false;
    }

    float t = b - Math::Sqrt(discr);
    if (t > l) {
        return false;
    }

    return true;
}

AABB Sphere::ToAABB() const {
    AABB aabb(center, center);
    aabb.ExpandSelf(radius);
    return aabb;
}

BE_NAMESPACE_END
