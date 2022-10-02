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
#include "Profiler/Profiler.h"

//#define DETERMINISTIC

BE_NAMESPACE_BEGIN

const SignalDef PhysicsWorld::SIG_PreStep("PhysicsWorld::PreStep", "f");
const SignalDef PhysicsWorld::SIG_PostStep("PhysicsWorld::PostStep", "f");

static const int MAX_SUBSTEPS = 16;

static void PreTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    static_cast<PhysicsWorld *>(world->getWorldUserInfo())->PreStep(timeStep);
}

static void PostTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    static_cast<PhysicsWorld *>(world->getWorldUserInfo())->PostStep(timeStep);
}

/*class CollisionFilterCallback : public btOverlapFilterCallback {
public:
    // return true when pairs need collision
    virtual bool needBroadphaseCollision(btBroadphaseProxy *proxy0, btBroadphaseProxy *proxy1) const {
        const btCollisionObject *colObj0 = (const btCollisionObject *)proxy0->m_clientObject;
        const btCollisionObject *colObj1 = (const btCollisionObject *)proxy1->m_clientObject;

        const PhysCollidable *userColObj0 = (const PhysCollidable *)colObj0->getUserPointer();
        const PhysCollidable *userColObj1 = (const PhysCollidable *)colObj1->getUserPointer();

        const PhysicsWorld *physicsWorld = userColObj0->physicsWorld;

        if (physicsWorld->GetCollisionFilterMask(userColObj0->customFilterIndex) & BIT(userColObj1->customFilterIndex)) {
            return true;
        }

        return false;
    }
};*/

PhysicsWorld::PhysicsWorld() {
    // collision configuration contains default setup for memory, collision setup
    collisionConfiguration = new btDefaultCollisionConfiguration();
    //collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();

    //collisionConfiguration->setConvexConvexMultipointIterations();

    // use the default collision dispatcher. For parallel processing you can use a different dispatcher (see Extras/BulletMultiThreaded)
    collisionDispatcher = new btCollisionDispatcher(collisionConfiguration);

    // register GImpact collision algorithm
    //btGImpactCollisionAlgorithm::registerAlgorithm(collisionDispatcher);

    // broadphase dynamic bounding volume tree
#if 1
    broadphase = new btDbvtBroadphase();
#else
    btVector3 worldMin(-100, -100, -100);
    btVector3 worldMax(+100, +100, +100);
    broadphase = new btAxisSweep3(worldMin, worldMax);
#endif

    // the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
    solverType = ConstraintSolver::SequentialImpulseSolver;
    constraintSolver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(collisionDispatcher, broadphase, constraintSolver, collisionConfiguration);

    //softBodySolver = new btDefaultSoftBodySolver;
    //dynamicsWorld = new btSoftRigidDynamicsWorld(collisionDispatcher, broadphase, solver, collisionConfiguration, softBodySolver);

    // for direct solver it is better to have a small A matrix 
    //dynamicsWorld ->getSolverInfo().m_minimumSolverBatchSize = 1; 
    
    // the polyhedral contact clipping can use either GJK or SAT test to find the separating axis
    //dynamicsWorld->getDispatchInfo().m_enableSatConvex = false;

    // NOTE: Bullet will clear all forces after the post-tick call
    dynamicsWorld->setInternalTickCallback(PreTickCallback, static_cast<void *>(this), true);
    dynamicsWorld->setInternalTickCallback(PostTickCallback, static_cast<void *>(this), false);

    ghostPairCallback = new btGhostPairCallback();
    dynamicsWorld->getPairCache()->setInternalGhostPairCallback(ghostPairCallback);

    //filterCallback = new CollisionFilterCallback();
    //dynamicsWorld->getPairCache()->setOverlapFilterCallback(filterCallback);

#ifdef DETERMINISTIC
    dynamicsWorld->getSimulationIslandManager()->setSplitIslands(false);

    dynamicsWorld->getSolverInfo().m_solverMode &= ~SOLVER_RANDMIZE_ORDER;
#else
    dynamicsWorld->getSolverInfo().m_solverMode |= SOLVER_RANDMIZE_ORDER;
#endif

#if 0
    //m_splitImpulse removes the penetration resolution from the applied impulse, otherwise objects might fracture due to deep penetrations.
    dynamicsWorld->getSolverInfo().m_splitImpulse = true;
#else
    dynamicsWorld->getSolverInfo().m_splitImpulse = false;
#endif
    //dynamicsWorld->setSynchronizeAllMotionStates(true);

    dynamicsWorld->setDebugDrawer(&physicsDebugDraw);

    memset(filterMasks, 0xFFFFFFFF, sizeof(filterMasks));

    SetFrameRate(50);
    maximumAllowedTimeStep = 0.2f;

    SetGravity(Vec3(0, 0, 0));
}

