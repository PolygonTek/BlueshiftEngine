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
#include "Core/Object.h"
#include "Render/Render.h"
#include "Game/GameWorld.h"
#include "Game/MapRenderSettings.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("MapRenderSettings", MapRenderSettings, Object)
BEGIN_EVENTS(MapRenderSettings)
END_EVENTS

void MapRenderSettings::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("skyboxMaterial", "Skybox Material", Guid, GetSkyboxMaterialGuid, SetSkyboxMaterialGuid, GuidMapper::defaultSkyboxMaterialGuid, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&MaterialAsset::metaObject);
}

MapRenderSettings::MapRenderSettings() {
    gameWorld = nullptr;
    skyboxMaterial = nullptr;
    initialized = false;
}

MapRenderSettings::~MapRenderSettings() {
    Purge();
}

void MapRenderSettings::Purge() {
    if (skyboxMaterial) {
        materialManager.ReleaseMaterial(skyboxMaterial);
        skyboxMaterial = nullptr;
    }
}

void MapRenderSettings::Init() {
    initialized = true;
}

void MapRenderSettings::ChangeSkyboxMaterial(const Guid &materialGuid) {
    if (skyboxMaterial) {
        materialManager.ReleaseMaterial(skyboxMaterial);
    }

    const Str materialPath = resourceGuidMapper.Get(materialGuid);

    skyboxMaterial = materialManager.GetMaterial(materialPath);

    gameWorld->GetRenderWorld()->SetSkyboxMaterial(skyboxMaterial);
}

Guid MapRenderSettings::GetSkyboxMaterialGuid() const {
    if (skyboxMaterial) {
        const Str materialPath = skyboxMaterial->GetHashName();
        return resourceGuidMapper.Get(materialPath);
    }
    return Guid();
}

void MapRenderSettings::SetSkyboxMaterialGuid(const Guid &materialGuid) {
    ChangeSkyboxMaterial(materialGuid);
}

BE_NAMESPACE_END
