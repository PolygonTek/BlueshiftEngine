#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Render/Shader.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterShaderAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Shader = module["Shader"];

    _Shader.SetClass<Shader>();

    LuaCpp::Selector _ShaderAsset = module["ShaderAsset"];

    _ShaderAsset.SetClass<ShaderAsset>(module["Asset"]);
    _ShaderAsset.AddClassMembers<ShaderAsset>(
        "shader", &ShaderAsset::GetShader);
}

BE_NAMESPACE_END