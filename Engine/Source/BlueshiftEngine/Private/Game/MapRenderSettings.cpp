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
BEGIN_PROPERTIES(MapRenderSettings)
    PROPERTY_OBJECT("skyboxMaterial", "Skybox Material", "", GuidMapper::defaultSkyboxMaterialGuid.ToString(), MaterialAsset::metaObject, PropertyInfo::ReadWrite),
END_PROPERTIES

void MapRenderSettings::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
#endif
}

MapRenderSettings::MapRenderSettings() {
    gameWorld = nullptr;
    skyboxMaterial = nullptr;
    initialized = false;

#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&MapRenderSettings::PropertyChanged);
#endif
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
    Purge();

    const Guid materialGuid = props->Get("skyboxMaterial").As<Guid>();
    const Str materialPath = resourceGuidMapper.Get(materialGuid);

    skyboxMaterial = materialManager.GetMaterial(materialPath);

    gameWorld->GetRenderWorld()->SetSkyboxMaterial(skyboxMaterial);

    initialized = true;
}

void MapRenderSettings::Serialize(Json::Value &value) const {
    props->Serialize(value);

    value["classname"] = MapRenderSettings::metaObject.ClassName();
}

void MapRenderSettings::PropertyChanged(const char *classname, const char *propName) {
    if (!initialized) {
        return;
    }

    if (!Str::Cmp(propName, "skyboxMaterial")) {
        SetSkyboxMaterial(props->Get(propName).As<Guid>());
        return;
    }
}

Guid MapRenderSettings::GetSkyboxMaterial() const {
    const Str materialPath = skyboxMaterial->GetHashName();
    return resourceGuidMapper.Get(materialPath);
}

void MapRenderSettings::SetSkyboxMaterial(const Guid &materialGuid) {
    materialManager.ReleaseMaterial(skyboxMaterial);

    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    skyboxMaterial = materialManager.GetMaterial(materialPath);

    gameWorld->GetRenderWorld()->SetSkyboxMaterial(skyboxMaterial);
}

BE_NAMESPACE_END
