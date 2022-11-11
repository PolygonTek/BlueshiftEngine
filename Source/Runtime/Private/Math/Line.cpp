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

Line::Line(const LineSegment &lineSegment) {
    pos = lineSegment.a;
    dir = lineSegment.Dir();
}

Vec3 Line::ClosestPoint(const Vec3 &point) const {
    float d = dir.Dot(point - pos);
    return GetPoint(d);
}

float Line::Distance(const Vec3 &point) const {
    Vec3 closestPoint = ClosestPoint(point);
    return closestPoint.Distance(point);
}

float Line::DistanceSqr(const Vec3 &point) const {
    Vec3 closestPoint = ClosestPoint(point);
    return closestPoint.DistanceSqr(point);
}

LineSegment Line::ToLineSegment(float d) const {
    return LineSegment(pos, GetPoint(d));
}

LineSegment Line::ToLineSegment(float dStart, float dEnd) const {
    return LineSegment(GetPoint(dStart), GetPoint(dEnd));
}

Ray Line::ToRay() const {
    return Ray(pos, dir);
}

Line Line::FromString(const char *str) {
    Line l;
    int count = sscanf(str, "%f %f %f %f %f %f", &l.pos.x, &l.pos.y, &l.pos.z, &l.dir.x, &l.dir.y, &l.dir.z);
    assert(count == l.GetDimension());
    return l;
}

BE_NAMESPACE_END
