#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterMapAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _MapAsset = module["MapAsset"];

    _MapAsset.SetClass<MapAsset>(module["Asset"]);
}

BE_NAMESPACE_END