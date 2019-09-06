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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Sound", SoundResource, Resource)
BEGIN_EVENTS(SoundResource)
END_EVENTS

void SoundResource::RegisterProperties() {
}

SoundResource::SoundResource() {
    sound = nullptr;
}

SoundResource::~SoundResource() {
    if (!sound) {
        const Str soundPath = resourceGuidMapper.Get(asset->GetGuid());
        sound = soundSystem.FindSound(soundPath);
    }

    if (sound) {
        soundSystem.ReleaseSound(sound);
    }
}

Sound *SoundResource::GetSound() {
    if (sound) {
        return sound;
    }

    const Str soundPath = resourceGuidMapper.Get(asset->GetGuid());
    sound = soundSystem.GetSound(soundPath);
    return sound;
}

void SoundResource::Rename(const Str &newName) {
    const Str soundPath = resourceGuidMapper.Get(asset->GetGuid());
    Sound *existingSound = soundSystem.FindSound(soundPath);
    if (existingSound) {
        soundSystem.RenameSound(existingSound, newName);
    }
}

bool SoundResource::Reload() {
    const Str soundPath = resourceGuidMapper.Get(asset->GetGuid());
    Sound *sound = soundSystem.FindSound(soundPath);
    if (sound) {
        sound->Reload();
        return true;
    }
    return false;
}

bool SoundResource::Save() {
    return false;
}

BE_NAMESPACE_END
