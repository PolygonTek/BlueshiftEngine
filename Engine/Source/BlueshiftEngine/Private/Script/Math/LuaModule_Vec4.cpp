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

void LuaVM::RegisterVec4(LuaCpp::Module &module) {
    LuaCpp::Selector _Vec4 = module["Vec4"];

    _Vec4.SetClass<Vec4>();
    _Vec4.AddClassCtor<Vec4, float, float, float, float>();
    _Vec4.AddClassMembers<Vec4>(
        "x", &Vec4::x,
        "y", &Vec4::y,
        "z", &Vec4::z,
        "w", &Vec4::w,
        "at", &Vec4::At, // index start from zero
        "add", &Vec4::Add,
        "add_self", &Vec4::AddSelf,
        "sub", &Vec4::Sub,
        "sub_self", &Vec4::SubSelf,
        "mul", &Vec4::Mul,
        "mul_self", &Vec4::MulSelf,
        "mul_comp", &Vec4::MulComp,
        "mul_comp_self", &Vec4::MulCompSelf,
        "div", &Vec4::Div,
        "div_self", &Vec4::DivSelf,
        "div_comp", &Vec4::DivComp,
        "div_comp_self", &Vec4::DivCompSelf,
        "equals", static_cast<bool(Vec4::*)(const Vec4&, const float)const>(&Vec4::Equals),
        "assign", static_cast<Vec4&(Vec4::*)(const Vec4&)>(&Vec4::operator=),
        "set", &Vec4::Set,
        "set_from_scalar", &Vec4::SetFromScalar,
        "length", &Vec4::Length,
        "length_squared", &Vec4::LengthSqr,
        "normalize", &Vec4::Normalize,
        "normalize_fast", &Vec4::NormalizeFast,
        "clamp", &Vec4::Clamp,
        "dot", &Vec4::Dot,
        "set_from_lerp", &Vec4::SetFromLerp,
        "to_vec2", static_cast<const Vec2&(Vec4::*)(void)const>(&Vec4::ToVec2),
        "to_vec3", static_cast<const Vec3&(Vec4::*)(void)const>(&Vec4::ToVec3),
        "to_string", static_cast<const char*(Vec4::*)(void)const>(&Vec4::ToString)
    );
    _Vec4.AddClassMembers<Vec4>(
        "__tostring", static_cast<const char*(Vec4::*)(void)const>(&Vec4::ToString),
        "__unm", static_cast<Vec4(Vec4::*)(void)const>(&Vec4::operator-),
        "__add", &Vec4::Add,
        "__sub", &Vec4::Sub,
        "__eq", static_cast<bool(Vec4::*)(const Vec4&)const>(&Vec4::operator==)
    );

    _Vec4["origin"] = Vec4::origin;
    _Vec4["zero"] = Vec4::zero;
    _Vec4["one"] = Vec4::one;
    _Vec4["unit_x"] = Vec4::unitX;
    _Vec4["unit_y"] = Vec4::unitY;
    _Vec4["unit_z"] = Vec4::unitZ;
    _Vec4["unit_w"] = Vec4::unitW;
}

BE_NAMESPACE_END
