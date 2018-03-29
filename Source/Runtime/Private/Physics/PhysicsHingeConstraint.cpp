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

PhysHingeConstraint::PhysHingeConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA) : 
    PhysConstraint(bodyA, nullptr) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));

    btHingeConstraint *hingeConstraint = new btHingeConstraint(*bodyA->GetRigidBody(), frameA);
    hingeConstraint->setUserConstraintPtr(this);

#if 1
    hingeConstraint->setParam(BT_CONSTRAINT_CFM, 0);
    hingeConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0);
    hingeConstraint->setParam(BT_CONSTRAINT_ERP, 1);
    hingeConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 1);
#endif

    constraint = hingeConstraint;

    lowerLimit = 0;
    upperLimit = 0;
}

PhysHingeConstraint::PhysHingeConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    btHingeConstraint *hingeConstraint = new btHingeConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB);
    hingeConstraint->setUserConstraintPtr(this);

#if 1
    hingeConstraint->setParam(BT_CONSTRAINT_CFM, 0);
    hingeConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0);
    hingeConstraint->setParam(BT_CONSTRAINT_ERP, 1);
    hingeConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 1);
#endif

    constraint = hingeConstraint;

    lowerLimit = 0;
    upperLimit = 0;
}

void PhysHingeConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    btHingeConstraint *hingeConstraint = static_cast<btHingeConstraint *>(constraint);

    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));

    hingeConstraint->setFrames(frameA, hingeConstraint->getFrameOffsetB());
}

void PhysHingeConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    btHingeConstraint *hingeConstraint = static_cast<btHingeConstraint *>(constraint);

    Vec3 anchorInBCentroid = bodyB ? anchorInB - bodyA->centroid : anchorInB;
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    hingeConstraint->setFrames(hingeConstraint->getFrameOffsetA(), frameB);
}

void PhysHingeConstraint::SetAngularLimits(float lowerLimit, float upperLimit) {
    btHingeConstraint *hingeConstraint = static_cast<btHingeConstraint *>(constraint);

    this->lowerLimit = lowerLimit;
    this->upperLimit = upperLimit;

    if (hingeConstraint->hasLimit()) {
        hingeConstraint->setLimit(lowerLimit, upperLimit, 0.9f, 0.3f);
    }
}

void PhysHingeConstraint::EnableAngularLimits(bool enable) {
    btHingeConstraint *hingeConstraint = static_cast<btHingeConstraint *>(constraint);

    if (enable) {
        hingeConstraint->setLimit(lowerLimit, upperLimit, 0.9f, 0.3f);
    } else {
        hingeConstraint->setLimit(1.0f, -1.0f);
    }
}

void PhysHingeConstraint::SetMotor(float motorTargetVelocity, float maxMotorImpulse) {
    btHingeConstraint *hingeConstraint = static_cast<btHingeConstraint *>(constraint);

    bool enabled = hingeConstraint->getEnableAngularMotor();
    hingeConstraint->enableAngularMotor(enabled, motorTargetVelocity, maxMotorImpulse);
}

void PhysHingeConstraint::EnableMotor(bool enable) {
    btHingeConstraint *hingeConstraint = static_cast<btHingeConstraint *>(constraint);

    btScalar motorTargetVelocity = hingeConstraint->getMotorTargetVelocity();
    btScalar maxMotorImpulse = hingeConstraint->getMaxMotorImpulse();

    hingeConstraint->enableAngularMotor(enable, motorTargetVelocity, maxMotorImpulse);
}

BE_NAMESPACE_END
