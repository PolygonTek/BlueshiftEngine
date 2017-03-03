#pragma once

#include "PhysicsConstraint.h"

BE_NAMESPACE_BEGIN

class PhysGenericConstraint : public PhysConstraint {
public:
    PhysGenericConstraint(PhysRigidBody *bodyA, PhysRigidBody *bodyB) : PhysConstraint(bodyA, bodyB) {}
    PhysGenericConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA);
    PhysGenericConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);

    void                SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA);
    void                SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB);

    const Vec3          GetAngularLowerLimit() const;
    void                SetAngularLowerLimit(const Vec3 &lower);

    const Vec3          GetAngularUpperLimit() const;
    void                SetAngularUpperLimit(const Vec3 &upper);

    const Vec3          GetLinearLowerLimit() const; 
    void                SetLinearLowerLimit(const Vec3 &lower);

    const Vec3          GetLinearUpperLimit() const; 
    void                SetLinearUpperLimit(const Vec3 &upper);
};

BE_NAMESPACE_END
