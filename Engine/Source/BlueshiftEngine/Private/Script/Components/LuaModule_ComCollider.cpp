#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComCollider.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterColliderComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComCollider = module["ComCollider"];

    _ComCollider.SetClass<ComCollider>(module["Component"]);

    _ComCollider["meta_object"] = ComCollider::metaObject;
}

BE_NAMESPACE_END