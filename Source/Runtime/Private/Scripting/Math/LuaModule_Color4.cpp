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

void LuaVM::RegisterColor4(LuaCpp::Module &module) {
    LuaCpp::Selector _Color4 = module["Color4"];

    _Color4.SetClass<Color4>();
    _Color4.AddClassCtor<Color4, float, float, float, float>();
    _Color4.AddClassMembers<Color4>(
        "r", &Color4::r,
        "g", &Color4::g,
        "b", &Color4::b,
        "a", &Color4::a,
        "at", &Color4::At, // index start from zero
        "add", &Color4::Add,
        "add_self", &Color4::AddSelf,
        "sub", &Color4::Sub,
        "sub_self", &Color4::SubSelf,
        "mul", &Color4::Mul,
        "mul_self", &Color4::MulSelf,
        "mul_comp", &Color4::MulComp,
        "mul_comp_self", &Color4::MulCompSelf,
        "div", &Color4::Div,
        "div_self", &Color4::DivSelf,
        "div_comp", &Color4::DivComp,
        "div_comp_self", &Color4::DivCompSelf,
        "equals", static_cast<bool(Color4::*)(const Color4&, const float)const>(&Color4::Equals),
        "set", &Color4::Set,
        "clip", &Color4::Clip,
        "invert", &Color4::Invert,
        "to_color3", static_cast<const Color3&(Color4::*)(void)const>(&Color4::ToColor3),
        "to_vec3", static_cast<const Vec3&(Color4::*)(void)const>(&Color4::ToVec3),
        "to_vec4", static_cast<const Vec4&(Color4::*)(void)const>(&Color4::ToVec4),
        "to_uint32", &Color4::ToUInt32,
        "to_string", static_cast<const char*(Color4::*)(void)const>(&Color4::ToString)
        );
    _Color4.AddClassMembers<Color4>(
        "__tostring", static_cast<const char *(Color4::*)(void) const>(&Color4::ToString),
        "__add", &Color4::Add,
        "__sub", &Color4::Sub,
        "__mul", &Color4::Mul,
        "__div", &Color4::Div,
        "__eq", static_cast<bool(Color4::*)(const Color4&)const>(&Color4::operator==)
        );

    _Color4["black"] = Color4::black;
    _Color4["white"] = Color4::white;
    _Color4["red"] = Color4::red;
    _Color4["green"] = Color4::green;
    _Color4["blue"] = Color4::blue;
    _Color4["yellow"] = Color4::yellow;
    _Color4["cyan"] = Color4::cyan;
    _Color4["magenta"] = Color4::magenta;
    _Color4["orange"] = Color4::orange;
    _Color4["pink"] = Color4::pink;
    _Color4["lawn"] = Color4::lawn;
    _Color4["mint"] = Color4::mint;
    _Color4["violet"] = Color4::violet;
    _Color4["teal"] = Color4::teal;
    _Color4["grey"] = Color4::grey;
    _Color4["darkGrey"] = Color4::darkGrey;
}

BE_NAMESPACE_END
