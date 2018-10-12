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

void LuaVM::RegisterVec3(LuaCpp::Module &module) {
    LuaCpp::Selector _Vec3 = module["Vec3"];

    _Vec3.SetClass<Vec3>();
    _Vec3.AddClassCtor<Vec3, float, float, float>();
    _Vec3.AddClassMembers<Vec3>(
        "x", &Vec3::x,
        "y", &Vec3::y,
        "z", &Vec3::z,
        "at", &Vec3::At,
        "add", &Vec3::Add,
        "add_self", &Vec3::AddSelf,
        "sub", &Vec3::Sub,
        "sub_self", &Vec3::SubSelf,
        "mul", &Vec3::Mul,
        "mul_self", &Vec3::MulSelf,
        "mul_comp", &Vec3::MulComp,
        "mul_comp_self", &Vec3::MulCompSelf,
        "div", &Vec3::Div,
        "div_self", &Vec3::DivSelf,
        "div_comp", &Vec3::DivComp,
        "div_comp_self", &Vec3::DivCompSelf,
        "equals", static_cast<bool(Vec3::*)(const Vec3&, const float)const>(&Vec3::Equals),
        "assign", static_cast<Vec3&(Vec3::*)(const Vec3&)>(&Vec3::operator=),
        "set", &Vec3::Set,
        "set_from_scalar", &Vec3::SetFromScalar,
        "length", &Vec3::Length,
        "length_squared", &Vec3::LengthSqr,
        "distance", &Vec3::Distance,
        "distance_squared", &Vec3::DistanceSqr,
        "normalize", &Vec3::Normalize,
        "normalize_fast", &Vec3::NormalizeFast,
        "scale_to_length", &Vec3::ScaleToLength,
        "clamp", &Vec3::Clamp,
        "dot", &Vec3::Dot,
        "cross", &Vec3::Cross,
        "set_from_cross", &Vec3::SetFromCross,
        "reflect", &Vec3::Reflect,
        "reflect_self", &Vec3::ReflectSelf,
        "refract", &Vec3::Refract,
        "refract_self", &Vec3::RefractSelf,
        "set_from_lerp", &Vec3::SetFromLerp,
        "set_from_slerp", &Vec3::SetFromSLerp,
        "compute_yaw", &Vec3::ComputeYaw,
        "compute_pitch", &Vec3::ComputePitch,
        "to_angles", &Vec3::ToAngles,
        "to_vec2", static_cast<const Vec2&(Vec3::*)(void)const>(&Vec3::ToVec2),
        "to_string", static_cast<const char*(Vec3::*)(void)const>(&Vec3::ToString)
    );
    _Vec3.AddClassMembers<Vec3>(
        "__tostring", static_cast<const char *(Vec3::*)(void) const>(&Vec3::ToString),
        "__unm", static_cast<Vec3(Vec3::*)()const>(&Vec3::operator-),
        "__add", &Vec3::Add,
        "__sub", &Vec3::Sub,
        "__mul", &Vec3::Mul,
        "__div", &Vec3::Div,
        "__eq", static_cast<bool(Vec3::*)(const Vec3&)const>(&Vec3::operator==)
    );

    _Vec3["origin"] = Vec3::origin;
    _Vec3["zero"] = Vec3::zero;
    _Vec3["one"] = Vec3::one;
    _Vec3["unit_x"] = Vec3::unitX;
    _Vec3["unit_y"] = Vec3::unitY;
    _Vec3["unit_z"] = Vec3::unitZ;

    _Vec3["from_lerp"].SetFunc(Vec3::FromLerp);
    _Vec3["from_slerp"].SetFunc(Vec3::FromSLerp);
}

BE_NAMESPACE_END
