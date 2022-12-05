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
    
PhysGenericSpringConstraint::PhysGenericSpringConstraint(PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) :
    PhysConstraint(nullptr, bodyB) {
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = new btGeneric6DofSpring2Constraint(*bodyB->GetRigidBody(), frameB, RO_XZY);
    generic6DofSpringConstraint->setUserConstraintPtr(this);

    constraint = generic6DofSpringConstraint;

    linearLowerLimits.SetFromScalar(0);
    linearUpperLimits.SetFromScalar(0);
    linearStiffness.SetFromScalar(0);
    linearDamping.SetFromScalar(0);

    angularLowerLimits.SetFromScalar(0);
    angularUpperLimits.SetFromScalar(0);
    angularStiffness.SetFromScalar(0);
    angularDamping.SetFromScalar(0);
}

PhysGenericSpringConstraint::PhysGenericSpringConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = new btGeneric6DofSpring2Constraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB, RO_XZY);
    generic6DofSpringConstraint->setUserConstraintPtr(this);

    constraint = generic6DofSpringConstraint;
   
    linearLowerLimits.SetFromScalar(0);
    linearUpperLimits.SetFromScalar(0);
    linearStiffness.SetFromScalar(0);
    linearDamping.SetFromScalar(0);

    angularLowerLimits.SetFromScalar(0);
    angularUpperLimits.SetFromScalar(0);
    angularStiffness.SetFromScalar(0);
    angularDamping.SetFromScalar(0);
}

void PhysGenericSpringConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    Vec3 anchorInACentroid = bodyA ? anchorInA - bodyB->centroid : anchorInA;
    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));

    if (!bodyA) {
        generic6DofSpringConstraint->setFrames(frameA, generic6DofSpringConstraint->getFrameOffsetA());
    } else {
        generic6DofSpringConstraint->setFrames(generic6DofSpringConstraint->getFrameOffsetB(), frameA);
    }
}

void PhysGenericSpringConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    if (!bodyA) {
        generic6DofSpringConstraint->setFrames(generic6DofSpringConstraint->getFrameOffsetB(), frameB);
    } else {
        generic6DofSpringConstraint->setFrames(frameB, generic6DofSpringConstraint->getFrameOffsetA());
    }
}

void PhysGenericSpringConstraint::SetLinearLowerLimits(const Vec3 &lower) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    this->linearLowerLimits = lower;

    if (generic6DofSpringConstraint->isLimited(0) || generic6DofSpringConstraint->isLimited(1) || generic6DofSpringConstraint->isLimited(2)) {
        generic6DofSpringConstraint->setLinearLowerLimit(ToBtVector3(SystemUnitToPhysicsUnit(lower)));
    }
}

void PhysGenericSpringConstraint::SetLinearUpperLimits(const Vec3 &upper) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    this->linearUpperLimits = upper;

    if (generic6DofSpringConstraint->isLimited(0) || generic6DofSpringConstraint->isLimited(1) || generic6DofSpringConstraint->isLimited(2)) {
        generic6DofSpringConstraint->setLinearUpperLimit(ToBtVector3(SystemUnitToPhysicsUnit(upper)));
    }
}

void PhysGenericSpringConstraint::EnableLinearLimits(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    if (enableX) {
        generic6DofSpringConstraint->setLimit(0, SystemUnitToPhysicsUnit(linearLowerLimits[0]), SystemUnitToPhysicsUnit(linearUpperLimits[0]));
    } else {
        generic6DofSpringConstraint->setLimit(0, 1.0f, -1.0f);
    }
    if (enableY) {
        generic6DofSpringConstraint->setLimit(1, SystemUnitToPhysicsUnit(linearLowerLimits[1]), SystemUnitToPhysicsUnit(linearUpperLimits[1]));
    } else {
        generic6DofSpringConstraint->setLimit(1, 1.0f, -1.0f);
    }
    if (enableZ) {
        generic6DofSpringConstraint->setLimit(2, SystemUnitToPhysicsUnit(linearLowerLimits[2]), SystemUnitToPhysicsUnit(linearUpperLimits[2]));
    } else {
        generic6DofSpringConstraint->setLimit(2, 1.0f, -1.0f);
    }

    generic6DofSpringConstraint->setEquilibriumPoint();
}

void PhysGenericSpringConstraint::SetLinearStiffness(const Vec3 &stiffness) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    float mA = bodyA ? bodyA->GetMass() : FLT_INFINITY;
    float mB = bodyB->GetMass();
    float m = Min(mA, mB);
    Vec3 omega = Math::TwoPi * stiffness;
    Vec3 k = m * omega * omega;

    if (stiffness.x > 0.0f) {
        generic6DofSpringConstraint->enableSpring(0, true);
        generic6DofSpringConstraint->setStiffness(0, k.x);
    } else {
        generic6DofSpringConstraint->enableSpring(0, false);
        generic6DofSpringConstraint->setStiffness(0, 0);
    }

    if (stiffness.y > 0.0f) {
        generic6DofSpringConstraint->enableSpring(1, true);
        generic6DofSpringConstraint->setStiffness(1, k.y);
    } else {
        generic6DofSpringConstraint->enableSpring(1, false);
        generic6DofSpringConstraint->setStiffness(1, 0);
    }

    if (stiffness.z > 0.0f) {
        generic6DofSpringConstraint->enableSpring(2, true);
        generic6DofSpringConstraint->setStiffness(2, k.z);
    } else {
        generic6DofSpringConstraint->enableSpring(2, false);
        generic6DofSpringConstraint->setStiffness(2, 0);
    }

    linearStiffness = stiffness;
}

