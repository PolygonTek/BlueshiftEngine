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

void LuaVM::RegisterRect(LuaCpp::Module &module) {
    LuaCpp::Selector _Rect = module["Rect"];

    _Rect.SetClass<Rect>();
    _Rect.AddClassCtor<Rect, int, int, int, int>();
    _Rect.AddClassMembers<Rect>(
        "x", &Rect::x,
        "y", &Rect::y,
        "w", &Rect::w,
        "h", &Rect::h,
        "__tostring", static_cast<const char*(Rect::*)(void)const>(&Rect::ToString),
        "element", static_cast<int &(Rect::*)(int)>(&Rect::operator[]), // index start from zero
        "assign", static_cast<Rect&(Rect::*)(const Rect&)>(&Rect::operator=),
        "set", &Rect::Set,
        "set_4coords", &Rect::SetFrom4Coords,
        "is_empty", &Rect::IsEmpty,
        "is_contain_point", static_cast<bool(Rect::*)(const Point&)const>(&Rect::IsContainPoint),
        "is_contain_rect", static_cast<bool(Rect::*)(const Rect&)const>(&Rect::IsContainRect),
        "is_intersect_rect", static_cast<bool(Rect::*)(const Rect&)const>(&Rect::IsIntersectRect),
        "add", static_cast<Rect(Rect::*)(const Rect&)const>(&Rect::Add),
        "add_self", &Rect::AddSelf,
        "add_point", static_cast<Rect(Rect::*)(const Point&)const>(&Rect::AddPoint),
        "add_point_self", &Rect::AddPointSelf,
        "intersect", static_cast<Rect(Rect::*)(const Rect&)const>(&Rect::Intersect),
        "intersect_self", &Rect::IntersectSelf,
        "move", &Rect::Move,
        "move_self", &Rect::MoveSelf,
        "shrink", &Rect::Shrink,
        "shrink_self", &Rect::ShrinkSelf,
        "expand", &Rect::Expand,
        "expand_self", &Rect::ExpandSelf,
        "to_string", static_cast<const char*(Rect::*)()const>(&Rect::ToString));
}

BE_NAMESPACE_END
