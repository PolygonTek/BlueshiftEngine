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
#include "Components/ComParticleSystem.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Particle System", ComParticleSystem, Component)
BEGIN_EVENTS(ComParticleSystem)
END_EVENTS
BEGIN_PROPERTIES(ComParticleSystem)
END_PROPERTIES

void ComParticleSystem::RegisterProperties() {}

ComParticleSystem::ComParticleSystem() {
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComParticleSystem::PropertyChanged);
}

ComParticleSystem::~ComParticleSystem() {
    Purge(false);
}

void ComParticleSystem::Purge(bool chainPurge) {
    if (chainPurge) {
        Component::Purge();
    }
}

void ComParticleSystem::Init() {
    Purge();

    Component::Init();
}

void ComParticleSystem::Awake() {}

void ComParticleSystem::Update() {
    if (!IsEnabled()) {
        return;
    }
}

void ComParticleSystem::Enable(bool enable) {
    Component::Enable(enable);
}

void ComParticleSystem::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