PhysicsWorld::~PhysicsWorld() {
    ClearScene();

    SAFE_DELETE(collisionConfiguration);
    SAFE_DELETE(collisionDispatcher);
    SAFE_DELETE(broadphase);
    SAFE_DELETE(constraintSolver);
    SAFE_DELETE(softBodySolver);
    SAFE_DELETE(dynamicsWorld);
    SAFE_DELETE(ghostPairCallback);
    //SAFE_DELETE(filterCallback);
}

void PhysicsWorld::ClearScene() {
    // Clean up in the reverse order of creation/initialization
    for (int i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--) {
        btTypedConstraint *constraint = dynamicsWorld->getConstraint(i);
        PhysConstraint *userConstraint = (PhysConstraint *)constraint->getUserConstraintPtr();

        userConstraint->RemoveFromWorld();
        physicsSystem.DestroyConstraint(userConstraint);
    }

    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject *colObj = dynamicsWorld->getCollisionObjectArray()[i];
        PhysCollidable *userColObj = (PhysCollidable *)colObj->getUserPointer();

        // Removed cached contact points (this is not necessary if all objects have been removed from the dynamics world)
        //dynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(colObj->getBroadphaseHandle(), dynamicsWorld->getDispatcher());
        
        userColObj->RemoveFromWorld();
        physicsSystem.DestroyCollidable(userColObj);
    }

    Reset();
}

void PhysicsWorld::Reset() {
    // Reset some internal cached data in the broadphase
    dynamicsWorld->getBroadphase()->resetPool(dynamicsWorld->getDispatcher());
    dynamicsWorld->getConstraintSolver()->reset();

    time = 0;
    accumulatedTimeDelta = 0;
}

void PhysicsWorld::SetConstraintSolver(ConstraintSolver::Enum solverType) {
    // Direct MLCP solvers are useful when higher quality simulation is needed, for example in robotics. 
    // The performance is less than the SI solver,
    // NOTE: rolling friction is not working with MLCP solver ! (bullet bug)
    switch (solverType) {
    case ConstraintSolver::SequentialImpulseSolver:
        constraintSolver = new btSequentialImpulseConstraintSolver;
        break;
    case ConstraintSolver::NNCGSolver:
        constraintSolver = new btNNCGConstraintSolver;
        break;
    case ConstraintSolver::ProjectedGaussSeidelSolver:
        constraintSolver = new btMLCPSolver(new btSolveProjectedGaussSeidel);
        break;
    case ConstraintSolver::DantzigSolver:
        constraintSolver = new btMLCPSolver(new btDantzigSolver());
        break;
    default:
        assert(0);
        break;
    }

    this->solverType = solverType;

    dynamicsWorld->setConstraintSolver(constraintSolver);

    // for direct solver it is better to have a small A matrix 
    //dynamicsWorld ->getSolverInfo().m_minimumSolverBatchSize = 128;
}

int PhysicsWorld::GetConstraintSolverIterations() const {
    return dynamicsWorld->getSolverInfo().m_numIterations;
}

