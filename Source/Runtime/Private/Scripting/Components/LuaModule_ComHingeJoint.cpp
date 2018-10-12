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
#include "Components/ComHingeJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterHingeJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComHingeJoint = module["ComHingeJoint"];

    _ComHingeJoint.SetClass<ComHingeJoint>(module["ComJoint"]);
    _ComHingeJoint.AddClassMembers<ComHingeJoint>(
        "local_anchor", &ComHingeJoint::GetLocalAnchor,
        "set_local_anchor", &ComHingeJoint::SetLocalAnchor,
        "local_angles", &ComHingeJoint::GetLocalAngles,
        "set_local_angles", &ComHingeJoint::SetLocalAngles,
        "connected_anchor", &ComHingeJoint::GetConnectedAnchor,
        "set_connected_anchor", &ComHingeJoint::SetConnectedAnchor,
        "connected_angles", &ComHingeJoint::GetConnectedAngles,
        "set_connected_angles", &ComHingeJoint::SetConnectedAngles,
        "enable_limit_angles", &ComHingeJoint::GetEnableLimitAngles,
        "set_enable_limit_angles", &ComHingeJoint::SetEnableLimitAngles,
        "minimum_angle", &ComHingeJoint::GetMinimumAngle,
        "set_minimum_angle", &ComHingeJoint::SetMinimumAngle,
        "maximum_angle", &ComHingeJoint::GetMaximumAngle,
        "set_maximum_angle", &ComHingeJoint::SetMaximumAngle,
        "motor_target_velocity", &ComHingeJoint::GetMotorTargetVelocity,
        "set_motor_target_velocity", &ComHingeJoint::SetMotorTargetVelocity,
        "max_motor_impulse", &ComHingeJoint::GetMaxMotorImpulse,
        "set_max_motor_impulse", &ComHingeJoint::SetMaxMotorImpulse);

    _ComHingeJoint["meta_object"] = ComHingeJoint::metaObject;
}

BE_NAMESPACE_END
