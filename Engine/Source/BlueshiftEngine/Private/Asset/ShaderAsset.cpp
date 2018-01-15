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
#include "Render/Shader.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Shader", ShaderAsset, Asset)
BEGIN_EVENTS(ShaderAsset)
END_EVENTS

void ShaderAsset::RegisterProperties() {
}

ShaderAsset::ShaderAsset() {
    shader = nullptr;
}

ShaderAsset::~ShaderAsset() {
    if (!shader) {
        const Str shaderPath = resourceGuidMapper.Get(GetGuid());
        shader = shaderManager.FindShader(shaderPath);
    }

    if (shader) {
        shaderManager.ReleaseShader(shader, true);
    }
}

Shader *ShaderAsset::GetShader() {
    if (shader) {
        return shader;
    }
    const Str shaderPath = resourceGuidMapper.Get(GetGuid());
    shader = shaderManager.GetShader(shaderPath);
    return shader;
}

void ShaderAsset::Rename(const Str &newName) {
    Shader *existingShader = shaderManager.FindShader(GetResourceFilename());
    if (existingShader) {
        shaderManager.RenameShader(existingShader, newName);
    }

    Asset::Rename(newName);
}

void ShaderAsset::Reload() {
    Shader *existingShader = shaderManager.FindShader(GetResourceFilename());
    if (existingShader) {
        existingShader->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END
