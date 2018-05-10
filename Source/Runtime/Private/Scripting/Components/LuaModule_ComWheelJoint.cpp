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
#include "Components/ComWheelJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterWheelJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComWheelJoint = module["ComWheelJoint"];

    _ComWheelJoint.SetClass<ComWheelJoint>(module["ComJoint"]);
    _ComWheelJoint.AddClassMembers<ComWheelJoint>(
        "local_anchor", &ComWheelJoint::GetLocalAnchor,
        "set_local_anchor", &ComWheelJoint::SetLocalAnchor,
        "local_angles", &ComWheelJoint::GetLocalAngles,
        "set_local_angles", &ComWheelJoint::SetLocalAngles,
        "connected_anchor", &ComWheelJoint::GetConnectedAnchor,
        "set_connected_anchor", &ComWheelJoint::SetConnectedAnchor,
        "connected_angles", &ComWheelJoint::GetConnectedAngles,
        "set_connected_angles", &ComWheelJoint::SetConnectedAngles,
        "get_enable_suspension_limit", &ComWheelJoint::GetEnableSuspensionLimit,
        "set_enable_suspension_limit", &ComWheelJoint::SetEnableSuspensionLimit,
        "minimum_suspension_distance", &ComWheelJoint::GetMinimumSuspensionDistance,
        "set_minimum_suspension_distance", &ComWheelJoint::SetMinimumSuspensionDistance,
        "maximum_suspension_distance", &ComWheelJoint::GetMaximumSuspensionDistance,
        "set_maximum_suspension_distance", &ComWheelJoint::SetMaximumSuspensionDistance,
        "suspension_stiffness", &ComWheelJoint::GetSuspensionStiffness,
        "set_suspension_stiffness", &ComWheelJoint::SetSuspensionStiffness,
        "suspension_damping", &ComWheelJoint::GetSuspensionDamping,
        "set_suspension_damping", &ComWheelJoint::SetSuspensionDamping,
        "get_enable_steering_limit", &ComWheelJoint::GetEnableSteeringLimit,
        "set_enable_steering_limit", &ComWheelJoint::SetEnableSteeringLimit,
        "minimum_steering_angle", &ComWheelJoint::GetMinimumSteeringAngle,
        "set_minimum_steering_angle", &ComWheelJoint::SetMinimumSteeringAngle,
        "maximum_steering_angle", &ComWheelJoint::GetMaximumSteeringAngle,
        "set_maximum_steering_angle", &ComWheelJoint::SetMaximumSteeringAngle);

    _ComWheelJoint["meta_object"] = ComWheelJoint::metaObject;
}

BE_NAMESPACE_END
