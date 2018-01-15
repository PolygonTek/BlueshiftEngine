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

PhysCollidable::PhysCollidable(PhysCollidable::Type type, btCollisionObject *collisionObject, const Vec3 &centroid) {
    this->type = type;
    this->collisionObject = collisionObject;
    this->centroid = centroid;
    this->customFilterIndex = 0;
    this->collisionListener = nullptr;
    this->userPointer = nullptr;
    this->physicsWorld = nullptr;
}

PhysCollidable::~PhysCollidable() {
}

const Vec3 PhysCollidable::GetOrigin() const {
    Vec3 transformedCentroid = GetAxis() * centroid;

    // world transform origin is the center of mass
    btVector3 origin = collisionObject->getWorldTransform().getOrigin() - ToBtVector3(transformedCentroid);
    return Vec3(origin.x(), origin.y(), origin.z());
}

const Mat3 PhysCollidable::GetAxis() const {
    const btMatrix3x3 &basis = collisionObject->getWorldTransform().getBasis();

    return Mat3(basis[0][0], basis[1][0], basis[2][0],
        basis[0][1], basis[1][1], basis[2][1],
        basis[0][2], basis[1][2], basis[2][2]);
}

void PhysCollidable::SetOrigin(const Vec3 &origin) {
    collisionObject->getWorldTransform().setOrigin(ToBtVector3(origin + GetAxis() * centroid));
}

void PhysCollidable::SetAxis(const Mat3 &axis) {
    collisionObject->getWorldTransform().setBasis(btMatrix3x3(
        axis[0][0], axis[1][0], axis[2][0],
        axis[0][1], axis[1][1], axis[2][1],
        axis[0][2], axis[1][2], axis[2][2]));
}

void PhysCollidable::SetTransform(const Mat3 &axis, const Vec3 &origin) {
    collisionObject->getWorldTransform().setOrigin(ToBtVector3(origin + axis * centroid));
    collisionObject->getWorldTransform().setBasis(btMatrix3x3(
        axis[0][0], axis[1][0], axis[2][0],
        axis[0][1], axis[1][1], axis[2][1],
        axis[0][2], axis[1][2], axis[2][2]));
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

void *PhysCollidable::GetUserPointer() const {
    return userPointer;
}

void PhysCollidable::SetUserPointer(void *pointer) {
    userPointer = pointer;
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

void PhysCollidable::SetCustomCollisionFilterIndex(unsigned int index) {
    assert(index >= 0 && index < 31);

    customFilterIndex = index;
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

void PhysCollidable::AddToWorld(PhysicsWorld *physicsWorld) {
    if (IsInWorld()) {
        BE_WARNLOG(L"PhysCollidable::AddToWorld: already added\n");
        return;
    }

    switch (type) {
    case Type::RigidBody: {
        btRigidBody *rigidBody = static_cast<btRigidBody *>(collisionObject);

        bool isStatic = rigidBody->isStaticObject();
        bool isKinematic = rigidBody->isKinematicObject();
        
        short filterGroup = isStatic ? (isKinematic ? (btBroadphaseProxy::KinematicFilter | btBroadphaseProxy::StaticFilter) : btBroadphaseProxy::StaticFilter) : btBroadphaseProxy::DefaultFilter; 
        short filterMask = btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter | btBroadphaseProxy::SensorTrigger | btBroadphaseProxy::CharacterFilter;
        if (isStatic) {
            filterMask = filterMask & ~btBroadphaseProxy::StaticFilter;
        }

        physicsWorld->dynamicsWorld->addRigidBody(rigidBody, filterGroup, filterMask);
        break; 
    }
    case Type::Character:
        physicsWorld->dynamicsWorld->addCollisionObject(collisionObject, btBroadphaseProxy::CharacterFilter, 
            btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter | btBroadphaseProxy::SensorTrigger | btBroadphaseProxy::CharacterFilter);
        break;
    case Type::Sensor:
        physicsWorld->dynamicsWorld->addCollisionObject(collisionObject, btBroadphaseProxy::SensorTrigger, 
            btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter /* FIXME: remove? */ | btBroadphaseProxy::KinematicFilter | btBroadphaseProxy::CharacterFilter);
        break;
    default:
        BE_WARNLOG(L"PhysCollidable::AddToWorld: invalid collidable type %i\n", type);
        break;
    }

    this->physicsWorld = physicsWorld;
}

void PhysCollidable::RemoveFromWorld() {
    if (!IsInWorld()) {
        BE_WARNLOG(L"PhysCollidable::RemoveFromWorld: already removed\n");
        return;
    }

    switch (type) {
    case Type::RigidBody:
        physicsWorld->dynamicsWorld->removeRigidBody(static_cast<btRigidBody *>(collisionObject));
        break;
    case Type::Character:
        physicsWorld->dynamicsWorld->removeCollisionObject(collisionObject);
        break;
    case Type::Sensor:
        physicsWorld->dynamicsWorld->removeCollisionObject(collisionObject);
        break;
    default:
        BE_WARNLOG(L"PhysCollidable::RemoveFromWorld: invalid collidable type %i\n", type);
        break;
    }

    physicsWorld = nullptr;
}

void PhysCollidable::SetDebugDraw(bool draw) {
    if (draw) {
        collisionObject->setCollisionFlags(collisionObject->getCollisionFlags() & ~btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
    } else {
        collisionObject->setCollisionFlags(collisionObject->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
    }
}

PhysCollisionListener *PhysCollidable::GetCollisionListener() const {
    return collisionListener;
}

void PhysCollidable::SetCollisionListener(PhysCollisionListener *listener) {
    collisionListener = listener;
}

BE_NAMESPACE_END
