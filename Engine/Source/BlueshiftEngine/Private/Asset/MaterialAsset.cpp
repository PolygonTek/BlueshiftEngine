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
#include "Render/Material.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Material", MaterialAsset, Asset)
BEGIN_EVENTS(MaterialAsset)
END_EVENTS

void MaterialAsset::RegisterProperties() {
}

MaterialAsset::MaterialAsset() {
    material = nullptr;
}

MaterialAsset::~MaterialAsset() {
    if (!material) {
        const Str materialPath = resourceGuidMapper.Get(GetGuid());
        material = materialManager.FindMaterial(materialPath);
    }

    if (material) {
        materialManager.ReleaseMaterial(material, true);
    }
}

Material *MaterialAsset::GetMaterial() {
    if (material) {
        return material;
    }
    const Str materialPath = resourceGuidMapper.Get(GetGuid());
    material = materialManager.GetMaterial(materialPath);
    return material;
}

void MaterialAsset::Rename(const Str &newName) {
    Material *existingMaterial = materialManager.FindMaterial(GetResourceFilename());
    if (existingMaterial) {
        materialManager.RenameMaterial(existingMaterial, newName);
    }

    Asset::Rename(newName);
}

void MaterialAsset::Reload() {
    Material *existingMaterial = materialManager.FindMaterial(GetResourceFilename());
    if (existingMaterial) {
        existingMaterial->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

void MaterialAsset::Save() {
    Material *existingMaterial = materialManager.FindMaterial(GetResourceFilename());
    if (existingMaterial) {
        existingMaterial->Write(existingMaterial->GetHashName());
        EmitSignal(&SIG_Modified, 0);
    }
}

BE_NAMESPACE_END
