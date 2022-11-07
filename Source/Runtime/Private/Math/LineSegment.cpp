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

LineSegment::LineSegment(const Ray &ray, float d) {
    a = ray.GetOrigin();
    b = ray.GetPoint(d);
}

Vec3 LineSegment::ClosestPoint(const Vec3 &point) const {
    Vec3 dir = b - a;
    float s = Clamp01(dir.Dot(point - a) / dir.LengthSqr());
    return a + s * dir;
}

float LineSegment::Distance(const Vec3 &point) const {
    Vec3 closestPoint = ClosestPoint(point);
    return closestPoint.Distance(point);
}

float LineSegment::DistanceSqr(const Vec3 &point) const {
    Vec3 closestPoint = ClosestPoint(point);
    return closestPoint.DistanceSqr(point);
}

Ray LineSegment::ToRay() const {
    return Ray(a, Dir());
}

LineSegment LineSegment::FromString(const char *str) {
    LineSegment t;
    sscanf(str, "%f %f %f %f %f %f", &t.a.x, &t.a.y, &t.a.z, &t.b.x, &t.b.y, &t.b.z);
    return t;
}

BE_NAMESPACE_END
