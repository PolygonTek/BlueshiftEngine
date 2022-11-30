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
#include "Physics/Collider.h"
#include "ColliderInternal.h"
#include "PhysicsInternal.h"

extern ContactAddedCallback     gContactAddedCallback;

BE_NAMESPACE_BEGIN

PhysicsSystem       physicsSystem;
btEmptyShape *      emptyShape = nullptr;

static bool CustomMaterialCombinerCallback(btManifoldPoint &cp, const btCollisionObjectWrapper *colObj0, int partId0, int index0, 
    const btCollisionObjectWrapper *colObj1, int partId1, int index1) {
    //btAdjustInternalEdgeContacts(cp, colObj1, colObj0, partId1, index1);
    //btAdjustInternalEdgeContacts(cp, colObj1, colObj0, partId1, index1, BT_TRIANGLE_CONVEX_BACKFACE_MODE);
    //btAdjustInternalEdgeContacts(cp, colObj1, colObj0, partId1, index1, BT_TRIANGLE_CONVEX_DOUBLE_SIDED + BT_TRIANGLE_CONCAVE_DOUBLE_SIDED);

    // this return value is currently ignored, but to be on the safe side: return false if you don't calculate friction
    return false;
}

void PhysicsSystem::Init() {
    gContactAddedCallback = CustomMaterialCombinerCallback;

    emptyShape = new btEmptyShape;

    colliderManager.Init();

    physics_showWireframe.SetModified();
    physics_showAABB.SetModified();
    physics_showContactPoints.SetModified();
    physics_showNormals.SetModified();
    physics_showConstraints.SetModified();
    physics_noDeactivation.SetModified();
    physics_enableCCD.SetModified();
}

void PhysicsSystem::Shutdown() {
    SAFE_DELETE(emptyShape);

    colliderManager.Shutdown();
}

PhysicsWorld *PhysicsSystem::AllocPhysicsWorld() {
    PhysicsWorld *physicsWorld = new PhysicsWorld;
    physicsWorlds.Append(physicsWorld);

    return physicsWorld;
}

void PhysicsSystem::FreePhysicsWorld(PhysicsWorld *physicsWorld) {
    if (physicsWorld) {
        physicsWorlds.Remove(physicsWorld);
        delete physicsWorld;
    }
}

