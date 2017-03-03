#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComBoxCollider.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterBoxColliderComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComBoxCollider = module["ComBoxCollider"];

    _ComBoxCollider.SetClass<ComBoxCollider>(module["ComCollider"]);
    _ComBoxCollider.AddClassMembers<ComBoxCollider>(
        "center", &ComBoxCollider::center,
        "extents", &ComBoxCollider::extents);
    
    _ComBoxCollider["meta_object"] = ComBoxCollider::metaObject;
}

BE_NAMESPACE_END