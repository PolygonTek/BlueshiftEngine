#include "Precompiled.h"
#include "Script/LuaVM.h"
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