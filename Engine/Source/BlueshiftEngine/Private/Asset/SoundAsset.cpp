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
BEGIN_PROPERTIES(SoundAsset)
END_PROPERTIES

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

void SoundAsset::Reload() {
    Sound *sound = soundSystem.FindSound(GetResourceFilename());
    if (sound) {
        sound->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END