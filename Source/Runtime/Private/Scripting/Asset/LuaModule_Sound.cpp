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
#include "Scripting/LuaVM.h"
#include "Sound/SoundSystem.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSound(LuaCpp::Module &module) {
    LuaCpp::Selector _Sound = module["Sound"];

    _Sound.SetClass<Sound>();
    _Sound.AddClassMembers<Sound>(
        "name", &Sound::GetName,
        "instantiate", &Sound::Instantiate,
        "play2d", &Sound::Play2D,
        "play3d", &Sound::Play3D,
        "stop", &Sound::Stop,
        "pause", &Sound::Pause,
        "resume", &Sound::Resume,
        "is_playing", &Sound::IsPlaying,
        "duration", &Sound::Duration,
        "volume", &Sound::GetVolume,
        "set_volume", &Sound::SetVolume,
        "get_playing_time", &Sound::GetPlayingTime,
        "set_playing_time", &Sound::SetPlayingTime,
        "update_position", &Sound::UpdatePosition);

    _Sound["new"].SetFunc([]() {
        Guid newGuid = Guid::CreateGuid();
        Str soundName = Str("Sound-") + newGuid.ToString();
        resourceGuidMapper.Set(newGuid, soundName);
        return soundSystem.AllocSound(soundName);
    });
    _Sound["release"].SetFunc([](Sound *sound) {
        soundSystem.ReleaseSound(sound);
    });
}

BE_NAMESPACE_END
