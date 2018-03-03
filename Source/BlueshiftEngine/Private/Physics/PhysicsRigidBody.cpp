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

PhysRigidBody::PhysRigidBody(btRigidBody *rigidBody, const Vec3 &centroid) : 
    PhysCollidable(PhysCollidable::Type::RigidBody, rigidBody, centroid) {
}

PhysRigidBody::~PhysRigidBody() {
}

btRigidBody *PhysRigidBody::GetRigidBody() {
    return static_cast<btRigidBody *>(collisionObject);
}

const btRigidBody *PhysRigidBody::GetRigidBody() const {
    return static_cast<const btRigidBody *>(collisionObject);
}

const Vec3 PhysRigidBody::GetOrigin() const {
    const btRigidBody *rigidBody = GetRigidBody();

    Vec3 transformedCentroid = GetAxis() * centroid;
    Vec3 origin;

    if (IsStatic()) {
        const btVector3 &centroidOrigin = rigidBody->getWorldTransform().getOrigin();

        origin = ToVec3(centroidOrigin) - transformedCentroid;
    } else {
        btTransform motionTransform;
        rigidBody->getMotionState()->getWorldTransform(motionTransform);

        const btVector3 &centroidOrigin = motionTransform.getOrigin();
        origin = ToVec3(centroidOrigin) - transformedCentroid;
    }
    return origin;
}

const Mat3 PhysRigidBody::GetAxis() const {
    const btRigidBody *rigidBody = GetRigidBody();
    Mat3 axis;

    if (IsStatic()) {
        const btMatrix3x3 &basis = rigidBody->getWorldTransform().getBasis();
        axis = Mat3(
            basis[0][0], basis[1][0], basis[2][0],
            basis[0][1], basis[1][1], basis[2][1],
            basis[0][2], basis[1][2], basis[2][2]);
    } else {
        btTransform motionTransform;
        rigidBody->getMotionState()->getWorldTransform(motionTransform);

        const btMatrix3x3 &basis = motionTransform.getBasis();
        axis = Mat3(
            basis[0][0], basis[1][0], basis[2][0],
            basis[0][1], basis[1][1], basis[2][1],
            basis[0][2], basis[1][2], basis[2][2]);
    }

    return axis;
}

void PhysRigidBody::SetOrigin(const Vec3 &origin) {
    btRigidBody *rigidBody = GetRigidBody();

    btTransform transform = rigidBody->getWorldTransform();
    transform.setOrigin(ToBtVector3(origin + GetAxis() * centroid));

    rigidBody->setWorldTransform(transform);

    if (!IsStatic()) {
        btTransform motionTransform;
        rigidBody->getMotionState()->getWorldTransform(motionTransform);
        motionTransform.setOrigin(transform.getOrigin());
        rigidBody->getMotionState()->setWorldTransform(motionTransform);

        if (!IsKinematic()) { // HACK
            btTransform interpTransform = rigidBody->getInterpolationWorldTransform();
            interpTransform.setOrigin(transform.getOrigin());
            rigidBody->setInterpolationWorldTransform(interpTransform);
        }
    }
}

void PhysRigidBody::SetAxis(const Mat3 &axis) {
    btRigidBody *rigidBody = GetRigidBody();

    btTransform transform = rigidBody->getWorldTransform();
    transform.setBasis(btMatrix3x3(
        axis[0][0], axis[1][0], axis[2][0],
        axis[0][1], axis[1][1], axis[2][1],
        axis[0][2], axis[1][2], axis[2][2]));

    rigidBody->setWorldTransform(transform);
    
    if (!IsStatic()) {
        btTransform motionTransform;
        rigidBody->getMotionState()->getWorldTransform(motionTransform);
        motionTransform.setBasis(transform.getBasis());
        rigidBody->getMotionState()->setWorldTransform(motionTransform);

        if (!IsKinematic()) { // HACK
            btTransform interpTransform = rigidBody->getInterpolationWorldTransform();
            interpTransform.setBasis(transform.getBasis());
            rigidBody->setInterpolationWorldTransform(interpTransform);
        }
    }
}

void PhysRigidBody::SetTransform(const Mat3 &axis, const Vec3 &origin) {
    btRigidBody *rigidBody = GetRigidBody();

    btTransform transform = rigidBody->getWorldTransform();
    transform.setOrigin(ToBtVector3(origin + axis * centroid));
    transform.setBasis(btMatrix3x3(
        axis[0][0], axis[1][0], axis[2][0],
        axis[0][1], axis[1][1], axis[2][1],
        axis[0][2], axis[1][2], axis[2][2]));

    rigidBody->setWorldTransform(transform);

    if (!IsStatic()) {
        btTransform motionTransform;
        rigidBody->getMotionState()->getWorldTransform(motionTransform);
        motionTransform.setOrigin(transform.getOrigin());
        motionTransform.setBasis(transform.getBasis());
        rigidBody->getMotionState()->setWorldTransform(motionTransform);

        btTransform interpTransform = rigidBody->getInterpolationWorldTransform();
        interpTransform.setOrigin(transform.getOrigin());
        interpTransform.setBasis(transform.getBasis());
        rigidBody->setInterpolationWorldTransform(interpTransform);
    }
}

float PhysRigidBody::GetMass() const {
    float invMass = GetRigidBody()->getInvMass();
    if (invMass > 0.0f) {
        return 1.0f / invMass;
    }
    return 0.0f;
}

