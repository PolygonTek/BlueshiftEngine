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
    if (initialized) {
        Shutdown();
    }

    prefabHashMap.Init(1024, 64, 64);

    prefabWorld = (GameWorld *)GameWorld::CreateInstance();
    prefabWorld->GetLuaVM().InitEngineModule(prefabWorld);

    initialized = true;
}

void PrefabManager::Shutdown() {
    if (!initialized) {
        return;
    }

    for (int i = 0; i < prefabHashMap.Count(); i++) {
        const auto *entry = prefabHashMap.GetByIndex(i);
        Prefab *prefab = entry->second;

        Prefab::DestroyInstanceImmediate(prefab);
    }

    if (prefabWorld) {
        GameWorld::DestroyInstanceImmediate(prefabWorld);
        prefabWorld = nullptr;
    }

    prefabHashMap.Clear();

    initialized = false;
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

void PrefabManager::DestroyPrefab(Prefab *prefab) {
    prefabHashMap.Remove(prefab->hashName);
    delete prefab;
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

    // Serialize source entity and it's children
    Json::Value originalEntitiesValue;
    BE1::Entity::SerializeHierarchy(originalEntity, originalEntitiesValue);

    // Clone entities value which is replaced by new GUIDs
    HashTable<Guid, Guid> guidMap;
    Json::Value prefabEntitiesValue = Entity::CloneEntitiesValue(originalEntitiesValue, guidMap);

    EntityPtrArray prefabEntities;

    for (int i = 0; i < originalEntitiesValue.size(); i++) {
        if (i == 0) {
            // Clear parent of this prefab root entity
            prefabEntitiesValue[0]["parent"] = Guid::zero.ToString();
        }

        prefabEntitiesValue[i]["prefab"] = true;
        prefabEntitiesValue[i]["prefabSource"] = Guid::zero.ToString();

        Entity *prefabEntity = Entity::CreateEntity(prefabEntitiesValue[i], prefabManager.GetPrefabWorld());
        prefabEntities.Append(prefabEntity);

        // Remap all GUID references to newly created
        Entity::RemapGuids(prefabEntity, guidMap);

        prefabEntity->Init();
        //prefabEntity->InitComponents();

        // Serialize prefab entity
        Json::Value value;
        prefabEntity->Serialize(value);
        prefabEntitiesValue[i] = value;

        // Set the original entity's prefab source
        originalEntitiesValue[i]["prefabSource"] = prefabEntity->GetGuid().ToString();
    }
    
    for (int i = 0; i < prefabEntities.Count(); i++) {
        Entity::DestroyInstanceImmediate(prefabEntities[i]);
    }

    return prefabEntitiesValue;
}

BE_NAMESPACE_END