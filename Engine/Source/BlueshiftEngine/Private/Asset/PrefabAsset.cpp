#include "Precompiled.h"
#include "Game/Prefab.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Prefab", PrefabAsset, Asset)
BEGIN_EVENTS(PrefabAsset)
END_EVENTS
BEGIN_PROPERTIES(PrefabAsset)
END_PROPERTIES

PrefabAsset::PrefabAsset() {
    prefab = nullptr;
}

PrefabAsset::~PrefabAsset() {
    if (prefab) {
        //prefabManager.ReleasePrefab(prefab);
    }
}

Prefab *PrefabAsset::GetPrefab() {
    if (prefab) {
        return prefab;
    }
    const Str prefabPath = resourceGuidMapper.Get(GetGuid());
    prefab = prefabManager.GetPrefab(prefabPath);
    return prefab;
}

void PrefabAsset::Reload() {
    Prefab *existingPrefab = prefabManager.FindPrefab(GetAssetFilename());
    if (existingPrefab) {
        existingPrefab->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

void PrefabAsset::Save() {
    Prefab *existingPrefab = prefabManager.FindPrefab(GetAssetFilename());
    if (existingPrefab) {
        existingPrefab->Write(existingPrefab->GetFilename());
        EmitSignal(&SIG_Modified, 0);
    }
}

BE_NAMESPACE_END