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

#pragma once

class btCollisionObject;
class btConvexShape;
class btTransform;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btGhostPairCallback;
struct btOverlapFilterCallback;
class btDiscreteDynamicsWorld;

#include "Core/SignalObject.h"
#include "Containers/HashTable.h"
#include "PhysicsCollidable.h"
#include "PhysicsCollisionListener.h"
#include "PhysicsConstraint.h"

BE_NAMESPACE_BEGIN

class Collider;
class PhysRigidBody;

struct PhysShapeDesc {
    Vec3                    localOrigin;
    Mat3                    localAxis;
    Collider *              collider;
};

struct PhysCollidableDesc {
    PhysCollidable::Type    type;
    Array<PhysShapeDesc>    shapes;
    Vec3                    origin;
    Mat3                    axis;
    bool                    kinematic;
    bool                    ccd;
    float                   mass;   // if mass is zero, then static object
    float                   restitution;
    float                   friction;
    float                   rollingFriction;
    float                   linearDamping;
    float                   angularDamping;
};

struct PhysConstraintDesc {
    PhysConstraint::Type    type;
    PhysRigidBody *         bodyA;
    Vec3                    anchorInA;
    Mat3                    axisInA;
    PhysRigidBody *         bodyB;
    Vec3                    anchorInB;
    Mat3                    axisInB;
    bool                    collision;
    float                   breakImpulse;
};

class PhysicsWorld : public SignalObject {
    friend class PhysicsSystem;
    friend class PhysCollidable;
    friend class PhysConstraint;
    friend class PhysSensor;

public:
    PhysicsWorld();
    ~PhysicsWorld();

    void                    ClearScene();
    void                    StepSimulation(int frameTime);

                            /// Returns fixed frame rate in physics simulation
    int                     GetFrameRate() const { return frameRate; }

                            /// Sets fixed frame rate in physics simulation
    void                    SetFrameRate(int frameRate) { this->frameRate = frameRate; }

                            /// Returns maximum allowed time step
    float                   GetMaximumAllowedTimeStep() const { return maximumAllowedTimeStep; }

                            /// Set maximum allowed time step
    void                    SetMaximumAllowedTimeStep(float maximumAllowedTimeStep) { this->maximumAllowedTimeStep = maximumAllowedTimeStep; }

    const Vec3              GetGravity() const;
    void                    SetGravity(const Vec3 &gravityAcceleration);

    uint32_t                GetCollisionFilterMask(int index) const;
    void                    SetCollisionFilterMask(int index, uint32_t mask);

    bool                    RayCast(const PhysCollidable *me, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, CastResult &trace) const;
    bool                    RayCastAll(const PhysCollidable *me, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, Array<CastResult> &traceList) const;
    bool                    ConvexCast(const PhysCollidable *me, const Collider *collider, const Mat3 &axis, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, CastResult &trace) const;

    void                    PreStep(float timeStep);
    void                    PostStep(float timeStep);

    void                    DebugDraw();

    static const SignalDef  SIG_PreStep;
    static const SignalDef  SIG_PostStep;

private:
    void                    ProcessCollision();
    bool                    ClosestRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, CastResult &trace) const;
    bool                    AllHitsRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, Array<CastResult> &traceList) const;
    bool                    ClosestConvexTest(const btCollisionObject *me, const btConvexShape *convexShape, const btTransform &shapeTransform, const Mat3 &axis, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, CastResult &trace) const;
    void                    CheckModifiedCVars();

    float                   time;
    float                   timeDelta;
    int                     frameRate;
    float                   maximumAllowedTimeStep;
    uint32_t                filterMasks[32];
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *  collisionDispatcher;
    btBroadphaseInterface *  broadphase;
    btSequentialImpulseConstraintSolver *solver;
    btGhostPairCallback *    ghostPairCallback;
    btOverlapFilterCallback *filterCallback;
    btDiscreteDynamicsWorld *dynamicsWorld;
};

BE_NAMESPACE_END
