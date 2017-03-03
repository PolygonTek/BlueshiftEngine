#pragma once

#include "PhysicsConstraint.h"

BE_NAMESPACE_BEGIN

// Point-to-point constraint, also known as ball socket joint
class PhysP2PConstraint : public PhysConstraint {
public:
    PhysP2PConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA);
    PhysP2PConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, PhysRigidBody *bodyB, const Vec3 &anchorInB);

    const Vec3              GetAnchorA() const;
    void                    SetAnchorA(const Vec3 &anchorInA);
    const Vec3              GetAnchorB() const;
    void                    SetAnchorB(const Vec3 &anchorInB);
};

BE_NAMESPACE_END
