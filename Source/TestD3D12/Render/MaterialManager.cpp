// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Material.h"

MaterialManager     materialManager;

void MaterialManager::Init() {
    materialHashMap.Init(1024, 1024, 1024);

    shaderTemplateReloadedHandle = shaderTemplateManager.shaderTemplateReloaded.Add<MaterialManager, &MaterialManager::OnShaderTemplateReloaded>(this);

    expiredMaterialHashMapCursor = 0;

    nextMaterialIndex = 0;

    CreateEngineMaterials();
}

void MaterialManager::Shutdown() {
    DestroyEngineMaterials();

    if (shaderTemplateReloadedHandle.IsValid()) {
        shaderTemplateManager.shaderTemplateReloaded.Remove(shaderTemplateReloadedHandle);
        shaderTemplateReloadedHandle.Clear();
    }

    materialHashMap.Clear();
}

void MaterialManager::UpdateFrame() {
    CleanExpiredMaterials();
}

std::shared_ptr<Material> MaterialManager::AllocMaterial(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("MaterialManager::AllocMaterial: empty material name");
    }
    if (const auto *entry = materialHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("MaterialManager::AllocMaterial: %s material already allocated", hashName);
        }
        materialHashMap.Remove(hashName);
    }

    std::shared_ptr<Material> material = std::make_shared<Material>();
    material->hashName = hashName;
    material->name = hashName;
    material->name.StripPath();
    material->name.StripFileExtension();
    material->index = nextMaterialIndex++;
    materialHashMap.Set(material->hashName, material);

    return material;
}

std::shared_ptr<Material> MaterialManager::FindMaterial(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = materialHashMap.Get(hashName);
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

std::shared_ptr<Material> MaterialManager::GetMaterial(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultUnlitMaterial;
    }

    std::shared_ptr<Material> material = FindMaterial(hashName);
    if (material) {
        return material;
    }

    material = AllocMaterial(hashName);
    bool loaded = material->Load(hashName);
    if (!loaded) {
        materialHashMap.Remove(material->hashName);
        return defaultUnlitMaterial;
    }

    return material;
}

static const char *TextureHintName(Material::TextureHint hint) {
    switch (hint) {
    case Material::TextureHint::Sprite:
        return "Sprite";
    case Material::TextureHint::Overlay:
        return "Overlay";
    default:
        break;
    }
    return "None";
}

std::shared_ptr<Material> MaterialManager::GetSingleTextureMaterial(const char *textureFilename, Material::TextureHint hint) {
    if (!textureFilename || !textureFilename[0]) {
        return defaultUnlitMaterial;
    }

    BE1::Str materialName = "_singleTextureMaterial/";
    materialName += TextureHintName(hint);
    materialName += "/";
    materialName += textureFilename;

    std::shared_ptr<Material> material = FindMaterial(materialName);
    if (material) {
        return material;
    }

    BE1::Str text;
    switch (hint) {
    case Material::TextureHint::None:
        text = BE1::va("{\n"
            "shaderTemplate \"_litShader\"\n"
            "properties {\n"
            "   baseColorTexture \"%s\"\n"
            "}\n"
            "}", textureFilename);
        break;
    case Material::TextureHint::Sprite:
        text = BE1::va("{\n"
            "noShadow\n"
            "renderingMode blend\n"
            "blendType alphaBlend\n"
            "shaderTemplate \"_unlitShader\"\n"
            "properties {\n"
            "   baseColorTexture \"%s\"\n"
            "}\n"
            "}", textureFilename);
        break;
    case Material::TextureHint::Overlay:
        text = BE1::va("{\n"
            "noShadow\n"
            "cull twoSided\n"
            "renderingMode blend\n"
            "blendType alphaBlend\n"
            "shaderTemplate \"_unlitShader\"\n"
            "properties {\n"
            "   baseColorTexture \"%s\"\n"
            "}\n"
            "}", textureFilename);
        break;
    }

    material = AllocMaterial(materialName);
    if (!material->Create(text, "")) {
        BE_WARNLOG("MaterialManager::GetSingleTextureMaterial: failed to create material '%s' for texture '%s' (hint: %s)\n", materialName.c_str(), textureFilename, TextureHintName(hint));
        materialHashMap.Remove(material->hashName);
        return defaultUnlitMaterial;
    }

    return material;
}

