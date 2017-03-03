#pragma once

class btCollisionObject;
class btConvexShape;
class btTransform;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btGhostPairCallback;
class btDiscreteDynamicsWorld;

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

class PhysicsWorld {
    friend class PhysicsSystem;
    friend class PhysCollidable;
    friend class PhysConstraint;
    friend class PhysSensor;

public:
    PhysicsWorld();
    ~PhysicsWorld();

    void                    ClearScene();
    void                    StepSimulation(int frameTime);

    const Vec3              GetGravity() const;
    void                    SetGravity(const Vec3 &gravityAcceleration);

    bool                    RayCast(const PhysCollidable *me, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, CastResult &trace) const;
    bool                    RayCastAll(const PhysCollidable *me, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, Array<CastResult> &traceList) const;
    bool                    ConvexCast(const PhysCollidable *me, const Collider *collider, const Mat3 &axis, const Vec3 &start, const Vec3 &end, short filterGroup, short filterMask, CastResult &trace) const;

    void                    ProcessPostTickCallback(float timeStep);

    void                    DebugDraw();

private:
    bool                    ClosestRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, CastResult &trace) const;
    bool                    AllHitsRayTest(const btCollisionObject *me, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, Array<CastResult> &traceList) const;
    bool                    ClosestConvexTest(const btCollisionObject *me, const btConvexShape *convexShape, const btTransform &shapeTransform, const Mat3 &axis, const Vec3 &origin, const Vec3 &dest, short filterGroup, short filterMask, CastResult &trace) const;
    void                    CheckModifiedCVars();

    float                   time;
    float                   timeDelta;
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *  collisionDispatcher;
    btBroadphaseInterface *  broadphase;
    btSequentialImpulseConstraintSolver *solver;
    btGhostPairCallback *    ghostPairCallback;
    btDiscreteDynamicsWorld *dynamicsWorld;
};

BE_NAMESPACE_END
