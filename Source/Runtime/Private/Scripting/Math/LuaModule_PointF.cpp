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

void LuaVM::RegisterPointF(LuaCpp::Module &module) {
    LuaCpp::Selector _PointF = module["PointF"];

    _PointF.SetClass<PointF>();
    _PointF.AddClassCtor<PointF, float, float>();
    _PointF.AddClassMembers<PointF>(
        "x", &PointF::x,
        "y", &PointF::y,
        "__tostring", static_cast<const char*(PointF::*)(void)const>(&PointF::ToString),
        "__unm", static_cast<PointF(PointF::*)(void)const&>(&PointF::operator-),
        "__add", static_cast<PointF(PointF::*)(const PointF&)const&>(&PointF::operator+),
        "__sub", static_cast<PointF(PointF::*)(const PointF&)const&>(&PointF::operator-),
        "element", static_cast<float &(PointF::*)(int)>(&PointF::operator[]), // index start from zero
        "assign", static_cast<PointF&(PointF::*)(const PointF&)>(&PointF::operator=),
        "set", &PointF::Set,
        "add_self", static_cast<PointF&(PointF::*)(const PointF&)>(&PointF::operator+=),
        "sub_self", static_cast<PointF&(PointF::*)(const PointF&)>(&PointF::operator-=),
        "distance", &PointF::Distance,
        "distance_squared", &PointF::DistanceSqr,
        "to_string", static_cast<const char*(PointF::*)()const>(&PointF::ToString));

    _PointF["zero"] = PointF::zero;
}

BE_NAMESPACE_END
