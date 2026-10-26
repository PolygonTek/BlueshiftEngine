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
#include "MaterialManager.h"
#include "Render/RenderCvars.h"
#include "Render/RenderSystem.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"

MaterialManager     materialManager;

void MaterialManager::Init() {
    materialHashMap.Init(1024, 1024, 1024);

    shaderTemplateReloadedHandle = ShaderTemplate::reloaded.Add<MaterialManager, &MaterialManager::OnShaderTemplateReloaded>(this);

    expiredMaterialHashMapCursor = 0;

    nextMaterialIndex = 0;

    CreateDefaultMaterials();
}

void MaterialManager::Shutdown() {
    DestroyDefaultMaterials();

    if (shaderTemplateReloadedHandle.IsValid()) {
        ShaderTemplate::reloaded.Remove(shaderTemplateReloadedHandle);
        shaderTemplateReloadedHandle.Clear();
    }

    materialHashMap.Clear();
    streamingMaterials.Clear();
}

void MaterialManager::UpdateFrame() {
    CleanExpiredMaterials();

    // Material sampler 갱신 처리
    const bool textureFilterModified = r_textureFilter.IsModified();
    const bool textureMaxAnisotropyModified = r_textureMaxAnisotropy.IsModified();
    if (!textureFilterModified && !textureMaxAnisotropyModified) {
        return;
    }

    r_textureFilter.ClearModified();
    r_textureMaxAnisotropy.ClearModified();

    const RHI::TextureFilter textureFilter = static_cast<RHI::TextureFilter>(r_textureFilter.GetInteger());
    if (!textureFilterModified && textureFilter != RHI::TextureFilter::Anisotropic) {
        return;
    }

    // Material sampler 갱신 시 기존 sampler 를 해제하므로 GPU 가 사용을 완료할 때까지 기다린다.
    renderSystem->WaitAllRenderCompleted();
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);

    const int numMaterials = materialHashMap.Count();
    for (int i = 0; i < numMaterials; i++) {
        const auto *entry = materialHashMap.GetByIndex(i);
        assert(entry);

        SharedPtr<Material> material = entry->second.lock();
        if (material) {
            material->UpdateSampler();
        }
    }
}

void MaterialManager::ApplyStreamingFeedback(const uint32_t *feedbackData, uint32_t feedbackCapacity) {
    const uint32_t numStreamingMaterials = BE1::Min(static_cast<uint32_t>(streamingMaterials.Count()), feedbackCapacity);
    for (uint32_t i = 0; i < numStreamingMaterials; ++i) {
        const uint32_t feedbackMask = feedbackData[i];
        if (!feedbackMask) {
            continue;
        }

        SharedPtr<Material> material = streamingMaterials[i].lock();
        if (!material) {
            continue;
        }
        assert(material->GetStreamingListIndex() == i);

        // UV 미분값 기준으로 계산된 가장 큰 요청 해상도
        uint32_t feedbackMaxDimension = 1u << FloorLog2(feedbackMask);

        material->RequestStreamingMaxDimension(feedbackMaxDimension);
    }
}

SharedPtr<Material> MaterialManager::AllocMaterial(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("MaterialManager::AllocMaterial: empty material name");
    }

    if (const auto *entry = materialHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("MaterialManager::AllocMaterial: %s material already allocated", hashName);
        }
        materialHashMap.RemoveFast(hashName);
    }

    SharedPtr<Material> material = MakeShared<Material>();
    material->hashName = hashName;
    material->name = hashName;
    material->name.StripPath();
    material->name.StripFileExtension();
    material->index = nextMaterialIndex++;
    materialHashMap.Set(material->hashName, material);

    return material;
}

