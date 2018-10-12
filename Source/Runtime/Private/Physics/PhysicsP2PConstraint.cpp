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
    
    btPoint2PointConstraint *p2pConstraint = new btPoint2PointConstraint(*bodyA->GetRigidBody(), ToBtVector3(SystemUnitToPhysicsUnit(anchorInACentroid)));
    p2pConstraint->setUserConstraintPtr(this);

    constraint = p2pConstraint;
}

PhysP2PConstraint::PhysP2PConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, PhysRigidBody *bodyB, const Vec3 &anchorInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btPoint2PointConstraint *p2pConstraint = new btPoint2PointConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(),
        ToBtVector3(SystemUnitToPhysicsUnit(anchorInACentroid)), ToBtVector3(SystemUnitToPhysicsUnit(anchorInBCentroid)));
    p2pConstraint->setUserConstraintPtr(this);

    constraint = p2pConstraint;
}

const Vec3 PhysP2PConstraint::GetAnchorA() const {
    btPoint2PointConstraint *p2pConstraint = static_cast<btPoint2PointConstraint *>(constraint);

    const btVector3 &anchorInACentroid = p2pConstraint->getPivotInA();

    return PhysicsUnitToSystemUnit(ToVec3(anchorInACentroid)) + bodyA->centroid;
}

void PhysP2PConstraint::SetAnchorA(const Vec3 &anchorInA) {
    btPoint2PointConstraint *p2pConstraint = static_cast<btPoint2PointConstraint *>(constraint);

    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    p2pConstraint->setPivotA(ToBtVector3(SystemUnitToPhysicsUnit(anchorInACentroid)));
}

const Vec3 PhysP2PConstraint::GetAnchorB() const {
    btPoint2PointConstraint *p2pConstraint = static_cast<btPoint2PointConstraint *>(constraint);

    Vec3 anchorInBCentroid = ToVec3(p2pConstraint->getPivotInB());

    // pivotInB stands for local anchor in B or world anchor if body B is not defined
    return bodyB ? PhysicsUnitToSystemUnit(anchorInBCentroid) + bodyB->centroid : PhysicsUnitToSystemUnit(anchorInBCentroid);
}

void PhysP2PConstraint::SetAnchorB(const Vec3 &anchorInB) {
    btPoint2PointConstraint *p2pConstraint = static_cast<btPoint2PointConstraint *>(constraint);

    Vec3 anchorInBCentroid = bodyB ? anchorInB - bodyA->centroid : anchorInB;

    // pivotInB stands for local anchor in B or world anchor if body B is not defined
    p2pConstraint->setPivotB(ToBtVector3(SystemUnitToPhysicsUnit(anchorInBCentroid)));
}

float PhysP2PConstraint::GetImpulseClamp() const {
    btPoint2PointConstraint *p2pConstraint = static_cast<btPoint2PointConstraint *>(constraint);

    return PhysicsUnitToSystemUnit(p2pConstraint->m_setting.m_impulseClamp);
}

void PhysP2PConstraint::SetImpulseClamp(float impulse) {
    btPoint2PointConstraint *p2pConstraint = static_cast<btPoint2PointConstraint *>(constraint);

    p2pConstraint->m_setting.m_impulseClamp = SystemUnitToPhysicsUnit(impulse);
}

BE_NAMESPACE_END
