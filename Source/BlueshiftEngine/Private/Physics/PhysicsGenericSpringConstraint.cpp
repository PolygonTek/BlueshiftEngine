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
#include "Physics/Physics.h"
#include "PhysicsInternal.h"

BE_NAMESPACE_BEGIN
    
PhysGenericSpringConstraint::PhysGenericSpringConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA) :
    PhysConstraint(bodyA, nullptr) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);

    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = new btGeneric6DofSpring2Constraint(*bodyA->GetRigidBody(), frameA, RO_XZY);
    generic6DofSpringConstraint->setUserConstraintPtr(this);

    constraint = generic6DofSpringConstraint;

    linearLowerLimit.SetFromScalar(0);
    linearUpperLimit.SetFromScalar(0);
    linearStiffness.SetFromScalar(0);
    linearDamping.SetFromScalar(0);

    angularLowerLimit.SetFromScalar(0);
    angularUpperLimits.SetFromScalar(0);
    angularStiffness.SetFromScalar(0);
    angularDamping.SetFromScalar(0);
}

PhysGenericSpringConstraint::PhysGenericSpringConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);
    btTransform frameB = ToBtTransform(axisInB, anchorInBCentroid);

    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = new btGeneric6DofSpring2Constraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB, RO_XZY);
    generic6DofSpringConstraint->setUserConstraintPtr(this);

    constraint = generic6DofSpringConstraint;
   
    linearLowerLimit.SetFromScalar(0);
    linearUpperLimit.SetFromScalar(0);
    linearStiffness.SetFromScalar(0);
    linearDamping.SetFromScalar(0);

    angularLowerLimit.SetFromScalar(0);
    angularUpperLimits.SetFromScalar(0);
    angularStiffness.SetFromScalar(0);
    angularDamping.SetFromScalar(0);
}

void PhysGenericSpringConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);

    generic6DofSpringConstraint->setFrames(frameA, generic6DofSpringConstraint->getFrameOffsetB());
}

void PhysGenericSpringConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    Vec3 anchorInBCentroid = bodyB ? anchorInB - bodyA->centroid : anchorInB;
    btTransform frameB = ToBtTransform(axisInB, anchorInBCentroid);

    generic6DofSpringConstraint->setFrames(generic6DofSpringConstraint->getFrameOffsetA(), frameB);
}

void PhysGenericSpringConstraint::SetLinearLowerLimit(const Vec3 &lower) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    this->linearLowerLimit = lower;

    if (generic6DofSpringConstraint->isLimited(0) || generic6DofSpringConstraint->isLimited(1) || generic6DofSpringConstraint->isLimited(2)) {
        generic6DofSpringConstraint->setLinearLowerLimit(ToBtVector3(lower));
    }
}

void PhysGenericSpringConstraint::SetLinearUpperLimit(const Vec3 &upper) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    this->linearUpperLimit = upper;

    if (generic6DofSpringConstraint->isLimited(0) || generic6DofSpringConstraint->isLimited(1) || generic6DofSpringConstraint->isLimited(2)) {
        generic6DofSpringConstraint->setLinearUpperLimit(ToBtVector3(upper));
    }
}

void PhysGenericSpringConstraint::EnableLinearLimits(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    if (enableX) {
        generic6DofSpringConstraint->setLimit(0, linearLowerLimit[0], linearUpperLimit[0]);
    } else {
        generic6DofSpringConstraint->setLimit(0, 1.0f, -1.0f);
    }
    if (enableY) {
        generic6DofSpringConstraint->setLimit(1, linearLowerLimit[1], linearUpperLimit[1]);
    } else {
        generic6DofSpringConstraint->setLimit(1, 1.0f, -1.0f);
    }
    if (enableZ) {
        generic6DofSpringConstraint->setLimit(2, linearLowerLimit[2], linearUpperLimit[2]);
    } else {
        generic6DofSpringConstraint->setLimit(2, 1.0f, -1.0f);
    }
}

void PhysGenericSpringConstraint::SetLinearStiffness(const Vec3 &stiffness) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    Vec3 omega = Math::TwoPi * stiffness;
    Vec3 k = omega * omega;

    generic6DofSpringConstraint->enableSpring(0, stiffness.x > 0.0f ? true : false);
    generic6DofSpringConstraint->enableSpring(1, stiffness.y > 0.0f ? true : false);
    generic6DofSpringConstraint->enableSpring(2, stiffness.z > 0.0f ? true : false);

    generic6DofSpringConstraint->setStiffness(0, k.x);
    generic6DofSpringConstraint->setStiffness(1, k.y);
    generic6DofSpringConstraint->setStiffness(2, k.z);

    generic6DofSpringConstraint->setEquilibriumPoint();

    linearStiffness = stiffness;
}

