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

    expiredMaterialHashMapCursor = 0;

    nextMaterialIndex = 0;
}

void MaterialManager::Shutdown() {
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
        return defaultMaterial;
    }

    std::shared_ptr<Material> material = FindMaterial(hashName);
    if (material) {
        return material;
    }

    material = AllocMaterial(hashName);
    bool loaded = material->Load(hashName);
    if (!loaded) {
        materialHashMap.Remove(material->hashName);
        return defaultMaterial;
    }

    return material;
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
