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
#include "Game/Prefab.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Prefab", PrefabAsset, Asset)
BEGIN_EVENTS(PrefabAsset)
END_EVENTS

void PrefabAsset::RegisterProperties() {
}

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

void PrefabAsset::Rename(const Str &newName) {
    Prefab *existingPrefab = prefabManager.FindPrefab(GetResourceFilename());
    if (existingPrefab) {
        prefabManager.RenamePrefab(existingPrefab, newName);
    }

    Asset::Rename(newName);
}

void PrefabAsset::Reload() {
    Prefab *existingPrefab = prefabManager.FindPrefab(GetResourceFilename());
    if (existingPrefab) {
        existingPrefab->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

void PrefabAsset::Save() {
    Prefab *existingPrefab = prefabManager.FindPrefab(GetResourceFilename());
    if (existingPrefab) {
        existingPrefab->Write(existingPrefab->GetFilename());
        EmitSignal(&SIG_Modified, 0);
    }
}

BE_NAMESPACE_END