void PhysGenericSpringConstraint::SetLinearDamping(const Vec3 &dampingRatio) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    generic6DofSpringConstraint->setDamping(0, dampingRatio.x);
    generic6DofSpringConstraint->setDamping(1, dampingRatio.y);
    generic6DofSpringConstraint->setDamping(2, dampingRatio.z);

    generic6DofSpringConstraint->setEquilibriumPoint();

    linearDamping = dampingRatio;
}

void PhysGenericSpringConstraint::SetAngularLowerLimit(const Vec3 &lower) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    this->angularLowerLimit = lower;

    if (generic6DofSpringConstraint->isLimited(3) || generic6DofSpringConstraint->isLimited(4) || generic6DofSpringConstraint->isLimited(5)) {
        generic6DofSpringConstraint->setAngularLowerLimit(ToBtVector3(lower));
    }
}

void PhysGenericSpringConstraint::SetAngularUpperLimit(const Vec3 &upper) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    this->angularUpperLimits = upper;

    if (generic6DofSpringConstraint->isLimited(3) || generic6DofSpringConstraint->isLimited(4) || generic6DofSpringConstraint->isLimited(5)) {
        generic6DofSpringConstraint->setAngularUpperLimit(ToBtVector3(upper));
    }
}

void PhysGenericSpringConstraint::EnableAngularLimits(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    if (enableX) {
        generic6DofSpringConstraint->setLimit(3, angularLowerLimit[0], angularUpperLimits[0]);
    } else {
        generic6DofSpringConstraint->setLimit(3, 1.0f, -1.0f);
    }
    if (enableY) {
        generic6DofSpringConstraint->setLimit(4, angularLowerLimit[1], angularUpperLimits[1]);
    } else {
        generic6DofSpringConstraint->setLimit(4, 1.0f, -1.0f);
    } 
    if (enableZ) {
        generic6DofSpringConstraint->setLimit(5, angularLowerLimit[2], angularUpperLimits[2]);
    } else {
        generic6DofSpringConstraint->setLimit(5, 1.0f, -1.0f);
    }
}

void PhysGenericSpringConstraint::SetAngularStiffness(const Vec3 &stiffness) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    Vec3 omega = Math::TwoPi * stiffness;
    Vec3 k = omega * omega;

    generic6DofSpringConstraint->enableSpring(3, stiffness.x > 0.0f ? true : false);
    generic6DofSpringConstraint->enableSpring(4, stiffness.y > 0.0f ? true : false);
    generic6DofSpringConstraint->enableSpring(5, stiffness.z > 0.0f ? true : false);

    generic6DofSpringConstraint->setStiffness(3, k.x);
    generic6DofSpringConstraint->setStiffness(4, k.y);
    generic6DofSpringConstraint->setStiffness(5, k.z);

    generic6DofSpringConstraint->setEquilibriumPoint();

    angularStiffness = stiffness;
}

void PhysGenericSpringConstraint::SetAngularDamping(const Vec3 &dampingRatio) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    generic6DofSpringConstraint->setDamping(3, dampingRatio.x);
    generic6DofSpringConstraint->setDamping(4, dampingRatio.y);
    generic6DofSpringConstraint->setDamping(5, dampingRatio.z);

    generic6DofSpringConstraint->setEquilibriumPoint();

    angularDamping = dampingRatio;
}

void PhysGenericSpringConstraint::SetMotor(const Vec3 &motorTargetVelocities, const Vec3 &maxMotorForces) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    generic6DofSpringConstraint->setTargetVelocity(0, motorTargetVelocities.x);
    generic6DofSpringConstraint->setTargetVelocity(1, motorTargetVelocities.y);
    generic6DofSpringConstraint->setTargetVelocity(2, motorTargetVelocities.z);
    
    generic6DofSpringConstraint->setMaxMotorForce(0, maxMotorForces.x);
    generic6DofSpringConstraint->setMaxMotorForce(1, maxMotorForces.y);
    generic6DofSpringConstraint->setMaxMotorForce(2, maxMotorForces.z);
}

void PhysGenericSpringConstraint::EnableMotor(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    generic6DofSpringConstraint->enableMotor(0, enableX);
    generic6DofSpringConstraint->enableMotor(1, enableY);
    generic6DofSpringConstraint->enableMotor(2, enableZ);
}

BE_NAMESPACE_END
