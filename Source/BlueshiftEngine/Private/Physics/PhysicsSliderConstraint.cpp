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

PhysSliderConstraint::PhysSliderConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA) : 
    PhysConstraint(bodyA, nullptr) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);

    btSliderConstraint *sliderConstraint = new btSliderConstraint(*bodyA->GetRigidBody(), frameA, true);
    sliderConstraint->setUserConstraintPtr(this);

    sliderConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0);
    sliderConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 1);
    sliderConstraint->setParam(BT_CONSTRAINT_CFM, 0);
    sliderConstraint->setParam(BT_CONSTRAINT_ERP, 1);

    constraint = sliderConstraint;

    linearLowerLimit = 0;
    linearUpperLimit = 0;
    angularLowerLimit = 1;
    angularUpperLimit = -1;
}

PhysSliderConstraint::PhysSliderConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);
    btTransform frameB = ToBtTransform(axisInB, anchorInBCentroid);

    btSliderConstraint *sliderConstraint = new btSliderConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB, true);
    sliderConstraint->setUserConstraintPtr(this);

    sliderConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0);
    sliderConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 1);
    sliderConstraint->setParam(BT_CONSTRAINT_CFM, 0);
    sliderConstraint->setParam(BT_CONSTRAINT_ERP, 1);

    constraint = sliderConstraint;

    linearLowerLimit = 0;
    linearUpperLimit = 0;
    angularLowerLimit = 1;
    angularUpperLimit = -1;
}

void PhysSliderConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);

    if (!bodyB) {
        sliderConstraint->setFrames(sliderConstraint->getFrameOffsetA(), frameA);
    } else {
        sliderConstraint->setFrames(frameA, sliderConstraint->getFrameOffsetB());
    }
}

void PhysSliderConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    Vec3 anchorInBCentroid = bodyB ? anchorInB - bodyA->centroid : anchorInB;
    btTransform frameB = ToBtTransform(axisInB, anchorInBCentroid);

    if (!bodyB) {
        sliderConstraint->setFrames(frameB, sliderConstraint->getFrameOffsetB());
    } else {
        sliderConstraint->setFrames(sliderConstraint->getFrameOffsetA(), frameB);
    }
}

void PhysSliderConstraint::SetLinearLimits(float lowerLimit, float upperLimit) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    this->angularLowerLimit = lowerLimit;
    this->angularUpperLimit = upperLimit;

    if (sliderConstraint->getLowerLinLimit() <= sliderConstraint->getUpperLinLimit()) {
        sliderConstraint->setLowerLinLimit(lowerLimit);
        sliderConstraint->setUpperLinLimit(upperLimit);
    }
}

void PhysSliderConstraint::EnableLinearLimits(bool enable) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    if (enable) {
        sliderConstraint->setLowerLinLimit(angularLowerLimit);
        sliderConstraint->setUpperLinLimit(angularUpperLimit);
    } else {
        sliderConstraint->setLowerLinLimit(1);
        sliderConstraint->setUpperLinLimit(-1);
    }
}

void PhysSliderConstraint::SetAngularLimits(float lowerLimit, float upperLimit) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    this->angularLowerLimit = lowerLimit;
    this->angularUpperLimit = upperLimit;

    if (sliderConstraint->getLowerAngLimit() <= sliderConstraint->getUpperAngLimit()) {
        sliderConstraint->setLowerAngLimit(lowerLimit);
        sliderConstraint->setUpperAngLimit(upperLimit);
    }
}

void PhysSliderConstraint::EnableAngularLimits(bool enable) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    if (enable) {
        sliderConstraint->setLowerAngLimit(angularLowerLimit);
        sliderConstraint->setUpperAngLimit(angularUpperLimit);
    } else {
        sliderConstraint->setLowerAngLimit(1);
        sliderConstraint->setUpperAngLimit(-1);
    }
}

void PhysSliderConstraint::SetLinearMotor(float motorTargetVelocity, float maxMotorForce) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    sliderConstraint->setTargetLinMotorVelocity(motorTargetVelocity);
    sliderConstraint->setMaxLinMotorForce(maxMotorForce);
}

void PhysSliderConstraint::EnableLinearMotor(bool enable) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    sliderConstraint->setPoweredLinMotor(enable);
}

void PhysSliderConstraint::SetAngularMotor(float motorTargetVelocity, float maxMotorTorque) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    sliderConstraint->setTargetAngMotorVelocity(motorTargetVelocity);
    sliderConstraint->setMaxAngMotorForce(maxMotorTorque);
}

void PhysSliderConstraint::EnableAngularMotor(bool enable) {
    btSliderConstraint *sliderConstraint = static_cast<btSliderConstraint *>(constraint);

    sliderConstraint->setPoweredAngMotor(enable);
}

BE_NAMESPACE_END
