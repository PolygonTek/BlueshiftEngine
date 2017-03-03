#include "Precompiled.h"
#include "Components/ComAnimator.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Animator", ComAnimator, Component)
BEGIN_EVENTS(ComAnimator)
END_EVENTS
BEGIN_PROPERTIES(ComAnimator)
END_PROPERTIES

void ComAnimator::RegisterProperties() {
}

ComAnimator::ComAnimator() {
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComAnimator::PropertyChanged);
}

ComAnimator::~ComAnimator() {
    Purge(false);
}

void ComAnimator::Purge(bool chainPurge) {
    if (chainPurge) {
        Component::Purge();
    }
}

void ComAnimator::Init() {
    Purge();

    Component::Init();
}

void ComAnimator::Awake() {
}

void ComAnimator::Update() {
    if (!IsEnabled()) {
        return;
    }
}

void ComAnimator::Enable(bool enable) {
    Component::Enable(enable);
}

void ComAnimator::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
