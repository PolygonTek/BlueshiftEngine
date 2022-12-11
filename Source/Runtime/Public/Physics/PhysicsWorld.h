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

struct btOverlapFilterCallback;
class btCollisionObject;
class btConvexShape;
class btTransform;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btConstraintSolver;
class btSoftBodySolver;
class btGhostPairCallback;
class btDynamicsWorld;

#include "Core/SignalObject.h"
#include "Containers/HashTable.h"
#include "PhysicsCollidable.h"
#include "PhysicsCollisionListener.h"
#include "PhysicsConstraint.h"

BE_NAMESPACE_BEGIN

class Collider;
class PhysCollidable;
class PhysRigidBody;
class PhysSoftBody;
class PhysSensor;

struct PhysShapeDesc {
    Vec3                    localOrigin;    // local position in system units
    Mat3                    localAxis;
    Collider *              collider;
};

struct PhysCollidableDesc {
    PhysCollidable::Type::Enum type;
    Array<PhysShapeDesc>    shapes;
    Vec3                    origin;         // position in system units
    Mat3                    axis;
    Array<Vec3>             points;         // soft body: points in world space
    Array<uint32_t>         pointIndexes;   // soft body: point indexes
    Array<float>            pointWeights;   // soft body: point weights (0: disable weight (no physics, only animation), 1: default weight)
    float                   mass;           // if mass is zero, then static object, but soft body must have greater than zero
    float                   restitution;
    float                   friction;
    float                   rollingFriction;
    float                   spinningFriction;
    float                   linearDamping;
    float                   angularDamping;
    bool                    character;
    bool                    kinematic;
    bool                    ccd;
};

struct PhysConstraintDesc {
    PhysConstraint::Type::Enum type;
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
    friend class PhysRigidBody;
    friend class PhysSoftBody;
    friend class PhysSensor;
    friend class PhysConstraint;
    friend class PhysSensor;
    friend class PhysVehicle;

public:
    struct ConstraintSolver {
        enum Enum {
            SequentialImpulseSolver,        // Default constraint solver.
            NNCGSolver,                     // Nonsmooth Nonlinear Conjugate Gradient Solver
            ProjectedGaussSeidelSolver,
            DantzigSolver,
            LemkeSolver
        };
    };

    PhysicsWorld();
    ~PhysicsWorld();

    void                    ClearScene();
    void                    Reset();

    void                    StepSimulation(float frameTime);

    int                     GetConstraintSolverIterations() const;
    void                    SetConstraintSolverIterations(int iterationCount);

    ConstraintSolver::Enum  GetConstraintSolver() const { return solverType; }
    void                    SetConstraintSolver(ConstraintSolver::Enum solver);

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

                            /// Returns true if there are any colliders overlapping the box
    bool                    CheckBox(const Vec3 &boxCenter, const Vec3 &boxExtents, int mask) const;
                            /// Returns true if there are any colliders overlapping the sphere.
    bool                    CheckSphere(const Vec3 &sphereOrigin, float sphereRadius, int mask) const;
                            /// Returns true if there are any colliders overlapping the triangle.
    bool                    CheckTriangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, int mask) const;

                            /// Returns an array with all colliders touching or inside the box.
    int                     OverlapBox(const Vec3 &boxCenter, const Vec3 &boxExtents, int mask, Array<PhysCollidable *> &colliders) const;
                            /// Returns an array with all colliders touching or inside the sphere.
    int                     OverlapSphere(const Vec3 &sphereCenter, float sphereRadius, int mask, Array<PhysCollidable *> &colliders) const;
                            /// Returns an array with all colliders touching the triangle.
    int                     OverlapTriangle(const Vec3 &a, const Vec3& b, const Vec3& c, int mask, Array<PhysCollidable *> &colliders) const;

    bool                    RayCast(const Vec3 &start, const Vec3 &end, int mask, CastResult &trace) const;
    bool                    RayCastAll(const Vec3 &start, const Vec3 &end, int mask, Array<CastResult> &traceList) const;

    bool                    BoxCast(const Vec3 &boxCenter, const Vec3 &boxExtents, const Mat3 &axis, const Vec3 &end, int mask, CastResult &trace) const;
    bool                    BoxCastAll(const Vec3 &boxCenter, const Vec3 &boxExtents, const Mat3 &axis, const Vec3 &end, int mask, Array<CastResult> &traceList) const;

    bool                    SphereCast(const Vec3 &sphereCenter, float sphereRadius, const Vec3 &end, int mask, CastResult &trace) const;
    bool                    SphereCastAll(const Vec3 &sphereCenter, float sphereRadius, const Vec3 &end, int mask, Array<CastResult> &traceList) const;

    bool                    ConvexCast(const PhysCollidable *me, const Collider *collider, const Mat3 &axis, const Vec3 &start, const Vec3 &end, int mask, CastResult &trace) const;
    bool                    ConvexCastAll(const PhysCollidable *me, const Collider *collider, const Mat3 &axis, const Vec3 &start, const Vec3 &end, int mask, Array<CastResult> &traceList) const;

    void                    PreStep(float timeStep);
    void                    PostStep(float timeStep);

    void                    DebugDraw();

    static const SignalDef  SIG_PreStep;
    static const SignalDef  SIG_PostStep;

private:
    void                    ProcessCollision();
    void                    OverlapShape(btCollisionObject *collisionObject, int filterGroup, int filterMask, Array<PhysCollidable *> &colliders) const;
    bool                    ClosestRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, CastResult &trace) const;
    bool                    AllHitsRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, Array<CastResult> &traceList) const;
    bool                    ClosestConvexTest(const btCollisionObject *me, const btConvexShape *convexShape, const btTransform &shapeTransform, const Mat3 &axis, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, CastResult &trace) const;
    bool                    AllHitsConvexTest(const btCollisionObject *me, const btConvexShape *convexShape, const btTransform &shapeTransform, const Mat3 &axis, const Vec3 &origin, const Vec3 &dest, int filterGroup, int filterMask, Array<CastResult> &traceList) const;

    void                    CheckModifiedCVars();

    float                   time = 0.0f;
    float                   accumulatedTimeDelta = 0.0f;
    int                     frameRate;
    float                   frameTimeDelta;
    float                   maximumAllowedTimeStep;
    ConstraintSolver::Enum  solverType;
    int                     filterMasks[32];
    btDefaultCollisionConfiguration *collisionConfiguration = nullptr;
    btCollisionDispatcher * collisionDispatcher = nullptr;
    btBroadphaseInterface * broadphase = nullptr;
    btConstraintSolver *    constraintSolver = nullptr; 
    btSoftBodySolver *      softBodySolver = nullptr;
    btGhostPairCallback *   ghostPairCallback = nullptr;
    //btOverlapFilterCallback *filterCallback = nullptr;
    btDynamicsWorld *       dynamicsWorld = nullptr;
};

BE_NAMESPACE_END