void PhysicsWorld::SetConstraintSolverIterations(int iterationCount) {
    dynamicsWorld->getSolverInfo().m_numIterations = iterationCount;
}

void PhysicsWorld::SetFrameRate(int frameRate) {
    this->frameRate = frameRate; 
    this->frameTimeDelta = 1.0f / frameRate; 
}

void PhysicsWorld::StepSimulation(float frameTime) {
    BE_PROFILE_CPU_SCOPE_STATIC("PhysicsWorld::StepSimulation");

    if (!physics_enable.GetBool()) {
        return;
    }

    accumulatedTimeDelta += frameTime;

#if 0
    int steps = Math::Floor(accumulatedTimeDelta / h);
    if (steps > 0) {
        int maxSubSteps = Min(steps, MAX_SUBSTEPS);
        float timeStep = steps * h;
        int maxSubSteps = Math::Ceil(frameRate * maximumAllowedTimeStep);

        dynamicsWorld->stepSimulation(timeStep, maxSubSteps, h);

        accumulatedTimeDelta -= timeStep;
    }
#else
    int steps = Math::Ceil(accumulatedTimeDelta / frameTimeDelta);
    int maxSubSteps = Max(1, (int)Math::Ceil(frameRate * maximumAllowedTimeStep));

    // The btDiscreteDynamicsWorld is guaranteed to call setWorldTransform() once per substep 
    // for every btRigidBody that : has a MotionState AND is active AND is not KINEMATIC or STATIC.

    // If maxSubSteps > 0, getWorldTransform() of motion state will have interpolated value.
    dynamicsWorld->stepSimulation(accumulatedTimeDelta, maxSubSteps, frameTimeDelta);
    
    accumulatedTimeDelta = 0.0f;
#endif
}

const Vec3 PhysicsWorld::GetGravity() const {
    if (!dynamicsWorld) {
        return Vec3::zero;
    }

    btVector3 gravity = dynamicsWorld->getGravity();
    return PhysicsUnitToSystemUnit(ToVec3(gravity));
}

void PhysicsWorld::SetGravity(const Vec3 &gravityAcceleration) {
    if (!dynamicsWorld) {
        return;
    }

    dynamicsWorld->setGravity(ToBtVector3(SystemUnitToPhysicsUnit(gravityAcceleration)));
}

int PhysicsWorld::GetCollisionFilterMask(int bit) const {
    assert(bit >= 0 && bit < COUNT_OF(filterMasks) - 6);

    return filterMasks[bit];
}

void PhysicsWorld::SetCollisionFilterMask(int bit, int mask) {
    assert(bit >= 0 && bit < COUNT_OF(filterMasks) - 6);

    filterMasks[bit] = mask;
}

bool PhysicsWorld::CheckBox(const Vec3 &boxCenter, const Vec3 &boxExtents, int mask) const {
    Array<PhysCollidable *> colliders;
    return OverlapBox(boxCenter, boxExtents, mask, colliders) > 0;
}

bool PhysicsWorld::CheckSphere(const Vec3 &sphereCenter, float sphereRadius, int mask) const {
    Array<PhysCollidable *> colliders;
    return OverlapSphere(sphereCenter, sphereRadius, mask, colliders) > 0;
}

int PhysicsWorld::OverlapBox(const Vec3 &boxCenter, const Vec3 &boxExtents, int mask, Array<PhysCollidable *> &colliders) const {
    int internalGroup = btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }

    btVector3 halfExtents = ToBtVector3(SystemUnitToPhysicsUnit(boxExtents));
    btBoxShape boxShape(halfExtents);
    btTransform worldTransform(btMatrix3x3::getIdentity(), ToBtVector3(SystemUnitToPhysicsUnit(boxCenter)));

    btCollisionObject collisionObject;
    collisionObject.setCollisionShape(&boxShape);
    collisionObject.setWorldTransform(worldTransform);

    OverlapShape(&collisionObject, internalGroup, internalMask, colliders);

    return colliders.Count();
}

