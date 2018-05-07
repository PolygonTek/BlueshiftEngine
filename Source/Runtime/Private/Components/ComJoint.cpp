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
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Joint", ComJoint, Component)
BEGIN_EVENTS(ComJoint)
END_EVENTS

void ComJoint::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("connectedBody", "Connected Body", Guid, GetConnectedBodyGuid, SetConnectedBodyGuid, Guid::zero, 
        "Optional reference to the RigidBody that the joint is dependent upon. If not set, the joint connects to the world.", PropertyInfo::EditorFlag).SetMetaObject(&ComRigidBody::metaObject);
    REGISTER_ACCESSOR_PROPERTY("collisionEnabled", "Collision Enabled", bool, IsCollisionEnabled, SetCollisionEnabled, false, 
        "Enable collisions between bodies connected with a joint.", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("breakImpulse", "Break Impulse", float, GetBreakImpulse, SetBreakImpulse, 1e30f, 
        "Maximum impulse the joint can withstand before breaking.", PropertyInfo::EditorFlag);
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

        // Wake up both rigid bodies in case they have been constrained by the joint.
        if (entity) {
            ComRigidBody *body = entity->GetComponent<ComRigidBody>();
            if (body) {
                body->Activate();
            }
        }

        connectedBody = Object::FindInstance(connectedBodyGuid)->Cast<ComRigidBody>();
        if (connectedBody) {
            connectedBody->Activate();
            connectedBody = nullptr;
        }
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComJoint::Init() {
    Component::Init();

    GetEntity()->GetTransform()->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComJoint::TransformUpdated, SignalObject::Unique);
}

void ComJoint::Awake() {
    ComRigidBody *body = GetEntity()->GetComponent<ComRigidBody>();
    if (!body->GetBody()) {
        body->CreateBody();
    }

    if (GetConnectedBody()) {
        if (!GetConnectedBody()->GetBody()) {
            GetConnectedBody()->CreateBody();
        }
    }

    CreateConstraint();

    if (IsActiveInHierarchy()) {
        constraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComJoint::OnActive() {
    if (constraint) {
        constraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComJoint::OnInactive() {
    if (constraint) {
        constraint->RemoveFromWorld();
    }
}

void ComJoint::SetConnectedBodyGuid(const Guid &guid) {
    if (connectedBodyGuid == guid) {
        return;
    }
    connectedBodyGuid = guid;

    if (!connectedBodyGuid.IsZero()) {
        connectedBody = Object::FindInstance(connectedBodyGuid)->Cast<ComRigidBody>();
    } else {
        connectedBody = nullptr;
    }

    if (constraint) {
        physicsSystem.DestroyConstraint(constraint);

        CreateConstraint();

        if (IsActiveInHierarchy()) {
            constraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
        }

        if (connectedBody) {
            connectedBody->Activate();
        }
    }
}

ComRigidBody *ComJoint::GetConnectedBody() const {
    if (!connectedBody && !connectedBodyGuid.IsZero()) {
        connectedBody = Object::FindInstance(connectedBodyGuid)->Cast<ComRigidBody>();
    }
    return connectedBody;
}

void ComJoint::SetConnectedBody(const ComRigidBody *connectedBody) {
    const Guid bodyGuid = connectedBody ? connectedBody->GetGuid() : Guid::zero;

    SetConnectedBodyGuid(bodyGuid);
}

void ComJoint::SetCollisionEnabled(bool enabled) {
    collisionEnabled = enabled;

    if (constraint) {
        constraint->EnableCollision(collisionEnabled);
    }
}

void ComJoint::SetBreakImpulse(float breakImpulse) {
    this->breakImpulse = breakImpulse;

    if (constraint) {
        constraint->SetBreakImpulse(breakImpulse);
    }
}

void ComJoint::TransformUpdated(const ComTransform *transform) {
    if (constraint) {
        if (GetConnectedBody()) {
            GetConnectedBody()->Activate();
        }
    }
}

BE_NAMESPACE_END
