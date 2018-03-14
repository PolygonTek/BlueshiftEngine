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

void LuaVM::RegisterAABB(LuaCpp::Module &module) {
    LuaCpp::Selector _AABB = module["AABB"];

    _AABB.SetClass<AABB>();
    _AABB.AddClassCtor<AABB, const Vec3 &, const Vec3 &>();
    _AABB.AddClassMembers<AABB>(
        "__add", static_cast<AABB(AABB::*)(const AABB&)const>(&AABB::operator+),
        "__eq", static_cast<bool(AABB::*)(const AABB&)const>(&AABB::operator==),
        "element", static_cast<Vec3&(AABB::*)(const int)>(&AABB::operator[]), // index start from zero
        "clear", &AABB::Clear,
        "set_zero", &AABB::SetZero,
        "is_cleared", &AABB::IsCleared,
        "center", &AABB::Center,
        "extents", &AABB::Extents,
        "outer_radius", static_cast<float(AABB::*)(void)const>(&AABB::OuterRadius),
        "outer_radius_centered", static_cast<float(AABB::*)(const Vec3&)const>(&AABB::OuterRadius),
        "inner_radius", &AABB::InnerRadius,
        "area", &AABB::Area,
        "volume", &AABB::Volume,
        "translate", static_cast<AABB(AABB::*)(const Vec3&)const>(&AABB::operator+),
        "translate_self", static_cast<AABB&(AABB::*)(const Vec3&)>(&AABB::operator+=),
        "scale", static_cast<AABB(AABB::*)(const float&)const>(&AABB::operator*),
        "scale_self", static_cast<AABB&(AABB::*)(const float&)>(&AABB::operator*=),
        "scale_vec3", static_cast<AABB(AABB::*)(const Vec3&)const>(&AABB::operator*),
        "scale_vec3_self", static_cast<AABB&(AABB::*)(const Vec3&)>(&AABB::operator*=),
        "transform", static_cast<AABB(AABB::*)(const Mat3&)const>(&AABB::operator*),
        "transform_self", static_cast<AABB&(AABB::*)(const Mat3&)>(&AABB::operator*=),
        "add_point", &AABB::AddPoint,
        "add_aabb", &AABB::AddAABB,
        "intersect", &AABB::Intersect,
        "intersect_self", &AABB::IntersectSelf,
        "expand", &AABB::Expand,
        "expand_self", &AABB::ExpandSelf,
        "plane_side", &AABB::PlaneSide,
        "plane_distance", &AABB::PlaneDistance,
        "nearest_vertex", &AABB::GetNearestVertex,
        "nearest_vertex_from_dir", &AABB::GetNearestVertexFromDir,
        "farthest_vertex", &AABB::GetFarthestVertex,
        "farthest_vertex_from_dir", &AABB::GetFarthestVertexFromDir,
        "closest_point", &AABB::GetClosestPoint,
        "distance_squared", &AABB::DistanceSqr,
        "distance", &AABB::Distance,
        "is_contain_point", &AABB::IsContainPoint,
        "is_contain_aabb", &AABB::IsContainAABB,
        "is_intersect_aabb", &AABB::IsIntersectAABB,
        "is_intersect_sphere", &AABB::IsIntersectSphere,
        "is_intersect_Triangle", &AABB::IsIntersectTriangle,
        "line_intersection", &AABB::LineIntersection,
        "ray_intersection", &AABB::RayIntersection,
        "set_from_points", &AABB::SetFromPoints,
        "set_from_point_translation", &AABB::SetFromPointTranslation,
        "set_from_aabb_translation", &AABB::SetFromAABBTranslation,
        "set_from_transformed_aabb", &AABB::SetFromTransformedAABB);

    _AABB["zero"] = AABB::zero;
}

BE_NAMESPACE_END