int PhysicsWorld::OverlapSphere(const Vec3 &sphereCenter, float sphereRadius, int mask, Array<PhysCollidable *> &colliders) const {
    int internalGroup = btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }

    btSphereShape sphereShape(SystemUnitToPhysicsUnit(sphereRadius));
    btTransform worldTransform(btMatrix3x3::getIdentity(), ToBtVector3(SystemUnitToPhysicsUnit(sphereCenter)));

    btCollisionObject collisionObject;
    collisionObject.setCollisionShape(&sphereShape);
    collisionObject.setWorldTransform(worldTransform);

    OverlapShape(&collisionObject, internalGroup, internalMask, colliders);

    return colliders.Count();
}

bool PhysicsWorld::RayCast(const Vec3 &start, const Vec3 &end, int mask, CastResult &trace) const {
    int internalGroup = btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }
    return ClosestRayTest(nullptr, start, end, internalGroup, internalMask, trace);
}

bool PhysicsWorld::RayCastAll(const Vec3 &start, const Vec3 &end, int mask, Array<CastResult> &traceList) const {
    int internalGroup = btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }
    return AllHitsRayTest(nullptr, start, end, internalGroup, internalMask, traceList);
}

bool PhysicsWorld::BoxCast(const Vec3 &boxCenter, const Vec3 &boxExtents, const Mat3& axis, const Vec3 &end, int mask, CastResult &trace) const {
    int internalGroup = btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }

    btTransform shapeTransform;
    shapeTransform.setBasis(btMatrix3x3::getIdentity());
    shapeTransform.setOrigin(btVector3(0, 0, 0));

    btVector3 halfExtents = ToBtVector3(SystemUnitToPhysicsUnit(boxExtents));
    btBoxShape boxShape(halfExtents);

    return ClosestConvexTest(nullptr, static_cast<btConvexShape *>(&boxShape), shapeTransform, axis, boxCenter, end, internalGroup, internalMask, trace);
}

bool PhysicsWorld::BoxCastAll(const Vec3 &boxCenter, const Vec3 &boxExtents, const Mat3 &axis, const Vec3 &end, int mask, Array<CastResult> &traceList) const {
    int internalGroup = btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }

    btTransform shapeTransform;
    shapeTransform.setBasis(btMatrix3x3::getIdentity());
    shapeTransform.setOrigin(btVector3(0, 0, 0));

    btVector3 halfExtents = ToBtVector3(SystemUnitToPhysicsUnit(boxExtents));
    btBoxShape boxShape(halfExtents);

    return AllHitsConvexTest(nullptr, static_cast<btConvexShape *>(&boxShape), shapeTransform, axis, boxCenter, end, internalGroup, internalMask, traceList);
}

bool PhysicsWorld::SphereCast(const Vec3 &sphereCenter, float sphereRadius, const Vec3 &end, int mask, CastResult &trace) const {
    int internalGroup = btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }

    btTransform shapeTransform;
    shapeTransform.setBasis(btMatrix3x3::getIdentity());
    shapeTransform.setOrigin(btVector3(0, 0, 0));

    btSphereShape sphereShape(sphereRadius);

    return ClosestConvexTest(nullptr, static_cast<btConvexShape *>(&sphereShape), shapeTransform, Mat3::identity, sphereCenter, end, internalGroup, internalMask, trace);
}

bool PhysicsWorld::SphereCastAll(const Vec3 &sphereCenter, float sphereRadius, const Vec3 &end, int mask, Array<CastResult> &traceList) const {
    int internalGroup = btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }

    btTransform shapeTransform;
    shapeTransform.setBasis(btMatrix3x3::getIdentity());
    shapeTransform.setOrigin(btVector3(0, 0, 0));

    btSphereShape sphereShape(sphereRadius);

    return AllHitsConvexTest(nullptr, static_cast<btConvexShape *>(&sphereShape), shapeTransform, Mat3::identity, sphereCenter, end, internalGroup, internalMask, traceList);
}

