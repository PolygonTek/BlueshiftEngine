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
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    
    btPoint2PointConstraint *point2PointConstraint = new btPoint2PointConstraint(*bodyA->GetRigidBody(), ToBtVector3(anchorInACentroid));
    point2PointConstraint->setUserConstraintPtr(this);

    constraint = point2PointConstraint;
}

PhysP2PConstraint::PhysP2PConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, PhysRigidBody *bodyB, const Vec3 &anchorInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btPoint2PointConstraint *point2PointConstraint = new btPoint2PointConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(),
        ToBtVector3(anchorInACentroid), ToBtVector3(anchorInBCentroid));
    point2PointConstraint->setUserConstraintPtr(this);

    constraint = point2PointConstraint;
}

const Vec3 PhysP2PConstraint::GetAnchorA() const {
    const btVector3 &anchorInACentroid = ((btPoint2PointConstraint *)constraint)->getPivotInA();

    return ToVec3(anchorInACentroid) + bodyA->centroid;
}

void PhysP2PConstraint::SetAnchorA(const Vec3 &anchorInA) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    ((btPoint2PointConstraint *)constraint)->setPivotA(ToBtVector3(anchorInACentroid));
}

const Vec3 PhysP2PConstraint::GetAnchorB() const {
    Vec3 anchorInBCentroid = ToVec3(((btPoint2PointConstraint *)constraint)->getPivotInB());

    // pivotInB stands for local anchor in B or world anchor if body B is not defined
    return bodyB ? anchorInBCentroid + bodyB->centroid : anchorInBCentroid;
}

void PhysP2PConstraint::SetAnchorB(const Vec3 &anchorInB) {
    Vec3 anchorInBCentroid = bodyB ? anchorInB - bodyA->centroid : anchorInB;

    // pivotInB stands for local anchor in B or world anchor if body B is not defined
    ((btPoint2PointConstraint *)constraint)->setPivotB(ToBtVector3(anchorInBCentroid));
}

BE_NAMESPACE_END
