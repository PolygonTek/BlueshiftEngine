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

void LuaVM::RegisterColor3(LuaCpp::Module &module) {
    LuaCpp::Selector _Color3 = module["Color3"];

    _Color3.SetClass<Color3>();
    _Color3.AddClassCtor<Color3, float, float, float>();
    _Color3.AddClassMembers<Color3>(
        "r", &Color3::r,
        "g", &Color3::g,
        "b", &Color3::b,
        "at", &Color3::At, // index start from zero
        "add", &Color3::Add,
        "add_self", &Color3::AddSelf,
        "sub", &Color3::Sub,
        "sub_self", &Color3::SubSelf,
        "mul", &Color3::Mul,
        "mul_self", &Color3::MulSelf,
        "mul_comp", &Color3::MulComp,
        "mul_comp_self", &Color3::MulCompSelf,
        "div", &Color3::Div,
        "div_self", &Color3::DivSelf,
        "div_comp", &Color3::DivComp,
        "div_comp_self", &Color3::DivCompSelf,
        "equals", static_cast<bool(Color3::*)(const Color3&, const float)const>(&Color3::Equals),
        "set", &Color3::Set,
        "clip", &Color3::Clip,
        "invert", &Color3::Invert,
        "to_vec3", static_cast<const Vec3&(Color3::*)(void)const>(&Color3::ToVec3),
        "to_uint32", &Color3::ToUInt32,
        "to_string", static_cast<const char*(Color3::*)(void)const>(&Color3::ToString)
    );
    _Color3.AddClassMembers<Color3>(
        "__tostring", static_cast<const char *(Color3::*)(void) const>(&Color3::ToString),
        "__add", &Color3::Add,
        "__sub", &Color3::Sub,
        "__mul", &Color3::Mul,
        "__div", &Color3::Div,
        "__eq", static_cast<bool(Color3::*)(const Color3&)const>(&Color3::operator==)
    );

    _Color3["black"] = Color3::black;
    _Color3["white"] = Color3::white;
    _Color3["red"] = Color3::red;
    _Color3["green"] = Color3::green;
    _Color3["blue"] = Color3::blue;
    _Color3["yellow"] = Color3::yellow;
    _Color3["cyan"] = Color3::cyan;
    _Color3["magenta"] = Color3::magenta;
    _Color3["orange"] = Color3::orange;
    _Color3["pink"] = Color3::pink;
    _Color3["lawn"] = Color3::lawn;
    _Color3["mint"] = Color3::mint;
    _Color3["violet"] = Color3::violet;
    _Color3["teal"] = Color3::teal;
    _Color3["grey"] = Color3::grey;
    _Color3["darkGrey"] = Color3::darkGrey;
}

BE_NAMESPACE_END
