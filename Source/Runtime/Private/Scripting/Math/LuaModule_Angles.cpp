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

void LuaVM::RegisterAngles(LuaCpp::Module &module) {
    LuaCpp::Selector _Angles = module["Angles"];

    _Angles.SetClass<Angles>();
    _Angles.AddClassCtor<Angles, float, float, float>();
    _Angles.AddClassMembers<Angles>(
        "roll", &Angles::GetRoll,
        "pitch", &Angles::GetPitch,
        "yaw", &Angles::GetYaw,
        "at", &Angles::At, // index start from zero
        "element", static_cast<float(Angles::*)(int)const>(&Angles::operator[]), // index start from zero
        "assign", static_cast<Angles&(Angles::*)(const Angles&)>(&Angles::operator=),
        "set", &Angles::Set,
        "set_roll", &Angles::SetRoll,
        "set_pitch", &Angles::SetPitch,
        "set_yaw", &Angles::SetYaw,
        "equals", static_cast<bool(Angles::*)(const Angles&, const float)const>(&Angles::Equals),
        "to_forward", &Angles::ToForward,
        "to_right", &Angles::ToRight,
        "to_up", &Angles::ToUp,
        "to_rotation", &Angles::ToRotation,
        "to_quat", &Angles::ToQuat,
        "to_mat3", &Angles::ToMat3,
        "to_mat4", &Angles::ToMat4,
        "to_string", static_cast<const char*(Angles::*)(void)const>(&Angles::ToString),
        "add", static_cast<Angles(Angles::*)(const Angles&)const>(&Angles::operator+),
        "add_self", static_cast<Angles&(Angles::*)(const Angles&)>(&Angles::operator+=),
        "subtract", static_cast<Angles(Angles::*)(const Angles&)const>(&Angles::operator-),
        "subtract_self", static_cast<Angles&(Angles::*)(const Angles&)>(&Angles::operator-=),
        "scale", static_cast<Angles(Angles::*)(const float)const>(&Angles::operator*),
        "scale_self", static_cast<Angles&(Angles::*)(const float)>(&Angles::operator*=),
        "inv_scale", static_cast<Angles(Angles::*)(const float)const>(&Angles::operator/),
        "inv_scale_self", static_cast<Angles&(Angles::*)(const float)>(&Angles::operator/=),
        "normalize360", &Angles::Normalize360,
        "normalize180", &Angles::Normalize180,
        "clamp", &Angles::Clamp,
        "set_from_lerp", &Angles::SetFromLerp
    );
    _Angles.AddClassMembers<Angles>(
        "__tostring", static_cast<const char*(Angles::*)(void)const>(&Angles::ToString),
        "__unm", static_cast<Angles(Angles::*)() const>(&Angles::operator-),
        "__add", static_cast<Angles(Angles::*)(const Angles&)const>(&Angles::operator+),
        "__sub", static_cast<Angles(Angles::*)(const Angles&)const>(&Angles::operator-),
        "__mul", static_cast<Angles(Angles::*)(float)const>(&Angles::operator*),
        "__div", static_cast<Angles(Angles::*)(float)const>(&Angles::operator/),
        "__eq", static_cast<bool(Angles::*)(const Angles&)const>(&Angles::operator==)
    );

    _Angles["zero"] = Angles::zero;

    _Angles["from_lerp"].SetFunc(Angles::FromLerp);
}

BE_NAMESPACE_END
