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

const char *Ray::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

Ray Ray::FromString(const char *str) {
    Ray r;
    int count = sscanf(str, "%f %f %f %f %f %f", &r.origin.x, &r.origin.y, &r.origin.z, &r.dir.x, &r.dir.y, &r.dir.z);
    assert(count == r.GetDimension());
    return r;
}

BE_NAMESPACE_END
