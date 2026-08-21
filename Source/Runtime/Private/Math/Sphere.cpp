// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

bool Sphere::AddPoint(const Vec3 &p) {
    if (radius < 0.0f) {
        center = p;
        radius = 0.0f;
        return true;
    }

    Vec3 dv = p - center;
    float dsq = dv.LengthSqr();
    if (dsq <= radius * radius) {
        return false;
    }

    float d = Math::Sqrt(dsq);
    float newRadius = (radius + d) * 0.5f;

    center += (newRadius - radius) / d * dv;
    radius = newRadius;
    return true;
}

bool Sphere::IsIntersectAABB(const AABB &aabb) const {
    return aabb.IsIntersectSphere(*this);
}

bool Sphere::IntersectRay(const Ray &ray, float *hitDistMin, float *hitDistMax) const {
    // 음수 반지름은 유효한 구를 나타내지 않는다.
    if (radius < 0.0f) {
        return false;
    }

    // 아래 투영식과 교차 거리는 ray.dir 이 단위 벡터인 것을 전제로 한다.
    assert(Math::Fabs(ray.dir.LengthSqr() - 1.0f) <= 1e-4f);

    // m 은 광선 원점에서 구 중심으로 향하는 벡터이고,
    // b 는 구 중심까지 광선 방향으로 투영한 부호 있는 거리다.
    const Vec3 m = center - ray.origin;
    const float b = m.Dot(ray.dir); // 이차방정식의 선형항에서 2 배를 생략한 값.
    const float c = m.Dot(m) - radius * radius;

    // 광선 원점이 구 밖에 있고 구 중심이 광선 반대편에 있으면 교차할 수 없다.
    if (b < 0 && c > 0) {
        return false;
    }

    // 판별식은 반지름 제곱에서 광선과 구 중심 사이의 최단거리 제곱을 뺀 값이다.
    // b*b-c 와 수학적으로 같지만, 큰 제곱값끼리 빼지 않아 원거리에서 더 안정적이다.
    const Vec3 perpendicular = m - b * ray.dir;
    const float discr = radius * radius - perpendicular.LengthSqr();
    // 음수이면 광선이 구를 지나지 않는다.
    if (discr < 0.0f) {
        return false;
    }

    if (hitDistMin || hitDistMax) {
        // q 는 광선 방향으로 측정한 교차 구간의 반길이다.
        float q = Math::Sqrt(discr);

        if (hitDistMin) {
            // 광선 원점이 구 내부에 있으면 진입 거리는 0 이다.
            *hitDistMin = Max(b - q, 0.0f);
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
    // 음수 반지름은 유효한 구를 나타내지 않는다.
    if (radius < 0.0f) {
        return false;
    }

    const Vec3 segment = end - start;
    const float segmentLengthSquared = segment.LengthSqr();

    // 구 중심을 선분에 투영한 비율을 [0, 1] 로 제한하면 최근접점이 선분 안에 놓인다.
    // 길이가 0 인 선분은 fraction 을 0 으로 유지하여 start 한 점으로 처리한다.
    float fraction = 0.0f;
    if (segmentLengthSquared > 0.0f) {
        fraction = Clamp01((center - start).Dot(segment) / segmentLengthSquared);
    }

    // 제곱거리를 비교하여 불필요한 제곱근 계산을 피한다.
    const Vec3 closestPoint = start + fraction * segment;
    return closestPoint.DistanceSqr(center) <= radius * radius;
}

AABB Sphere::ToAABB() const {
    AABB aabb(center, center);
    aabb.ExpandSelf(radius);
    return aabb;
}

BE_NAMESPACE_END
