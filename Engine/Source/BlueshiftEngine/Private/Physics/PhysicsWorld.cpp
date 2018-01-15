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
#include "Core/Signal.h"
#include "Physics/Physics.h"
#include "Physics/Collider.h"
#include "ColliderInternal.h"
#include "PhysicsInternal.h"

BE_NAMESPACE_BEGIN

const SignalDef PhysicsWorld::SIG_PreStep("PhysicsWorld::PreStep", "f");
const SignalDef PhysicsWorld::SIG_PostStep("PhysicsWorld::PostStep", "f");

//#define USE_MLCP_SOLVER

static const int MAX_SUBSTEPS = 16;

static void PreTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    static_cast<PhysicsWorld *>(world->getWorldUserInfo())->PreStep(timeStep);
}

static void PostTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    static_cast<PhysicsWorld *>(world->getWorldUserInfo())->PostStep(timeStep);
}

class CollisionFilterCallback : public btOverlapFilterCallback {
public:
    // return true when pairs need collision
    virtual bool needBroadphaseCollision(btBroadphaseProxy *proxy0, btBroadphaseProxy *proxy1) const {
        const btCollisionObject *colObj0 = (const btCollisionObject *)proxy0->m_clientObject;
        const btCollisionObject *colObj1 = (const btCollisionObject *)proxy1->m_clientObject;

        const PhysCollidable *userColObj0 = (const PhysCollidable *)colObj0->getUserPointer();
        const PhysCollidable *userColObj1 = (const PhysCollidable *)colObj1->getUserPointer();

        const PhysicsWorld *pw = userColObj0->physicsWorld;

        if (pw->GetCollisionFilterMask(userColObj0->customFilterIndex) & BIT(userColObj1->customFilterIndex)) {
            return true;
        }

        /*if (pw->GetCollisionFilterMask(userColObj1->customFilterIndex) & BIT(userColObj0->customFilterIndex)) {
            return true;
        }*/

        return false;
    }
};

PhysicsWorld::PhysicsWorld() {
    // collision configuration contains default setup for memory, collision setup
    collisionConfiguration = new btDefaultCollisionConfiguration();
    //collisionConfiguration->setConvexConvexMultipointIterations();

    // use the default collision dispatcher. For parallel processing you can use a different dispatcher (see Extras/BulletMultiThreaded)
    collisionDispatcher = new btCollisionDispatcher(collisionConfiguration);

    // register GImpact collision algorithm
    //btGImpactCollisionAlgorithm::registerAlgorithm(collisionDispatcher);

    // broadphase dynamic bounding volume tree
#if 1
    broadphase = new btDbvtBroadphase();
#else
    btVector3 worldMin(-MeterToUnit(100), -MeterToUnit(100), -MeterToUnit(100));
    btVector3 worldMax(MeterToUnit(100), MeterToUnit(100), MeterToUnit(100));
    broadphase = new btAxisSweep3(worldMin, worldMax);
#endif

// direct MLCP solvers are useful when higher quality simulation is needed, for example in robotics. 
// The performance is less than the PGS / SI solver,
#ifdef USE_MLCP_SOLVER
    //btDantzigSolver *mlcp = new btDantzigSolver();
    btSolveProjectedGaussSeidel *mlcp = new btSolveProjectedGaussSeidel;
    solver = new btMLCPSolver(mlcp);
    dynamicsWorld = new btDiscreteDynamicsWorld(collisionDispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld ->getSolverInfo().m_minimumSolverBatchSize = 32; // for direct solver, it is better to solve multiple objects together, small batches have high overhead
#else
    // the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(collisionDispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld ->getSolverInfo().m_minimumSolverBatchSize = 1; // for direct solver it is better to have a small A matrix 
#endif
    
    // the polyhedral contact clipping can use either GJK or SAT test to find the separating axis
    //dynamicsWorld->getDispatchInfo().m_enableSatConvex = false;

    // NOTE: Bullet will clear all forces after the post-tick call
    dynamicsWorld->setInternalTickCallback(PreTickCallback, static_cast<void *>(this), true);
    dynamicsWorld->setInternalTickCallback(PostTickCallback, static_cast<void *>(this), false);

    ghostPairCallback = new btGhostPairCallback();
    dynamicsWorld->getPairCache()->setInternalGhostPairCallback(ghostPairCallback);

    filterCallback = new CollisionFilterCallback();
    dynamicsWorld->getPairCache()->setOverlapFilterCallback(filterCallback);

    //dynamicsWorld->getSolverInfo().m_numIterations = 10;
    dynamicsWorld->getSolverInfo().m_splitImpulse = false;
    //dynamicsWorld->setSynchronizeAllMotionStates(true);

    dynamicsWorld->setDebugDrawer(&physicsDebugDraw);

    memset(filterMasks, 0xFFFFFFFF, sizeof(filterMasks));

    timeDelta = 0;
    time = 0;

    frameRate = 50;
    maximumAllowedTimeStep = 1.0f / 5;

    SetGravity(Vec3(0, 0, 0));
}

PhysicsWorld::~PhysicsWorld() {
    ClearScene();

    SAFE_DELETE(collisionConfiguration);
    SAFE_DELETE(collisionDispatcher);
    SAFE_DELETE(broadphase);
    SAFE_DELETE(solver);
    SAFE_DELETE(dynamicsWorld);
    SAFE_DELETE(ghostPairCallback);
    SAFE_DELETE(filterCallback);
}

void PhysicsWorld::ClearScene() {
    // cleanup in the reverse order of creation/initialization
    for (int i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--) {
        btTypedConstraint *constraint = dynamicsWorld->getConstraint(i);
        PhysConstraint *userConstraint = (PhysConstraint *)constraint->getUserConstraintPtr();

        userConstraint->RemoveFromWorld();
        physicsSystem.DestroyConstraint(userConstraint);
    }

    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject *colObj = dynamicsWorld->getCollisionObjectArray()[i];
        PhysCollidable *userColObj = (PhysCollidable *)colObj->getUserPointer();
        
        userColObj->RemoveFromWorld();
        physicsSystem.DestroyCollidable(userColObj);
    }
    
    // reset some internal cached data in the broadphase
    dynamicsWorld->getBroadphase()->resetPool(dynamicsWorld->getDispatcher());
    dynamicsWorld->getConstraintSolver()->reset();

    time = 0;
}

