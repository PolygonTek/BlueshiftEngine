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

void ComJoint::RegisterProperties() {
    REGISTER_PROPERTY("connectedBody", "Connected Body", Guid, connectedBodyGuid, Guid::zero, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&ComRigidBody::metaObject);
    REGISTER_ACCESSOR_PROPERTY("collisionEnabled", "Collision Enabled", bool, IsCollisionEnabled, SetCollisionEnabled, true, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("breakImpulse", "Break Impulse", float, GetBreakImpulse, SetBreakImpulse, 1e30f, "", PropertyInfo::EditorFlag);
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
    Component::Init();
}

void ComJoint::Start() {
    connectedBody = nullptr;

    // Rigid body component will be created after calling Awake() function
    // So we can connect this joint to the connected rigid body in Start() function
    if (!connectedBodyGuid.IsZero()) {
        connectedBody = Object::FindInstance(connectedBodyGuid)->Cast<ComRigidBody>();
    }
}

void ComJoint::OnActive() {
    if (constraint) {
        constraint->SetEnabled(true);
    }
}

void ComJoint::OnInactive() {
    if (constraint) {
        constraint->SetEnabled(false);
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
