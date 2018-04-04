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
    delete constraint;
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
    return PhysicsUnitToSystemUnit(constraint->getBreakingImpulseThreshold());
}

void PhysConstraint::SetBreakImpulse(float impulse) {
    constraint->setBreakingImpulseThreshold(SystemUnitToPhysicsUnit(impulse));
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
