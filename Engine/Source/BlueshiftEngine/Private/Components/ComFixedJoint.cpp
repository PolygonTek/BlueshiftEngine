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
#include "Render/Render.h"
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Components/ComFixedJoint.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Fixed Joint", ComFixedJoint, ComJoint)
BEGIN_EVENTS(ComFixedJoint)
END_EVENTS
BEGIN_PROPERTIES(ComFixedJoint)
END_PROPERTIES

void ComFixedJoint::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
#endif
}

ComFixedJoint::ComFixedJoint() {
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComFixedJoint::PropertyChanged);
#endif
}

ComFixedJoint::~ComFixedJoint() {
}

void ComFixedJoint::Init() {
    ComJoint::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComFixedJoint::Start() {
    ComJoint::Start();

    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    PhysConstraintDesc desc;
    desc.type           = PhysConstraint::Generic;
    desc.bodyA          = rigidBody->GetBody();
    desc.axisInA        = Mat3::identity;
    desc.anchorInA      = Vec3::zero;
    desc.bodyB          = connectedBody ? connectedBody->GetBody() : nullptr;          
    desc.axisInB        = Mat3::identity;
    desc.anchorInB      = Vec3::zero;

    desc.collision      = collisionEnabled;
    desc.breakImpulse   = breakImpulse;

    constraint          = physicsSystem.CreateConstraint(&desc);

    PhysGenericConstraint *genericConstraint = static_cast<PhysGenericConstraint *>(constraint);
    genericConstraint->SetLinearLowerLimit(Vec3::zero);
    genericConstraint->SetLinearUpperLimit(Vec3::zero);
    genericConstraint->SetAngularLowerLimit(Vec3::zero);
    genericConstraint->SetAngularUpperLimit(Vec3::zero);

    if (IsEnabled()) {
        genericConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComFixedJoint::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    ComJoint::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
