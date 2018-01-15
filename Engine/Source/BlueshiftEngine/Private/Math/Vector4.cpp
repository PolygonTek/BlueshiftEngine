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

const Vec4 Vec4::origin(0.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::zero(0.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::one(1.0f, 1.0f, 1.0f, 1.0f);
const Vec4 Vec4::unitX(1.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::unitY(0.0f, 1.0f, 0.0f, 0.0f);
const Vec4 Vec4::unitZ(0.0f, 0.0f, 1.0f, 0.0f);
const Vec4 Vec4::unitW(0.0f, 0.0f, 0.0f, 1.0f);

Vec4 Vec4::FromString(const char *str) {
    Vec4 v;
    sscanf(str, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
    return v;
}

const Vec4 Vec4::Compute4DBarycentricCoords(const Vec3 &s1, const Vec3 &s2, const Vec3 &s3, const Vec3 &s4, const Vec3 &p) {
    Mat3 matInv(s1 - s4, s2 - s4, s3 - s4);
    if (!matInv.InverseSelf()) {
        return Vec4::zero;
    }
    Vec3 b = matInv * (p - s4);

    return Vec4(b[0], b[1], b[2], 1.0f - b[0] - b[1] - b[2]);
}

BE_NAMESPACE_END
