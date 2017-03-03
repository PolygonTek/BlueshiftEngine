#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Asset = module["Asset"];

    _Asset.SetClass<Asset>(module["Object"]);
    _Asset.AddClassMembers<Asset>(
        "to_string", &Asset::ToString);
}

BE_NAMESPACE_END