#include "Precompiled.h"
#include "Physics/Physics.h"
#include "PhysicsInternal.h"

BE_NAMESPACE_BEGIN

PhysConstraint::PhysConstraint(PhysRigidBody *bodyA, PhysRigidBody *bodyB) {
    this->constraint = nullptr;
    this->bodyA = bodyA;
    this->bodyB = bodyB;
    this->collisionEnabled = true;
    this->physicsWorld = nullptr;
}

PhysConstraint::~PhysConstraint() {
}

void PhysConstraint::EnableCollision(bool enable) {
    collisionEnabled = enable;
    
    if (IsInWorld()) {
        PhysicsWorld *_physicsWorld = physicsWorld;
        RemoveFromWorld();
        AddToWorld(_physicsWorld);
    }
}

float PhysConstraint::GetBreakImpulse() const {
    return constraint->getBreakingImpulseThreshold();
}

void PhysConstraint::SetBreakImpulse(float impulse) {
    constraint->setBreakingImpulseThreshold(impulse);
}

bool PhysConstraint::IsEnabled() const {
    return constraint->isEnabled();
}

void PhysConstraint::SetEnabled(bool enabled) {
    constraint->setEnabled(enabled);
}

bool PhysConstraint::IsInWorld() const {
    return physicsWorld != nullptr ? true : false;
}

void PhysConstraint::AddToWorld(PhysicsWorld *physicsWorld) {
    if (IsInWorld()) {
        BE_WARNLOG(L"PhysConstraint::AddToWorld: already added\n");
        return;
    }

    physicsWorld->dynamicsWorld->addConstraint(constraint, !collisionEnabled);

    this->physicsWorld = physicsWorld;
}

void PhysConstraint::RemoveFromWorld() {
    if (!IsInWorld()) {
        BE_WARNLOG(L"PhysConstraint::RemoveFromWorld: already removed\n");
        return;
    }

    physicsWorld->dynamicsWorld->removeConstraint(constraint);
    physicsWorld = nullptr;
}

void PhysConstraint::Reset() {
    //constraint->buildJacobian();
}

BE_NAMESPACE_END
