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

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);

    btGeneric6DofConstraint *generic6DofConstraint = new btGeneric6DofConstraint(*bodyA->GetRigidBody(), frameA, true);
    generic6DofConstraint->setUserConstraintPtr(this);

    constraint = generic6DofConstraint;
}

PhysGenericConstraint::PhysGenericConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;
    
    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);
    btTransform frameB = ToBtTransform(axisInB, anchorInBCentroid);

    btGeneric6DofConstraint *generic6DofConstraint = new btGeneric6DofConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB, true);
    generic6DofConstraint->setUserConstraintPtr(this);

    constraint = generic6DofConstraint;
}

void PhysGenericConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);

    btGeneric6DofConstraint *genericConstraint = static_cast<btGeneric6DofConstraint *>(constraint);
    genericConstraint->setFrames(frameA, genericConstraint->getFrameOffsetB());
}

void PhysGenericConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameB = ToBtTransform(axisInB, anchorInBCentroid);

    btGeneric6DofConstraint *genericConstraint = static_cast<btGeneric6DofConstraint *>(constraint);
    genericConstraint->setFrames(genericConstraint->getFrameOffsetA(), frameB);
}

const Vec3 PhysGenericConstraint::GetAngularLowerLimit() const {
    btVector3 lower;
    ((btGeneric6DofConstraint *)constraint)->getAngularLowerLimit(lower);
    return ToVec3(lower);
}

void PhysGenericConstraint::SetAngularLowerLimit(const Vec3 &lower) {
    ((btGeneric6DofConstraint *)constraint)->setAngularLowerLimit(ToBtVector3(lower));
}

const Vec3 PhysGenericConstraint::GetAngularUpperLimit() const {
    btVector3 upper;
    ((btGeneric6DofConstraint *)constraint)->getAngularUpperLimit(upper);
    return ToVec3(upper);
}

void PhysGenericConstraint::SetAngularUpperLimit(const Vec3 &upper) {
    ((btGeneric6DofConstraint *)constraint)->setAngularUpperLimit(ToBtVector3(upper));
}

const Vec3 PhysGenericConstraint::GetLinearLowerLimit() const {
    btVector3 lower;
    ((btGeneric6DofConstraint *)constraint)->getLinearLowerLimit(lower);
    return ToVec3(lower);
}

void PhysGenericConstraint::SetLinearLowerLimit(const Vec3 &lower) {
    ((btGeneric6DofConstraint *)constraint)->setLinearLowerLimit(ToBtVector3(lower));
}

const Vec3 PhysGenericConstraint::GetLinearUpperLimit() const {
    btVector3 upper;
    ((btGeneric6DofConstraint *)constraint)->getLinearUpperLimit(upper);
    return ToVec3(upper);
}

void PhysGenericConstraint::SetLinearUpperLimit(const Vec3 &upper) {
    ((btGeneric6DofConstraint *)constraint)->setLinearUpperLimit(ToBtVector3(upper));
}

BE_NAMESPACE_END
