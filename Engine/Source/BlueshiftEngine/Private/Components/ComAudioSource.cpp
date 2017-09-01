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
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Components/ComAudioSource.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "File/FileSystem.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Audio Source", ComAudioSource, Component)
BEGIN_EVENTS(ComAudioSource)
END_EVENTS
BEGIN_PROPERTIES(ComAudioSource)
    PROPERTY_OBJECT("audioClip", "Audio Clip", "", GuidMapper::defaultSoundGuid.ToString(), SoundAsset::metaObject, PropertySpec::ReadWrite),
    PROPERTY_BOOL("playOnAwake", "Play On Awake", "Play the sound when the map loaded.", "false", PropertySpec::ReadWrite),
    PROPERTY_BOOL("looping", "Looping", "", "false", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("volume", "Volume", "", BE1::Rangef(0, 1, 0.1), "1.0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("minDistance", "Min Distance", "", BE1::Rangef(0, 100, 1), "4", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("maxDistance", "Max Distance", "", BE1::Rangef(0, 100, 1), "16", PropertySpec::ReadWrite),
END_PROPERTIES

void ComAudioSource::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Audio Clip", SoundAsset, GetAudioClip, SetAudioClip, GuidMapper::defaultSoundGuid.ToString(), "", PropertySpec::ReadWrite);
    //REGISTER_PROPERTY("Play On Awake", bool, playOnAwake, "false", "Play the sound when the map loaded.", PropertySpec::ReadWrite);
    //REGISTER_PROPERTY("Looping", bool, looping, "false", "", PropertySpec::ReadWrite);
    //REGISTER_PROPERTY("Min Distance", float, minDistance, "4", PropertySpec::ReadWrite).SetRange(0, 100, 1);
    //REGISTER_PROPERTY("Max Distance", float, maxDistance, "16", PropertySpec::ReadWrite).SetRange(0, 100, 1);
    //REGISTER_PROPERTY("Volume", float, volume, "1.0", PropertySpec::ReadWrite).SetRange(0, 1, 0.1);
}

ComAudioSource::ComAudioSource() {
    referenceSound = nullptr;
    sound = nullptr;
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComAudioSource::PropertyChanged);
}

ComAudioSource::~ComAudioSource() {
    Purge(false);
}

void ComAudioSource::Purge(bool chainPurge) {
    if (chainPurge) {
        Component::Purge();
    }

    if (sound) {
        sound->Stop();
        sound = nullptr;
    }

    if (referenceSound) {
        soundSystem.ReleaseSound(referenceSound);
        referenceSound = nullptr;
    }
}

void ComAudioSource::Init() {
    Purge();

    Component::Init();

    Guid audioClipGuid = props->Get("audioClip").As<Guid>();
    if (!audioClipGuid.IsZero()) {
        audioClipPath = resourceGuidMapper.Get(audioClipGuid);
    }

    looping = props->Get("looping").As<bool>();
    playOnAwake = props->Get("playOnAwake").As<bool>();

    referenceSound = soundSystem.GetSound(audioClipPath);

    minDistance = BE1::MeterToUnit(props->Get("minDistance").As<float>());
    maxDistance = BE1::MeterToUnit(props->Get("maxDistance").As<float>());
    volume = props->Get("volume").As<float>();

    Stop();
}

void ComAudioSource::Awake() {
    if (playOnAwake) {
        Play();
    }

    ComTransform *transform = GetEntity()->GetTransform();
    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComAudioSource::TransformUpdated, SignalObject::Unique);
}

void ComAudioSource::Update() {
    if (!IsEnabled()) {
        return;
    }
}

void ComAudioSource::Enable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            if (sound) {
                sound->SetVolume(volume);
            }
            Component::Enable(true);
        }
    } else {
        if (IsEnabled()) {
            if (sound) {
                sound->SetVolume(0);
            }
            Component::Enable(false);
        }
    }
}

void ComAudioSource::Play() {
    const ComTransform *transform = GetEntity()->GetTransform();

    if (referenceSound) {
        sound = referenceSound->Instantiate();
        sound->Play3D(transform->GetOrigin(), minDistance, maxDistance, volume * (IsEnabled() ? 1.0f : 0.0f), looping);
    }
}

void ComAudioSource::Stop() {
    if (sound) {
        sound->Stop();
    }
}

void ComAudioSource::TransformUpdated(const ComTransform *transform) {
    if (sound) {
        sound->UpdatePosition(transform->GetOrigin());
    }
}

void ComAudioSource::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "audioClip")) {
        SetAudioClip(props->Get("audioClip").As<Guid>());
        return;
    }

    Component::PropertyChanged(classname, propName);
}

const Guid ComAudioSource::GetAudioClip() const {
    Guid guid = resourceGuidMapper.Get(audioClipPath);
    return guid;
}

void ComAudioSource::SetAudioClip(const Guid &guid) {
    if (!guid.IsZero()) {
        audioClipPath = resourceGuidMapper.Get(guid);
    }

    if (sound) {
        sound->Stop();
        sound = nullptr;
    }

    if (referenceSound) {
        soundSystem.ReleaseSound(referenceSound);
        referenceSound = nullptr;
    }

    referenceSound = soundSystem.GetSound(audioClipPath);
}

BE_NAMESPACE_END
