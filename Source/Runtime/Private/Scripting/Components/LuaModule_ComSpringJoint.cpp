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
#include "Components/ComSpringJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSpringJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSpringJoint = module["ComSpringJoint"];

    _ComSpringJoint.SetClass<ComSpringJoint>(module["ComJoint"]);
    _ComSpringJoint.AddClassMembers<ComSpringJoint>(
        "local_anchor", &ComSpringJoint::GetLocalAnchor,
        "set_local_anchor", &ComSpringJoint::SetLocalAnchor,
        "local_angles", &ComSpringJoint::GetLocalAngles,
        "set_local_angles", &ComSpringJoint::SetLocalAngles,
        "connected_anchor", &ComSpringJoint::GetConnectedAnchor,
        "set_connected_anchor", &ComSpringJoint::SetConnectedAnchor,
        "connected_angles", &ComSpringJoint::GetConnectedAngles,
        "set_connected_angles", &ComSpringJoint::SetConnectedAngles,
        "minimum_distance", &ComSpringJoint::GetMinimumDistance,
        "set_minimum_distance", &ComSpringJoint::SetMinimumDistance,
        "maximum_distance", &ComSpringJoint::GetMaximumDistance,
        "set_maximum_distance", &ComSpringJoint::SetMaximumDistance,
        "stiffness", &ComSpringJoint::GetStiffness,
        "set_stiffness", &ComSpringJoint::SetStiffness,
        "damping", &ComSpringJoint::GetDamping,
        "set_damping", &ComSpringJoint::SetDamping);

    _ComSpringJoint["meta_object"] = ComSpringJoint::metaObject;
}

BE_NAMESPACE_END