bool PhysicsWorld::ConvexCast(const PhysCollidable *me, const Collider *collider, const Mat3 &axis, const Vec3 &start, const Vec3 &end, int mask, CastResult &trace) const {
    btTransform shapeTransform;

    btCollisionShape *shape = collider->shape;
    if (shape->isCompound()) {
        btCompoundShape *compoundShape = static_cast<btCompoundShape *>(shape);
        if (compoundShape->getNumChildShapes() != 1) {
            BE_WARNLOG("PhysicsWorld::ConvexCast: multiple compound shape is not allowed\n");
            return false;
        }

        shape = compoundShape->getChildShape(0);
        shapeTransform = compoundShape->getChildTransform(0).inverse();
    } else {
        const Vec3 centroid = collider->GetCentroid();
    
        shapeTransform.setBasis(btMatrix3x3::getIdentity());
        shapeTransform.setOrigin(ToBtVector3(SystemUnitToPhysicsUnit(centroid)));
    }

    if (!shape->isConvex()) {
        BE_WARNLOG("PhysicsWorld::ConvexCast: non convex collision shape is not allowed\n");
        return false;
    }

    int internalGroup = me ? (((BIT(me->collisionFilterBit) & ~1) << 5) | (BIT(me->collisionFilterBit) & 1)) : btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }

    return ClosestConvexTest(me ? me->collisionObject : nullptr, static_cast<btConvexShape *>(shape), shapeTransform, axis, start, end, internalGroup, internalMask, trace);
}

bool PhysicsWorld::ConvexCastAll(const PhysCollidable *me, const Collider *collider, const Mat3 &axis, const Vec3 &start, const Vec3 &end, int mask, Array<CastResult> &traceList) const {
    btTransform shapeTransform;

    btCollisionShape *shape = collider->shape;
    if (shape->isCompound()) {
        btCompoundShape *compoundShape = static_cast<btCompoundShape *>(shape);
        if (compoundShape->getNumChildShapes() != 1) {
            BE_WARNLOG("PhysicsWorld::ConvexCastAll: multiple compound shape is not allowed\n");
            return false;
        }

        shape = compoundShape->getChildShape(0);
        shapeTransform = compoundShape->getChildTransform(0).inverse();
    }
    else {
        const Vec3 centroid = collider->GetCentroid();

        shapeTransform.setBasis(btMatrix3x3::getIdentity());
        shapeTransform.setOrigin(ToBtVector3(SystemUnitToPhysicsUnit(centroid)));
    }

    if (!shape->isConvex()) {
        BE_WARNLOG("PhysicsWorld::ConvexCastAll: non convex collision shape is not allowed\n");
        return false;
    }

    int internalGroup = me ? (((BIT(me->collisionFilterBit) & ~1) << 5) | (BIT(me->collisionFilterBit) & 1)) : btBroadphaseProxy::DefaultFilter;
    int internalMask = (mask & ~1) << 5;
    if (mask & BIT(0)) { // & BIT(TagLayerSettings::DefaultLayer)
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter);
    }

    return AllHitsConvexTest(me ? me->collisionObject : nullptr, static_cast<btConvexShape *>(shape), shapeTransform, axis, start, end, internalGroup, internalMask, traceList);
}

void PhysicsWorld::OverlapShape(btCollisionObject *collisionObject, int filterGroup, int filterMask, Array<PhysCollidable *> &colliders) const {
    class MyContactResultCallback : public btCollisionWorld::ContactResultCallback {
    public:
        virtual btScalar addSingleResult(btManifoldPoint &cp,
            const btCollisionObjectWrapper *colObj0Wrap, int partId0, int index0,
            const btCollisionObjectWrapper *colObj1Wrap, int partId1, int index1) {
            const btCollisionObject *collisionObject = colObj1Wrap->getCollisionObject();
            if (collisionObject->getUserPointer()) {
                m_colliders->Append((PhysCollidable *)collisionObject->getUserPointer());
            }
            return 1.0f;
        }

        Array<PhysCollidable *> *m_colliders;
    };

    MyContactResultCallback cb;
    cb.m_collisionFilterGroup = filterGroup;
    cb.m_collisionFilterMask = filterMask;
    cb.m_colliders = &colliders;

    dynamicsWorld->contactTest(collisionObject, cb);
}

