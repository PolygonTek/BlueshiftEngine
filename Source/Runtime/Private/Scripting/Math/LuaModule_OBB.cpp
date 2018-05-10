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

void LuaVM::RegisterOBB(LuaCpp::Module &module) {
    LuaCpp::Selector _OBB = module["OBB"];

    _OBB.SetClass<OBB>();
    _OBB.AddClassMembers<OBB>(
        "__add", static_cast<OBB(OBB::*)(const OBB&)const>(&OBB::operator+),
        "__eq", static_cast<bool(OBB::*)(const OBB&)const>(&OBB::operator==),
        "center", &OBB::Center,
        "extents", &OBB::Extents,
        "axis", &OBB::Axis,
        "set_center", &OBB::SetCenter,
        "set_extents", &OBB::SetExtents,
        "set_axis", &OBB::SetAxis,
        "clear", &OBB::Clear,
        "is_cleared", &OBB::IsCleared,
        "set_zero", &OBB::SetZero,        
        "volume", &OBB::Volume,
        "translate", &OBB::Translate,
        "translate_self", &OBB::TranslateSelf,
        "rotate", &OBB::Rotate,
        "rotate_self", &OBB::RotateSelf,
        "add", static_cast<OBB(OBB::*)(const OBB&)const>(&OBB::operator+),
        "add_self", static_cast<OBB&(OBB::*)(const OBB&)>(&OBB::operator+=),
        "expand", &OBB::Expand,
        "expand_self", &OBB::ExpandSelf,
        "add_point", &OBB::AddPoint,
        "add_obb", &OBB::AddOBB,        
        "plane_side", &OBB::PlaneSide,
        "plane_distance", &OBB::PlaneDistance,
        "nearest_vertex", &OBB::GetNearestVertex,
        "farthest_vertex", &OBB::GetFarthestVertex,
        "closest_point", &OBB::GetClosestPoint,
        "distance_squared", &OBB::DistanceSqr,
        "distance", &OBB::Distance,
        "is_contain_point", &OBB::IsContainPoint,
        "is_intersect_aabb", &OBB::IsIntersectOBB,
        "is_intersect_sphere", &OBB::IsIntersectSphere,
        "line_intersection", &OBB::LineIntersection,
        "ray_intersection", &OBB::RayIntersection,
        //"set_from_points", &OBB::SetFromPoints,
        "to_points", &OBB::ToPoints,
        "to_aabb", &OBB::ToAABB,
        "to_sphere", &OBB::ToSphere);

    _OBB["zero"] = OBB::zero;
}

BE_NAMESPACE_END
