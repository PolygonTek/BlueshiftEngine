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

void ComAudioSource::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("audioClip", "Audio Clip", Guid, GetAudioClipGuid, SetAudioClipGuid, GuidMapper::defaultSoundGuid, 
        "", PropertyInfo::EditorFlag).SetMetaObject(&SoundAsset::metaObject);
    REGISTER_PROPERTY("playOnAwake", "Play On Awake", bool, playOnAwake, false, 
        "Play the sound when the map loaded.", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("spatial", "Spatial", bool, spatial, true, 
        "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("looping", "Looping", bool, looping, false, 
        "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("volume", "Volume", float, volume, 1.f, 
        "", PropertyInfo::EditorFlag).SetRange(0, 1, 0.1);
    REGISTER_PROPERTY("minDistance", "Min Distance", float, minDistance, MeterToUnit(4.0f),
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("maxDistance", "Max Distance", float, maxDistance, MeterToUnit(16.0f),
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
}

ComAudioSource::ComAudioSource() {
    referenceSound = nullptr;
    sound = nullptr;
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
    if (!IsActiveInHierarchy()) {
        return;
    }
}

void ComAudioSource::OnActive() {
    if (sound) {
        sound->SetVolume(volume);
    }
}

void ComAudioSource::OnInactive() {
    if (sound) {
        sound->SetVolume(0);
    }
}

void ComAudioSource::Play() {
    const ComTransform *transform = GetEntity()->GetTransform();

    if (referenceSound) {
        sound = referenceSound->Instantiate();

        if (spatial) {
            sound->Play3D(transform->GetOrigin(), minDistance, maxDistance, 
                volume * (IsActiveInHierarchy() ? 1.0f : 0.0f), looping);
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

Guid ComAudioSource::GetAudioClipGuid() const {
    if (referenceSound) {
        return resourceGuidMapper.Get(referenceSound->GetHashName());
    }
    return Guid();
}

void ComAudioSource::SetAudioClipGuid(const Guid &guid) {
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

void ComAudioSource::SetVolume(float volume) {
    this->volume = volume;
    if (sound) {
        sound->SetVolume(volume);
    }
}

BE_NAMESPACE_END
