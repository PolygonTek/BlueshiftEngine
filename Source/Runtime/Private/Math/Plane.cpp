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

int Plane::GetType() const {
    if (normal[0] == 0.0f) {
        if (normal[1] == 0.0f) {
            return normal[2] > 0.0f ? Type::Z : Type::NegZ;
        } else if (normal[2] == 0.0f) {
            return normal[1] > 0.0f ? Type::Y : Type::NegY;
        } else {
            return Type::ZeroX;
        }
    } else if (normal[1] == 0.0f) {
        if (normal[2] == 0.0f) {
            return normal[0] > 0.0f ? Type::X : Type::NegX;
        } else {
            return Type::ZeroY;
        }
    } else if (normal[2] == 0.0f) {
        return Type::ZeroZ;
    } else {
        return Type::NonAxial;
    }
}

bool Plane::IsIntersectLine(const Vec3 &p1, const Vec3 &p2) const {
    float d1 = normal.Dot(p1) - offset;
    float d2 = normal.Dot(p2) - offset;

    if (d1 == d2) {
        return false;
    }

    if (d1 > 0.0f && d2 > 0.0f) {
        return false;
    }

    if (d1 < 0.0f && d2 < 0.0f) {
        return false;
    }

    float fraction = (d1 / (d1 - d2));

    return (fraction >= 0.0f && fraction <= 1.0f);
}

bool Plane::IntersectRay(const Ray &ray, bool ignoreBackside, float *hitDist) const {
    float d1 = normal.Dot(ray.dir);
    if (ignoreBackside && d1 > 0) {
        // No collision if the ray hit the plane from behind.
        return false;
    }

    if (Math::Abs(d1) == 0.000001f) {
        return false;
    }

    float d2 = offset - normal.Dot(ray.origin);
    float dist = d2 / d1;
    if (dist < 0.0f) {
        return false;
    }

    if (hitDist) {
        *hitDist = dist;
    }
    return true;
}

float Plane::IntersectRay(const Ray &ray, bool ignoreBackside) const { 
    float hitDist;

    if (IntersectRay(ray, ignoreBackside, &hitDist)) { 
        return hitDist;
    } 
    return FLT_MAX;
}

const char *Plane::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), 4, precision);
}

BE_NAMESPACE_END
