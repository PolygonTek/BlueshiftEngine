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

const Vec2 Vec2::Compute2DBarycentricCoords(const float s1, const float s2, const float p) {
    float a = s1 - s2;
    if (Math::Fabs(a) < Math::FloatEpsilon) {
        return Vec2::zero;
    }
    float b = (p - s2) / a;

    return Vec2(b, 1.0f - b);
}

BE_NAMESPACE_END
