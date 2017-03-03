#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Render/Render.h"
#include "Components/ComCamera.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterCameraComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComCamera = module["ComCamera"];

    _ComCamera.SetClass<ComCamera>();
    _ComCamera.AddClassMembers<ComCamera>(
        "world_to_screen", &ComCamera::WorldToScreen,
        "screen_to_ray", &ComCamera::ScreenToRay);

    _ComCamera["meta_object"] = ComCamera::metaObject;
}

BE_NAMESPACE_END