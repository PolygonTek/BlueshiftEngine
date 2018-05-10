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
#include "Components/ComCharacterController.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterCharacterControllerComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComCharacterController = module["ComCharacterController"];

    _ComCharacterController.SetClass<ComCharacterController>(module["Component"]);
    _ComCharacterController.AddClassMembers<ComCharacterController>(
        "mass", &ComCharacterController::GetMass,
        "set_mass", &ComCharacterController::SetMass,
        "capsule_radius", &ComCharacterController::GetCapsuleRadius,
        "set_capsule_radius", &ComCharacterController::SetCapsuleRadius,
        "capsule_height", &ComCharacterController::GetCapsuleHeight,
        "set_capsule_height", &ComCharacterController::SetCapsuleHeight,
        "step_offset", &ComCharacterController::GetStepOffset,
        "set_Step_offset", &ComCharacterController::SetStepOffset,
        "slope_limit", &ComCharacterController::GetSlopeLimit,
        "set_slope_limit", &ComCharacterController::SetSlopeLimit);

    _ComCharacterController.AddClassMembers<ComCharacterController>(
        "is_on_ground", &ComCharacterController::IsOnGround,
        "move", &ComCharacterController::Move);

    _ComCharacterController["meta_object"] = ComCharacterController::metaObject;
}

BE_NAMESPACE_END
