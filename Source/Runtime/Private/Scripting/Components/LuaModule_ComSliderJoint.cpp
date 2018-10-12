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
#include "Components/ComSliderJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSliderJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSliderJoint = module["ComSliderJoint"];

    _ComSliderJoint.SetClass<ComSliderJoint>(module["ComJoint"]);
    _ComSliderJoint.AddClassMembers<ComSliderJoint>(
        "local_anchor", &ComSliderJoint::GetLocalAnchor,
        "set_local_anchor", &ComSliderJoint::SetLocalAnchor,
        "local_angles", &ComSliderJoint::GetLocalAngles,
        "set_local_angles", &ComSliderJoint::SetLocalAngles,
        "connected_anchor", &ComSliderJoint::GetConnectedAnchor,
        "set_connected_anchor", &ComSliderJoint::SetConnectedAnchor,
        "connected_angles", &ComSliderJoint::GetConnectedAngles,
        "set_connected_angles", &ComSliderJoint::SetConnectedAngles,
        "enable_limit_angles", &ComSliderJoint::GetEnableLimitAngles,
        "set_enable_limit_angles", &ComSliderJoint::SetEnableLimitAngles,
        "minimum_angle", &ComSliderJoint::GetMinimumAngle,
        "set_minimum_angle", &ComSliderJoint::SetMinimumAngle,
        "maximum_angle", &ComSliderJoint::GetMaximumAngle,
        "set_maximum_angle", &ComSliderJoint::SetMaximumAngle,
        "linear_motor_target_velocity", &ComSliderJoint::GetLinearMotorTargetVelocity,
        "set_linear_motor_target_velocity", &ComSliderJoint::SetLinearMotorTargetVelocity,
        "max_linear_motor_impulse", &ComSliderJoint::GetMaxLinearMotorImpulse,
        "set_max_linear_motor_impulse", &ComSliderJoint::SetMaxLinearMotorImpulse,
        "angular_motor_target_velocity", &ComSliderJoint::GetAngularMotorTargetVelocity,
        "set_angular_motor_target_velocity", &ComSliderJoint::SetAngularMotorTargetVelocity,
        "max_angular_motor_impulse", &ComSliderJoint::GetMaxAngularMotorImpulse,
        "set_max_angular_motor_impulse", &ComSliderJoint::SetMaxAngularMotorImpulse);

    _ComSliderJoint["meta_object"] = ComSliderJoint::metaObject;
}

BE_NAMESPACE_END
