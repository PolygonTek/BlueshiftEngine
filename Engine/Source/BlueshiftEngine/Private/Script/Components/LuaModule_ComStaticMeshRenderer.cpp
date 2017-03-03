#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComStaticMeshRenderer.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterStaticMeshRendererComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComStaticMeshRenderer = module["ComStaticMeshRenderer"];

    _ComStaticMeshRenderer.SetClass<ComStaticMeshRenderer>(module["ComMeshRenderer"]);

    _ComStaticMeshRenderer["meta_object"] = ComStaticMeshRenderer::metaObject;
}

BE_NAMESPACE_END