void PhysGenericSpringConstraint::SetLinearDamping(const Vec3 &dampingRatio) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    generic6DofSpringConstraint->setDamping(0, dampingRatio.x);
    generic6DofSpringConstraint->setDamping(1, dampingRatio.y);
    generic6DofSpringConstraint->setDamping(2, dampingRatio.z);

    linearDamping = dampingRatio;
}

void PhysGenericSpringConstraint::SetAngularLowerLimits(const Vec3 &lower) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    this->angularLowerLimits = lower;

    if (generic6DofSpringConstraint->isLimited(3) || generic6DofSpringConstraint->isLimited(4) || generic6DofSpringConstraint->isLimited(5)) {
        // NOTE: setAngularLowerLimit 은 시계방향으로 회전하도록 가정했으므로 역으로 세팅한다
        generic6DofSpringConstraint->setAngularUpperLimit(-ToBtVector3(lower));
    }
}

void PhysGenericSpringConstraint::SetAngularUpperLimits(const Vec3 &upper) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    this->angularUpperLimits = upper;

    if (generic6DofSpringConstraint->isLimited(3) || generic6DofSpringConstraint->isLimited(4) || generic6DofSpringConstraint->isLimited(5)) {
        // NOTE: setAngularUpperLimit 은 시계방향으로 회전하도록 가정했으므로 역으로 세팅한다
        generic6DofSpringConstraint->setAngularLowerLimit(-ToBtVector3(upper));
    }
}

void PhysGenericSpringConstraint::EnableAngularLimits(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    if (enableX) {
        generic6DofSpringConstraint->setLimit(3, -angularUpperLimits[0], -angularLowerLimits[0]);
    } else {
        generic6DofSpringConstraint->setLimit(3, 1.0f, -1.0f);
    }
    if (enableY) {
        generic6DofSpringConstraint->setLimit(4, -angularUpperLimits[1], -angularLowerLimits[1]);
    } else {
        generic6DofSpringConstraint->setLimit(4, 1.0f, -1.0f);
    } 
    if (enableZ) {
        generic6DofSpringConstraint->setLimit(5, -angularUpperLimits[2], -angularLowerLimits[2]);
    } else {
        generic6DofSpringConstraint->setLimit(5, 1.0f, -1.0f);
    }

    generic6DofSpringConstraint->setEquilibriumPoint();
}

void PhysGenericSpringConstraint::SetAngularStiffness(const Vec3 &stiffness) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    float mA = bodyA ? bodyA->GetMass() : FLT_INFINITY;
    float mB = bodyB->GetMass();
    float m = Min(mA, mB);
    Vec3 omega = Math::TwoPi * stiffness;
    Vec3 k = m * omega * omega;

    if (stiffness.x > 0.0f) {
        generic6DofSpringConstraint->enableSpring(3, true);
        generic6DofSpringConstraint->setStiffness(3, k.x);
    } else {
        generic6DofSpringConstraint->enableSpring(3, false);
        generic6DofSpringConstraint->setStiffness(3, 0);
    }

    if (stiffness.y > 0.0f) {
        generic6DofSpringConstraint->enableSpring(4, true);
        generic6DofSpringConstraint->setStiffness(4, k.y);
    } else {
        generic6DofSpringConstraint->enableSpring(4, false);
        generic6DofSpringConstraint->setStiffness(4, 0);
    }

    if (stiffness.z > 0.0f) {
        generic6DofSpringConstraint->enableSpring(5, true);
        generic6DofSpringConstraint->setStiffness(5, k.z);
    } else {
        generic6DofSpringConstraint->enableSpring(5, false);
        generic6DofSpringConstraint->setStiffness(5, 0);
    }

    angularStiffness = stiffness;
}

void PhysGenericSpringConstraint::SetAngularDamping(const Vec3 &dampingRatio) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    generic6DofSpringConstraint->setDamping(3, dampingRatio.x);
    generic6DofSpringConstraint->setDamping(4, dampingRatio.y);
    generic6DofSpringConstraint->setDamping(5, dampingRatio.z);

    angularDamping = dampingRatio;
}

void PhysGenericSpringConstraint::SetMotor(const Vec3 &motorTargetVelocities, const Vec3 &maxMotorForces) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    generic6DofSpringConstraint->setTargetVelocity(3, motorTargetVelocities.x);
    generic6DofSpringConstraint->setTargetVelocity(4, motorTargetVelocities.y);
    generic6DofSpringConstraint->setTargetVelocity(5, motorTargetVelocities.z);
    
    generic6DofSpringConstraint->setMaxMotorForce(3, maxMotorForces.x);
    generic6DofSpringConstraint->setMaxMotorForce(4, maxMotorForces.y);
    generic6DofSpringConstraint->setMaxMotorForce(5, maxMotorForces.z);
}

void PhysGenericSpringConstraint::EnableMotor(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofSpring2Constraint *generic6DofSpringConstraint = static_cast<btGeneric6DofSpring2Constraint *>(constraint);

    generic6DofSpringConstraint->enableMotor(3, enableX);
    generic6DofSpringConstraint->enableMotor(4, enableY);
    generic6DofSpringConstraint->enableMotor(5, enableZ);
}

BE_NAMESPACE_END
