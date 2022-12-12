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
#include "Game/Entity.h"
#include "Components/Physics/ComSoftBody.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSoftBodyComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSoftBody = module["ComSoftBody"];

    _ComSoftBody.SetClass<ComSoftBody>(module["Component"]);
    _ComSoftBody.AddClassMembers<ComSoftBody>(
        "mass", &ComSoftBody::GetMass,
        "set_mass", &ComSoftBody::SetMass,
        "friction", &ComSoftBody::GetFriction,
        "set_friction", &ComSoftBody::SetFriction,
        "gravity", &ComSoftBody::GetGravity,
        "set_gravity", &ComSoftBody::SetGravity,
        "is_ccd_enabled", &ComSoftBody::IsCCDEnabled,
        "set_ccd_enabled", &ComSoftBody::SetCCDEnabled);

    _ComSoftBody["meta_object"] = ComSoftBody::metaObject;
}

BE_NAMESPACE_END