void PhysicsWorld::StepSimulation(int frameTime) {
    //startFrameMsec = PlatformTime::Milliseconds();

    if (!physics_enable.GetBool()) {
        return;
    }

    timeDelta += frameTime * 0.001f;

    const float h = 1.0f / frameRate;

#if 0
    int steps = Math::Floor(timeDelta / h);
    if (steps > 0) {
        int maxSubSteps = Min(steps, MAX_SUBSTEPS);
        float timeStep = steps * h;
        int maxSubSteps = Math::Ceil(frameRate * maximumAllowedTimeStep);

        dynamicsWorld->stepSimulation(timeStep, maxSubSteps, h);

        timeDelta -= timeStep;
    }
#else
    int steps = Math::Ceil(timeDelta / h);
    int maxSubSteps = Max(1, (int)Math::Ceil(frameRate * maximumAllowedTimeStep));

    // The btDiscreteDynamicsWorld is guaranteed to call setWorldTransform() once per substep 
    // for every btRigidBody that : has a MotionState AND is active AND is not KINEMATIC or STATIC.

    // maxSubSteps > 0 이면 motion state 의 getWorldTransform() 은 interpolation 된 결과를 반환한다.
    dynamicsWorld->stepSimulation(timeDelta, maxSubSteps, h);
    
    timeDelta = 0.0f;
#endif

    //fc.frameTime = PlatformTime::Milliseconds() - startFrameMsec;
}

const Vec3 PhysicsWorld::GetGravity() const {
    if (!dynamicsWorld) {
        return Vec3::zero;
    }

    btVector3 gravity = dynamicsWorld->getGravity();
    return Vec3(gravity.x(), gravity.y(), gravity.z());
}

void PhysicsWorld::SetGravity(const Vec3 &gravityAcceleration) {
    if (!dynamicsWorld) {
        return;
    }

    dynamicsWorld->setGravity(ToBtVector3(gravityAcceleration));
}

uint32_t PhysicsWorld::GetCollisionFilterMask(int index) const {
    assert(index >= 0 && index < COUNT_OF(filterMasks));

    return filterMasks[index];
}

void PhysicsWorld::SetCollisionFilterMask(int index, uint32_t mask) {
    assert(index >= 0 && index < COUNT_OF(filterMasks));

    filterMasks[index] = mask;
}