PhysCollidable *PhysicsSystem::CreateCollidable(const PhysCollidableDesc &desc) {
    btCollisionShape *shape;
    btTransform initialTransform;
    Vec3 totalCentroid = Vec3::origin;

    if (desc.shapes.Count() == 0) {
        shape = emptyShape;

        initialTransform.setBasis(btMatrix3x3::getIdentity());
        initialTransform.setOrigin(btVector3(0, 0, 0));
    } else if (desc.shapes.Count() == 1) {
        const PhysShapeDesc *shapeDesc = &desc.shapes[0];
        shape = shapeDesc->collider->shape;

        totalCentroid = shapeDesc->localOrigin + shapeDesc->localAxis * shapeDesc->collider->GetCentroid();

        // Construct initial world transform 
        Vec3 worldCentroid = desc.origin + desc.axis * totalCentroid;
        Mat3 worldAxis = desc.axis * shapeDesc->localAxis;
        initialTransform.setBasis(ToBtMatrix3x3(worldAxis));
        initialTransform.setOrigin(ToBtVector3(SystemUnitToPhysicsUnit(worldCentroid)));
    } else {
        btCompoundShape *compoundShape = new btCompoundShape;
        shape = compoundShape;

        // Compute total centroid & volume
        float totalVolume = 0.0f;

        for (int i = 0; i < desc.shapes.Count(); i++) {
            const PhysShapeDesc *shapeDesc = &desc.shapes[i];

            Vec3 centroid = shapeDesc->localOrigin + shapeDesc->localAxis * shapeDesc->collider->GetCentroid();
            float volume = shapeDesc->collider->GetVolume();

            totalVolume += volume;
            totalCentroid += centroid * volume;
        }

        if (totalVolume > 0.0f) {
            totalCentroid /= totalVolume;
        }

        for (int i = 0; i < desc.shapes.Count(); i++) {
            const PhysShapeDesc *shapeDesc = &desc.shapes[i];

            Vec3 centroid = shapeDesc->localOrigin + shapeDesc->localAxis * shapeDesc->collider->GetCentroid();
            Vec3 localCentroid = centroid - totalCentroid;

            // Construct local transform for each child shapes
            btTransform localTransform;
            localTransform.setBasis(ToBtMatrix3x3(shapeDesc->localAxis));
            localTransform.setOrigin(ToBtVector3(SystemUnitToPhysicsUnit(localCentroid)));

            compoundShape->addChildShape(localTransform, shapeDesc->collider->shape);
        }

        // Construct initial world transform 
        Vec3 worldCentroid = desc.origin + desc.axis * totalCentroid;
        initialTransform.setBasis(ToBtMatrix3x3(desc.axis));
        initialTransform.setOrigin(ToBtVector3(SystemUnitToPhysicsUnit(worldCentroid)));
    }

    btVector3 inertia(0, 0, 0);
    if (shape != emptyShape) {
        if (desc.mass != 0.0f) {
            // NOTE: The bullet assumes that the shape is aligned to the center of mass, 
            // and in most cases computes the inertia tensor by AABB approximation.
            shape->calculateLocalInertia(desc.mass, inertia);
        }
    }

    if (desc.type == PhysCollidable::Type::RigidBody) {
        btDefaultMotionState *motionState = new btDefaultMotionState(initialTransform);
        btRigidBody *rigidBody = new btRigidBody(desc.mass, motionState, shape, inertia);

        if (desc.mass == 0) {
            rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        } else {
            // the constraint solver can discard solving contacts, if the distance is above this threshold. 
            //rigidBody->setContactProcessingThreshold(BT_LARGE_FLOAT);
        }

        if (shape->isConcave()) {
            rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
        }

        rigidBody->setSleepingThresholds(SystemUnitToPhysicsUnit(0.2f), 2.0f);

        PhysRigidBody *body = new PhysRigidBody(rigidBody, totalCentroid);
        body->SetRestitution(desc.restitution);
        body->SetFriction(desc.friction);
        body->SetRollingFriction(desc.rollingFriction);
        body->SetSpinningFriction(desc.spinningFriction);
        body->SetLinearDamping(desc.linearDamping);
        body->SetAngularDamping(desc.angularDamping);
        body->SetKinematic(desc.kinematic);
        body->SetCharacter(desc.character);
        // NOTE: CCD cannot be used with the compound shape.
        body->SetCCD(desc.ccd);

        rigidBody->setUserPointer(body);

        return body;
    }
    
    if (desc.type == PhysCollidable::Type::Sensor) {
        btPairCachingGhostObject *ghost = new btPairCachingGhostObject;
        ghost->setWorldTransform(initialTransform);
        ghost->setCollisionShape(shape);
        ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        ghost->setActivationState(DISABLE_DEACTIVATION);

        PhysSensor *sensor = new PhysSensor(ghost, totalCentroid);

        ghost->setUserPointer(sensor);

        return sensor;
    }

    btCollisionObject *collisionObject = new btCollisionObject;
    collisionObject->setCollisionShape(shape);

    PhysCollidable *object = new PhysCollidable(desc.type, collisionObject, totalCentroid);
    collisionObject->setUserPointer(object);

    return object;
}

