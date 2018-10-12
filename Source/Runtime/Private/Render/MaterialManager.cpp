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
#include "Asset/GuidMapper.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

Material *          MaterialManager::defaultMaterial;
Material *          MaterialManager::whiteMaterial;
Material *          MaterialManager::blendMaterial;
Material *          MaterialManager::whiteLightMaterial;
Material *          MaterialManager::zeroClampLightMaterial;
Material *          MaterialManager::defaultSkyboxMaterial;

MaterialManager     materialManager;

static int          materialCounter = 0;
static const int    MaxMaterialCount = 65536;

void MaterialManager::Init() {
    cmdSystem.AddCommand(L"listMaterials", Cmd_ListMaterials);
    cmdSystem.AddCommand(L"reloadMaterial", Cmd_ReloadMaterial);

    materialHashMap.Init(1024, 65536, 1024);

    CreateEngineMaterials();
}

void MaterialManager::CreateEngineMaterials() {
    // Create default lit surface material
    defaultMaterial = AllocMaterial("_defaultMaterial");
    defaultMaterial->Create(va(
        "pass {\n"
        "   shader \"%s\" {\n"
        "       _ALBEDO \"1\"\n"
        "       albedoMap \"%s\"\n"
        "   }\n"
        "}", GuidMapper::standardShaderGuid.ToString(), GuidMapper::defaultTextureGuid.ToString()));
    defaultMaterial->permanence = true;

    // Create white lit surface material
    whiteMaterial = AllocMaterial("_whiteMaterial");
    whiteMaterial->Create(va(
        "pass {\n"
        "   useOwnerColor\n"
        "   shader \"%s\" {\n"
        "       _ALBEDO \"1\"\n"
        "       albedoMap \"%s\"\n"
        "   }\n"
        "}", GuidMapper::standardShaderGuid.ToString(), GuidMapper::whiteTextureGuid.ToString()));
    whiteMaterial->permanence = true;

    // Create blend unlit surface material
    blendMaterial = AllocMaterial("_blendMaterial");
    blendMaterial->Create(va(
        "pass {\n"
        "   renderingMode alphaBlend\n"
        "   useOwnerColor\n"
        "   blendFunc SRC_ALPHA ONE_MINUS_SRC_ALPHA\n"
        "   shader \"%s\" {\n"
        "       albedoMap \"%s\"\n"
        "   }\n"
        "}", GuidMapper::simpleShaderGuid.ToString(), GuidMapper::whiteTextureGuid.ToString()));
    blendMaterial->permanence = true;

    // Create white light material
    whiteLightMaterial = AllocMaterial("_whiteLightMaterial");
    whiteLightMaterial->Create(va(
        "light\n"
        "pass {\n"
        "   useOwnerColor\n"
        "   map \"%s\"\n"
        "}", GuidMapper::whiteTextureGuid.ToString()));
    whiteLightMaterial->permanence = true;

    // Create zero clamped white light material
    zeroClampLightMaterial = AllocMaterial("_zeroClampLightMaterial");
    zeroClampLightMaterial->Create(va(
        "light\n"
        "pass {\n"
        "   useOwnerColor\n"
        "   map \"%s\"\n"
        "}", GuidMapper::zeroClampTextureGuid.ToString()));
    zeroClampLightMaterial->permanence = true;

    // Create default skybox material
    defaultSkyboxMaterial = AllocMaterial("_defaultSkyboxMaterial");
    defaultSkyboxMaterial->Create(va(
        "pass {\n"
        "   cull twoSided\n"
        "   shader \"%s\" {\n"
        "       skyCubeMap \"%s\"\n"
        "   }\n"
        "}", GuidMapper::skyboxCubemapShaderGuid.ToString(), GuidMapper::defaultCubeTextureGuid.ToString()));
    defaultSkyboxMaterial->permanence = true;
}

void MaterialManager::Shutdown() {
    cmdSystem.RemoveCommand(L"listMaterials");
    cmdSystem.RemoveCommand(L"reloadMaterial");
    
    for (int i = 0; i < materialHashMap.Count(); i++) {
        const auto *entry = materialManager.materialHashMap.GetByIndex(i);
        Material *material = entry->second;
        material->Purge();
    }
    
    materialHashMap.DeleteContents(true);
}

void MaterialManager::DestroyUnusedMaterials() {
    Array<Material *> removeArray;

    for (int i = 0; i < materialHashMap.Count(); i++) {
        const auto *entry = materialHashMap.GetByIndex(i);
        Material *material = entry->second;

        if (material && !material->permanence && material->refCount == 0) {
            removeArray.Append(material);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyMaterial(removeArray[i]);
    }
}

Material *MaterialManager::AllocMaterial(const char *hashName) {
    if (materialHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs material already allocated", hashName);
    }

    if (materialHashMap.Count() == MaterialManager::MaxCount) {
        BE_FATALERROR(L"Material exceed maximum limits %i", MaterialManager::MaxCount);
    }
    
    Material *material = new Material;
    material->hashName = hashName;
    material->name = hashName;
    material->name.StripPath();
    material->name.StripFileExtension();
    material->refCount = 1;
    material->index = materialCounter++ % MaterialManager::MaxCount;
    materialHashMap.Set(material->hashName, material);

    return material;
}

void MaterialManager::DestroyMaterial(Material *material) {
    if (material->refCount > 1) {
        BE_LOG(L"MaterialManager::DestroyMaterial: material '%hs' has %i reference count\n", material->hashName.c_str(), material->refCount);
    }

    materialHashMap.Remove(material->hashName);

    delete material;
}

Material *MaterialManager::FindMaterial(const char *hashName) const {
    const auto *entry = materialHashMap.Get(Str(hashName));
    if (entry) {
        return entry->second;
    }
    
    return nullptr;
}

Material *MaterialManager::GetMaterial(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultMaterial;
    }

    Material *material = FindMaterial(hashName);
    if (material) {
        material->refCount++;
        return material;
    }

    material = AllocMaterial(hashName);
    if (!material->Load(hashName)) {
        DestroyMaterial(material);
        return defaultMaterial;
    }

    return material;
}