void PhysRigidBody::SetMass(float mass) {
    btVector3 inertia(0, 0, 0);
    if (mass != 0.0f) {
        GetRigidBody()->getCollisionShape()->calculateLocalInertia(mass, inertia);
    }
    GetRigidBody()->setMassProps(mass, inertia);
}

const Vec3 PhysRigidBody::GetGravity() const {
    btVector3 gravityAcceleration = GetRigidBody()->getGravity();
    return ToVec3(gravityAcceleration);
}

void PhysRigidBody::SetGravity(const Vec3 &gravityAcceleration) {
    GetRigidBody()->setGravity(ToBtVector3(gravityAcceleration));
}

float PhysRigidBody::GetLinearDamping() const {
    return GetRigidBody()->getLinearDamping();
}

void PhysRigidBody::SetLinearDamping(float linearDamping) {
    GetRigidBody()->setDamping(linearDamping, GetRigidBody()->getAngularDamping());
}

float PhysRigidBody::GetAngularDamping() const {
    return GetRigidBody()->getAngularDamping();
}

void PhysRigidBody::SetAngularDamping(float angularDamping) {
    GetRigidBody()->setDamping(GetRigidBody()->getLinearDamping(), angularDamping);
}

const Vec3 PhysRigidBody::GetLinearVelocity() const {
    const btVector3 &linearVelocity = GetRigidBody()->getLinearVelocity();
    return ToVec3(linearVelocity);
}

void PhysRigidBody::SetLinearVelocity(const Vec3 &linearVelocity) {
    GetRigidBody()->setLinearVelocity(ToBtVector3(linearVelocity));
    GetRigidBody()->setInterpolationLinearVelocity(ToBtVector3(linearVelocity));
}

const Vec3 PhysRigidBody::GetAngularVelocity() const {
    const btVector3 &angularVelocity = GetRigidBody()->getAngularVelocity();
    return ToVec3(angularVelocity);
}

void PhysRigidBody::SetAngularVelocity(const Vec3 &angularVelocity) {
    GetRigidBody()->setAngularVelocity(ToBtVector3(angularVelocity));
    GetRigidBody()->setInterpolationAngularVelocity(ToBtVector3(angularVelocity));
}

const Vec3 PhysRigidBody::GetLinearFactor() const {
    const btVector3 &linearFactor = GetRigidBody()->getLinearFactor();
    return ToVec3(linearFactor);
}

void PhysRigidBody::SetLinearFactor(const Vec3 &linearFactor) {
    GetRigidBody()->setLinearFactor(ToBtVector3(linearFactor));
}

const Vec3 PhysRigidBody::GetAngularFactor() const {
    const btVector3 &angularFactor = GetRigidBody()->getAngularFactor();
    return ToVec3(angularFactor);
}

void PhysRigidBody::SetAngularFactor(const Vec3 &angularFactor) {
    GetRigidBody()->setAngularFactor(ToBtVector3(angularFactor));
}

const Vec3 PhysRigidBody::GetTotalForce() const {
    const btVector3 &totalForce = GetRigidBody()->getTotalForce();
    return ToVec3(totalForce);
}

const Vec3 PhysRigidBody::GetTotalTorque() const {
    const btVector3 &totalTorque = GetRigidBody()->getTotalTorque();
    return ToVec3(totalTorque);
}

bool PhysRigidBody::IsCCD() const {
    return GetRigidBody()->getCcdMotionThreshold() > 0.0f ? true : false;
}

void PhysRigidBody::SetCCD(bool enableCcd) {
    btVector3 center;
    btScalar radius;

    if (enableCcd) {
        GetRigidBody()->getCollisionShape()->getBoundingSphere(center, radius);
        // TODO:
        GetRigidBody()->setCcdMotionThreshold(radius * 0.5f);
        GetRigidBody()->setCcdSweptSphereRadius(radius * 0.25f);
    } else {
        GetRigidBody()->setCcdMotionThreshold(0.0f);
    }
}

void PhysRigidBody::ClearForces() {
    GetRigidBody()->clearForces();
}

void PhysRigidBody::ClearVelocities() {
    GetRigidBody()->setLinearVelocity(btVector3(0, 0, 0));
    GetRigidBody()->setAngularVelocity(btVector3(0, 0, 0));
}

void PhysRigidBody::ApplyCentralForce(const Vec3 &force) {
    GetRigidBody()->activate();
    GetRigidBody()->applyCentralForce(ToBtVector3(force));
}

void PhysRigidBody::ApplyForce(const Vec3 &force, const Vec3 &relativePos) {
    GetRigidBody()->activate();
    GetRigidBody()->applyForce(ToBtVector3(force), ToBtVector3(relativePos));
}

void PhysRigidBody::ApplyTorque(const Vec3 &torque) {
    GetRigidBody()->activate();
    GetRigidBody()->applyTorque(ToBtVector3(torque));
}

void PhysRigidBody::ApplyCentralImpulse(const Vec3 &impulse) {
    GetRigidBody()->activate();
    GetRigidBody()->applyCentralImpulse(ToBtVector3(impulse));
}

void PhysRigidBody::ApplyImpulse(const Vec3 &impulse, const Vec3 &relativePos) {
    GetRigidBody()->activate();
    GetRigidBody()->applyImpulse(ToBtVector3(impulse), ToBtVector3(relativePos));
}

void PhysRigidBody::ApplyAngularImpulse(const Vec3 &impulse) {
    GetRigidBody()->activate();
    GetRigidBody()->applyTorqueImpulse(ToBtVector3(impulse));
}

BE_NAMESPACE_END
