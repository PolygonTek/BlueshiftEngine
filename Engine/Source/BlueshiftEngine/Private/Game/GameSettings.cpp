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
#include "Game/GameWorld.h"
#include "Game/GameSettings.h"
#include "Game/TagLayerSettings.h"
#include "Game/PhysicsSettings.h"
#include "Game/PlayerSettings.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

TagLayerSettings *  GameSettings::tagLayerSettings = nullptr;
PhysicsSettings *   GameSettings::physicsSettings = nullptr;
PlayerSettings *    GameSettings::playerSettings = nullptr;

void GameSettings::Init() {
}

void GameSettings::Shutdown() {
    if (tagLayerSettings) {
        TagLayerSettings::DestroyInstanceImmediate(tagLayerSettings);
        tagLayerSettings = nullptr;
    }

    if (physicsSettings) {
        PhysicsSettings::DestroyInstanceImmediate(physicsSettings);
        physicsSettings = nullptr;
    }

    if (playerSettings) {
        PlayerSettings::DestroyInstanceImmediate(playerSettings);
        playerSettings = nullptr;
    }
}

void GameSettings::LoadSettings(GameWorld *gameWorld) {
    Shutdown();

    tagLayerSettings = TagLayerSettings::Load("ProjectSettings/tagLayer.settings");
    physicsSettings = PhysicsSettings::Load("ProjectSettings/physics.settings", gameWorld->GetPhysicsWorld());
    playerSettings = PlayerSettings::Load("ProjectSettings/player.settings");
}

void GameSettings::SaveSettings() {
    SaveObject(tagLayerSettings, "ProjectSettings/tagLayer.settings");
    SaveObject(physicsSettings, "ProjectSettings/physics.settings");
    SaveObject(playerSettings, "ProjectSettings/player.settings");
}

void GameSettings::SaveObject(Object *object, const char *filename) {
    Json::Value jsonNode;
    object->Serialize(jsonNode);

    Json::StyledWriter jsonWriter;
    Str jsonText = jsonWriter.write(jsonNode).c_str();

    fileSystem.WriteFile(filename, jsonText.c_str(), jsonText.Length());
}

BE_NAMESPACE_END
