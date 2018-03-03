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
#include "Components/ComSpringJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSpringJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSpringJoint = module["ComSpringJoint"];

    _ComSpringJoint.SetClass<ComSpringJoint>(module["ComJoint"]);
    _ComSpringJoint.AddClassMembers<ComSpringJoint>(
        "anchor", &ComSpringJoint::GetAnchor,
        "set_anchor", &ComSpringJoint::SetAnchor,
        "angles", &ComSpringJoint::GetAngles,
        "set_angles", &ComSpringJoint::SetAngles,
        "lower_limit", &ComSpringJoint::GetLowerLimit,
        "set_lower_limit", &ComSpringJoint::SetLowerLimit,
        "upper_limit", &ComSpringJoint::GetUpperLimit,
        "set_upper_limit", &ComSpringJoint::SetUpperLimit,
        "stiffness", &ComSpringJoint::GetStiffness,
        "set_stiffness", &ComSpringJoint::SetStiffness,
        "damping", &ComSpringJoint::GetDamping,
        "set_damping", &ComSpringJoint::SetDamping);

    _ComSpringJoint["meta_object"] = ComSpringJoint::metaObject;
}

BE_NAMESPACE_END
