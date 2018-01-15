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

void LuaVM::RegisterSphere(LuaCpp::Module &module) {
    LuaCpp::Selector _Sphere = module["Sphere"];

    _Sphere.SetClass<Sphere>();
    _Sphere.AddClassCtor<Sphere, const Vec3 &, float>();
    _Sphere.AddClassMembers<Sphere>(
        "origin", &Sphere::origin,
        "radius", &Sphere::radius,
        "clear", &Sphere::Clear,
        "set_zero", &Sphere::SetZero,
        "area", &Sphere::Area,
        "volume", &Sphere::Volume,
        "is_contain_point", &Sphere::IsContainPoint,
        "is_intersect_sphere", &Sphere::IsIntersectSphere,
        "is_intersect_aabb", &Sphere::IsIntersectAABB,
        "to_aabb", &Sphere::ToAABB,
        "line_intersection", &Sphere::LineIntersection,
        "ray_intersection", &Sphere::RayIntersection,
        "axis_projection", &Sphere::AxisProjection
    );
    _Sphere.AddClassMembers<Sphere>(
        "__eq", static_cast<bool(Sphere::*)(const Sphere&)const>(&Sphere::operator==)
    );
}

BE_NAMESPACE_END
