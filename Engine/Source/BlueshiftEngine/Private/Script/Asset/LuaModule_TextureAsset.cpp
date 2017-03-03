#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Render/Texture.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterTextureAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Texture = module["Texture"];

    _Texture.SetClass<Texture>();

    LuaCpp::Selector _TextureAsset = module["TextureAsset"];

    _TextureAsset.SetClass<TextureAsset>(module["Asset"]);
    _TextureAsset.AddClassMembers<TextureAsset>(
        "texture", &TextureAsset::GetTexture);
}

BE_NAMESPACE_END