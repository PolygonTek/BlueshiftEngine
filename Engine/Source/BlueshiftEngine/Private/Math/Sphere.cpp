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
    
float Sphere::RayIntersection(const Vec3 &start, const Vec3 &dir) const {
    Vec3 m = start - origin;
    float b = m.Dot(dir); // * 2 생략
    float c = m.Dot(m) - radius * radius;

    if (c > 0 && b > 0) {
        return FLT_MAX;
    }

    float discr = b * b - c; // determinant = (b^2 - 4ac) / 4
    // no intersection occured if determinant is less than 0
    if (discr < 0.0f) { 
        return FLT_MAX;
    }

    float scale = -b - Math::Sqrt(discr);
    if (scale < 0) {
        scale = 0;
        //scale = -b + Math::Sqrt(discr);
    }
    
    return scale;
}

bool Sphere::LineIntersection(const Vec3 &start, const Vec3 &end) const {
    Vec3 m = start - origin;
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
    AABB aabb(origin, origin);
    aabb.ExpandSelf(radius);
    return aabb;
}

BE_NAMESPACE_END
