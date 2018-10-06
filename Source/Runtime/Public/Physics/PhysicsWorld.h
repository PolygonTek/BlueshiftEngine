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
class btConstraintSolver;
class btSoftBodySolver;
class btGhostPairCallback;
struct btOverlapFilterCallback;
class btDynamicsWorld;

#include "Core/SignalObject.h"
#include "Containers/HashTable.h"
#include "PhysicsCollidable.h"
#include "PhysicsCollisionListener.h"
#include "PhysicsConstraint.h"

BE_NAMESPACE_BEGIN

class Collider;
class PhysRigidBody;

struct PhysShapeDesc {
    Vec3                    localOrigin;    // local position in system units
    Mat3                    localAxis;
    Collider *              collider;
};

struct PhysCollidableDesc {
    PhysCollidable::Type    type;
    Array<PhysShapeDesc>    shapes;
    Vec3                    origin;         // position in system units
    Mat3                    axis;
    bool                    character;
    bool                    kinematic;
    bool                    ccd;
    float                   mass;           // if mass is zero, then static object
    float                   restitution;
    float                   friction;
    float                   rollingFriction;
    float                   spinningFriction;
    float                   linearDamping;
    float                   angularDamping;
};

struct PhysConstraintDesc {
    PhysConstraint::Type    type;
    PhysRigidBody *         bodyA;
    Vec3                    anchorInA;      // local position in A in system units
    Mat3                    axisInA;        // local axis in A
    PhysRigidBody *         bodyB;
    Vec3                    anchorInB;      // local position in B in system units
    Mat3                    axisInB;        // local axis in B
    bool                    collision;
    float                   breakImpulse;
};

struct PhysWheelDesc {
    Vec3                    chassisLocalOrigin;
    Mat3                    chassisLocalAxis;
    float                   radius;
    float                   suspensionRestLength;
    float                   suspensionMaxDistance;
    float                   suspensionMaxForce;
    float                   suspensionStiffness;
    float                   suspensionDampingRelaxation;
    float                   suspensionDampingCompression;
    float                   rollingFriction;
    float                   rollingInfluence;
};

struct PhysVehicleDesc {
    PhysRigidBody *         chassisBody;
    Array<PhysWheelDesc>    wheels;
};

class PhysicsWorld : public SignalObject {
    friend class PhysicsSystem;
    friend class PhysCollidable;
    friend class PhysConstraint;
    friend class PhysSensor;
    friend class PhysVehicle;

public:
    enum ConstraintSolver {
        SequentialImpulseSolver,        // Default constraint solver.
        NNCGSolver,                     // Nonsmooth Nonlinear Conjugate Gradient Solver
        ProjectedGaussSeidelSolver,
        DantzigSolver
    };

    PhysicsWorld();
    ~PhysicsWorld();

    void                    ClearScene();
    void                    Reset();
    void                    StepSimulation(int frameTime);

    int                     GetConstraintSolverIterations() const;
    void                    SetConstraintSolverIterations(int iterationCount);

    ConstraintSolver        GetConstraintSolver() const;
    void                    SetConstraintSolver(ConstraintSolver solver);

                            /// Returns fixed frame rate in physics simulation
    int                     GetFrameRate() const { return frameRate; }
                            /// Returns fixed time delta in physics simulation
    float                   GetFrameTimeDelta() const { return frameTimeDelta; }

                            /// Sets fixed frame rate in physics simulation
    void                    SetFrameRate(int frameRate);

                            /// Returns maximum allowed time step
    float                   GetMaximumAllowedTimeStep() const { return maximumAllowedTimeStep; }

                            /// Set maximum allowed time step
    void                    SetMaximumAllowedTimeStep(float maximumAllowedTimeStep) { this->maximumAllowedTimeStep = maximumAllowedTimeStep; }

    const Vec3              GetGravity() const;
    void                    SetGravity(const Vec3 &gravityAcceleration);

    int                     GetCollisionFilterMask(int bit) const;
    void                    SetCollisionFilterMask(int bit, int mask);

    bool                    RayCast(const PhysCollidable *me, const Vec3 &start, const Vec3 &end, int mask, CastResult &trace) const;
    bool                    RayCastAll(const PhysCollidable *me, const Vec3 &start, const Vec3 &end, int mask, Array<CastResult> &traceList) const;
    bool                    ConvexCast(const PhysCollidable *me, const Collider *collider, const Mat3 &axis, const Vec3 &start, const Vec3 &end, int mask, CastResult &trace) const;

    void                    PreStep(float timeStep);
    void                    PostStep(float timeStep);

    void                    DebugDraw();

    static const SignalDef  SIG_PreStep;
    static const SignalDef  SIG_PostStep;

private:
    void                    ProcessCollision();
    bool                    ClosestRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, CastResult &trace) const;
    bool                    AllHitsRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, Array<CastResult> &traceList) const;
    bool                    ClosestConvexTest(const btCollisionObject *me, const btConvexShape *convexShape, const btTransform &shapeTransform, const Mat3 &axis, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, CastResult &trace) const;
    void                    CheckModifiedCVars();

    float                   time;
    float                   accumulatedTimeDelta;
    int                     frameRate;
    float                   frameTimeDelta;
    float                   maximumAllowedTimeStep;
    ConstraintSolver        solverType;
    int                     filterMasks[32];
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *  collisionDispatcher;
    btBroadphaseInterface *  broadphase;
    btConstraintSolver *    constraintSolver;
    btSoftBodySolver *      softBodySolver;
    btGhostPairCallback *   ghostPairCallback;
    //btOverlapFilterCallback *filterCallback;
    btDynamicsWorld *       dynamicsWorld;
};

BE_NAMESPACE_END
