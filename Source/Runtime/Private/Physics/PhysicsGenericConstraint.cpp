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
    
PhysGenericConstraint::PhysGenericConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA) : 
    PhysConstraint(bodyA, nullptr) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));

    btGeneric6DofConstraint *generic6DofConstraint = new btGeneric6DofConstraint(*bodyA->GetRigidBody(), frameA, true);
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

    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));

    if (!bodyB) {
        generic6DofConstraint->setFrames(generic6DofConstraint->getFrameOffsetA(), frameA);
    } else {
        generic6DofConstraint->setFrames(frameA, generic6DofConstraint->getFrameOffsetB());
    }
}

void PhysGenericConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    Vec3 anchorInBCentroid = bodyB ? anchorInB - bodyA->centroid : anchorInB;
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    if (!bodyB) {
        generic6DofConstraint->setFrames(frameB, generic6DofConstraint->getFrameOffsetB());
    } else {
        generic6DofConstraint->setFrames(generic6DofConstraint->getFrameOffsetA(), frameB);
    }
}

void PhysGenericConstraint::SetLinearLowerLimit(const Vec3 &lower) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    this->linearLowerLimit = lower;

    if (generic6DofConstraint->isLimited(0) || generic6DofConstraint->isLimited(1) || generic6DofConstraint->isLimited(2)) {
        generic6DofConstraint->setLinearLowerLimit(ToBtVector3(SystemUnitToPhysicsUnit(lower)));
    }
}

void PhysGenericConstraint::SetLinearUpperLimit(const Vec3 &upper) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    this->linearUpperLimit = upper;

    if (generic6DofConstraint->isLimited(0) || generic6DofConstraint->isLimited(1) || generic6DofConstraint->isLimited(2)) {
        generic6DofConstraint->setLinearUpperLimit(ToBtVector3(SystemUnitToPhysicsUnit(upper)));
    }
}

void PhysGenericConstraint::EnableLinearLimits(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    if (enableX) {
        generic6DofConstraint->setLimit(0, SystemUnitToPhysicsUnit(linearLowerLimit[0]), SystemUnitToPhysicsUnit(linearUpperLimit[0]));
    } else {
        generic6DofConstraint->setLimit(0, 1.0f, -1.0f);
    }
    if (enableY) {
        generic6DofConstraint->setLimit(1, SystemUnitToPhysicsUnit(linearLowerLimit[1]), SystemUnitToPhysicsUnit(linearUpperLimit[1]));
    } else {
        generic6DofConstraint->setLimit(1, 1.0f, -1.0f);
    }
    if (enableZ) {
        generic6DofConstraint->setLimit(2, SystemUnitToPhysicsUnit(linearLowerLimit[2]), SystemUnitToPhysicsUnit(linearUpperLimit[2]));
    } else {
        generic6DofConstraint->setLimit(2, 1.0f, -1.0f);
    }
}

void PhysGenericConstraint::SetAngularLowerLimit(const Vec3 &lower) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    this->angularLowerLimit = lower;

    if (generic6DofConstraint->isLimited(3) || generic6DofConstraint->isLimited(4) || generic6DofConstraint->isLimited(5)) {
        generic6DofConstraint->setAngularLowerLimit(ToBtVector3(lower));
    }
}

void PhysGenericConstraint::SetAngularUpperLimit(const Vec3 &upper) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    this->angularUpperLimits = upper;

    if (generic6DofConstraint->isLimited(3) || generic6DofConstraint->isLimited(4) || generic6DofConstraint->isLimited(5)) {
        generic6DofConstraint->setAngularUpperLimit(ToBtVector3(upper));
    }
}

void PhysGenericConstraint::EnableAngularLimits(bool enableX, bool enableY, bool enableZ) {
    btGeneric6DofConstraint *generic6DofConstraint = static_cast<btGeneric6DofConstraint *>(constraint);

    if (enableX) {
        generic6DofConstraint->setLimit(3, angularLowerLimit[0], angularUpperLimits[0]);
    } else {
        generic6DofConstraint->setLimit(3, 1.0f, -1.0f);
    }
    if (enableY) {
        generic6DofConstraint->setLimit(4, angularLowerLimit[1], angularUpperLimits[1]);
    } else {
        generic6DofConstraint->setLimit(4, 1.0f, -1.0f);
    }
    if (enableZ) {
        generic6DofConstraint->setLimit(5, angularLowerLimit[2], angularUpperLimits[2]);
    } else {
        generic6DofConstraint->setLimit(5, 1.0f, -1.0f);
    }
}

BE_NAMESPACE_END
