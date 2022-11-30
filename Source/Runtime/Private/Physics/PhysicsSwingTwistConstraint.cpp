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

#define CONTROL_CFM_ERP

PhysSwingTwistConstraint::PhysSwingTwistConstraint(PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(nullptr, bodyB) {
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    btConeTwistConstraint *coneTwistConstraint = new btConeTwistConstraint(*bodyB->GetRigidBody(), frameB);
    //btConeTwistConstraint* coneTwistConstraint = new btConeTwistConstraint(*bodyB->GetRigidBody(), ToBtVector3(SystemUnitToPhysicsUnit(anchorInBCentroid)), ToBtVector3(axisInB[2]));

    coneTwistConstraint->setUserConstraintPtr(this);

#ifdef CONTROL_CFM_ERP
    coneTwistConstraint->setParam(BT_CONSTRAINT_CFM, 0);
    coneTwistConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0);
    coneTwistConstraint->setParam(BT_CONSTRAINT_ERP, 1);
    coneTwistConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 1);
#endif

    constraint = coneTwistConstraint;

    swing1SpanLimit = 0;
    swing2SpanLimit = 0;
    twistSpanLimit = 0;
}

PhysSwingTwistConstraint::PhysSwingTwistConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    btConeTwistConstraint *coneTwistConstraint = new btConeTwistConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB);
    coneTwistConstraint->setUserConstraintPtr(this);

#ifdef CONTROL_CFM_ERP
    coneTwistConstraint->setParam(BT_CONSTRAINT_CFM, 0);
    coneTwistConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0);
    coneTwistConstraint->setParam(BT_CONSTRAINT_ERP, 1);
    coneTwistConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 1);
#endif

    constraint = coneTwistConstraint;

    swing1SpanLimit = 0;
    swing2SpanLimit = 0;
    twistSpanLimit = 0;
}

void PhysSwingTwistConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    btConeTwistConstraint *coneTwistConstraint = static_cast<btConeTwistConstraint *>(constraint);

    Vec3 anchorInACentroid = bodyA ? anchorInA - bodyB->centroid : anchorInA;
    btTransform frameA = ToBtTransform(axisInA, SystemUnitToPhysicsUnit(anchorInACentroid));

    coneTwistConstraint->setFrames(coneTwistConstraint->getFrameOffsetB(), frameA);
}

void PhysSwingTwistConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    btConeTwistConstraint *coneTwistConstraint = static_cast<btConeTwistConstraint *>(constraint);

    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;
    btTransform frameB = ToBtTransform(axisInB, SystemUnitToPhysicsUnit(anchorInBCentroid));

    coneTwistConstraint->setFrames(frameB, coneTwistConstraint->getFrameOffsetA());
}

void PhysSwingTwistConstraint::SetSpanLimits(float swing1SpanLimit, float swing2SpanLimit, float twistSpanLimit) {
    btConeTwistConstraint *coneTwistConstraint = static_cast<btConeTwistConstraint *>(constraint);

    this->swing1SpanLimit = swing1SpanLimit;
    this->swing2SpanLimit = swing2SpanLimit;
    this->twistSpanLimit = twistSpanLimit;

    coneTwistConstraint->setLimit(twistSpanLimit, swing2SpanLimit, swing1SpanLimit);
}

BE_NAMESPACE_END
