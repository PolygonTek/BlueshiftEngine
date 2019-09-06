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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Shader", ShaderResource, Resource)
BEGIN_EVENTS(ShaderResource)
END_EVENTS

void ShaderResource::RegisterProperties() {
}

ShaderResource::ShaderResource() {
    shader = nullptr;
}

ShaderResource::~ShaderResource() {
    if (shader) {
        shaderManager.ReleaseShader(shader);
    }
}

Shader *ShaderResource::GetShader() {
    if (shader) {
        return shader;
    }
    const Str shaderPath = resourceGuidMapper.Get(asset->GetGuid());
    shader = shaderManager.GetShader(shaderPath);
    return shader;
}

void ShaderResource::Rename(const Str &newName) {
    const Str shaderPath = resourceGuidMapper.Get(asset->GetGuid());
    Shader *existingShader = shaderManager.FindShader(shaderPath);
    if (existingShader) {
        shaderManager.RenameShader(existingShader, newName);
    }
}

bool ShaderResource::Reload() {
    const Str shaderPath = resourceGuidMapper.Get(asset->GetGuid());
    Shader *existingShader = shaderManager.FindShader(shaderPath);
    if (existingShader) {
        existingShader->Reload();
        return true;
    }
    return false;
}

bool ShaderResource::Save() {
    return false;
}

BE_NAMESPACE_END