void PhysicsSystem::DestroyCollidable(PhysCollidable *collidable) {
    if (collidable->IsInWorld()) {
        collidable->RemoveFromWorld();
    }

    btCollisionShape *shape = collidable->collisionObject->getCollisionShape();
    if (shape->isCompound()) {
        delete shape;
    }

    if (collidable->type == PhysCollidable::Type::RigidBody) {
        btRigidBody *rigidBody = btRigidBody::upcast(collidable->collisionObject);
        assert(rigidBody);

        // Remove attached constraints of rigid body.
        int numConstraintRefs = rigidBody->getNumConstraintRefs();
        if (numConstraintRefs > 0) {
            PhysConstraint **constraintRefs = (PhysConstraint **)_alloca(numConstraintRefs * sizeof(constraintRefs[0]));
            for (int i = 0; i < numConstraintRefs; i++) {
                constraintRefs[i] = (PhysConstraint *)rigidBody->getConstraintRef(i)->getUserConstraintPtr();
            }

            for (int i = 0; i < numConstraintRefs; i++) {
                constraintRefs[i]->RemoveFromWorld();
            }
        }

        if (static_cast<PhysRigidBody *>(collidable)->vehiclePtr) {
            static_cast<PhysRigidBody *>(collidable)->vehiclePtr->RemoveFromWorld();
        }

        if (rigidBody->getMotionState()) {
            delete rigidBody->getMotionState();
        }

        delete rigidBody;
    } else {
        delete collidable->collisionObject;
    }

    delete collidable;
}

PhysConstraint *PhysicsSystem::CreateConstraint(const PhysConstraintDesc &desc) {
    PhysConstraint *constraint = nullptr;

    switch (desc.type) {
    case PhysConstraint::Type::Generic:
        if (!desc.bodyA) {
            constraint = new PhysGenericConstraint(desc.bodyB, desc.anchorInB, desc.axisInB);
        } else {
            constraint = new PhysGenericConstraint(desc.bodyA, desc.anchorInA, desc.axisInA, desc.bodyB, desc.anchorInB, desc.axisInB);
        }
        break;
    case PhysConstraint::Type::GenericSpring:
        if (!desc.bodyA) {
            constraint = new PhysGenericSpringConstraint(desc.bodyB, desc.anchorInB, desc.axisInB);
        } else {
            constraint = new PhysGenericSpringConstraint(desc.bodyA, desc.anchorInA, desc.axisInA, desc.bodyB, desc.anchorInB, desc.axisInB);
        }
        break;
    case PhysConstraint::Type::Point2Point:
        if (!desc.bodyA) {
            constraint = new PhysP2PConstraint(desc.bodyB, desc.anchorInB);
        } else {
            constraint = new PhysP2PConstraint(desc.bodyA, desc.anchorInA, desc.bodyB, desc.anchorInB);
        }
        break;
    case PhysConstraint::Type::Hinge:
        if (!desc.bodyA) {
            constraint = new PhysHingeConstraint(desc.bodyB, desc.anchorInB, desc.axisInB);
        } else {
            constraint = new PhysHingeConstraint(desc.bodyA, desc.anchorInA, desc.axisInA, desc.bodyB, desc.anchorInB, desc.axisInB);
        }
        break;
    case PhysConstraint::Type::Slider:
        if (!desc.bodyA) {
            constraint = new PhysSliderConstraint(desc.bodyB, desc.anchorInB, desc.axisInB);
        } else {
            constraint = new PhysSliderConstraint(desc.bodyA, desc.anchorInA, desc.axisInA, desc.bodyB, desc.anchorInB, desc.axisInB);
        }
        break;
    default:
        assert(0);
        return nullptr;
    }

    constraint->EnableCollision(desc.collision);
    constraint->SetBreakImpulse(desc.breakImpulse);
    constraint->constraint->setDbgDrawSize(2.0f);

    return constraint;
}

void PhysicsSystem::DestroyConstraint(PhysConstraint *constraint) {
    if (constraint->IsInWorld()) {
        constraint->RemoveFromWorld();
    }

    delete constraint;
}

