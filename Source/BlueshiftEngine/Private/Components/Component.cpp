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
#include "Components/ComTransform.h"
#include "Components/Component.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN
    
ABSTRACT_DECLARATION("Component", Component, Object)
BEGIN_EVENTS(Component)
END_EVENTS

void Component::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("enabled", "Enabled", bool, IsEnabled, SetEnabled, true, 
        "", 0);
}

Component::Component() {
    entity = nullptr;
    enabled = true;
    initialized = false;
}

Component::~Component() {
    Purge(false);
}

void Component::Purge(bool chainPurge) {
    initialized = false;
}

Str Component::ToString() const { 
    return entity->ToString(); 
}

void Component::Init() {
}

bool Component::IsActiveInHierarchy() const {
    if (!IsEnabled() || !GetEntity()->IsActiveInHierarchy()) {
        return false;
    }
    return true;
}

void Component::SetEnabled(bool enable) {
    if (enable == enabled) {
        return;
    }

    enabled = enable;

    if (GetEntity()->IsActiveInHierarchy()) {
        if (enable) {
            OnActive();
        } else {
            OnInactive();
        }
    }
}

GameWorld *Component::GetGameWorld() const {
    if (entity) {
        return entity->GetGameWorld();
    }
    return nullptr;
}

void Component::Event_ImmediateDestroy() {
    if (entity) {
        if (!entity->components.Remove(this)) {
            assert(0);
            return;
        }

        entity->EmitSignal(&Entity::SIG_ComponentRemoved, this);
    }

    Object::Event_ImmediateDestroy();
}

BE_NAMESPACE_END
