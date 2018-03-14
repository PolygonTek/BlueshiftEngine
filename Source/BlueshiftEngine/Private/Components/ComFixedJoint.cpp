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
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Fixed Joint", ComFixedJoint, ComJoint)
BEGIN_EVENTS(ComFixedJoint)
END_EVENTS

void ComFixedJoint::RegisterProperties() {
}

ComFixedJoint::ComFixedJoint() {
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

    // Fill up a constraint description 
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

    // Create a constraint by description
    constraint          = physicsSystem.CreateConstraint(&desc);

    PhysGenericConstraint *genericConstraint = static_cast<PhysGenericConstraint *>(constraint);
    genericConstraint->SetLinearLowerLimit(Vec3::zero);
    genericConstraint->SetLinearUpperLimit(Vec3::zero);
    genericConstraint->SetAngularLowerLimit(Vec3::zero);
    genericConstraint->SetAngularUpperLimit(Vec3::zero);

    if (IsActiveInHierarchy()) {
        genericConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

BE_NAMESPACE_END
