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

PhysCollidable::PhysCollidable(PhysCollidable::Type::Enum type, btCollisionObject *collisionObject, const Vec3 &centroid) {
    this->type = type;
    this->collisionObject = collisionObject;
    this->centroid = centroid;
}

PhysCollidable::~PhysCollidable() {
}

const Vec3 PhysCollidable::GetOrigin() const {
    Vec3 transformedCentroid = GetAxis() * centroid;

    // bullet world transform origin stands for the center of mass
    Vec3 worldCentroid = PhysicsUnitToSystemUnit(ToVec3(collisionObject->getWorldTransform().getOrigin()));

    return worldCentroid - transformedCentroid;
}

void PhysCollidable::SetOrigin(const Vec3 &origin) {
    btVector3 worldCentroid = ToBtVector3(SystemUnitToPhysicsUnit(origin + GetAxis() * centroid));

    collisionObject->getWorldTransform().setOrigin(worldCentroid);
}

const Mat3 PhysCollidable::GetAxis() const {
    const btMatrix3x3 &basis = collisionObject->getWorldTransform().getBasis();

    return Mat3(
        basis[0][0], basis[1][0], basis[2][0],
        basis[0][1], basis[1][1], basis[2][1],
        basis[0][2], basis[1][2], basis[2][2]);
}

void PhysCollidable::SetAxis(const Mat3 &axis) {
    collisionObject->getWorldTransform().setBasis(ToBtMatrix3x3(axis));
}

void PhysCollidable::SetTransform(const Mat3x4 &transform) {
    btVector3 worldCentroid = ToBtVector3(SystemUnitToPhysicsUnit(transform.TransformPos(centroid)));

    collisionObject->getWorldTransform().setOrigin(worldCentroid);
    collisionObject->getWorldTransform().setBasis(btMatrix3x3(
        transform[0][0], transform[0][1], transform[0][2],
        transform[1][0], transform[1][1], transform[1][2],
        transform[2][0], transform[2][1], transform[2][2]));
}

float PhysCollidable::GetRestitution() const {
    return collisionObject->getRestitution();
}

void PhysCollidable::SetRestitution(float rest) {
    collisionObject->setRestitution(rest);
}

float PhysCollidable::GetFriction() const {
    return collisionObject->getFriction();
}

void PhysCollidable::SetFriction(float friction) {
    collisionObject->setFriction(friction);
}

float PhysCollidable::GetRollingFriction() const {
    return collisionObject->getRollingFriction();
}

void PhysCollidable::SetRollingFriction(float friction) {
    collisionObject->setRollingFriction(friction);
}

float PhysCollidable::GetSpinningFriction() const {
    return collisionObject->getSpinningFriction();
}

void PhysCollidable::SetSpinningFriction(float friction) {
    collisionObject->setSpinningFriction(friction);
}

void PhysCollidable::Activate() {
    collisionObject->activate();
}

bool PhysCollidable::IsStatic() const {
    return collisionObject->isStaticObject();
}

bool PhysCollidable::IsKinematic() const {
    return collisionObject->isKinematicObject();
}

void PhysCollidable::SetKinematic(bool kinematic) {
    if (kinematic) {
        collisionObject->setCollisionFlags(collisionObject->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        collisionObject->setActivationState(DISABLE_DEACTIVATION);
    } else {
        collisionObject->setCollisionFlags(collisionObject->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
        collisionObject->setActivationState(ACTIVE_TAG);
    }
}

bool PhysCollidable::IsCharacter() const {
    return (collisionObject->getCollisionFlags() & btCollisionObject::CF_CHARACTER_OBJECT) ? true : false;
}

void PhysCollidable::SetCharacter(bool character) {
    if (character) {
        collisionObject->setCollisionFlags(collisionObject->getCollisionFlags() | btCollisionObject::CF_CHARACTER_OBJECT);
        collisionObject->setActivationState(DISABLE_DEACTIVATION);
    } else {
        collisionObject->setCollisionFlags(collisionObject->getCollisionFlags() & ~btCollisionObject::CF_CHARACTER_OBJECT);
        collisionObject->setActivationState(ACTIVE_TAG);
    }
}

void PhysCollidable::SetIgnoreCollisionCheck(const PhysCollidable &collidable, bool ignoreCollisionCheck) {
    // not working ?
    collisionObject->setIgnoreCollisionCheck(collidable.collisionObject, ignoreCollisionCheck);
}

void PhysCollidable::SetCollisionFilterBit(int bit) {
    assert(bit >= 0 && bit <= 31 - 6);

    collisionFilterBit = bit;
}

/*void PhysCollidable::SetCollisionFilterMask(short mask) {
    this->filterMask = mask;

    if (IsInWorld()) {
        PhysicsWorld *_physicsWorld = physicsWorld;
        RemoveFromWorld();
        AddToWorld(_physicsWorld);
    }
}*/

bool PhysCollidable::IsActive() const {
    if (IsInWorld()) {
        return collisionObject->isActive();
    }
    return false;
}

bool PhysCollidable::IsInWorld() const {
    if (physicsWorld) {
        btCollisionObjectArray &collisionObjectArray = physicsWorld->dynamicsWorld->getCollisionObjectArray();
    
        if (collisionObjectArray.findLinearSearch(collisionObject) < collisionObjectArray.size()) {
            return true;
        }
    }
    return false;
}

void PhysCollidable::SetDebugDraw(bool draw) {
    if (draw) {
        collisionObject->setCollisionFlags(collisionObject->getCollisionFlags() & ~btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
    } else {
        collisionObject->setCollisionFlags(collisionObject->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
    }
}

BE_NAMESPACE_END
