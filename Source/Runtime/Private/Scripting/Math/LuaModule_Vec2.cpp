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

void LuaVM::RegisterVec2(LuaCpp::Module &module) {
    LuaCpp::Selector _Vec2 = module["Vec2"];

    _Vec2.SetClass<Vec2>();
    _Vec2.AddClassCtor<Vec2, float, float>();
    _Vec2.AddClassMembers<Vec2>(
        "x", &Vec2::x,
        "y", &Vec2::y,
        "at", &Vec2::At,
        "add", &Vec2::Add,
        "add_self", &Vec2::AddSelf,
        "sub", &Vec2::Sub,
        "sub_self", &Vec2::SubSelf,
        "mul", &Vec2::Mul,
        "mul_self", &Vec2::MulSelf,
        "mul_comp", &Vec2::MulComp,
        "mul_comp_self", &Vec2::MulCompSelf,
        "div", &Vec2::Div,
        "div_self", &Vec2::DivSelf,
        "div_comp", &Vec2::DivComp,
        "div_comp_self", &Vec2::DivCompSelf,
        "equals", static_cast<bool(Vec2::*)(const Vec2&, const float)const>(&Vec2::Equals),
        "assign", static_cast<Vec2&(Vec2::*)(const Vec2&)>(&Vec2::operator=),
        "set", &Vec2::Set,
        "set_from_scalar", &Vec2::SetFromScalar,
        "length", &Vec2::Length,
        "length_squared", &Vec2::LengthSqr,
        "distance", &Vec2::Distance,
        "distance_squared", &Vec2::DistanceSqr,
        "normalize", &Vec2::Normalize,
        "normalize_fast", &Vec2::NormalizeFast,
        "scale_to_length", &Vec2::ScaleToLength,
        "clamp", &Vec2::Clamp,
        "dot", &Vec2::Dot,
        "cross", &Vec2::Cross,
        "angle_between", &Vec2::AngleBetween,
        "angle_between_norm", &Vec2::AngleBetweenNorm,
        "set_from_lerp", &Vec2::SetFromLerp,
        "to_angle", &Vec2::ToAngle,
        "set_from_angle", &Vec2::SetFromAngle,
        "to_string", static_cast<const char*(Vec2::*)(void)const>(&Vec2::ToString)
    );
    _Vec2.AddClassMembers<Vec2>(
        "__tostring", static_cast<const char *(Vec2::*)(void)const>(&Vec2::ToString),
        "__unm", static_cast<Vec2(Vec2::*)() const>(&Vec2::operator-),
        "__add", &Vec2::Add,
        "__sub", &Vec2::Sub,
        "__mul", &Vec2::Mul,
        "__div", &Vec2::Div,
        "__eq", static_cast<bool(Vec2::*)(const Vec2&)const>(&Vec2::operator==)
    );

    _Vec2["origin"] = Vec2::origin;
    _Vec2["zero"] = Vec2::zero;
    _Vec2["one"] = Vec2::one;
    _Vec2["unit_x"] = Vec2::unitX;
    _Vec2["unit_y"] = Vec2::unitY;

    _Vec2["from_angle"].SetFunc(Vec2::FromAngle);
    _Vec2["from_lerp"].SetFunc(Vec2::FromLerp);
}

BE_NAMESPACE_END
