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

ALIGN_AS32 const Vec4 Vec4::origin(0.0f, 0.0f, 0.0f, 0.0f);
ALIGN_AS32 const Vec4 Vec4::zero(0.0f, 0.0f, 0.0f, 0.0f);
ALIGN_AS32 const Vec4 Vec4::one(1.0f, 1.0f, 1.0f, 1.0f);
ALIGN_AS32 const Vec4 Vec4::unitX(1.0f, 0.0f, 0.0f, 0.0f);
ALIGN_AS32 const Vec4 Vec4::unitY(0.0f, 1.0f, 0.0f, 0.0f);
ALIGN_AS32 const Vec4 Vec4::unitZ(0.0f, 0.0f, 1.0f, 0.0f);
ALIGN_AS32 const Vec4 Vec4::unitW(0.0f, 0.0f, 0.0f, 1.0f);
ALIGN_AS32 const Vec4 Vec4::infinity(Math::Infinity, Math::Infinity, Math::Infinity, Math::Infinity);

Vec4 Vec4::FromString(const char *str) {
    Vec4 v;
    int count = sscanf(str, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
    assert(count == v.GetDimension());
    return v;
}

BE_NAMESPACE_END
