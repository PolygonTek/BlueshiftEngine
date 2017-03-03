#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Render/Material.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterMaterialAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Material = module["Material"];

    _Material.SetClass<Material>();

    LuaCpp::Selector _MaterialAsset = module["MaterialAsset"];

    _MaterialAsset.SetClass<MaterialAsset>(module["Asset"]);
    _MaterialAsset.AddClassMembers<MaterialAsset>(
        "material", &MaterialAsset::GetMaterial);
}

BE_NAMESPACE_END