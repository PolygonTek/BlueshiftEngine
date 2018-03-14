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
#include "Components/ComSocketJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSocketJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSocketJoint = module["ComSocketJoint"];

    _ComSocketJoint.SetClass<ComSocketJoint>(module["ComJoint"]);
    _ComSocketJoint.AddClassMembers<ComSocketJoint>(
        "anchor", &ComSocketJoint::GetAnchor,
        "set_anchor", &ComSocketJoint::SetAnchor);

    _ComSocketJoint["meta_object"] = ComSocketJoint::metaObject;
}

BE_NAMESPACE_END
