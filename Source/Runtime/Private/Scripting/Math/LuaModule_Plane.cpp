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
#include "Scripting/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterPlane(LuaCpp::Module &module) {
    LuaCpp::Selector _Plane = module["Plane"];

    _Plane.SetClass<Plane>();
    _Plane.AddClassCtor<Plane, const Vec3 &, float>();
    _Plane.AddClassMembers<Plane>(
        "normal", &Plane::normal,
        "offset", &Plane::offset,
        "element", static_cast<float&(Plane::*)(int)>(&Plane::operator[]), // index start from zero
        "set_from_points", &Plane::SetFromPoints,
        "fit_through_point", &Plane::FitThroughPoint,
        "to_string", static_cast<const char*(Plane::*)()const>(&Plane::ToString),
        "flip", &Plane::Flip,
        "normalize", &Plane::Normalize,
        "fix_degenerate_normal", &Plane::FixDegenerateNormal,
        "translate", static_cast<Plane(Plane::*)(const Vec3&)const&>(&Plane::Translate),
        "translate_self", &Plane::TranslateSelf,
        "rotate", static_cast<Plane(Plane::*)(const Vec3&, const Mat3&)const&>(&Plane::Rotate),
        "rotate_self", &Plane::RotateSelf,
        "transform_by_mat3", static_cast<Plane(Plane::*)(const Mat3&)const&>(&Plane::Transform),
        "transform_by_mat3_self", static_cast<Plane&(Plane::*)(const Mat3&)>(&Plane::TransformSelf),
        "transform_by_mat3x4", static_cast<Plane(Plane::*)(const Mat3x4&)const&>(&Plane::Transform),
        "transform_by_mat3x4_self", static_cast<Plane&(Plane::*)(const Mat3x4&)>(&Plane::TransformSelf),
        "distance", &Plane::Distance,
        "side", &Plane::GetSide,
        "is_intersect_line", &Plane::IsIntersectLine,
        "intersect_ray", static_cast<float(Plane::*)(const Ray&, bool)const>(&Plane::IntersectRay)
    );
    _Plane.AddClassMembers<Plane>(
        "__tostring", static_cast<const char*(Plane::*)(void)const>(&Plane::ToString)
    );
}

BE_NAMESPACE_END