bool PhysicsWorld::RayCast(const PhysCollidable *me, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, CastResult &trace) const {
    return ClosestRayTest(me ? me->collisionObject : nullptr, start, end, filterGroup, filterMask, trace);
}

bool PhysicsWorld::RayCastAll(const PhysCollidable *me, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, Array<CastResult> &resultArray) const {
    return AllHitsRayTest(me ? me->collisionObject : nullptr, start, end, filterGroup, filterMask, resultArray);
}

bool PhysicsWorld::ConvexCast(const PhysCollidable *me, const Collider *collider, const Mat3 &axis, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, CastResult &trace) const {
    btTransform shapeTransform;

    btCollisionShape *shape = collider->shape;
    if (shape->isCompound()) {
        btCompoundShape *compoundShape = static_cast<btCompoundShape *>(shape);
        if (compoundShape->getNumChildShapes() != 1) {
            BE_WARNLOG(L"PhysicsWorld::ConvexCast: multiple compound shape is not allowed\n");	
            return false;
        }

        shape = compoundShape->getChildShape(0);
        shapeTransform = compoundShape->getChildTransform(0).inverse();
    } else {
        const Vec3 centroid = collider->GetCentroid();
    
        shapeTransform.setIdentity();
        shapeTransform.setOrigin(ToBtVector3(centroid));
    }

    if (!shape->isConvex()) {
        BE_WARNLOG(L"PhysicsWorld::ConvexCast: non convex collision shape is not allowed\n");
        return false;
    }

    return ClosestConvexTest(me ? me->collisionObject : nullptr, static_cast<btConvexShape *>(shape), shapeTransform, axis, start, end, filterGroup, filterMask, trace);
}

bool PhysicsWorld::ClosestRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, CastResult &trace) const {
    if (origin.Equals(dest)) {
        return false;
    }
    
    class MyClosestRayResultCallback : public btCollisionWorld::ClosestRayResultCallback {
    public:
        MyClosestRayResultCallback(const btCollisionObject *me, const btVector3 &rayFromWorld, const btVector3 &rayToWorld) : 
            ClosestRayResultCallback(rayFromWorld, rayToWorld) {
            m_me = me;
        }

        virtual	btScalar addSingleResult(btCollisionWorld::LocalRayResult &rayResult, bool normalInWorldSpace) {
            if (rayResult.m_collisionObject == m_me) {
                return 1.0f;
            }
            m_localShapeInfo = rayResult.m_localShapeInfo;
            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }

        btCollisionWorld::LocalShapeInfo *m_localShapeInfo;
        const btCollisionObject *m_me;
    };

    btVector3 rayFromWorld(origin.x, origin.y, origin.z);
    btVector3 rayToWorld(dest.x, dest.y, dest.z);

    MyClosestRayResultCallback cb(me, rayFromWorld, rayToWorld);

    cb.m_collisionFilterGroup = filterGroup;
    cb.m_collisionFilterMask = filterMask;

    dynamicsWorld->rayTest(rayFromWorld, rayToWorld, cb);

    if (cb.hasHit()) {
        trace.hitObject = (PhysCollidable *)(cb.m_collisionObject->getUserPointer());
        trace.point = Vec3(cb.m_hitPointWorld.x(), cb.m_hitPointWorld.y(), cb.m_hitPointWorld.z());
        trace.normal = Vec3(cb.m_hitNormalWorld.x(), cb.m_hitNormalWorld.y(), cb.m_hitNormalWorld.z());
        trace.fraction = cb.m_closestHitFraction;
        trace.endpos = origin + trace.fraction * (dest - origin);

        const btCollisionShape *shape = cb.m_collisionObject->getCollisionShape();
        if (shape) {
            if (shape->getShapeType() == MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE) {
                btMultimaterialTriangleMeshShape *_shape = (btMultimaterialTriangleMeshShape *)shape;
                const CollisionMaterial *props = (CollisionMaterial *)_shape->getMaterialProperties(cb.m_localShapeInfo->m_shapePart, cb.m_localShapeInfo->m_triangleIndex);
                trace.surfaceFlags = props->surfaceFlags;
            } else {
                trace.surfaceFlags = 0;
            }
        } else {
            trace.surfaceFlags = 0;
        }
        return true;
    } else {
        trace.hitObject = nullptr;
        trace.fraction = 1.0f;
        trace.surfaceFlags = 0;
    }

    return false;
}

