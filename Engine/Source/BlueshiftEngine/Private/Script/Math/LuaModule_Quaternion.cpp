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

void LuaVM::RegisterQuaternion(LuaCpp::Module &module) {
    LuaCpp::Selector _Quat = module["Quat"];

    _Quat.SetClass<Quat>();
    _Quat.AddClassCtor<Quat, float, float, float, float>();
    _Quat.AddClassMembers<Quat>(
        "x", &Quat::x,
        "y", &Quat::y,
        "z", &Quat::z,
        "w", &Quat::w,
        "element", static_cast<float &(Quat::*)(int)>(&Quat::operator[]), // index start from zero
        "assign", static_cast<Quat&(Quat::*)(const Quat&)>(&Quat::operator=),
        "set", &Quat::Set,
        "set_identity", &Quat::SetIdentity,
        "to_string", static_cast<const char*(Quat::*)()const>(&Quat::ToString),
        "calc_w", &Quat::CalcW,
        "transform", static_cast<Vec3(Quat::*)(const Vec3&)const>(&Quat::operator*),
        "length", &Quat::Length,
        "normalize", &Quat::Normalize,
        "inverse", &Quat::Inverse,
        "set_from_angle_axis", &Quat::SetFromAngleAxis,
        "set_from_two_vectors", &Quat::SetFromTwoVectors,
        "set_from_slerp", &Quat::SetFromSlerp,
        "set_from_slerp_fast", &Quat::SetFromSlerpFast,
        "to_angles", &Quat::ToAngles,
        "to_rotation", &Quat::ToRotation,
        "to_mat3", &Quat::ToMat3
    );
    _Quat.AddClassMembers<Quat>(
        "__tostring", static_cast<const char*(Quat::*)(void)const>(&Quat::ToString),
        "__unm", static_cast<Quat(Quat::*)(void)const>(&Quat::operator-),
        "__add", static_cast<Quat(Quat::*)(const Quat&)const>(&Quat::operator+),
        "__sub", static_cast<Quat(Quat::*)(const Quat&)const>(&Quat::operator-),
        "__mul", static_cast<Quat(Quat::*)(const Quat&)const>(&Quat::operator*)
    );

    _Quat["from_angle_axis"].SetFunc(Quat::FromAngleAxis);
    _Quat["from_two_vectors"].SetFunc(Quat::FromTwoVectors);
    _Quat["from_slerp"].SetFunc(Quat::FromSlerp);
    _Quat["from_slerp_fast"].SetFunc(Quat::FromSlerpFast);
}

BE_NAMESPACE_END
