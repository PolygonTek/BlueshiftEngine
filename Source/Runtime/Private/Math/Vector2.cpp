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

const Vec2 Vec2::origin(0.0f, 0.0f);
const Vec2 Vec2::zero(0.0f, 0.0f);
const Vec2 Vec2::one(1.0f, 1.0f);
const Vec2 Vec2::unitX(1.0f, 0.0f);
const Vec2 Vec2::unitY(0.0f, 1.0f);

Vec2 Vec2::FromString(const char *str) {
    Vec2 v;
    sscanf(str, "%f %f", &v.x, &v.y);
    return v;
}

// Mapping unit square to the point on unit disk (filled circle)
Vec2 Vec2::FromUniformSampleDisk(float u1, float u2) {
    float r = Math::Sqrt(u1);
    float s, c;
    Math::SinCos(Math::TwoPi * u2, s, c);
    return Vec2(r * c, r * s);
}

// Mapping unit square to the point on unit disk (filled circle) 
// Peter Shirley (1997): Concentric mapping gives less distortion than above function
Vec2 Vec2::FromConcentricSampleDisk(float u1, float u2) {
    // Map uniform random numbers to [-1, 1]
    float ox = 2.0f * u1 - 1.0f;
    float oy = 2.0f * u2 - 1.0f;
    float theta, r, s, c;

    // Handle degeneracy at the origin
    if (ox == 0 && oy == 0) {
        return Vec2::origin;
    }

    // Apply concentric mapping to point
    if (ox * ox > oy * oy) {
        r = ox;
        theta = Math::OneFourthPi * (oy / ox);
    } else {
        r = oy;
        theta = Math::HalfPi - Math::OneFourthPi * (ox / oy);
    }

    Math::SinCos(theta, s, c);
    return Vec2(r * c, r * s);
}

const Vec2 Vec2::Compute2DBarycentricCoords(const float s1, const float s2, const float p) {
    float a = s1 - s2;
    if (Math::Fabs(a) < VECTOR_EPSILON) {
        return Vec2::zero;
    }
    float b = (p - s2) / a;

    return Vec2(b, 1.0f - b);
}

BE_NAMESPACE_END