bool PhysicsWorld::AllHitsRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, Array<CastResult> &resultArray) const {
    class MyAllHitsRayResultCallback : public btCollisionWorld::AllHitsRayResultCallback {
    public:
        MyAllHitsRayResultCallback(const btCollisionObject *me, const btVector3 &rayFromWorld, const btVector3 &rayToWorld) : 
            AllHitsRayResultCallback(rayFromWorld, rayToWorld) {
            m_me = me;
        }

        virtual	btScalar addSingleResult(btCollisionWorld::LocalRayResult &rayResult, bool normalInWorldSpace) {
            if (rayResult.m_collisionObject == m_me) {
                return 1.0f;
            }
            m_localShapeInfo = rayResult.m_localShapeInfo;
            return AllHitsRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }

        btCollisionWorld::LocalShapeInfo *m_localShapeInfo;
        const btCollisionObject *m_me;
    };

    btVector3 rayFromWorld(origin.x, origin.y, origin.z);
    btVector3 rayToWorld(dest.x, dest.y, dest.z);

    MyAllHitsRayResultCallback cb(me, rayFromWorld, rayToWorld);

    cb.m_collisionFilterGroup = filterGroup;
    cb.m_collisionFilterMask = filterMask;

    dynamicsWorld->rayTest(rayFromWorld, rayToWorld, cb);

    if (cb.hasHit()) {
        for (int i = 0; i < cb.m_collisionObjects.size(); i++) {
            CastResult trace;
            trace.hitObject = (PhysCollidable *)(cb.m_collisionObjects[i]->getUserPointer());
            trace.point = Vec3(cb.m_hitPointWorld[i].x(), cb.m_hitPointWorld[i].y(), cb.m_hitPointWorld[i].z());
            trace.normal = Vec3(cb.m_hitNormalWorld[i].x(), cb.m_hitNormalWorld[i].y(), cb.m_hitNormalWorld[i].z());
            trace.fraction = cb.m_hitFractions[i];
            trace.endpos = origin + trace.fraction * (dest - origin);

            const btCollisionShape *shape = cb.m_collisionObject->getCollisionShape();
            if (shape) {
                if (shape->getShapeType() == MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE) {
                    btMultimaterialTriangleMeshShape *_shape = (btMultimaterialTriangleMeshShape *)shape;
                    const CollisionMaterial *props = (CollisionMaterial *)_shape->getMaterialProperties(cb.m_localShapeInfo->m_shapePart, cb.m_localShapeInfo->m_triangleIndex);
                    trace.surfaceFlags = props->surfaceFlags;
                } else {
                    trace.surfaceFlags = 0;
                }
            } else {
                trace.surfaceFlags = 0;
            }

            resultArray.Append(trace);
        }
        return true;
    }
    return false;
}

