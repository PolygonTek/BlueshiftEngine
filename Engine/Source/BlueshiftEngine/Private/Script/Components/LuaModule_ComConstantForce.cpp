#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComConstantForce.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterConstantForceComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComConstantForce = module["ComConstantForce"];

    _ComConstantForce.SetClass<ComConstantForce>(module["Component"]);

    _ComConstantForce["meta_object"] = ComConstantForce::metaObject;
}

BE_NAMESPACE_END