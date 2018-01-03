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

BE_NAMESPACE_BEGIN

TagLayerSettings *  GameSettings::tagLayerSettings = nullptr;
PhysicsSettings *   GameSettings::physicsSettings = nullptr;
PlayerSettings *    GameSettings::playerSettings = nullptr;

void GameSettings::Init() {
    tagLayerSettings = static_cast<TagLayerSettings *>(TagLayerSettings::CreateInstance());
    physicsSettings = static_cast<PhysicsSettings *>(PhysicsSettings::CreateInstance());
    playerSettings = static_cast<PlayerSettings *>(PlayerSettings::CreateInstance());
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
    tagLayerSettings->Load("ProjectSettings/tagLayer.settings");
    //inputSettings->Load("ProjectSettings/input.settings");

    physicsSettings->physicsWorld = gameWorld->GetPhysicsWorld();
    physicsSettings->Load("ProjectSettings/physics.settings");

    //rendererSettings->Load("ProjectSettings/renderer.settings");
    playerSettings->Load("ProjectSettings/player.settings");
}

void GameSettings::SaveSettings() {
    tagLayerSettings->Save("ProjectSettings/tagLayer.settings");
    physicsSettings->Save("ProjectSettings/physics.settings");
    playerSettings->Save("ProjectSettings/player.settings");
}

BE_NAMESPACE_END
