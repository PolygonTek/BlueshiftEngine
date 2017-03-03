#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "AnimController/AnimController.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterAnimControllerAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _AnimController = module["AnimController"];

    _AnimController.SetClass<AnimController>();

    LuaCpp::Selector _AnimControllerAsset = module["AnimControllerAsset"];

    _AnimControllerAsset.SetClass<AnimControllerAsset>(module["Asset"]);
    _AnimControllerAsset.AddClassMembers<AnimControllerAsset>(
        "anim_controller", &AnimControllerAsset::GetAnimController);
}

BE_NAMESPACE_END