SharedPtr<Material> MaterialManager::FindMaterial(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = materialHashMap.Get(hashName);
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

SharedPtr<Material> MaterialManager::GetMaterial(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultUnlitMaterial;
    }

    SharedPtr<Material> material = FindMaterial(hashName);
    if (material) {
        return material;
    }

    material = AllocMaterial(hashName);
    bool loaded = material->Load(hashName);
    if (!loaded) {
        materialHashMap.RemoveFast(material->hashName);
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

SharedPtr<Material> MaterialManager::GetSingleTextureMaterial(const char *textureFilename, Material::TextureHint hint) {
    if (!textureFilename || !textureFilename[0]) {
        return defaultUnlitMaterial;
    }

    BE1::Str materialName = "_singleTextureMaterial/";
    materialName += TextureHintName(hint);
    materialName += "/";
    materialName += textureFilename;

    SharedPtr<Material> material = FindMaterial(materialName);
    if (material) {
        return material;
    }

    BE1::Str text;
    switch (hint) {
    case Material::TextureHint::None:
        text = BE1::va("{\n"
            "shaderTemplate \"Source/TestD3D12/Shaders/Lit\"\n"
            "properties {\n"
            "   baseColorMap \"%s\"\n"
            "}\n"
            "}", textureFilename);
        break;
    case Material::TextureHint::Sprite:
        text = BE1::va("{\n"
            "noShadow\n"
            "renderingMode blend\n"
            "blendType alphaBlend\n"
            "shaderTemplate \"Source/TestD3D12/Shaders/Unlit\"\n"
            "properties {\n"
            "   baseColorMap \"%s\"\n"
            "}\n"
            "}", textureFilename);
        break;
    case Material::TextureHint::Overlay:
        text = BE1::va("{\n"
            "noShadow\n"
            "cull twoSided\n"
            "renderingMode blend\n"
            "blendType alphaBlend\n"
            "shaderTemplate \"Source/TestD3D12/Shaders/Unlit\"\n"
            "properties {\n"
            "   baseColorMap \"%s\"\n"
            "}\n"
            "}", textureFilename);
        break;
    }

    material = AllocMaterial(materialName);
    if (!material->Create(text, "")) {
        BE_WARNLOG("MaterialManager::GetSingleTextureMaterial: failed to create material '%s' for texture '%s' (hint: %s)\n", materialName.c_str(), textureFilename, TextureHintName(hint));
        materialHashMap.RemoveFast(material->hashName);
        return defaultUnlitMaterial;
    }

    return material;
}

void MaterialManager::OnShaderTemplateReloaded(const ShaderTemplate *shaderTemplate, const BE1::Array<ShaderPropertyDesc> &oldProperties) {
    if (!shaderTemplate) {
        return;
    }

    const int numMaterials = materialHashMap.Count();
    for (int i = 0; i < numMaterials; i++) {
        const auto *entry = materialHashMap.GetByIndex(i);
        assert(entry);

        SharedPtr<Material> material = entry->second.lock();
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
            materialHashMap.RemoveFast(entry->first);
            continue;
        }

        expiredMaterialHashMapCursor++;
    }
}

void MaterialManager::RefreshStreamingMaterial(Material *material) {
    if (!material) {
        return;
    }

    if (material->HasStreamingTextureProperties()) {
        RegisterStreamingMaterial(material);
    } else {
        UnregisterStreamingMaterial(material);
    }
}

void MaterialManager::RegisterStreamingMaterial(Material *material) {
    if (!material) {
        return;
    }

    if (material->streamingListIndex != TextureStreamingInvalidIndex) {
        return;
    }

    material->streamingListIndex = streamingMaterials.Append(material->weak_from_this());
}

void MaterialManager::UnregisterStreamingMaterial(Material *material) {
    if (!material) {
        return;
    }

    if (material->streamingListIndex == TextureStreamingInvalidIndex) {
        return;
    }

    if (streamingMaterials.IsValidIndex(material->streamingListIndex)) {
        streamingMaterials[material->streamingListIndex].reset();
    }

    material->streamingListIndex = TextureStreamingInvalidIndex;
}

void MaterialManager::CreateDefaultMaterials() {
#define DEFAULT_MATERIAL(...) #__VA_ARGS__

    static constexpr const char *defaultLitMaterialText = DEFAULT_MATERIAL(
    {
        shaderTemplate "Source/TestD3D12/Shaders/Lit"

        properties {
            baseColorMap "_missingTexture"
            metallicFactor "0.0"
        }
    }
    );

    static constexpr const char *defaultUnlitMaterialText = DEFAULT_MATERIAL(
    {
        shaderTemplate "Source/TestD3D12/Shaders/Unlit"

        properties {
            baseColorMap "_missingTexture"
        }
    }
    );

    static constexpr const char *whiteLitMaterialText = DEFAULT_MATERIAL(
    {
        shaderTemplate "Source/TestD3D12/Shaders/Lit"

        properties {
            baseColorMap "_whiteTexture"
            metallicFactor "0.0"
        }
    }
    );

    static constexpr const char *whiteUnlitMaterialText = DEFAULT_MATERIAL(
    {
        shaderTemplate "Source/TestD3D12/Shaders/Unlit"

        properties {
            baseColorMap "_whiteTexture"
        }
    }
    );

    static constexpr const char *blendUnlitMaterialText = DEFAULT_MATERIAL(
    {
        shaderTemplate "Source/TestD3D12/Shaders/Unlit"
        renderingMode "Blend"
        blendType "AlphaBlend"

        properties {
            baseColorMap "_whiteTexture"
        }
    }
    );

    static constexpr const char *blendLitMaterialText = DEFAULT_MATERIAL(
    {
        shaderTemplate "Source/TestD3D12/Shaders/Lit"
        renderingMode "Blend"
        blendType "AlphaBlend"

        properties {
            baseColorMap "_whiteTexture"
            metallicFactor "0.0"
        }
    }
    );

    static constexpr const char *defaultSkyboxMaterialText = DEFAULT_MATERIAL(
    {
        cull twoSided
        shaderTemplate "Source/TestD3D12/Shaders/Skybox/SkyboxCubemap"

        properties {
            skyCubeMap "Data/EngineTextures/default_sky.dds"
        }
    }
    );

#undef DEFAULT_MATERIAL

    defaultUnlitMaterial = AllocMaterial("_defaultUnlitMaterial");
    if (!defaultUnlitMaterial->Create(defaultUnlitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateDefaultMaterials: failed to create defaultUnlit material");
    }

    defaultLitMaterial = AllocMaterial("_defaultLitMaterial");
    if (!defaultLitMaterial->Create(defaultLitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateDefaultMaterials: failed to create defaultLit material");
    }

    whiteUnlitMaterial = AllocMaterial("_whiteUnlitMaterial");
    if (!whiteUnlitMaterial->Create(whiteUnlitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateDefaultMaterials: failed to create whiteUnlit material");
    }

    whiteLitMaterial = AllocMaterial("_whiteLitMaterial");
    if (!whiteLitMaterial->Create(whiteLitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateDefaultMaterials: failed to create whiteLit material");
    }

    blendUnlitMaterial = AllocMaterial("_blendUnlitMaterial");
    if (!blendUnlitMaterial->Create(blendUnlitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateDefaultMaterials: failed to create blendUnlit material");
    }

    blendLitMaterial = AllocMaterial("_blendLitMaterial");
    if (!blendLitMaterial->Create(blendLitMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateDefaultMaterials: failed to create blendLit material");
    }

    defaultSkyboxMaterial = AllocMaterial("_defaultSkyboxMaterial");
    if (!defaultSkyboxMaterial->Create(defaultSkyboxMaterialText, nullptr)) {
        BE_FATALERROR("MaterialManager::CreateDefaultMaterials: failed to create defaultSkybox material");
    }
}

void MaterialManager::DestroyDefaultMaterials() {
    defaultUnlitMaterial.reset();
    defaultLitMaterial.reset();
    whiteUnlitMaterial.reset();
    whiteLitMaterial.reset();
    blendUnlitMaterial.reset();
    blendLitMaterial.reset();
    defaultSkyboxMaterial.reset();
}
