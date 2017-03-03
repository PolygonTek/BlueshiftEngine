#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Render/Anim.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterAnimAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Anim = module["Anim"];

    _Anim.SetClass<Anim>();

    LuaCpp::Selector _AnimAsset = module["AnimAsset"];

    _AnimAsset.SetClass<AnimAsset>(module["Asset"]);
    _AnimAsset.AddClassMembers<AnimAsset>(
        "anim", &AnimAsset::GetAnim);
}

BE_NAMESPACE_END