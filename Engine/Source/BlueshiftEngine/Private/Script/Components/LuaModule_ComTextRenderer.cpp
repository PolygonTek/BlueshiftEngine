#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComTextRenderer.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterTextRendererComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComTextRenderer = module["ComTextRenderer"];

    _ComTextRenderer.SetClass<ComTextRenderer>(module["ComRenderable"]);
    _ComTextRenderer.AddClassMembers<ComTextRenderer>(
        "set_text", &ComTextRenderer::SetText);

    _ComTextRenderer["meta_object"] = ComTextRenderer::metaObject;
}

BE_NAMESPACE_END