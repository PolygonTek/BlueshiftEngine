// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
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
#include "Collider.h"
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
    Vec3                    localOrigin;        // local position in system units
    Mat3                    localAxis;
    Collider *              collider;
};

struct PhysBuoyancySurfaceQuery {
    uint32_t                queryId;            ///< Provider 가 결과에 그대로 연결해야 하는 opaque ID
    Vec3                    worldPosition;
    float                   sampleDiameter;
};

struct PhysBuoyancySurfaceQueryResult {
    Vec3                    surfacePosition;
    Vec3                    surfaceNormal;
};

class PhysBuoyancySurfaceProvider {
public:
    virtual ~PhysBuoyancySurfaceProvider() = default;

    virtual bool            IsActive() const = 0;

    virtual uint32_t        GetMaxQueryCount() const = 0;

                            /// 물의 질량 밀도 (kg/m^3) 를 리턴한다. 예를 들어 현실의 물은 약 1000.0f 이고 공기는 약 1.225f 이다.
    virtual float           GetFluidDensity() const = 0;

                            /// 수면 변위를 포함하는 world Z 범위를 반환한다.
    virtual bool            GetSurfaceHeightRange(float &minimumHeight, float &maximumHeight) const = 0;

                            /// queryCount 가 0이면 이전 query batch 를 비운다.
    virtual bool            SubmitQueries(const PhysBuoyancySurfaceQuery *queries, uint32_t queryCount) = 0;

    virtual bool            GetQueryResult(uint32_t queryId, PhysBuoyancySurfaceQueryResult &result) const = 0;
};

struct PhysCollidableDesc {
    // common
    PhysCollidable::Type::Enum type;
    Array<PhysShapeDesc>    shapes;
    Vec3                    origin;             // position in system units
    Mat3                    axis;
    float                   mass;               // if mass is zero, then static object, but soft body must have greater than zero
    float                   restitution;
    float                   friction;
    bool                    enableCCD;

    // soft body
    Array<Vec3>             points;             // proxy points in world space
    Array<uint32_t>         pointIndexes;       // proxy point indexes
    Array<float>            pointWeights;       // proxy point weights (0: disable weight (no physics, only animation), 1: default weight)
    int                     bendingConstraintDistance;
    float                   thickness;
    float                   stiffness;
    Vec3                    windVelocity;
    bool                    enableSelfCollision;

    // rigid body
    float                   rollingFriction;
    float                   spinningFriction;
    float                   linearDamping;
    float                   angularDamping;
    bool                    character;
    bool                    kinematic;
    bool                    enableBuoyancy = false;
    PhysBuoyancyForceMode::Enum buoyancyForceMode = PhysBuoyancyForceMode::PhysicalDensity;
    float                   buoyancyRelativeDensity = 1.0f; ///< 물체 밀도 / 유체 밀도. RelativeDensity 모드에서는 정지 상태의 목표 침수율에 해당한다.
    PhysBuoyancyPontoonSettings buoyancyPontoonSettings;
};

struct PhysConstraintDesc {
    PhysConstraint::Type::Enum type;
    PhysRigidBody *         bodyA;
    Vec3                    anchorInA;          // local position in A in system units
    Mat3                    axisInA;            // local axis in A
    PhysRigidBody *         bodyB;
    Vec3                    anchorInB;          // local position in B in system units
    Mat3                    axisInB;            // local axis in B
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
            SequentialImpulseSolver,            // Default constraint solver.
            NNCGSolver,                         // Nonsmooth Nonlinear Conjugate Gradient Solver
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

    float                   GetAirDensity() const;
    void                    SetAirDensity(float airDensity);

    PhysBuoyancySurfaceProvider *GetBuoyancySurfaceProvider() const { return buoyancySurfaceProvider; }
    void                    SetBuoyancySurfaceProvider(PhysBuoyancySurfaceProvider *provider);

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
    struct BuoyantRigidBody {
        PhysRigidBody *     rigidBody = nullptr;
        uint32_t            firstQueryId = 0;
        Array<uint8_t>      activeQueries;
    };

    void                    AddBuoyantRigidBody(PhysRigidBody *rigidBody);
    void                    RemoveBuoyantRigidBody(PhysRigidBody *rigidBody);
    void                    ClearActiveBuoyancyQueries();
    void                    ApplyBuoyancyForces();
    void                    UpdateBuoyancyQueries();

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
    PhysBuoyancySurfaceProvider *buoyancySurfaceProvider = nullptr;
    Array<BuoyantRigidBody> buoyantRigidBodies;
    Array<PhysBuoyancySurfaceQuery> buoyancyQueries;
    uint64_t                nextBuoyancyQueryId = 0;
};

BE_NAMESPACE_END
