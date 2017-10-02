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
#include "Components/ComRigidBody.h"
#include "Components/ComJoint.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Joint", ComJoint, Component)
BEGIN_EVENTS(ComJoint)
END_EVENTS
BEGIN_PROPERTIES(ComJoint)
    PROPERTY_OBJECT("connectedBody", "Connected Body", "", Guid::zero.ToString(), ComRigidBody::metaObject, PropertySpec::ReadWrite),
    PROPERTY_BOOL("collisionEnabled", "Collision Enabled", "", "true", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("breakImpulse", "Break Impulse", "", "1e30f", PropertySpec::ReadWrite),
END_PROPERTIES

void ComJoint::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_MIXED_ACCESSOR_PROPERTY("Connected Body", ComRigidBody, GetConnectedBody, SetConnectedBody, Guid::zero.ToString(), "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Collision Enabled", bool, IsCollisionEnabled, SetCollisionEnabled, "true", "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Break Impulse", float, GetBreakImpulse, SetBreakImpulse, "1e30f", "", PropertySpec::ReadWrite);
#endif
}

ComJoint::ComJoint() {
    constraint = nullptr;
    connectedBody = nullptr;
}

ComJoint::~ComJoint() {
    Purge(false);
}

void ComJoint::Purge(bool chainPurge) {
    if (constraint) {
        physicsSystem.DestroyConstraint(constraint);
        constraint = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComJoint::Init() {
    Purge();

    Component::Init();

    collisionEnabled = props->Get("collisionEnabled").As<bool>();
    // FIXME: atof("infinity") parsing 안됨
    breakImpulse = props->Get("breakImpulse").As<float>();
}

void ComJoint::Start() {
    connectedBody = nullptr;
    const Guid guid = props->Get("connectedBody").As<Guid>();
    if (!guid.IsZero()) {
        connectedBody = Object::FindInstance(guid)->Cast<ComRigidBody>();
    }
}

void ComJoint::Enable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            if (constraint) {
                constraint->SetEnabled(true);
            }
            Component::Enable(true);
        }
    } else {
        if (IsEnabled()) {
            if (constraint) {
                constraint->SetEnabled(false);
            }
            Component::Enable(false);
        }
    }
}

void ComJoint::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "connectedBody")) {        
        SetConnectedBody(props->Get("connectedBody").As<Guid>());
        return;
    }

    if (!Str::Cmp(propName, "collisionEnabled")) {
        SetCollisionEnabled(props->Get("collisionEnabled").As<bool>());
        return;
    }

    if (!Str::Cmp(propName, "breakImpulse")) {
        SetBreakImpulse(props->Get("breakImpulse").As<float>());
        return;
    }

    Component::PropertyChanged(classname, propName);
}

Guid ComJoint::GetConnectedBody() const {
    if (connectedBody) {
        return connectedBody->GetGuid();
    }
    return Guid();
}

void ComJoint::SetConnectedBody(const Guid &guid) {
    if (!guid.IsZero()) {
        connectedBody = Object::FindInstance(guid)->Cast<ComRigidBody>();
    } else {
        connectedBody = nullptr;
    }
}

bool ComJoint::IsCollisionEnabled() const {
    return collisionEnabled;
}

void ComJoint::SetCollisionEnabled(bool enabled) {
    collisionEnabled = enabled;
    if (constraint) {
        constraint->EnableCollision(collisionEnabled);
    }
}

float ComJoint::GetBreakImpulse() const {
    return breakImpulse;
}

void ComJoint::SetBreakImpulse(float breakImpulse) {
    this->breakImpulse = breakImpulse;
    if (constraint) {
        constraint->SetBreakImpulse(breakImpulse);
    }
}

BE_NAMESPACE_END
