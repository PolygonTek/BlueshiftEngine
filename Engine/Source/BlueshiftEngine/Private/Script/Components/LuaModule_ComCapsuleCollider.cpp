#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComCapsuleCollider.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterCapsuleColliderComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComCapsuleCollider = module["ComCapsuleCollider"];

    _ComCapsuleCollider.SetClass<ComCapsuleCollider>(module["ComCollider"]);
    _ComCapsuleCollider.AddClassMembers<ComCapsuleCollider>(
        "center", &ComCapsuleCollider::center,
        "radius", &ComCapsuleCollider::radius,
        "height", &ComCapsuleCollider::height);

    _ComCapsuleCollider["meta_object"] = ComCapsuleCollider::metaObject;
}

BE_NAMESPACE_END