bool PhysicsWorld::ClosestRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, CastResult &trace) const {
    trace.hitObject = nullptr;
    trace.fraction = 1.0f;
    trace.endPos = dest;
    trace.surfaceFlags = 0;

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

    btVector3 rayFromWorld = ToBtVector3(SystemUnitToPhysicsUnit(origin));
    btVector3 rayToWorld = ToBtVector3(SystemUnitToPhysicsUnit(dest));

    MyClosestRayResultCallback cb(me, rayFromWorld, rayToWorld);

    cb.m_collisionFilterGroup = filterGroup;
    cb.m_collisionFilterMask = filterMask;

    dynamicsWorld->rayTest(rayFromWorld, rayToWorld, cb);

    if (cb.hasHit()) {
        trace.hitObject = (PhysCollidable *)(cb.m_collisionObject->getUserPointer());
        trace.point = PhysicsUnitToSystemUnit(ToVec3(cb.m_hitPointWorld));
        trace.normal = ToVec3(cb.m_hitNormalWorld);
        trace.fraction = cb.m_closestHitFraction;
        trace.endPos = origin + trace.fraction * (dest - origin);

        const btCollisionShape *shape = cb.m_collisionObject->getCollisionShape();
        if (shape) {
            if (shape->getShapeType() == MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE) {
                btMultimaterialTriangleMeshShape *_shape = (btMultimaterialTriangleMeshShape *)shape;
                const CollisionMaterial *props = (CollisionMaterial *)_shape->getMaterialProperties(cb.m_localShapeInfo->m_shapePart, cb.m_localShapeInfo->m_triangleIndex);
                trace.surfaceFlags = props->surfaceFlags;
            }
        }
        return true;
    }
    return false;
}

bool PhysicsWorld::AllHitsRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, Array<CastResult> &traceList) const {
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
            m_localShapeInfo.push_back(rayResult.m_localShapeInfo);

            return AllHitsRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }

        btAlignedObjectArray<btCollisionWorld::LocalShapeInfo *> m_localShapeInfo;
        const btCollisionObject *m_me;
    };

    btVector3 rayFromWorld = ToBtVector3(SystemUnitToPhysicsUnit(origin));
    btVector3 rayToWorld = ToBtVector3(SystemUnitToPhysicsUnit(dest));

    MyAllHitsRayResultCallback cb(me, rayFromWorld, rayToWorld);

    cb.m_collisionFilterGroup = filterGroup;
    cb.m_collisionFilterMask = filterMask;

    dynamicsWorld->rayTest(rayFromWorld, rayToWorld, cb);

    if (cb.hasHit()) {
        for (int i = 0; i < cb.m_collisionObjects.size(); i++) {
            CastResult trace;
            trace.hitObject = (PhysCollidable *)(cb.m_collisionObjects[i]->getUserPointer());
            trace.point = PhysicsUnitToSystemUnit(ToVec3(cb.m_hitPointWorld[i]));
            trace.normal = ToVec3(cb.m_hitNormalWorld[i]);
            trace.fraction = cb.m_hitFractions[i];
            trace.endPos = origin + trace.fraction * (dest - origin);

            const btCollisionShape *shape = cb.m_collisionObjects[i]->getCollisionShape();
            if (shape) {
                if (shape->getShapeType() == MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE) {
                    btMultimaterialTriangleMeshShape *_shape = (btMultimaterialTriangleMeshShape *)shape;
                    const CollisionMaterial *props = (CollisionMaterial *)_shape->getMaterialProperties(cb.m_localShapeInfo[i]->m_shapePart, cb.m_localShapeInfo[i]->m_triangleIndex);
                    trace.surfaceFlags = props->surfaceFlags;
                } else {
                    trace.surfaceFlags = 0;
                }
            } else {
                trace.surfaceFlags = 0;
            }
            traceList.Append(trace);
        }
        return true;
    }
    return false;
}