void MaterialManager::OnShaderTemplateReloaded(const ShaderTemplate *shaderTemplate, const BE1::Array<ShaderPropertyInfo> &oldProperties) {
    if (!shaderTemplate) {
        return;
    }

    const int numMaterials = materialHashMap.Count();
    for (int i = 0; i < numMaterials; i++) {
        const auto *entry = materialHashMap.GetByIndex(i);
        assert(entry);

        std::shared_ptr<Material> material = entry->second.lock();
        if (!material) {
            continue;
        }

        if (material->internalData.shaderTemplate.get() == shaderTemplate) {
            material->RemapShaderTemplateProperties(oldProperties);
        }
    }
}

void MaterialManager::CleanExpiredMaterials() {
    constexpr int ExpiredMaterialHashMapScanBudget = 32;
    int scansLeft = ExpiredMaterialHashMapScanBudget;

    while (scansLeft-- > 0 && materialHashMap.Count() > 0) {
        if (expiredMaterialHashMapCursor >= materialHashMap.Count()) {
            expiredMaterialHashMapCursor = 0;
        }

        const auto *entry = materialHashMap.GetByIndex(expiredMaterialHashMapCursor);
        assert(entry);

        if (entry->second.expired()) {
            materialHashMap.Remove(entry->first);
            continue;
        }

        expiredMaterialHashMapCursor++;
    }
}

void MaterialManager::CreateEngineMaterials() {
#define ENGINE_MATERIAL(...) #__VA_ARGS__

    static constexpr const char *defaultLitMaterialText = ENGINE_MATERIAL(
    {
        shaderTemplate "_litShader"

        properties {
            baseColorTexture "_defaultTexture"
        }
    }
    );

    static constexpr const char *defaultUnlitMaterialText = ENGINE_MATERIAL(
        {
            shaderTemplate "_unlitShader"

            properties {
                baseColorTexture "_defaultTexture"
            }
        }
    );

    static constexpr const char *whiteLitMaterialText = ENGINE_MATERIAL(
    {
        shaderTemplate "_litShader"

        properties {
            baseColorTexture "_whiteTexture"
        }
    }
    );

    static constexpr const char *whiteUnlitMaterialText = ENGINE_MATERIAL(
    {
        shaderTemplate "_unlitShader"

        properties {
            baseColorTexture "_whiteTexture"
        }
    }
    );

    static constexpr const char *blendUnlitMaterialText = ENGINE_MATERIAL(
    {
        shaderTemplate "_unlitShader"
        renderingMode "Blend"
        blendType "AlphaBlend"

        properties {
            baseColorTexture "_whiteTexture"
        }
    }
    );

    static constexpr const char *blendLitMaterialText = ENGINE_MATERIAL(
        {
            shaderTemplate "_litShader"
            renderingMode "Blend"
            blendType "AlphaBlend"

            properties {
                baseColorTexture "_whiteTexture"
            }
        }
    );

#undef ENGINE_MATERIAL

    defaultUnlitMaterial = AllocMaterial("_defaultUnlitMaterial");
    if (!defaultUnlitMaterial->Create(defaultUnlitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateEngineMaterials: failed to create defaultUnlit material");
    }

    defaultLitMaterial = AllocMaterial("_defaultLitMaterial");
    if (!defaultLitMaterial->Create(defaultLitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateEngineMaterials: failed to create defaultLit material");
    }

    whiteUnlitMaterial = AllocMaterial("_whiteUnlitMaterial");
    if (!whiteUnlitMaterial->Create(whiteUnlitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateEngineMaterials: failed to create whiteUnlit material");
    }

    whiteLitMaterial = AllocMaterial("_whiteLitMaterial");
    if (!whiteLitMaterial->Create(whiteLitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateEngineMaterials: failed to create whiteLit material");
    }

    blendUnlitMaterial = AllocMaterial("_blendUnlitMaterial");
    if (!blendUnlitMaterial->Create(blendUnlitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateEngineMaterials: failed to create blendUnlit material");
    }

    blendLitMaterial = AllocMaterial("_blendLitMaterial");
    if (!blendLitMaterial->Create(blendLitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateEngineMaterials: failed to create blendLit material");
    }
}

void MaterialManager::DestroyEngineMaterials() {
    defaultUnlitMaterial.reset();
    defaultLitMaterial.reset();
    whiteUnlitMaterial.reset();
    whiteLitMaterial.reset();
    blendUnlitMaterial.reset();
    blendLitMaterial.reset();
}
