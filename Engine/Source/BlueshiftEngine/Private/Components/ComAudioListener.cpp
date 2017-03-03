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
}

ComAudioListener::ComAudioListener() {
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComAudioListener::PropertyChanged);
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
    transform->Connect(&SIG_TransformUpdated, this, (SignalCallback)&ComAudioListener::TransformUpdated, SignalObject::Unique);
    transform->Connect(&SIG_PhysicsUpdated, this, (SignalCallback)&ComAudioListener::PhysicsUpdated, SignalObject::Unique);

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

void ComAudioListener::PhysicsUpdated(const PhysRigidBody *body) {
    soundSystem.PlaceListener(body->GetOrigin(), body->GetAxis());
}

void ComAudioListener::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
