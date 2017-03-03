#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComCylinderCollider.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterCylinderColliderComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComCylinderCollider = module["ComCylinderCollider"];

    _ComCylinderCollider.SetClass<ComCylinderCollider>(module["ComCollider"]);
    _ComCylinderCollider.AddClassMembers<ComCylinderCollider>(
        "center", &ComCylinderCollider::center,
        "radius", &ComCylinderCollider::radius,
        "height", &ComCylinderCollider::height);

    _ComCylinderCollider["meta_object"] = ComCylinderCollider::metaObject;
}

BE_NAMESPACE_END