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

        //prefab->Write(prefab->hashName);

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

Prefab *PrefabManager::AllocPrefab(const char *hashName, const Guid guid) {
    if (prefabHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs prefab already allocated", hashName);
    }

    Prefab *prefab = (Prefab *)Prefab::CreateInstance(guid);
    prefab->hashName = hashName;
    prefabHashMap.Set(hashName, prefab);

    return prefab;
}

Prefab *PrefabManager::GetPrefab(const char *filename) {
    Str normalizedFilename = filename;
    normalizedFilename.BackSlashesToSlashes();

    Prefab *prefab = FindPrefab(normalizedFilename);
    if (prefab) {
        return prefab;
    }

    prefab = AllocPrefab(normalizedFilename);
    if (!prefab->Load(normalizedFilename)) {
        prefabHashMap.Remove(prefab->hashName);
        Prefab::DestroyInstanceImmediate(prefab);
        return nullptr;
    }

    return prefab;
}

Json::Value PrefabManager::CreatePrefabValue(const Entity *originalEntity) {
    assert(!originalEntity->IsPrefabParent());

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

        prefabEntityValue["isPrefabParent"] = true;
        prefabEntityValue["prefabParent"] = Guid::zero.ToString();

        Entity *prefabEntity = Entity::CreateEntity(prefabEntityValue);
        prefabEntities.Append(prefabEntity);

        // Set the original entity's prefab parent
        originalEntities[i]->props->Set("prefabParent", prefabEntity->GetGuid());
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