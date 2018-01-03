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
#include "Components/ComHingeJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterHingeJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComHingeJoint = module["ComHingeJoint"];

    _ComHingeJoint.SetClass<ComHingeJoint>(module["ComJoint"]);
    _ComHingeJoint.AddClassMembers<ComHingeJoint>(
        "anchor", &ComHingeJoint::GetAnchor,
        "set_anchor", &ComHingeJoint::SetAnchor,
        "angles", &ComHingeJoint::GetAngles,
        "set_angles", &ComHingeJoint::SetAngles,
        "motor_speed", &ComHingeJoint::GetMotorSpeed,
        "set_motor_speed", &ComHingeJoint::SetMotorSpeed,
        "max_motor_impulse", &ComHingeJoint::GetMaxMotorImpulse,
        "set_max_motor_impulse", &ComHingeJoint::SetMaxMotorImpulse);

    _ComHingeJoint["meta_object"] = ComHingeJoint::metaObject;
}

BE_NAMESPACE_END