bool PhysicsWorld::ClosestConvexTest(const btCollisionObject *me, const btConvexShape *convexShape, const btTransform &shapeTransform, const Mat3 &axis, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, CastResult &trace) const {
    class MyClosestConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback {
    public:
        MyClosestConvexResultCallback(const btCollisionObject *me, const btVector3 &rayFromWorld, const btVector3 &rayToWorld) : 
            ClosestConvexResultCallback(rayFromWorld, rayToWorld) {
                m_me = me;
            }

        virtual	btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace) {
            if (convexResult.m_hitCollisionObject == m_me) {
                return 1.0f;
            }

            const btCollisionShape *shape = convexResult.m_hitCollisionObject->getCollisionShape();
            /*if (shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE) {
                const btTriangleMeshShape *triMeshShape = static_cast<const btTriangleMeshShape *>(shape);
                const btTriangleIndexVertexArray *triIndexVertexArray = static_cast<const btTriangleIndexVertexArray *>(triMeshShape->getMeshInterface());
                const btIndexedMesh &indexedMesh = triIndexVertexArray->getIndexedMeshArray()[convexResult.m_localShapeInfo->m_shapePart];
                 
                int indexTypeSize = indexedMesh.m_indexType == PHY_INTEGER ? 4 : 2;
                const byte *idxbase = indexedMesh.m_triangleIndexBase + indexedMesh.m_triangleIndexStride * convexResult.m_localShapeInfo->m_triangleIndex;
                int idx[3];

                if (indexTypeSize == 4) {
                    const int32_t *idxptr = (int32_t *)idxbase;

                    idx[0] = *idxptr++;
                    idx[1] = *idxptr++;	
                    idx[2] = *idxptr++;	
                } else {
                    const int16_t *idxptr = (int16_t *)idxbase;

                    idx[0] = *idxptr++;
                    idx[1] = *idxptr++;	
                    idx[2] = *idxptr++;	
                }

                Vec3 v0 = *(Vec3 *)(indexedMesh.m_vertexBase + indexedMesh.m_vertexStride * idx[0]);
                Vec3 v1 = *(Vec3 *)(indexedMesh.m_vertexBase + indexedMesh.m_vertexStride * idx[1]);
                Vec3 v2 = *(Vec3 *)(indexedMesh.m_vertexBase + indexedMesh.m_vertexStride * idx[2]);

                const Vec3 side0 = v1 - v0;
                const Vec3 side1 = v2 - v0;

                Vec3 normal = side1.Cross(side0);
                normal.Normalize();
                
                convexResult.m_hitNormalLocal.setValue(normal.x, normal.y, normal.z);
            }*/

            m_localShapeInfo = convexResult.m_localShapeInfo;
            return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
        }

        btCollisionWorld::LocalShapeInfo *m_localShapeInfo;
        const btCollisionObject *m_me;
    };

    if (origin.DistanceSqr(dest) < Math::FloatEpsilon) {
        trace.hitObject = nullptr;
        trace.fraction = 1.0f;
        trace.endpos = origin;
        trace.surfaceFlags = 0;
        return false;
    }

    Quat q = axis.ToQuat();

    btTransform fromTrans;
    fromTrans.setRotation(ToBtQuaternion(q));
    fromTrans.setOrigin(ToBtVector3(origin));
    fromTrans.mult(shapeTransform, fromTrans);

    btTransform toTrans;
    toTrans.setRotation(ToBtQuaternion(q));
    toTrans.setOrigin(ToBtVector3(dest));
    toTrans.mult(shapeTransform, toTrans);

    MyClosestConvexResultCallback cb(me, fromTrans.getOrigin(), toTrans.getOrigin());

    cb.m_collisionFilterGroup = filterGroup;
    cb.m_collisionFilterMask = filterMask;

    dynamicsWorld->convexSweepTest(convexShape, fromTrans, toTrans, cb, dynamicsWorld->getDispatchInfo().m_allowedCcdPenetration);
    
    if (cb.hasHit()) {
        trace.hitObject = (PhysCollidable *)(cb.m_hitCollisionObject->getUserPointer());
        trace.point = Vec3(cb.m_hitPointWorld.x(), cb.m_hitPointWorld.y(), cb.m_hitPointWorld.z());
        trace.normal = Vec3(cb.m_hitNormalWorld.x(), cb.m_hitNormalWorld.y(), cb.m_hitNormalWorld.z());
        trace.fraction = cb.m_closestHitFraction;
        trace.endpos = origin + trace.fraction * (dest - origin);

        const btCollisionShape *shape = cb.m_hitCollisionObject->getCollisionShape();
        if (shape) {
            if (shape->getShapeType() == MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE) {
                btMultimaterialTriangleMeshShape *_shape = (btMultimaterialTriangleMeshShape *)shape;
                const CollisionMaterial *props = (CollisionMaterial *)_shape->getMaterialProperties(cb.m_localShapeInfo->m_shapePart, cb.m_localShapeInfo->m_triangleIndex);
                trace.surfaceFlags = props->surfaceFlags;
            } else {
                trace.surfaceFlags = 0;
            }
        } else {
            trace.surfaceFlags = 0;
        }
        return true;
    } else {
        trace.hitObject = nullptr;
        trace.fraction = 1.0f;
        trace.endpos = dest;
        trace.surfaceFlags = 0;
    }

    return false;
}

void PhysicsWorld::DebugDraw() {
    if (!dynamicsWorld) {
        return;
    }

    dynamicsWorld->debugDrawWorld();
}

void PhysicsWorld::PreStep(float timeStep) {
    EmitSignal(&SIG_PreStep, timeStep);
}

void PhysicsWorld::PostStep(float timeStep) {
    time += timeStep;

    ProcessCollision();

    EmitSignal(&SIG_PostStep, timeStep);
}

