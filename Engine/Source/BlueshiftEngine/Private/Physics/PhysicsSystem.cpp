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

PhysicsSystem     physicsSystem;

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

PhysCollidable *PhysicsSystem::CreateCollidable(const PhysCollidableDesc *desc) {
    btCollisionShape *shape;
    Vec3 totalCentroid = Vec3::origin;
    btTransform initialTransform;

    assert(desc->shapes.Count() > 0);

    if (desc->shapes.Count() > 1) {
        btCompoundShape *compoundShape = new btCompoundShape;
        shape = compoundShape;

        float totalVolume = 0.0f;

        for (int i = 0; i < desc->shapes.Count(); i++) {
            const PhysShapeDesc *shapeDesc = &desc->shapes[i];

            Vec3 centroid = shapeDesc->localOrigin + shapeDesc->localAxis * shapeDesc->collider->GetCentroid();
            float volume = shapeDesc->collider->GetVolume();

            totalVolume += volume;
            totalCentroid += centroid * volume;
        }

        if (totalVolume > 0.0f) {
            totalCentroid /= totalVolume;
        }

        // local transform for child shapes
        for (int i = 0; i < desc->shapes.Count(); i++) {
            const PhysShapeDesc *shapeDesc = &desc->shapes[i];

            Vec3 centroid = shapeDesc->localOrigin + shapeDesc->localAxis * shapeDesc->collider->GetCentroid();
            Vec3 localCentroid = centroid - totalCentroid;

            btTransform localTransform;
            localTransform.setBasis(btMatrix3x3(
                shapeDesc->localAxis[0][0], shapeDesc->localAxis[1][0], shapeDesc->localAxis[2][0],
                shapeDesc->localAxis[0][1], shapeDesc->localAxis[1][1], shapeDesc->localAxis[2][1],
                shapeDesc->localAxis[0][2], shapeDesc->localAxis[1][2], shapeDesc->localAxis[2][2]));
            localTransform.setOrigin(ToBtVector3(localCentroid));

            compoundShape->addChildShape(localTransform, shapeDesc->collider->shape);
        }

        // initial world transform 
        Vec3 worldCentroid = desc->origin + desc->axis * totalCentroid;
        initialTransform.setBasis(btMatrix3x3(
            desc->axis[0][0], desc->axis[1][0], desc->axis[2][0],
            desc->axis[0][1], desc->axis[1][1], desc->axis[2][1],
            desc->axis[0][2], desc->axis[1][2], desc->axis[2][2]));
        initialTransform.setOrigin(ToBtVector3(worldCentroid));
    } else {
        const PhysShapeDesc *singleShapeDesc = &desc->shapes[0];
        shape = singleShapeDesc->collider->shape;

        totalCentroid = singleShapeDesc->localOrigin + singleShapeDesc->localAxis * singleShapeDesc->collider->GetCentroid();

        // initial world transform 
        Vec3 worldCentroid = desc->origin + desc->axis * totalCentroid;
        Mat3 worldAxis = desc->axis * singleShapeDesc->localAxis;
        initialTransform.setBasis(btMatrix3x3(
            worldAxis[0][0], worldAxis[1][0], worldAxis[2][0],
            worldAxis[0][1], worldAxis[1][1], worldAxis[2][1],
            worldAxis[0][2], worldAxis[1][2], worldAxis[2][2]));
        initialTransform.setOrigin(ToBtVector3(worldCentroid));
    }

    btVector3 inertia(0, 0, 0);
    if (desc->mass != 0.0f) {
        // NOTE: bullet 에서는 shape 이 center of mass 에 정렬되어 있다고 가정하고,
        // 대부분의 경우에 AABB approximation 으로 inertia tensor 를 계산한다.
        shape->calculateLocalInertia(desc->mass, inertia);
    }

    if (desc->type == PhysCollidable::Type::RigidBody || desc->type == PhysCollidable::Type::Character) {
        btDefaultMotionState *motionState = new btDefaultMotionState(initialTransform);
        btRigidBody *rigidBody = new btRigidBody(desc->mass, motionState, shape, inertia);

        if (desc->mass == 0) {
            rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        } else {
            // the constraint solver can discard solving contacts, if the distance is above this threshold. 
            rigidBody->setContactProcessingThreshold(1e18f);//0.03f);
        }

        if (shape->isConcave()) {
            rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
        }

        rigidBody->setSleepingThresholds(CentiToUnit(8), 2.0f);

        PhysRigidBody *body = new PhysRigidBody(rigidBody, totalCentroid);
        body->SetRestitution(desc->restitution);
        body->SetFriction(desc->friction);
        body->SetRollingFriction(desc->rollingFriction);
        body->SetLinearDamping(desc->linearDamping);
        body->SetAngularDamping(desc->angularDamping);
        body->SetKinematic(desc->kinematic);
        // NOTE: compound shape 일 경우 CCD 적용 안됨
        body->SetCCD(desc->ccd);
                
        rigidBody->setUserPointer(body);

        return body;
    }
    
    if (desc->type == PhysCollidable::Sensor) {
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

    PhysCollidable *object = new PhysCollidable(desc->type, collisionObject, totalCentroid);
    collisionObject->setUserPointer(object);

    return object;
}

void PhysicsSystem::DestroyCollidable(PhysCollidable *object) {
    if (object->IsInWorld()) {
        object->RemoveFromWorld();
    }

    btCollisionShape *shape = object->collisionObject->getCollisionShape();
    if (shape->isCompound()) {
        delete shape;
    }

    btRigidBody *rigidBody = btRigidBody::upcast(object->collisionObject);
    if (rigidBody) {
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

        if (rigidBody->getMotionState()) {
            delete rigidBody->getMotionState();
        }
       
        delete rigidBody;
    } else {
        delete object->collisionObject;
    }

    delete object;
}

PhysConstraint *PhysicsSystem::CreateConstraint(const PhysConstraintDesc *desc) {
    PhysConstraint *constraint = nullptr;

    switch (desc->type) {
    case PhysConstraint::Generic:
        if (!desc->bodyB) {
            constraint = new PhysGenericConstraint(desc->bodyA, desc->anchorInA, desc->axisInA);
        } else {
            constraint = new PhysGenericConstraint(desc->bodyA, desc->anchorInA, desc->axisInA, desc->bodyB, desc->anchorInB, desc->axisInB);
        }
        break;
    case PhysConstraint::GenericSpring:
        if (!desc->bodyB) {
            constraint = new PhysGenericSpringConstraint(desc->bodyA, desc->anchorInA, desc->axisInA);
        } else {
            constraint = new PhysGenericSpringConstraint(desc->bodyA, desc->anchorInA, desc->axisInA, desc->bodyB, desc->anchorInB, desc->axisInB);
        }
        break;
    case PhysConstraint::Point2Point:
        if (!desc->bodyB) {
            constraint = new PhysP2PConstraint(desc->bodyA, desc->anchorInA);
        } else {
            constraint = new PhysP2PConstraint(desc->bodyA, desc->anchorInA, desc->bodyB, desc->anchorInB);
        }
        break;
    case PhysConstraint::Hinge:
        if (!desc->bodyB) {
            constraint = new PhysHingeConstraint(desc->bodyA, desc->anchorInA, desc->axisInA);
        } else {
            constraint = new PhysHingeConstraint(desc->bodyA, desc->anchorInA, desc->axisInA, desc->bodyB, desc->anchorInB, desc->axisInB);
        }
        break;
    default:
        assert(0);
        return nullptr;
    }

    constraint->EnableCollision(desc->collision);
    constraint->SetBreakImpulse(desc->breakImpulse);
    constraint->constraint->setDbgDrawSize(btScalar(CentiToUnit(10.0f)));

    return constraint;
}

void PhysicsSystem::DestroyConstraint(PhysConstraint *constraint) {
    if (constraint->IsInWorld()) {
        constraint->RemoveFromWorld();
    }

    delete constraint->constraint;
    delete constraint;
}

void PhysicsSystem::CheckModifiedCVars() {
    if (physics_showWireframe.IsModified()) {
        physics_showWireframe.ClearModified();

        for (int i = 0; i < physicsWorlds.Count(); i++) {
            btDiscreteDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

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
            btDiscreteDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

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
            btDiscreteDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

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
            btDiscreteDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

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
            btDiscreteDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

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
            btDiscreteDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

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
            btDiscreteDynamicsWorld *dynamicsWorld = physicsWorlds[i]->dynamicsWorld;

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