void MaterialManager::PrecacheMaterial(const char *filename) {
    Material *material = GetMaterial(filename);
    ReleaseMaterial(material);
}

static const char *HintName(Material::TextureHint hint) {
    switch (hint) {
    case Material::LightHint:
        return "LightHint";
    case Material::VertexColorHint:
        return "VertexColorHint";
    case Material::LightmappedHint:
        return "LightmappedHint";
    case Material::SpriteHint:
        return "SpriteHint";
    case Material::OverlayHint:
        return "OverlayHint";
    default:
        break;
    }

    return "NoHint";
}

Material *MaterialManager::GetSingleTextureMaterial(const Texture *texture, Material::TextureHint hint) {
    if (!texture) {
        return defaultMaterial;
    }

    Str materialName = texture->GetHashName();
    materialName += Str("_") + HintName(hint);

    Material *material = FindMaterial(materialName.c_str());
    if (material) {
        material->refCount++;
        return material;
    }

    char buffer[1024];
    switch (hint) {
    case Material::SpriteHint:
        Str::snPrintf(buffer, sizeof(buffer),
            "noShadow\n"
            "pass {\n"
            "   renderingMode alphaBlend\n"
            "   blendFunc blend\n"
            "   mapPath \"%hs\"\n"
            "}\n", texture->GetHashName());
        break;
    case Material::LightHint:
        Str::snPrintf(buffer, sizeof(buffer),
            "light\n"
            "pass {\n"
            "   mapPath \"%hs\"\n"
            "   useOwnerColor\n"
            "}\n", texture->GetHashName());
        break;
    case Material::OverlayHint:
        Str::snPrintf(buffer, sizeof(buffer),
            "noShadow\n"
            "pass {\n"
            "   cull twoSided\n"
            "   renderingMode alphaBlend\n"
            "   blendFunc blend\n"
            "   vertexColor\n"
            "   mapPath \"%hs\"\n"
            "}\n", texture->GetHashName());
        break;
    default:
        Str::snPrintf(buffer, sizeof(buffer),
            "pass {\n"
            "   mapPath \"%hs\"\n"
            "}\n", texture->GetHashName());
        break;
    }

    material = AllocMaterial(materialName);
    if (!material->Create(buffer)) {
        DestroyMaterial(material);
        return defaultMaterial;
    }

    return material;
}

void MaterialManager::RenameMaterial(Material *material, const Str &newName) {
    const auto *entry = materialHashMap.Get(material->hashName);
    if (entry) {
        materialHashMap.Remove(material->hashName);

        material->hashName = newName;
        material->name = newName;
        material->name.StripPath();
        material->name.StripFileExtension();

        materialHashMap.Set(newName, material);
    }
}

void MaterialManager::ReleaseMaterial(Material *material, bool immediateDestroy) {
    if (material->permanence) {
        return;
    }

    if (material->refCount > 0) {
        material->refCount--;
    }

    if (immediateDestroy && material->refCount == 0) {
        DestroyMaterial(material);
    }
}

//--------------------------------------------------------------------------------------------------

void MaterialManager::Cmd_ListMaterials(const CmdArgs &args) {
    int count = 0;
    
    BE_LOG(L"NUM. REF. NAME\n");

    for (int i = 0; i < materialManager.materialHashMap.Count(); i++) {
        const auto *entry = materialManager.materialHashMap.GetByIndex(i);
        Material *material = entry->second;
 
        BE_LOG(L"%4d %4d %hs\n",
            i,
            material->refCount,
            material->hashName.c_str());

        count++;
    }

    BE_LOG(L"total %i materials\n", count);
}

void MaterialManager::Cmd_ReloadMaterial(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"reloadMaterial <filename>\n");
        return;
    }

    if (!WStr::Icmp(args.Argv(1), L"all")) {
        int count = materialManager.materialHashMap.Count();

        for (int i = 0; i < count; i++) {
            const auto *entry = materialManager.materialHashMap.GetByIndex(i);
            Material *material = entry->second;
            if (!material) {
                continue;
            }

            material->Reload();
        }
    } else {
        Material *material = materialManager.FindMaterial(WStr::ToStr(args.Argv(1)));
        if (!material) {
            BE_WARNLOG(L"Couldn't find material to reload \"%ls\"\n", args.Argv(1));
            return;
        }

        material->Reload();
    }
}

BE_NAMESPACE_END
