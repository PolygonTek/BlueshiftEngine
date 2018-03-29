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
#include "Script/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterRay(LuaCpp::Module &module) {
    LuaCpp::Selector _Ray = module["Ray"];

    _Ray.SetClass<Ray>();
    _Ray.AddClassCtor<Ray, const Vec3 &, const Vec3 &>();
    _Ray.AddClassMembers<Ray>(
        "set", &Ray::Set,
        "origin", &Ray::GetOrigin,
        "direction", &Ray::GetDirection,
        "distance_point", &Ray::GetDistancePoint);
}

BE_NAMESPACE_END