bool PhysicsWorld::ClosestConvexTest(const btCollisionObject *me, const btConvexShape *convexShape, const btTransform &shapeTransform, const Mat3 &axis, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, CastResult &trace) const {
    trace.hitObject = nullptr;
    trace.fraction = 1.0f;
    trace.endPos = dest;
    trace.surfaceFlags = 0;

    class MyClosestConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback {
    public:
        MyClosestConvexResultCallback(const btCollisionObject *me, const btVector3 &convexFromWorld, const btVector3 &convexToWorld) : 
            ClosestConvexResultCallback(convexFromWorld, convexToWorld) {
                m_me = me;
            }

        virtual	btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace) {
            if (convexResult.m_hitCollisionObject == m_me) {
                return 1.0f;
            }

            /*const btCollisionShape *shape = convexResult.m_hitCollisionObject->getCollisionShape();
            if (shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE) {
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
        return false;
    }

    btTransform fromTrans;
    fromTrans = ToBtTransform(axis, SystemUnitToPhysicsUnit(origin));
    fromTrans.mult(shapeTransform, fromTrans);

    btTransform toTrans;
    toTrans = ToBtTransform(axis, SystemUnitToPhysicsUnit(dest));
    toTrans.mult(shapeTransform, toTrans);

    MyClosestConvexResultCallback cb(me, fromTrans.getOrigin(), toTrans.getOrigin());

    cb.m_collisionFilterGroup = filterGroup;
    cb.m_collisionFilterMask = filterMask;

    dynamicsWorld->convexSweepTest(convexShape, fromTrans, toTrans, cb, dynamicsWorld->getDispatchInfo().m_allowedCcdPenetration);
    
    if (cb.hasHit()) {
        trace.hitObject = (PhysCollidable *)(cb.m_hitCollisionObject->getUserPointer());
        trace.point = PhysicsUnitToSystemUnit(ToVec3(cb.m_hitPointWorld));
        trace.normal = ToVec3(cb.m_hitNormalWorld);
        trace.fraction = cb.m_closestHitFraction;
        trace.endPos = origin + trace.fraction * (dest - origin);

        const btCollisionShape *shape = cb.m_hitCollisionObject->getCollisionShape();
        if (shape) {
            if (shape->getShapeType() == MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE) {
                btMultimaterialTriangleMeshShape *_shape = (btMultimaterialTriangleMeshShape *)shape;
                const CollisionMaterial *props = (CollisionMaterial *)_shape->getMaterialProperties(cb.m_localShapeInfo->m_shapePart, cb.m_localShapeInfo->m_triangleIndex);
                trace.surfaceFlags = props->surfaceFlags;
            }
        }
        return true;
    }
    return false;
}

bool PhysicsWorld::AllHitsConvexTest(const btCollisionObject *me, const btConvexShape *convexShape, const btTransform &shapeTransform, const Mat3 &axis, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, Array<CastResult> &traceList) const {
    class MyAllHitsConvexResultCallback : public btCollisionWorld::ConvexResultCallback {
    public:
        MyAllHitsConvexResultCallback(const btCollisionObject *me, const btVector3 &convexFromWorld, const btVector3 &convexToWorld) :
            ConvexResultCallback() {
            m_convexFromWorld = convexFromWorld;
            m_convexToWorld = convexToWorld;
            m_me = me;
        }

        virtual	btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace) {
            if (convexResult.m_hitCollisionObject == m_me) {
                return 1.0f;
            }

            m_localShapeInfo.push_back(convexResult.m_localShapeInfo);

            m_collisionObjects.push_back(convexResult.m_hitCollisionObject);
            btVector3 hitNormalWorld;
            if (normalInWorldSpace)
            {
                hitNormalWorld = convexResult.m_hitNormalLocal;
            }
            else
            {
                ///need to transform normal into worldspace
                hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
            }
            m_hitNormalWorld.push_back(hitNormalWorld);
            btVector3 hitPointWorld;
            hitPointWorld.setInterpolate3(m_convexFromWorld, m_convexToWorld, convexResult.m_hitFraction);
            m_hitPointWorld.push_back(hitPointWorld);
            m_hitFractions.push_back(convexResult.m_hitFraction);
            return m_closestHitFraction;
        }

        btVector3 m_convexFromWorld;
        btVector3 m_convexToWorld;
        btAlignedObjectArray<const btCollisionObject *> m_collisionObjects;
        btAlignedObjectArray<btVector3> m_hitNormalWorld;
        btAlignedObjectArray<btVector3> m_hitPointWorld;
        btAlignedObjectArray<btScalar> m_hitFractions;
        btAlignedObjectArray<btCollisionWorld::LocalShapeInfo *> m_localShapeInfo;
        const btCollisionObject *m_me;
    };

    if (origin.DistanceSqr(dest) < Math::FloatEpsilon) {
        return false;
    }

    btTransform fromTrans;
    fromTrans = ToBtTransform(axis, SystemUnitToPhysicsUnit(origin));
    fromTrans.mult(shapeTransform, fromTrans);

    btTransform toTrans;
    toTrans = ToBtTransform(axis, SystemUnitToPhysicsUnit(dest));
    toTrans.mult(shapeTransform, toTrans);

    MyAllHitsConvexResultCallback cb(me, fromTrans.getOrigin(), toTrans.getOrigin());

    cb.m_collisionFilterGroup = filterGroup;
    cb.m_collisionFilterMask = filterMask;

    dynamicsWorld->convexSweepTest(convexShape, fromTrans, toTrans, cb, dynamicsWorld->getDispatchInfo().m_allowedCcdPenetration);

    if (cb.hasHit()) {
        for (int i = 0; i < cb.m_collisionObjects.size(); i++) {
            CastResult trace;
            trace.hitObject = (PhysCollidable *)(cb.m_collisionObjects[i]->getUserPointer());
            trace.point = PhysicsUnitToSystemUnit(ToVec3(cb.m_hitPointWorld[i]));
            trace.normal = ToVec3(cb.m_hitNormalWorld[i]);
            trace.fraction = cb.m_hitFractions[i];
            trace.endPos = origin + trace.fraction * (dest - origin);

            const btCollisionShape *shape = cb.m_collisionObjects[i]->getCollisionShape();
            if (shape) {
                if (shape->getShapeType() == MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE) {
                    btMultimaterialTriangleMeshShape *_shape = (btMultimaterialTriangleMeshShape *)shape;
                    const CollisionMaterial *props = (CollisionMaterial *)_shape->getMaterialProperties(cb.m_localShapeInfo[i]->m_shapePart, cb.m_localShapeInfo[i]->m_triangleIndex);
                    trace.surfaceFlags = props->surfaceFlags;
                } else {
                    trace.surfaceFlags = 0;
                }
            } else {
                trace.surfaceFlags = 0;
            }
            traceList.Append(trace);
        }
        return true;
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
                        listenerA->Collide(a, b, PhysicsUnitToSystemUnit(ToVec3(ptB)), ToVec3(normalOnB), PhysicsUnitToSystemUnit(distance), PhysicsUnitToSystemUnit(impulse));
                    }
                    if (listenerB) {
                        listenerB->Collide(b, a, PhysicsUnitToSystemUnit(ToVec3(ptA)), ToVec3(-normalOnB), PhysicsUnitToSystemUnit(distance), PhysicsUnitToSystemUnit(impulse));
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
