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

PhysP2PConstraint::PhysP2PConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA) : 
    PhysConstraint(bodyA, nullptr) { 
    Vec3 _anchorInA = anchorInA - bodyA->centroid;
    
    btPoint2PointConstraint *point2PointConstraint = new btPoint2PointConstraint(*bodyA->GetRigidBody(), ToBtVector3(_anchorInA));
    point2PointConstraint->setUserConstraintPtr(this);

    constraint = point2PointConstraint;
}

PhysP2PConstraint::PhysP2PConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, PhysRigidBody *bodyB, const Vec3 &anchorInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 _anchorInA = anchorInA - bodyA->centroid;
    Vec3 _anchorInB = anchorInB - bodyB->centroid;

    btPoint2PointConstraint *point2PointConstraint = new btPoint2PointConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(),
        ToBtVector3(_anchorInA), ToBtVector3(_anchorInB));
    point2PointConstraint->setUserConstraintPtr(this);

    constraint = point2PointConstraint;
}

const Vec3 PhysP2PConstraint::GetAnchorA() const {
    const btVector3 &anchorA = ((btPoint2PointConstraint *)constraint)->getPivotInA();

    return Vec3(anchorA.x(), anchorA.y(), anchorA.z()) + bodyA->centroid;
}

void PhysP2PConstraint::SetAnchorA(const Vec3 &anchorInA) {
    Vec3 _anchorInA = anchorInA - bodyA->centroid;

    ((btPoint2PointConstraint *)constraint)->setPivotA(ToBtVector3(_anchorInA));
}

const Vec3 PhysP2PConstraint::GetAnchorB() const {
    const btVector3 &anchorB = ((btPoint2PointConstraint *)constraint)->getPivotInB();

    return Vec3(anchorB.x(), anchorB.y(), anchorB.z()) + bodyB->centroid;
}

void PhysP2PConstraint::SetAnchorB(const Vec3 &anchorInB) {
    Vec3 _anchorInB = anchorInB - bodyA->centroid;

    ((btPoint2PointConstraint *)constraint)->setPivotB(ToBtVector3(_anchorInB));
}

BE_NAMESPACE_END
