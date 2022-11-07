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

void LuaVM::RegisterRectF(LuaCpp::Module &module) {
    LuaCpp::Selector _RectF = module["RectF"];

    _RectF.SetClass<RectF>();
    _RectF.AddClassCtor<RectF, float, float, float, float>();
    _RectF.AddClassMembers<RectF>(
        "x", &RectF::x,
        "y", &RectF::y,
        "w", &RectF::w,
        "h", &RectF::h,
        "__tostring", static_cast<const char*(RectF::*)(void)const>(&RectF::ToString),
        "element", static_cast<float &(RectF::*)(int)>(&RectF::operator[]), // index start from zero
        "assign", static_cast<RectF&(RectF::*)(const RectF&)>(&RectF::operator=),
        "set", &RectF::Set,
        "set_4coords", &RectF::SetFrom4Coords,
        "is_empty", &RectF::IsEmpty,
        "is_contain_point", static_cast<bool(RectF::*)(const PointF&)const>(&RectF::IsContainPoint),
        "is_contain_rect", static_cast<bool(RectF::*)(const RectF&)const>(&RectF::IsContainRect),
        "is_intersect_rect", static_cast<bool(RectF::*)(const RectF&)const>(&RectF::IsIntersectRect),
        "add", static_cast<RectF(RectF::*)(const RectF&)const&>(&RectF::Add),
        "add_self", &RectF::AddSelf,
        "add_point", static_cast<RectF(RectF::*)(const PointF&)const&>(&RectF::AddPoint),
        "add_point_self", &RectF::AddPointSelf,
        "intersect", static_cast<RectF(RectF::*)(const RectF&)const&>(&RectF::Intersect),
        "intersect_self", &RectF::IntersectSelf,
        "move", static_cast<RectF(RectF::*)(float, float)const&>(&RectF::Move),
        "move_self", &RectF::MoveSelf,
        "shrink", static_cast<RectF(RectF::*)(float, float)const&>(&RectF::Shrink),
        "shrink_self", &RectF::ShrinkSelf,
        "expand", static_cast<RectF(RectF::*)(float, float)const&>(&RectF::Expand),
        "expand_self", &RectF::ExpandSelf,
        "to_string", static_cast<const char*(RectF::*)()const>(&RectF::ToString));
}

BE_NAMESPACE_END
