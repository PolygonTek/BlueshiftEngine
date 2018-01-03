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
#include "Components/ComCharacterJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterCharacterJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComCharacterJoint = module["ComCharacterJoint"];

    _ComCharacterJoint.SetClass<ComCharacterJoint>(module["ComJoint"]);
    _ComCharacterJoint.AddClassMembers<ComCharacterJoint>(
        "anchor", &ComCharacterJoint::GetAnchor,
        "set_anchor", &ComCharacterJoint::SetAnchor,
        "angles", &ComCharacterJoint::GetAngles,
        "set_angles", &ComCharacterJoint::SetAngles,
        "swing1_lower_limit", &ComCharacterJoint::GetSwing1LowerLimit,
        "set_swing1_lower_limit", &ComCharacterJoint::SetSwing1LowerLimit,
        "swing1_upper_limit", &ComCharacterJoint::GetSwing1UpperLimit,
        "set_swing1_upper_limit", &ComCharacterJoint::SetSwing1UpperLimit,
        "swing1_stiffness", &ComCharacterJoint::GetSwing1Stiffness,
        "set_swing1_stiffness", &ComCharacterJoint::SetSwing1Stiffness,
        "swing1_damping", &ComCharacterJoint::GetSwing1Damping,
        "set_swing1_damping", &ComCharacterJoint::SetSwing1Damping,
        "swing2_lower_limit", &ComCharacterJoint::GetSwing2LowerLimit,
        "set_swing2_lower_limit", &ComCharacterJoint::SetSwing2LowerLimit,
        "swing2_upper_limit", &ComCharacterJoint::GetSwing2UpperLimit,
        "set_swing2_upper_limit", &ComCharacterJoint::SetSwing2UpperLimit,
        "swing2_stiffness", &ComCharacterJoint::GetSwing2Stiffness,
        "set_swing2_stiffness", &ComCharacterJoint::SetSwing2Stiffness,
        "swing2_damping", &ComCharacterJoint::GetSwing2Damping,
        "set_swing2_damping", &ComCharacterJoint::SetSwing2Damping,
        "twist_lower_limit", &ComCharacterJoint::GetTwistLowerLimit,
        "set_twist_lower_limit", &ComCharacterJoint::SetTwistLowerLimit,
        "twist_upper_limit", &ComCharacterJoint::GetTwistUpperLimit,
        "set_twist_upper_limit", &ComCharacterJoint::SetTwistUpperLimit,
        "twist_stiffness", &ComCharacterJoint::GetTwistStiffness,
        "set_twist_stiffness", &ComCharacterJoint::SetTwistStiffness,
        "twist_damping", &ComCharacterJoint::GetTwistDamping,
        "set_twist_damping", &ComCharacterJoint::SetTwistDamping);

    _ComCharacterJoint["meta_object"] = ComCharacterJoint::metaObject;
}

BE_NAMESPACE_END
