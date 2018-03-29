// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
