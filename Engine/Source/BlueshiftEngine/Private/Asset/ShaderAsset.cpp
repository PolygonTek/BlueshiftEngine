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
BEGIN_PROPERTIES(ShaderAsset)
END_PROPERTIES

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

void ShaderAsset::Reload() {
    Shader *existingShader = shaderManager.FindShader(GetResourceFilename());
    if (existingShader) {
        existingShader->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END