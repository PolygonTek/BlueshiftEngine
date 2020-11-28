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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Prefab", PrefabResource, Resource)
BEGIN_EVENTS(PrefabResource)
END_EVENTS

void PrefabResource::RegisterProperties() {
}

PrefabResource::PrefabResource() {
    prefab = nullptr;
}

PrefabResource::~PrefabResource() {
    if (prefab) {
        //prefabManager.ReleasePrefab(prefab);
    }
}

Prefab *PrefabResource::GetPrefab() {
    if (prefab) {
        return prefab;
    }
    const Str prefabPath = resourceGuidMapper.Get(asset->GetGuid());
    prefab = prefabManager.GetPrefab(prefabPath);
    return prefab;
}

void PrefabResource::Rename(const Str &newName) {
    const Str prefabPath = resourceGuidMapper.Get(asset->GetGuid());
    Prefab *existingPrefab = prefabManager.FindPrefab(prefabPath);
    if (existingPrefab) {
        prefabManager.RenamePrefab(existingPrefab, newName);
    }
}

bool PrefabResource::Reload() {
    const Str prefabPath = resourceGuidMapper.Get(asset->GetGuid());
    Prefab *existingPrefab = prefabManager.FindPrefab(prefabPath);
    if (existingPrefab) {
        existingPrefab->Reload();
        return true;
    }
    return false;
}

bool PrefabResource::Save() {
    const Str prefabPath = resourceGuidMapper.Get(asset->GetGuid());
    Prefab *existingPrefab = prefabManager.FindPrefab(prefabPath);
    if (existingPrefab) {
        existingPrefab->Write(existingPrefab->GetFileName());
        return true;
    }
    return false;
}

BE_NAMESPACE_END
