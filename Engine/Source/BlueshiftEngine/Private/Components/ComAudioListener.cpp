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
#include "Components/ComAudioListener.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Audio Listener", ComAudioListener, Component)
BEGIN_EVENTS(ComAudioListener)
END_EVENTS
BEGIN_PROPERTIES(ComAudioListener)
END_PROPERTIES

void ComAudioListener::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
#endif
}

ComAudioListener::ComAudioListener() {
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComAudioListener::PropertyChanged);
#endif
}

ComAudioListener::~ComAudioListener() {
    Purge(false);
}

void ComAudioListener::Purge(bool chainPurge) {
    if (chainPurge) {
        Component::Purge();
    }
}

void ComAudioListener::Init() {
    Purge();

    Component::Init();
}

void ComAudioListener::Awake() {
    ComTransform *transform = GetEntity()->GetTransform();
    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComAudioListener::TransformUpdated, SignalObject::Unique);

    soundSystem.PlaceListener(transform->GetOrigin(), transform->GetAxis());
}

void ComAudioListener::Update() {
    if (!IsEnabled()) {
        return;
    }
}

void ComAudioListener::Enable(bool enable) {
    Component::Enable(enable);
}

void ComAudioListener::TransformUpdated(const ComTransform *transform) {
    soundSystem.PlaceListener(transform->GetOrigin(), transform->GetAxis());
}

void ComAudioListener::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
