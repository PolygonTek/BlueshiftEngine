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
}

ComFixedJoint::ComFixedJoint() {
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComFixedJoint::PropertyChanged);
}

ComFixedJoint::~ComFixedJoint() {
}

void ComFixedJoint::Init() {
    ComJoint::Init();
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
