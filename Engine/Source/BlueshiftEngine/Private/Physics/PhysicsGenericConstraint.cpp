#include "Precompiled.h"
#include "Physics/Physics.h"
#include "PhysicsInternal.h"

BE_NAMESPACE_BEGIN
    
PhysGenericConstraint::PhysGenericConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA) : 
    PhysConstraint(bodyA, nullptr) {
    Vec3 _anchorInA = anchorInA - bodyA->centroid;

    btTransform frameA(btMatrix3x3(
        axisInA[0][0], axisInA[1][0], axisInA[2][0],
        axisInA[0][1], axisInA[1][1], axisInA[2][1],
        axisInA[0][2], axisInA[1][2], axisInA[2][2]), btVector3(_anchorInA.x, _anchorInA.y, _anchorInA.z));

    btGeneric6DofConstraint *generic6DofConstraint = new btGeneric6DofConstraint(*bodyA->GetRigidBody(), frameA, true);
    generic6DofConstraint->setUserConstraintPtr(this);

    constraint = generic6DofConstraint;
}

PhysGenericConstraint::PhysGenericConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 _anchorInA = anchorInA - bodyA->centroid;
    Vec3 _anchorInB = anchorInB - bodyB->centroid;
    
    btTransform frameA(btMatrix3x3(
        axisInA[0][0], axisInA[1][0], axisInA[2][0],
        axisInA[0][1], axisInA[1][1], axisInA[2][1],
        axisInA[0][2], axisInA[1][2], axisInA[2][2]), btVector3(_anchorInA.x, _anchorInA.y, _anchorInA.z));

    btTransform frameB(btMatrix3x3(
        axisInB[0][0], axisInB[1][0], axisInB[2][0],
        axisInB[0][1], axisInB[1][1], axisInB[2][1],
        axisInB[0][2], axisInB[1][2], axisInB[2][2]), btVector3(_anchorInB.x, _anchorInB.y, _anchorInB.z));

    btGeneric6DofConstraint *generic6DofConstraint = new btGeneric6DofConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB, true);
    generic6DofConstraint->setUserConstraintPtr(this);

    constraint = generic6DofConstraint;
}

void PhysGenericConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    Vec3 _anchorInA = anchorInA - bodyA->centroid;

    btTransform frameA(btMatrix3x3(
        axisInA[0][0], axisInA[1][0], axisInA[2][0],
        axisInA[0][1], axisInA[1][1], axisInA[2][1],
        axisInA[0][2], axisInA[1][2], axisInA[2][2]), btVector3(_anchorInA.x, _anchorInA.y, _anchorInA.z));

    btGeneric6DofConstraint *genericConstraint = static_cast<btGeneric6DofConstraint *>(constraint);
    genericConstraint->setFrames(frameA, genericConstraint->getFrameOffsetB());
}

void PhysGenericConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    Vec3 _anchorInB = anchorInB - bodyB->centroid;

    btTransform frameB(btMatrix3x3(
        axisInB[0][0], axisInB[1][0], axisInB[2][0],
        axisInB[0][1], axisInB[1][1], axisInB[2][1],
        axisInB[0][2], axisInB[1][2], axisInB[2][2]), btVector3(_anchorInB.x, _anchorInB.y, _anchorInB.z));

    btGeneric6DofConstraint *genericConstraint = static_cast<btGeneric6DofConstraint *>(constraint);
    genericConstraint->setFrames(genericConstraint->getFrameOffsetA(), frameB);
}

const Vec3 PhysGenericConstraint::GetAngularLowerLimit() const {
    btVector3 lower;
    ((btGeneric6DofConstraint *)constraint)->getAngularLowerLimit(lower);
    return Vec3(lower.x(), lower.y(), lower.z());
}

void PhysGenericConstraint::SetAngularLowerLimit(const Vec3 &lower) {
    ((btGeneric6DofConstraint *)constraint)->setAngularLowerLimit(btVector3(lower.x, lower.y, lower.z));
}

const Vec3 PhysGenericConstraint::GetAngularUpperLimit() const {
    btVector3 upper;
    ((btGeneric6DofConstraint *)constraint)->getAngularUpperLimit(upper);
    return Vec3(upper.x(), upper.y(), upper.z());
}

void PhysGenericConstraint::SetAngularUpperLimit(const Vec3 &upper) {
    ((btGeneric6DofConstraint *)constraint)->setAngularUpperLimit(btVector3(upper.x, upper.y, upper.z));
}

const Vec3 PhysGenericConstraint::GetLinearLowerLimit() const {
    btVector3 lower;
    ((btGeneric6DofConstraint *)constraint)->getLinearLowerLimit(lower);
    return Vec3(lower.x(), lower.y(), lower.z());
}

void PhysGenericConstraint::SetLinearLowerLimit(const Vec3 &lower) {
    ((btGeneric6DofConstraint *)constraint)->setLinearLowerLimit(btVector3(lower.x, lower.y, lower.z));
}

const Vec3 PhysGenericConstraint::GetLinearUpperLimit() const {
    btVector3 upper;
    ((btGeneric6DofConstraint *)constraint)->getLinearUpperLimit(upper);
    return Vec3(upper.x(), upper.y(), upper.z());
}

void PhysGenericConstraint::SetLinearUpperLimit(const Vec3 &upper) {
    ((btGeneric6DofConstraint *)constraint)->setLinearUpperLimit(btVector3(upper.x, upper.y, upper.z));
}

BE_NAMESPACE_END
