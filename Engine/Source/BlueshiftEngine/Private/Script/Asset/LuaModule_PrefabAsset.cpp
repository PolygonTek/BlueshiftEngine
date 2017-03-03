#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Game/Prefab.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterPrefabAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Prefab = module["Prefab"];

    _Prefab.SetClass<Prefab>();
    _Prefab.AddClassMembers<Prefab>(
        "root_entity", &Prefab::GetRootEntity);

    LuaCpp::Selector _PrefabAsset = module["PrefabAsset"];

    _PrefabAsset.SetClass<PrefabAsset>(module["Asset"]);
    _PrefabAsset.AddClassMembers<PrefabAsset>(
        "prefab", &PrefabAsset::GetPrefab);
}

BE_NAMESPACE_END