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
    PhysGenericConstraint(bodyA, nullptr) {
    Vec3 _anchorInA = anchorInA - bodyA->centroid;

    btTransform frameA(btMatrix3x3(
        axisInA[0][0], axisInA[1][0], axisInA[2][0],
        axisInA[0][1], axisInA[1][1], axisInA[2][1],
        axisInA[0][2], axisInA[1][2], axisInA[2][2]), ToBtVector3(_anchorInA));

    btGeneric6DofSpringConstraint *generic6DofSpringConstraint = new btGeneric6DofSpringConstraint(*bodyA->GetRigidBody(), frameA, true);
    generic6DofSpringConstraint->setUserConstraintPtr(this);

    /*
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8, 0);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8, 1);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8, 2);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8, 3);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8, 4);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8, 5);

    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1, 0);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1, 1);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1, 2);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1, 3);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1, 4);
    generic6DofSpringConstraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1, 5);*/

    constraint = generic6DofSpringConstraint;

    bodyA->GetRigidBody()->setActivationState(DISABLE_DEACTIVATION);

    angularStiffness.SetFromScalar(0);
    angularDamping.SetFromScalar(0);
    linearStiffness.SetFromScalar(0);
    linearDamping.SetFromScalar(0);
}

PhysGenericSpringConstraint::PhysGenericSpringConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysGenericConstraint(bodyA, bodyB) { 
    Vec3 _anchorInA = anchorInA - bodyA->centroid;
    Vec3 _anchorInB = anchorInB - bodyB->centroid;

    btTransform frameA(btMatrix3x3(
        axisInA[0][0], axisInA[1][0], axisInA[2][0],
        axisInA[0][1], axisInA[1][1], axisInA[2][1],
        axisInA[0][2], axisInA[1][2], axisInA[2][2]), ToBtVector3(_anchorInA));

    btTransform frameB(btMatrix3x3(
        axisInB[0][0], axisInB[1][0], axisInB[2][0],
        axisInB[0][1], axisInB[1][1], axisInB[2][1],
        axisInB[0][2], axisInB[1][2], axisInB[2][2]), ToBtVector3(_anchorInB));

    btGeneric6DofSpringConstraint *generic6DofSpringConstraint = new btGeneric6DofSpringConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB, true);
    generic6DofSpringConstraint->setUserConstraintPtr(this);

    constraint = generic6DofSpringConstraint;
   
    angularStiffness.SetFromScalar(0);
    angularDamping.SetFromScalar(0);
    linearStiffness.SetFromScalar(0);
    linearDamping.SetFromScalar(0);
}

void PhysGenericSpringConstraint::SetAngularStiffness(const Vec3 &stiffness) {
    btGeneric6DofSpringConstraint *genericSpringConstraint = static_cast<btGeneric6DofSpringConstraint *>(constraint);

    genericSpringConstraint->enableSpring(3, stiffness.x > 0.0f ? true : false);
    genericSpringConstraint->enableSpring(4, stiffness.y > 0.0f ? true : false);
    genericSpringConstraint->enableSpring(5, stiffness.z > 0.0f ? true : false);

    genericSpringConstraint->setStiffness(3, stiffness.x);
    genericSpringConstraint->setStiffness(4, stiffness.y);
    genericSpringConstraint->setStiffness(5, stiffness.z);

    genericSpringConstraint->setEquilibriumPoint();

    angularStiffness = stiffness;
}

void PhysGenericSpringConstraint::SetAngularDamping(const Vec3 &damping) {
    btGeneric6DofSpringConstraint *genericSpringConstraint = static_cast<btGeneric6DofSpringConstraint *>(constraint);

    genericSpringConstraint->setDamping(3, damping.x);
    genericSpringConstraint->setDamping(4, damping.y);
    genericSpringConstraint->setDamping(5, damping.z);

    genericSpringConstraint->setEquilibriumPoint();

    angularDamping = damping;
}

void PhysGenericSpringConstraint::SetLinearStiffness(const Vec3 &stiffness) {
     btGeneric6DofSpringConstraint *genericSpringConstraint = static_cast<btGeneric6DofSpringConstraint *>(constraint);

    genericSpringConstraint->enableSpring(0, stiffness.x > 0.0f ? true : false);
    genericSpringConstraint->enableSpring(1, stiffness.y > 0.0f ? true : false);
    genericSpringConstraint->enableSpring(2, stiffness.z > 0.0f ? true : false);

    genericSpringConstraint->setStiffness(0, stiffness.x);
    genericSpringConstraint->setStiffness(1, stiffness.y);
    genericSpringConstraint->setStiffness(2, stiffness.z);

    genericSpringConstraint->setEquilibriumPoint();

    linearStiffness = stiffness;
}

void PhysGenericSpringConstraint::SetLinearDamping(const Vec3 &damping) {
    btGeneric6DofSpringConstraint *genericSpringConstraint = static_cast<btGeneric6DofSpringConstraint *>(constraint);

    genericSpringConstraint->setStiffness(0, damping.x);
    genericSpringConstraint->setStiffness(1, damping.y);
    genericSpringConstraint->setStiffness(2, damping.z);

    genericSpringConstraint->setEquilibriumPoint();

    linearDamping = damping;
}

BE_NAMESPACE_END
