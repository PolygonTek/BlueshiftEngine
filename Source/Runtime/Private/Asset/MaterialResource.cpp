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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Material", MaterialResource, Resource)
BEGIN_EVENTS(MaterialResource)
END_EVENTS

void MaterialResource::RegisterProperties() {
}

MaterialResource::MaterialResource() {
    material = nullptr;
}

MaterialResource::~MaterialResource() {
    if (material) {
        materialManager.ReleaseMaterial(material);
    }
}

Material *MaterialResource::GetMaterial() {
    if (material) {
        return material;
    }
    const Str materialPath = resourceGuidMapper.Get(asset->GetGuid());
    material = materialManager.GetMaterial(materialPath);
    return material;
}

void MaterialResource::Rename(const Str &newName) {
    const Str materialPath = resourceGuidMapper.Get(asset->GetGuid());
    Material *existingMaterial = materialManager.FindMaterial(materialPath);
    if (existingMaterial) {
        materialManager.RenameMaterial(existingMaterial, newName);
    }
}

bool MaterialResource::Reload() {
    const Str materialPath = resourceGuidMapper.Get(asset->GetGuid());
    Material *existingMaterial = materialManager.FindMaterial(materialPath);
    if (existingMaterial) {
        existingMaterial->Reload();
        return true;
    }
    return false;
}

bool MaterialResource::Save() {
    const Str materialPath = resourceGuidMapper.Get(asset->GetGuid());
    Material *existingMaterial = materialManager.FindMaterial(materialPath);
    if (existingMaterial) {
        existingMaterial->Write(existingMaterial->GetHashName());
        return true;
    }
    return false;
}

BE_NAMESPACE_END