PhysVehicle *PhysicsSystem::CreateVehicle(const PhysVehicleDesc &desc) {
    PhysVehicle *vehicle = new PhysVehicle(desc.chassisBody);

    for (int i = 0; i < desc.wheels.Count(); i++) {
        const PhysWheelDesc &wheelDesc = desc.wheels[i];

        vehicle->AddWheel(wheelDesc.chassisLocalOrigin, wheelDesc.chassisLocalAxis, wheelDesc.radius,
            wheelDesc.suspensionRestLength, wheelDesc.suspensionMaxDistance, wheelDesc.suspensionMaxForce,
            wheelDesc.suspensionStiffness, wheelDesc.suspensionDampingRelaxation, wheelDesc.suspensionDampingCompression,
            wheelDesc.rollingFriction, wheelDesc.rollingInfluence);
    }

    return vehicle;
}

void PhysicsSystem::DestroyVehicle(PhysVehicle *vehicle) {
    if (vehicle->IsInWorld()) {
        vehicle->RemoveFromWorld();
    }

    delete vehicle;
}

void PhysicsSystem::CheckModifiedCVars() {
    if (physics_showWireframe.IsModified()) {
        physics_showWireframe.ClearModified();

        for (int i = 0; i < physicsWorlds.Count(); i++) {
            btDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

            int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
            if (physics_showWireframe.GetBool()) {
                debugMode |= btIDebugDraw::DBG_DrawWireframe;
            } else {
                debugMode &= ~btIDebugDraw::DBG_DrawWireframe;
            }

            dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
        }
    }

    if (physics_showAABB.IsModified()) {
        physics_showAABB.ClearModified();

         for (int i = 0; i < physicsWorlds.Count(); i++) {
             btDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

            int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
            if (physics_showAABB.GetBool()) {
                debugMode |= btIDebugDraw::DBG_DrawAabb;
            } else {
                debugMode &= ~btIDebugDraw::DBG_DrawAabb;
            }

            dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
         }
    }

    if (physics_showContactPoints.IsModified()) {
        physics_showContactPoints.ClearModified();

         for (int i = 0; i < physicsWorlds.Count(); i++) {
             btDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

            int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
            if (physics_showContactPoints.GetBool()) {
                debugMode |= btIDebugDraw::DBG_DrawContactPoints;
            } else {
                debugMode &= ~btIDebugDraw::DBG_DrawContactPoints;
            }

            dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
         }
    }

    if (physics_showNormals.IsModified()) {
        physics_showNormals.ClearModified();

         for (int i = 0; i < physicsWorlds.Count(); i++) {
             btDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

            int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
            if (physics_showNormals.GetBool()) {
                debugMode |= btIDebugDraw::DBG_DrawNormals;
            } else {
                debugMode &= ~btIDebugDraw::DBG_DrawNormals;
            }

            dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
         }
    }

    if (physics_showConstraints.IsModified()) {
        physics_showConstraints.ClearModified();

         for (int i = 0; i < physicsWorlds.Count(); i++) {
             btDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

            int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
            if (physics_showConstraints.GetBool()) {
                debugMode |= (btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits);
            } else {
                debugMode &= ~(btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits);
            }

            dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
         }
    }

    if (physics_noDeactivation.IsModified()) {
        physics_noDeactivation.ClearModified();

         for (int i = 0; i < physicsWorlds.Count(); i++) {
            btDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

            int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
            if (physics_noDeactivation.GetBool()) {
                debugMode |= btIDebugDraw::DBG_NoDeactivation;
            } else {
                debugMode &= ~btIDebugDraw::DBG_NoDeactivation;
            }

            dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
        }
    }

    if (physics_enableCCD.IsModified()) {
        physics_enableCCD.ClearModified();

         for (int i = 0; i < physicsWorlds.Count(); i++) {
             btDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

            int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
            if (physics_enableCCD.GetBool()) {
                dynamicsWorld->getDispatchInfo().m_useContinuous = true;
                debugMode |= btIDebugDraw::DBG_EnableCCD;
            } else {
                dynamicsWorld->getDispatchInfo().m_useContinuous = false;
                debugMode &= ~btIDebugDraw::DBG_EnableCCD;
            }

            dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
         }
    }
}

BE_NAMESPACE_END
