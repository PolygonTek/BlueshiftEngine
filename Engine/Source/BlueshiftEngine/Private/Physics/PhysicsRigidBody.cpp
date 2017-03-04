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

    btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);

    btVector3 origin = transform.getOrigin() - btVector3(transformedCentroid.x, transformedCentroid.y, transformedCentroid.z);
    return Vec3(origin.x(), origin.y(), origin.z());
}

void PhysRigidBody::SetOrigin(const Vec3 &origin) {
    btRigidBody *rigidBody = GetRigidBody();
    
    Vec3 centroidOrigin = origin + GetAxis() * centroid;

    btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);

    transform.setOrigin(btVector3(centroidOrigin.x, centroidOrigin.y, centroidOrigin.z));
    rigidBody->getMotionState()->setWorldTransform(transform);
    rigidBody->setWorldTransform(transform);
}

const Mat3 PhysRigidBody::GetAxis() const {
    const btRigidBody *rigidBody = GetRigidBody();

    btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);

    const btMatrix3x3 &basis = transform.getBasis();

    return Mat3(
        basis[0][0], basis[1][0], basis[2][0],
        basis[0][1], basis[1][1], basis[2][1],
        basis[0][2], basis[1][2], basis[2][2]);
}

void PhysRigidBody::SetAxis(const Mat3 &axis) {
    btRigidBody *rigidBody = GetRigidBody();

    btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);

    transform.setBasis(btMatrix3x3(
        axis[0][0], axis[1][0], axis[2][0],
        axis[0][1], axis[1][1], axis[2][1],
        axis[0][2], axis[1][2], axis[2][2]));

    rigidBody->getMotionState()->setWorldTransform(transform);
    rigidBody->setWorldTransform(transform);
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
    return Vec3(gravityAcceleration.x(), gravityAcceleration.y(), gravityAcceleration.z());
}

void PhysRigidBody::SetGravity(const Vec3 &gravityAcceleration) {
    GetRigidBody()->setGravity(btVector3(gravityAcceleration.x, gravityAcceleration.y, gravityAcceleration.z));
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
    return Vec3(linearVelocity.x(), linearVelocity.y(), linearVelocity.z());
}

void PhysRigidBody::SetLinearVelocity(const Vec3 &linearVelocity) {
    GetRigidBody()->setLinearVelocity(btVector3(linearVelocity.x, linearVelocity.y, linearVelocity.z));
}

const Vec3 PhysRigidBody::GetAngularVelocity() const {
    const btVector3 &angularVelocity = GetRigidBody()->getAngularVelocity();
    return Vec3(angularVelocity.x(), angularVelocity.y(), angularVelocity.z());
}

void PhysRigidBody::SetAngularVelocity(const Vec3 &angularVelocity) {
    GetRigidBody()->setAngularVelocity(btVector3(angularVelocity.x, angularVelocity.y, angularVelocity.z));
}

const Vec3 PhysRigidBody::GetLinearFactor() const {
    const btVector3 &linearFactor = GetRigidBody()->getLinearFactor();
    return Vec3(linearFactor.x(), linearFactor.y(), linearFactor.z());
}

void PhysRigidBody::SetLinearFactor(const Vec3 &linearFactor) {
    GetRigidBody()->setLinearFactor(btVector3(linearFactor.x, linearFactor.y, linearFactor.z));
}

const Vec3 PhysRigidBody::GetAngularFactor() const {
    const btVector3 &angularFactor = GetRigidBody()->getAngularFactor();
    return Vec3(angularFactor.x(), angularFactor.y(), angularFactor.z());
}

void PhysRigidBody::SetAngularFactor(const Vec3 &angularFactor) {
    GetRigidBody()->setAngularFactor(btVector3(angularFactor.x, angularFactor.y, angularFactor.z));
}

const Vec3 PhysRigidBody::GetTotalForce() const {
    const btVector3 &totalForce = GetRigidBody()->getTotalForce();
    return Vec3(totalForce.x(), totalForce.y(), totalForce.z());
}

const Vec3 PhysRigidBody::GetTotalTorque() const {
    const btVector3 &totalTorque = GetRigidBody()->getTotalTorque();
    return Vec3(totalTorque.x(), totalTorque.y(), totalTorque.z());
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
    GetRigidBody()->applyCentralForce(btVector3(force.x, force.y, force.z));
}

void PhysRigidBody::ApplyForce(const Vec3 &force, const Vec3 &relativePos) {
    GetRigidBody()->activate();
    GetRigidBody()->applyForce(btVector3(force.x, force.y, force.z), btVector3(relativePos.x, relativePos.y, relativePos.z));
}

void PhysRigidBody::ApplyTorque(const Vec3 &torque) {
    GetRigidBody()->activate();
    GetRigidBody()->applyTorque(btVector3(torque.x, torque.y, torque.z));
}

void PhysRigidBody::ApplyCentralImpulse(const Vec3 &impulse) {
    GetRigidBody()->activate();
    GetRigidBody()->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
}

void PhysRigidBody::ApplyImpulse(const Vec3 &impulse, const Vec3 &relativePos) {
    GetRigidBody()->activate();
    GetRigidBody()->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(relativePos.x, relativePos.y, relativePos.z));
}

void PhysRigidBody::ApplyAngularImpulse(const Vec3 &impulse) {
    GetRigidBody()->activate();
    GetRigidBody()->applyTorqueImpulse(btVector3(impulse.x, impulse.y, impulse.z));
}

BE_NAMESPACE_END
