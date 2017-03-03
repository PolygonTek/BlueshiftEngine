#pragma once

#include "PhysicsConstraint.h"

BE_NAMESPACE_BEGIN

// Hinge constraint or revolute joint
class PhysHingeConstraint : public PhysConstraint {
public:
    PhysHingeConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA);
    PhysHingeConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);

    void                SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA);
    void                SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB);

    void                EnableLimit(bool enable);
    float               GetLowerLimit() const { return lowerLimit; }
    float               GetUpperLimit() const { return upperLimit; }
    void                SetLimit(float lowerLimit, float upperLimit);

    void                EnableMotor(bool enable);
    void                SetMotor(float motorSpeed, float maxMotorImpulse);

private:
    float               lowerLimit;
    float               upperLimit;
};

BE_NAMESPACE_END
