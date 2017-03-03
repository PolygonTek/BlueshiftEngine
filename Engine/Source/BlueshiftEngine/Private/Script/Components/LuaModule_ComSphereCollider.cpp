#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComSphereCollider.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSphereColliderComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSphereCollider = module["ComSphereCollider"];

    _ComSphereCollider.SetClass<ComSphereCollider>(module["ComCollider"]);
    _ComSphereCollider.AddClassMembers<ComSphereCollider>(
        "center", &ComSphereCollider::center,
        "radius", &ComSphereCollider::radius);

    _ComSphereCollider["meta_object"] = ComSphereCollider::metaObject;
}

BE_NAMESPACE_END