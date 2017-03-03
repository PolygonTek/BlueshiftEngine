#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComSkinnedMeshRenderer.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSkinnedMeshRendererComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSkinnedMeshRenderer = module["ComSkinnedMeshRenderer"];

    _ComSkinnedMeshRenderer.SetClass<ComSkinnedMeshRenderer>(module["ComMeshRenderer"]);
    _ComSkinnedMeshRenderer.AddClassMembers<ComSkinnedMeshRenderer>(
        "translation", &ComSkinnedMeshRenderer::GetTranslation,
        "translation_delta", &ComSkinnedMeshRenderer::GetTranslationDelta,
        "rotation_delta", &ComSkinnedMeshRenderer::GetRotationDelta,
        "set_anim_parameter", &ComSkinnedMeshRenderer::SetAnimParameter,
        "reset_anim_state", &ComSkinnedMeshRenderer::ResetAnimState,
        "current_anim_state", &ComSkinnedMeshRenderer::GetCurrentAnimState,
        "transit_anim_state", &ComSkinnedMeshRenderer::TransitAnimState,
        "play_starttime", &ComSkinnedMeshRenderer::GetPlayStartTime,
        "update_animation", &ComSkinnedMeshRenderer::UpdateAnimation);

    _ComSkinnedMeshRenderer["meta_object"] = ComSkinnedMeshRenderer::metaObject;
}

BE_NAMESPACE_END