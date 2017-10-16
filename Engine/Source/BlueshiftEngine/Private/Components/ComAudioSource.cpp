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
    PROPERTY_BOOL("spatial", "Spatial", "", "true", PropertySpec::ReadWrite),
    PROPERTY_BOOL("looping", "Looping", "", "false", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("volume", "Volume", "", BE1::Rangef(0, 1, 0.1), "1.0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("minDistance", "Min Distance", "", BE1::Rangef(0, 10000, 1), "400", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("maxDistance", "Max Distance", "", BE1::Rangef(0, 10000, 1), "1600", PropertySpec::ReadWrite),
END_PROPERTIES

void ComAudioSource::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_MIXED_ACCESSOR_PROPERTY("Audio Clip", ObjectRef, GetAudioClipRef, SetAudioClipRef, ObjectRef(SoundAsset::metaObject, GuidMapper::defaultSoundGuid), "", PropertySpec::ReadWrite);
    REGISTER_PROPERTY("Play On Awake", bool, playOnAwake, "false", "Play the sound when the map loaded.", PropertySpec::ReadWrite);
    REGISTER_PROPERTY("Spatial", bool, spatial, true, "", PropertySpec::ReadWrite);
    REGISTER_PROPERTY("Looping", bool, looping, false, "", PropertySpec::ReadWrite);
    REGISTER_PROPERTY("Volume", float, volume, 1.f, "", PropertySpec::ReadWrite).SetRange(0, 1, 0.1);
    REGISTER_PROPERTY("Min Distance", float, minDistance, 4.f, "", PropertySpec::ReadWrite).SetRange(0, 100, 1);
    REGISTER_PROPERTY("Max Distance", float, maxDistance, 16.f, "", PropertySpec::ReadWrite).SetRange(0, 100, 1);
#endif
}

ComAudioSource::ComAudioSource() {
    referenceSound = nullptr;
    sound = nullptr;
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComAudioSource::PropertyChanged);
#endif
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
    Component::Init();

#ifndef NEW_PROPERTY_SYSTEM
    spatial = props->Get("spatial").As<bool>();
    looping = props->Get("looping").As<bool>();
    playOnAwake = props->Get("playOnAwake").As<bool>();
    minDistance = props->Get("minDistance").As<float>();
    maxDistance = props->Get("maxDistance").As<float>();
    volume = props->Get("volume").As<float>();

    Guid audioClipGuid = props->Get("audioClip").As<Guid>();
    Str audioClipPath;
    if (!audioClipGuid.IsZero()) {
        audioClipPath = resourceGuidMapper.Get(audioClipGuid);
    }
    referenceSound = soundSystem.GetSound(audioClipPath);
#endif

    Stop();

    // Mark as initialized
    SetInitialized(true);
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

void ComAudioSource::SetEnable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            if (sound) {
                sound->SetVolume(volume);
            }
            Component::SetEnable(true);
        }
    } else {
        if (IsEnabled()) {
            if (sound) {
                sound->SetVolume(0);
            }
            Component::SetEnable(false);
        }
    }
}

void ComAudioSource::Play() {
    const ComTransform *transform = GetEntity()->GetTransform();

    if (referenceSound) {
        sound = referenceSound->Instantiate();

        if (spatial) {
            sound->Play3D(transform->GetOrigin(), minDistance, maxDistance, volume * (IsEnabled() ? 1.0f : 0.0f), looping);
        } else {
            sound->Play2D(volume, looping);
        }
    }
}

void ComAudioSource::Stop() {
    if (sound) {
        sound->Stop();
    }
}

bool ComAudioSource::IsPlaying() const {
    if (sound) {
        return sound->IsPlaying();
    }
    return false;
}

void ComAudioSource::TransformUpdated(const ComTransform *transform) {
    if (spatial) {
        if (sound) {
            sound->UpdatePosition(transform->GetOrigin());
        }
    }
}

void ComAudioSource::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    if (!Str::Cmp(propName, "audioClip")) {
        SetAudioClip(props->Get("audioClip").As<Guid>());
        return;
    }

    Component::PropertyChanged(classname, propName);
}

Guid ComAudioSource::GetAudioClip() const {
    return referenceSound ? resourceGuidMapper.Get(referenceSound->GetHashName()) : Guid::zero;
}

void ComAudioSource::SetAudioClip(const Guid &guid) {
    Str audioClipPath;

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

ObjectRef ComAudioSource::GetAudioClipRef() const {
    return ObjectRef(SoundAsset::metaObject, referenceSound ? resourceGuidMapper.Get(referenceSound->GetHashName()) : Guid::zero);
}

void ComAudioSource::SetAudioClipRef(const ObjectRef &soundRef) {
    Str audioClipPath;

    if (!soundRef.objectGuid.IsZero()) {
        audioClipPath = resourceGuidMapper.Get(soundRef.objectGuid);
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
