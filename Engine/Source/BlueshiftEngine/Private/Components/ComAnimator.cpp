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
#include "Components/ComAnimator.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Animator", ComAnimator, Component)
BEGIN_EVENTS(ComAnimator)
END_EVENTS
BEGIN_PROPERTIES(ComAnimator)
END_PROPERTIES

#ifdef NEW_PROPERTY_SYSTEM
void ComAnimator::RegisterProperties() {
}
#endif

ComAnimator::ComAnimator() {
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComAnimator::PropertyChanged);
#endif
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
    Component::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComAnimator::Awake() {
}

void ComAnimator::Update() {
    if (!IsEnabled()) {
        return;
    }
}

void ComAnimator::SetEnable(bool enable) {
    Component::SetEnable(enable);
}

void ComAnimator::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
