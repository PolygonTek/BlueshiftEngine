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

PhysSensor::PhysSensor(btPairCachingGhostObject *pairCachingGhostObject, const Vec3 &centroid) : 
    PhysCollidable(PhysCollidable::Sensor, pairCachingGhostObject, centroid) {
}

PhysSensor::~PhysSensor() {
}

btPairCachingGhostObject *PhysSensor::GetPairCachingGhostObject() {
    return static_cast<btPairCachingGhostObject *>(collisionObject);
}

const btPairCachingGhostObject *PhysSensor::GetPairCachingGhostObject() const {
    return static_cast<const btPairCachingGhostObject *>(collisionObject);
}

void PhysSensor::GetContacts(Array<Contact> &contacts) {
    btManifoldArray manifoldArray;
    Contact contact;

    btPairCachingGhostObject *ghostObject = GetPairCachingGhostObject();
    btHashedOverlappingPairCache *overlappingPairCache = ghostObject->getOverlappingPairCache();

    const btCollisionShape *shape = collisionObject->getCollisionShape();
    btVector3 minAabb, maxAabb;
    shape->getAabb(ghostObject->getWorldTransform(), minAabb, maxAabb);

    btDynamicsWorld *dynamicsWorld = physicsWorld->dynamicsWorld;
    dynamicsWorld->getBroadphase()->setAabb(ghostObject->getBroadphaseHandle(), minAabb, maxAabb, dynamicsWorld->getDispatcher());
    dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(overlappingPairCache, dynamicsWorld->getDispatchInfo(), dynamicsWorld->getDispatcher());
    
    for (int i = 0; i < overlappingPairCache->getNumOverlappingPairs(); i++) {
        btBroadphasePair *collisionPair = &overlappingPairCache->getOverlappingPairArray()[i];
        
        manifoldArray.resize(0);

        if (collisionPair->m_algorithm) {
            collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);
        }

        for (int j = 0; j < manifoldArray.size(); j++) {
            const btPersistentManifold *manifold = manifoldArray[j];

            const btCollisionObject *objA = manifold->getBody0();
            const btCollisionObject *objB = manifold->getBody1();

            for (int k = 0; k < manifold->getNumContacts(); k++) {
                const btManifoldPoint &pt = manifold->getContactPoint(k);
                
                float dist = pt.getDistance();

                if (dist < 0.0f) {
                    const btVector3 &ptA = pt.getPositionWorldOnA();
                    const btVector3 &ptB = pt.getPositionWorldOnB();
                    const btVector3 &normalOnB = pt.m_normalWorldOnB;

                    if (objA == collisionObject) {
                        contact.object = (PhysCollidable *)objB->getUserPointer();
                        contact.point = PhysicsUnitToSystemUnit(ToVec3(ptB));
                        contact.normal = ToVec3(normalOnB);
                        contact.dist = PhysicsUnitToSystemUnit(dist);
                        contact.impulse = PhysicsUnitToSystemUnit(pt.getAppliedImpulse());
                    } else {
                        contact.object = (PhysCollidable *)objA->getUserPointer();
                        contact.point = PhysicsUnitToSystemUnit(ToVec3(ptA));
                        contact.normal = -ToVec3(normalOnB);
                        contact.dist = PhysicsUnitToSystemUnit(dist);
                        contact.impulse = PhysicsUnitToSystemUnit(pt.getAppliedImpulse());
                    }

                    contacts.Append(contact);
                }
            }
        }
    }
}

void PhysSensor::GetOverlaps(Array<PhysCollidable *> &collidables) {
    btManifoldArray manifoldArray;

    btPairCachingGhostObject *ghostObject = GetPairCachingGhostObject();
    btHashedOverlappingPairCache *overlappingPairCache = ghostObject->getOverlappingPairCache();

    const btCollisionShape *shape = collisionObject->getCollisionShape();
    btVector3 minAabb, maxAabb;
    shape->getAabb(ghostObject->getWorldTransform(), minAabb, maxAabb);

    btDynamicsWorld *dynamicsWorld = physicsWorld->dynamicsWorld;
    dynamicsWorld->getBroadphase()->setAabb(ghostObject->getBroadphaseHandle(), minAabb, maxAabb, dynamicsWorld->getDispatcher());
    dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(overlappingPairCache, dynamicsWorld->getDispatchInfo(), dynamicsWorld->getDispatcher());

    for (int i = 0; i < overlappingPairCache->getNumOverlappingPairs(); i++) {
        btBroadphasePair *collisionPair = &overlappingPairCache->getOverlappingPairArray()[i];
        //btBroadphasePair *collisionPair = dynamicsWorld->getPairCache()->findPair(pair->m_pProxy0, pair->m_pProxy1);
        if (!collisionPair) {
            continue;
        }

        manifoldArray.resize(0);

        if (collisionPair->m_algorithm) {
            collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);
        }

        for (int j = 0; j < manifoldArray.size(); j++) {
            const btPersistentManifold *manifold = manifoldArray[j];

            const btCollisionObject *objA = manifold->getBody0();
            const btCollisionObject *objB = manifold->getBody1();

            for (int p = 0; p < manifold->getNumContacts(); p++) {
                const btManifoldPoint &pt = manifold->getContactPoint(p);

                if (pt.getDistance() < 0.0f) {
                    if (objA == collisionObject) {
                        collidables.AddUnique((PhysCollidable *)objB->getUserPointer());
                    } else {
                        collidables.AddUnique((PhysCollidable *)objA->getUserPointer());
                    }
                }
            }
        }
    }
}

BE_NAMESPACE_END
