#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComMeshCollider.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterMeshColliderComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComMeshCollider = module["ComMeshCollider"];

    _ComMeshCollider.SetClass<ComMeshCollider>(module["ComCollider"]);

    _ComMeshCollider["meta_object"] = ComMeshCollider::metaObject;
}

BE_NAMESPACE_END