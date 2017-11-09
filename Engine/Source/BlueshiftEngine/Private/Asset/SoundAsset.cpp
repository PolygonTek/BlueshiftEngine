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
#include "Sound/SoundSystem.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Sound", SoundAsset, Asset)
BEGIN_EVENTS(SoundAsset)
END_EVENTS

void SoundAsset::RegisterProperties() {
}

SoundAsset::SoundAsset() {
    sound = nullptr;
}

SoundAsset::~SoundAsset() {
    if (!sound) {
        const Str soundPath = resourceGuidMapper.Get(GetGuid());
        sound = soundSystem.FindSound(soundPath);
    }

    if (sound) {
        soundSystem.ReleaseSound(sound, true);
    }
}

Sound *SoundAsset::GetSound() {
    if (sound) {
        return sound;
    }

    const Str soundPath = resourceGuidMapper.Get(GetGuid());
    sound = soundSystem.GetSound(soundPath);
    return sound;
}

void SoundAsset::Rename(const Str &newName) {
    Sound *existingSound = soundSystem.FindSound(GetResourceFilename());
    if (existingSound) {
        soundSystem.RenameSound(existingSound, newName);
    }

    Asset::Rename(newName);
}

void SoundAsset::Reload() {
    Sound *sound = soundSystem.FindSound(GetResourceFilename());
    if (sound) {
        sound->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END
