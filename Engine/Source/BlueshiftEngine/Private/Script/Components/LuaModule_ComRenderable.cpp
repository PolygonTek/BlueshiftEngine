#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComRenderable.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterRenderableComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComRenderable = module["ComRenderable"];

    _ComRenderable.SetClass<ComRenderable>(module["Component"]);
    _ComRenderable.AddClassMembers<ComRenderable>(
        "max_vis_dist", &ComRenderable::GetMaxVisDist,
        "set_max_vis_dist", &ComRenderable::SetMaxVisDist,
        "color", &ComRenderable::GetColor,
        "set_color", &ComRenderable::SetColor,
        "alpha", &ComRenderable::GetAlpha,
        "set_alpha", &ComRenderable::SetAlpha);

    _ComRenderable["meta_object"] = ComRenderable::metaObject;
}

BE_NAMESPACE_END