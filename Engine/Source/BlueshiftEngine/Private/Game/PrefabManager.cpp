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
#include "Game/Entity.h"
#include "Game/Prefab.h"
#include "Game/GameWorld.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

PrefabManager   prefabManager;

void PrefabManager::Init() {
    prefabHashMap.Init(1024, 64, 64);
}

void PrefabManager::Shutdown() {
    for (int i = 0; i < prefabHashMap.Count(); i++) {
        const auto *entry = prefabHashMap.GetByIndex(i);
        Prefab *prefab = entry->second;

        Prefab::DestroyInstanceImmediate(prefab);
    }

    prefabHashMap.Clear();
}

Prefab *PrefabManager::FindPrefab(const char *hashName) const {
    const auto *entry = prefabHashMap.Get(hashName);
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

Prefab *PrefabManager::AllocPrefab(const char *hashName, const Guid &guid) {
    if (prefabHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs prefab already allocated", hashName);
    }

    Prefab *prefab = (Prefab *)Prefab::CreateInstance(guid);
    prefab->hashName = hashName;
    prefab->name = hashName;
    prefab->name.StripPath();
    prefab->name.StripFileExtension();
    prefabHashMap.Set(hashName, prefab);

    return prefab;
}

Prefab *PrefabManager::GetPrefab(const char *hashName) {
    Prefab *prefab = FindPrefab(hashName);
    if (prefab) {
        return prefab;
    }

    prefab = AllocPrefab(hashName);
    if (!prefab->Load(hashName)) {
        prefabHashMap.Remove(prefab->hashName);
        Prefab::DestroyInstanceImmediate(prefab);
        return nullptr;
    }

    return prefab;
}

void PrefabManager::RenamePrefab(Prefab *prefab, const Str &newName) {
    const auto *entry = prefabHashMap.Get(prefab->hashName);
    if (entry) {
        prefabHashMap.Remove(prefab->hashName);

        prefab->hashName = newName;
        prefab->name = newName;
        prefab->name.StripPath();
        prefab->name.StripFileExtension();

        prefabHashMap.Set(newName, prefab);
    }
}

Json::Value PrefabManager::CreatePrefabValue(const Entity *originalEntity) {
    assert(!originalEntity->IsPrefabSource());

    // Get the original entity and all of it's children
    EntityPtrArray originalEntities;
    originalEntities.Append(const_cast<Entity *>(originalEntity));
    originalEntity->GetChildren(originalEntities);

    EntityPtrArray prefabEntities;
    HashTable<Guid, Guid> guidMap;

    int numEntities = originalEntities.Count();

    for (int i = 0; i < numEntities; i++) {
        Json::Value value;
        originalEntities[i]->Serialize(value);

        Json::Value prefabEntityValue = Entity::CloneEntityValue(value, guidMap);

        if (i == 0) {
            // Clear parent of this prefab root entity
            prefabEntityValue["parent"] = Guid::zero.ToString();
        }

        prefabEntityValue["prefab"] = true;
        prefabEntityValue["prefabSource"] = Guid::zero.ToString();

        Entity *prefabEntity = Entity::CreateEntity(prefabEntityValue, nullptr);
        prefabEntity->Init();

        prefabEntities.Append(prefabEntity);

        // Set the original entity's prefab source
        originalEntities[i]->SetProperty("prefabSource", prefabEntity->GetGuid());
    }

    // Remap GUIDs for the cloned entities
    Entity::RemapGuids(prefabEntities, guidMap);

    // Serialize prefab entities
    Json::Value prefabEntitiesValue;
    for (int i = 0; i < numEntities; i++) {
        Json::Value value;
        prefabEntities[i]->Serialize(value);
        prefabEntitiesValue.append(value);

        Entity::DestroyInstanceImmediate(prefabEntities[i]);
    }

    return prefabEntitiesValue;
}

BE_NAMESPACE_END