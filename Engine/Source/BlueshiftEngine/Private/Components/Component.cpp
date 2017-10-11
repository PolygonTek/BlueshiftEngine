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
BEGIN_PROPERTIES(Component)
    PROPERTY_BOOL("enabled", "Enabled", "is enabled component ?", "true", PropertySpec::ReadWrite | PropertySpec::Hidden),
END_PROPERTIES

void Component::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_ACCESSOR_PROPERTY("Enabled", bool, IsEnabled, SetEnable, true, "", PropertySpec::ReadWrite | PropertySpec::Hidden);
#endif
}

Component::Component() {
    entity = nullptr;
    initialized = false;
    enabled = true;
}

Component::~Component() {
    Purge(false);
}

void Component::Purge(bool chainPurge) {
    initialized = false;
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

const Str Component::ToString() const { 
    return entity->ToString(); 
}

GameWorld *Component::GetGameWorld() const {
    if (entity) {
        return entity->GetGameWorld();
    }
    return nullptr;
}

void Component::Init() {
    Purge();

#ifndef NEW_PROPERTY_SYSTEM
    enabled = props->Get("enabled").As<bool>();
#endif
}

void Component::Reload() {
    Init();
}

void Component::PropertyChanged(const char *classname, const char *propName) {
    if (!Str::Cmp(propName, "enabled")) {
        SetEnable(props->Get("enabled").As<bool>());
        return;
    }
}

BE_NAMESPACE_END
