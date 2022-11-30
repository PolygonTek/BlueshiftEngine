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
    
PhysGenericConstraint::PhysGenericConstraint(PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(nullptr, bodyB) {
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    btGeneric6DofConstraint *generic6DofConstraint = new btGeneric6DofConstraint(*bodyB->GetRigidBody(), frameB, true);
    generic6DofConstraint->setUserConstraintPtr(this);

    constraint = generic6DofConstraint;
}

PhysGenericConstraint::PhysGenericConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;
    
    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    btGeneric6DofConstraint *generic6DofConstraint = new btGeneric6DofConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB, true);
    generic6DofConstraint->setUserConstraintPtr(this);

    constraint = generic6DofConstraint;
}

void PhysGenericConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    Vec3 anchorInACentroid = bodyA ? anchorInA - bodyB->centroid : anchorInA;
    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));

    if (!bodyA) {
        generic6DofConstraint->setFrames(frameA, generic6DofConstraint->getFrameOffsetA());
    } else {
        generic6DofConstraint->setFrames(generic6DofConstraint->getFrameOffsetB(), frameA);
    }
}

void PhysGenericConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    if (!bodyA) {
        generic6DofConstraint->setFrames(generic6DofConstraint->getFrameOffsetB(), frameB);
    } else {
        generic6DofConstraint->setFrames(frameB, generic6DofConstraint->getFrameOffsetA());
    }
}

void PhysGenericConstraint::SetLinearLowerLimits(const Vec3 &lower) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    this->linearLowerLimits = lower;

    if (generic6DofConstraint->isLimited(0) || generic6DofConstraint->isLimited(1) || generic6DofConstraint->isLimited(2)) {
        generic6DofConstraint->setLinearLowerLimit(ToBtVector3(SystemUnitToPhysicsUnit(lower)));
    }
}

void PhysGenericConstraint::SetLinearUpperLimits(const Vec3 &upper) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    this->linearUpperLimits = upper;

    if (generic6DofConstraint->isLimited(0) || generic6DofConstraint->isLimited(1) || generic6DofConstraint->isLimited(2)) {
        generic6DofConstraint->setLinearUpperLimit(ToBtVector3(SystemUnitToPhysicsUnit(upper)));
    }
}

void PhysGenericConstraint::EnableLinearLimits(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    if (enableX) {
        generic6DofConstraint->setLimit(0, SystemUnitToPhysicsUnit(linearLowerLimits[0]), SystemUnitToPhysicsUnit(linearUpperLimits[0]));
    } else {
        generic6DofConstraint->setLimit(0, 1.0f, -1.0f);
    }
    if (enableY) {
        generic6DofConstraint->setLimit(1, SystemUnitToPhysicsUnit(linearLowerLimits[1]), SystemUnitToPhysicsUnit(linearUpperLimits[1]));
    } else {
        generic6DofConstraint->setLimit(1, 1.0f, -1.0f);
    }
    if (enableZ) {
        generic6DofConstraint->setLimit(2, SystemUnitToPhysicsUnit(linearLowerLimits[2]), SystemUnitToPhysicsUnit(linearUpperLimits[2]));
    } else {
        generic6DofConstraint->setLimit(2, 1.0f, -1.0f);
    }
}

void PhysGenericConstraint::SetAngularLowerLimits(const Vec3 &lower) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    this->angularLowerLimits = lower;

    if (generic6DofConstraint->isLimited(3) || generic6DofConstraint->isLimited(4) || generic6DofConstraint->isLimited(5)) {
        // setAngularLowerLimit 은 시계방향으로 회전하도록 가정했으므로 역으로 세팅한다
        generic6DofConstraint->setAngularUpperLimit(-ToBtVector3(lower));
    }
}

void PhysGenericConstraint::SetAngularUpperLimits(const Vec3 &upper) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    this->angularUpperLimits = upper;

    if (generic6DofConstraint->isLimited(3) || generic6DofConstraint->isLimited(4) || generic6DofConstraint->isLimited(5)) {
        // NOTE: setAngularUpperLimit 은 시계방향으로 회전하도록 가정했으므로 역으로 세팅한다
        generic6DofConstraint->setAngularLowerLimit(-ToBtVector3(upper));
    }
}

void PhysGenericConstraint::EnableAngularLimits(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    if (enableX) {
        generic6DofConstraint->setLimit(3, -angularUpperLimits[0], -angularLowerLimits[0]);
    } else {
        generic6DofConstraint->setLimit(3, 1.0f, -1.0f);
    }
    if (enableY) {
        generic6DofConstraint->setLimit(4, -angularUpperLimits[1], -angularLowerLimits[1]);
    } else {
        generic6DofConstraint->setLimit(4, 1.0f, -1.0f);
    }
    if (enableZ) {
        generic6DofConstraint->setLimit(5, -angularUpperLimits[2], -angularLowerLimits[2]);
    } else {
        generic6DofConstraint->setLimit(5, 1.0f, -1.0f);
    }
}

BE_NAMESPACE_END
