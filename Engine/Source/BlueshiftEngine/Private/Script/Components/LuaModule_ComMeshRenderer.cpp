#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComMeshRenderer.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterMeshRendererComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComMeshRenderer = module["ComMeshRenderer"];

    _ComMeshRenderer.SetClass<ComMeshRenderer>(module["ComRenderable"]);

    _ComMeshRenderer["meta_object"] = ComMeshRenderer::metaObject;
}

BE_NAMESPACE_END