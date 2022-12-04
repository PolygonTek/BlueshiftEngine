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
#include "Components/Transform/ComTransform.h"
#include "Components/Joint/ComFixedJoint.h"
#include "Components/Physics/ComRigidBody.h"
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

void ComFixedJoint::CreateConstraint() {
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    // Fill up a constraint description.
    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Type::Hinge;
    desc.collision = collisionEnabled;
    desc.breakImpulse = breakImpulse;

    desc.bodyB = rigidBody->GetBody();
    desc.axisInB = Mat3::identity;
    desc.anchorInB = Vec3::zero;

    const ComRigidBody *connectedBody = GetConnectedBody();
    if (connectedBody) {
        desc.bodyA = connectedBody->GetBody();
        desc.axisInA = Mat3::identity;
        desc.anchorInA = connectedBody->GetBody()->GetAxis().TransposedMulVec(desc.bodyB->GetOrigin() - connectedBody->GetBody()->GetOrigin());
    } else {
        desc.bodyA = nullptr;
    }

    // Create a constraint with the given description.
    PhysHingeConstraint *hingeConstraint = (PhysHingeConstraint *)physicsSystem.CreateConstraint(desc);
    constraint = hingeConstraint;

    hingeConstraint->SetAngularLimits(0, 0);
    hingeConstraint->EnableAngularLimits(true);
}

BE_NAMESPACE_END
