#pragma once

class btTypedConstraint;

BE_NAMESPACE_BEGIN

class PhysRigidBody;
class PhysicsWorld;
class PhysicsSystem;

class PhysConstraint {
    friend class PhysicsSystem;

public:
    enum Type {
        Generic,
        GenericSpring,
        Point2Point,
        Hinge
    };

    PhysConstraint() {}
    PhysConstraint(PhysRigidBody *bodyA, PhysRigidBody *bodyB);
    virtual ~PhysConstraint();

    const PhysRigidBody *   RigidBodyA() const { return bodyA; } 
    PhysRigidBody *         RigidBodyA() { return bodyA; }

    const PhysRigidBody *   RigidBodyB() const { return bodyB; } 
    PhysRigidBody *         RigidBodyB() { return bodyB; }

    bool                    IsCollisionEnabled() const { return collisionEnabled; }
    void                    EnableCollision(bool enable);
    
    float                   GetBreakImpulse() const;
    void                    SetBreakImpulse(float impulse);

    bool                    IsEnabled() const;
    void                    SetEnabled(bool enabled);

    bool                    IsInWorld() const;
    void                    AddToWorld(PhysicsWorld *physicsWorld);
    void                    RemoveFromWorld();

    void                    Reset();

protected:
    PhysRigidBody *         bodyA;
    PhysRigidBody *         bodyB;
    btTypedConstraint *     constraint;
    bool                    collisionEnabled;
    PhysicsWorld *          physicsWorld;
};

BE_NAMESPACE_END
