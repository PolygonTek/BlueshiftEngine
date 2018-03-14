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

void LuaVM::RegisterFrustum(LuaCpp::Module &module) {
    LuaCpp::Selector _Frustum = module["Frustum"];

    _Frustum.SetClass<Frustum>();
    _Frustum.AddClassMembers<Frustum>(
        "set_origin", &Frustum::SetOrigin,
        "set_axis", &Frustum::SetAxis,
        "set_size", &Frustum::SetSize,
        "move_near_distance", &Frustum::MoveNearDistance,
        "move_far_distance", &Frustum::MoveFarDistance,
        "is_valid", &Frustum::IsValid,
        "near_distance", &Frustum::GetNearDistance,
        "far_distance", &Frustum::GetFarDistance,
        "left", &Frustum::GetLeft,
        "up", &Frustum::GetUp,
        "expand", &Frustum::Expand,
        "expand_self", &Frustum::ExpandSelf,
        "translate", &Frustum::Translate,
        "translate_self", &Frustum::TranslateSelf,
        "rotate", &Frustum::Rotate,
        "rotate_self", &Frustum::RotateSelf,
        "plane_side", &Frustum::PlaneSide,
        "plane_distane", &Frustum::PlaneDistance,
        "cull_point", &Frustum::CullPoint,
        "cull_aabb", &Frustum::CullAABB,
        "cull_obb", &Frustum::CullOBB,
        "cull_sphere", &Frustum::CullSphere,
        "cull_frustum", &Frustum::CullFrustum,
        "is_contain_point", &Frustum::IsContainPoint,
        "is_intersect_abbb", &Frustum::IsIntersectAABB,
        "is_intersect_obb", &Frustum::IsIntersectOBB,
        "is_intersect_sphere", &Frustum::IsIntersectSphere,
        "is_intersect_frustum", &Frustum::IsIntersectFrustum,
        "line_intersection", &Frustum::LineIntersection,
        "ray_intersection", &Frustum::RayIntersection,
        "to_obb", &Frustum::ToOBB,
        "to_minimum_sphere", &Frustum::ToMinimumSphere,
        "to_planes", &Frustum::ToPlanes,
        "to_points", &Frustum::ToPoints);
}

BE_NAMESPACE_END
