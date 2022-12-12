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
    Vec3 totalCentroid = Vec3::origin;

    if (desc.type == PhysCollidable::Type::RigidBody ||
        desc.type == PhysCollidable::Type::Sensor) {
        btCollisionShape *shape;
        btTransform initialTransform;

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

            PhysRigidBody *rb = new PhysRigidBody(rigidBody, totalCentroid);
            rb->SetRestitution(desc.restitution);
            rb->SetFriction(desc.friction);
            rb->SetRollingFriction(desc.rollingFriction);
            rb->SetSpinningFriction(desc.spinningFriction);
            rb->SetLinearDamping(desc.linearDamping);
            rb->SetAngularDamping(desc.angularDamping);
            rb->SetKinematic(desc.kinematic);
            rb->SetCharacter(desc.character);
            // NOTE: CCD cannot be used with the compound shape.
            rb->SetContinuousCollisionDetectionEnabled(desc.enableCCD);

            rigidBody->setUserPointer(rb);

            return rb;
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
    } else if (desc.type == PhysCollidable::Type::SoftBody) {
        btSoftBody *softBody = new btSoftBody(nullptr);

        softBody->getCollisionShape()->setMargin(SystemUnitToPhysicsUnit(desc.thickness));

        softBody->m_cfg.aeromodel = btSoftBody::eAeroModel::F_TwoSidedLiftDrag;

        // Dynamic friction coefficient [0, 1]
        // kDF = 0 mean sliding, kDF = 1 mean sticking.
        softBody->m_cfg.kDF = desc.friction;
        // Damping coefficient [0, 1]
        // zero = no damping, one = full damping.
        softBody->m_cfg.kDP = btScalar(0.0005);
        // Velocities correction factor (Baumgarte)
        // Define the amount of correction per time step for drift solver(sometimes referred as ERP in rigid bodies solvers).
        softBody->m_cfg.kVCF = btScalar(1.0);
        // Anchor hardness [0, 1]
        // Define how 'soft' anchor constraint(joint) are, kAHR = 0 mean no drift correction, 1 mean full correction.
        softBody->m_cfg.kAHR = btScalar(.69);
        // Rigid contact hardness [0, 1]
        // Define how 'soft' contact with rigid bodies are, kCHR = 0 mean no penetration correction, 1 mean full correction.
        softBody->m_cfg.kCHR = btScalar(1.0);
        // Kinetic contact hardness [0, 1]
        // Define how 'soft' contact with kinetic / static bodies are, kKHR = 0 mean no penetration correction, 1 mean full correction.
        softBody->m_cfg.kKHR = btScalar(0.1);
        // Soft contacts hardness [0, 1]
        // Define how 'soft' contact with other soft bodies are, kSHR = 0 mean no penetration correction, 1 mean full correction.
        softBody->m_cfg.kSHR = btScalar(1.0);
        // Drag coefficient [0, +inf]
        // [aerodynamic] kDG = 0 mean no drag.
        softBody->m_cfg.kDG = btScalar(0.00001);
        // Lift coefficient [0, +inf]
        // [aerodynamic] = > is a factor of the lift force kLF = 0 mean no lift
        softBody->m_cfg.kLF = btScalar(0.01);
        // Pressure coefficient [-inf, +inf]
        // [aerodynamic] = > is a factor of pressure.
        softBody->m_cfg.kPR = btScalar(0.0);
        // Pose matching coefficient [0, 1]
        // When 'setPose(..., true)' as been called, define the factor used for pose matching.
        softBody->m_cfg.kMT = btScalar(0.0);
        // Volume conservation coefficient [0, +inf]
        // When 'setPose(true, ...)' as been called, define the magnitude of the force used to conserve volume.
        softBody->m_cfg.kVC = btScalar(0.0);
        
        softBody->m_cfg.kSKHR_CL = btScalar(1.0);       // Soft vs kinetic hardness [0, 1] (cluster only)
        softBody->m_cfg.kSK_SPLT_CL = btScalar(0.5);    // Soft vs kinetic impulse split [0, 1] (cluster only)
        softBody->m_cfg.kSRHR_CL = btScalar(0.1);       // Soft vs rigid hardness [0, 1] (cluster only)
        softBody->m_cfg.kSR_SPLT_CL = btScalar(0.5);    // Soft vs rigid impulse split [0, 1] (cluster only)
        softBody->m_cfg.kSSHR_CL = btScalar(0.5);       // Soft vs soft hardness [0, 1] (cluster only)
        softBody->m_cfg.kSS_SPLT_CL = btScalar(0.5);    // Soft vs soft impulse split [0, 1] (cluster only)

        //softBody->m_cfg.viterations = 10;
        //softBody->m_cfg.piterations = 10;
        //softBody->m_cfg.citerations = 10;
        //softBody->m_cfg.diterations = 10;

        softBody->m_cfg.collisions = btSoftBody::fCollision::SDF_RS | btSoftBody::fCollision::CL_SS;

        if (desc.enableSelfCollision) {
            // FIXME: not working
            softBody->m_cfg.collisions |= btSoftBody::fCollision::CL_SELF;
        }

        // Default material
        btSoftBody::Material *softBodyMaterial = softBody->appendMaterial();
        softBodyMaterial->m_kLST = desc.stiffness;
        softBodyMaterial->m_kAST = desc.stiffness;
        softBodyMaterial->m_kVST = desc.stiffness;
        softBodyMaterial->m_flags = btSoftBody::fMaterial::Default;

        // Nodes
        const btScalar margin = softBody->getCollisionShape()->getMargin();
        softBody->m_nodes.resize(desc.points.Count());
        for (int nodeIndex = 0; nodeIndex < softBody->m_nodes.size(); nodeIndex++) {
            btSoftBody::Node &node = softBody->m_nodes[nodeIndex];
            memset(&node, 0, sizeof(node));
            node.m_x = ToBtVector3(SystemUnitToPhysicsUnit(desc.points[nodeIndex]));
            node.m_q = node.m_x;
            node.m_im = 1;
            node.m_leaf = softBody->m_ndbvt.insert(btDbvtVolume::FromCR(node.m_x, margin), &node);
            node.m_material = softBodyMaterial;
        }
        softBody->updateBounds();

        int maxidx = desc.points.Count();
        if (maxidx > 0) {
            btAlignedObjectArray<bool> chks;
            chks.resize(maxidx * maxidx, false);

            for (int i = 0; i < desc.pointIndexes.Count(); i += 3) {
                const int triIdx[] = { (int)desc.pointIndexes[i], (int)desc.pointIndexes[i + 1], (int)desc.pointIndexes[i + 2] };

#define IDX(_x_, _y_) ((_y_)*maxidx + (_x_))
                for (int j = 2, k = 0; k < 3; j = k++) {
                    if (!chks[IDX(triIdx[j], triIdx[k])]) {
                        chks[IDX(triIdx[j], triIdx[k])] = true;
                        chks[IDX(triIdx[k], triIdx[j])] = true;
                        softBody->appendLink(triIdx[j], triIdx[k]);
                    }
                }
#undef IDX
                softBody->appendFace(triIdx[0], triIdx[1], triIdx[2]);
            }

            // Generate clusters (K - mean).
            // Call this function if you want enable soft vs soft collision
            //softBody->generateClusters(16);

            // Generate bending constraints based on distance in the adjency graph.
            softBody->generateBendingConstraints(desc.bendingConstraintDistance, softBodyMaterial);

            // Randomize constraints to reduce solver bias.
            softBody->randomizeConstraints();
        }

        if (desc.pointWeights.Count() > 0) {
            for (int i = 0; i < desc.pointWeights.Count(); i++) {
                softBody->setMass(i, desc.pointWeights[i]);
            }
            // this must be AFTER softbody->setMass(), so that weights will be averaged.
            softBody->setTotalMass(desc.mass, true);

            softBody->setPose(true, true);
        }

        PhysSoftBody *sb = new PhysSoftBody(softBody, totalCentroid);
        sb->SetRestitution(desc.restitution);
        sb->SetFriction(desc.friction);
        sb->SetContinuousCollisionDetectionEnabled(desc.enableCCD);
        sb->bendingConstraintDistance = desc.bendingConstraintDistance;

        softBody->setUserPointer(sb);

        return sb;
    }

    BE_FATALERROR("Invalid physics collidable type: %i", (int)desc.type);
    return nullptr;
}

void PhysicsSystem::DestroyCollidable(PhysCollidable *collidable) {
    if (collidable->IsInWorld()) {
        collidable->RemoveFromWorld();
    }

    btCollisionShape *shape = collidable->collisionObject->getCollisionShape();
    if (shape) {
        if (shape->isCompound()) {
            delete shape;
        }
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
    case PhysConstraint::Type::SwingTwist:
        if (!desc.bodyA) {
            constraint = new PhysSwingTwistConstraint(desc.bodyB, desc.anchorInB, desc.axisInB);
        } else {
            constraint = new PhysSwingTwistConstraint(desc.bodyA, desc.anchorInA, desc.axisInA, desc.bodyB, desc.anchorInB, desc.axisInB);
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
