#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Sound/SoundSystem.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSoundAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Sound = module["Sound"];

    _Sound.SetClass<Sound>();
    _Sound.AddClassMembers<Sound>(
        "instantiate", &Sound::Instantiate,
        "play2d", &Sound::Play2D,
        "play3d", &Sound::Play3D,
        "stop", &Sound::Stop,
        "is_playing", &Sound::IsPlaying,
        "set_volume", &Sound::SetVolume,
        "get_playing_time", &Sound::GetPlayingTime,
        "set_playing_time", &Sound::SetPlayingTime,
        "update_position", &Sound::UpdatePosition);

    LuaCpp::Selector _SoundAsset = module["SoundAsset"];

    _SoundAsset.SetClass<SoundAsset>(module["Asset"]);
    _SoundAsset.AddClassMembers<SoundAsset>(
        "sound", &SoundAsset::GetSound);
}

BE_NAMESPACE_END