void PhysicsWorld::ProcessCollision() {
    int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold *contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);

        int numContacts = contactManifold->getNumContacts();
        if (numContacts > 0) {
            const btCollisionObject *objA = static_cast<const btCollisionObject *>(contactManifold->getBody0());
            const btCollisionObject *objB = static_cast<const btCollisionObject *>(contactManifold->getBody1());

            const PhysCollidable *a = reinterpret_cast<PhysCollidable *>(objA->getUserPointer());
            const PhysCollidable *b = reinterpret_cast<PhysCollidable *>(objB->getUserPointer());

            //const short colMaskA = a->GetCollisionFilterMask();
            //const short colMaskB = b->GetCollisionFilterMask();

            /*if (!(GetCollisionFilterMask(a->customFilterIndex) & BIT(b->customFilterIndex))) {
                continue;
            }*/

            PhysCollisionPair pair = PhysCollisionPair(a, b);

            PhysCollisionListener *listenerA = a->GetCollisionListener();
            PhysCollisionListener *listenerB = b->GetCollisionListener();

            if (!listenerA && !listenerB) {
                continue;
            }

            for (int j = 0; j < numContacts; j++) {
                btManifoldPoint &pt = contactManifold->getContactPoint(j);
                if (pt.getDistance() < 0.0f) {
                    const btVector3 &ptA = pt.getPositionWorldOnA();
                    const btVector3 &ptB = pt.getPositionWorldOnB();
                    const btVector3 &normalOnB = pt.m_normalWorldOnB;
                    const btScalar distance = pt.getDistance();
                    const btScalar impulse = pt.getAppliedImpulse();

                    if (listenerA) {
                        listenerA->Collide(a, b, Vec3(ptB.x(), ptB.y(), ptB.z()), Vec3(normalOnB.x(), normalOnB.y(), normalOnB.z()), distance, impulse);
                    }
                    if (listenerB) {
                        listenerB->Collide(b, a, Vec3(ptA.x(), ptA.y(), ptA.z()), Vec3(-normalOnB.x(), -normalOnB.y(), -normalOnB.z()), distance, impulse);
                    }
                }
            }
        }
    }
}

void PhysicsWorld::CheckModifiedCVars() {
    if (physics_showWireframe.IsModified()) {
        physics_showWireframe.ClearModified();

        int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
        if (physics_showWireframe.GetBool()) {
            debugMode |= btIDebugDraw::DBG_DrawWireframe;
        } else {
            debugMode &= ~btIDebugDraw::DBG_DrawWireframe;
        }

        dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
    }

    if (physics_showAABB.IsModified()) {
        physics_showAABB.ClearModified();

        int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
        if (physics_showAABB.GetBool()) {
            debugMode |= btIDebugDraw::DBG_DrawAabb;
        } else {
            debugMode &= ~btIDebugDraw::DBG_DrawAabb;
        }

        dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
    }

    if (physics_showContactPoints.IsModified()) {
        physics_showContactPoints.ClearModified();

        int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
        if (physics_showContactPoints.GetBool()) {
            debugMode |= btIDebugDraw::DBG_DrawContactPoints;
        } else {
            debugMode &= ~btIDebugDraw::DBG_DrawContactPoints;
        }

        dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
    }

    if (physics_showNormals.IsModified()) {
        physics_showNormals.ClearModified();

        int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
        if (physics_showNormals.GetBool()) {
            debugMode |= btIDebugDraw::DBG_DrawNormals;
        } else {
            debugMode &= ~btIDebugDraw::DBG_DrawNormals;
        }

        dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
    }

    if (physics_showConstraints.IsModified()) {
        physics_showConstraints.ClearModified();

        int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
        if (physics_showConstraints.GetBool()) {
            debugMode |= (btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits);
        } else {
            debugMode &= ~(btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits);
        }

        dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
    }

    if (physics_noDeactivation.IsModified()) {
        physics_noDeactivation.ClearModified();

        int debugMode = dynamicsWorld->getDebugDrawer()->getDebugMode();
        if (physics_noDeactivation.GetBool()) {
            debugMode |= btIDebugDraw::DBG_NoDeactivation;
        } else {
            debugMode &= ~btIDebugDraw::DBG_NoDeactivation;
        }

        dynamicsWorld->getDebugDrawer()->setDebugMode(debugMode);
    }

    if (physics_enableCCD.IsModified()) {
        physics_enableCCD.ClearModified();

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

BE_NAMESPACE_END
