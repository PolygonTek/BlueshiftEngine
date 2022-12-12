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

PhysSoftBody::PhysSoftBody(btSoftBody *softBody, const Vec3 &centroid) : 
    PhysCollidable(PhysCollidable::Type::SoftBody, softBody, centroid) {
}

PhysSoftBody::~PhysSoftBody() {
}

btSoftBody *PhysSoftBody::GetSoftBody() {
    return static_cast<btSoftBody *>(collisionObject);
}

const btSoftBody *PhysSoftBody::GetSoftBody() const {
    return static_cast<const btSoftBody *>(collisionObject);
}

const Vec3 PhysSoftBody::GetOrigin() const {
    const btSoftBody *softBody = GetSoftBody();

    Vec3 transformedCentroid = GetAxis() * centroid;
    Vec3 worldCentroid = PhysicsUnitToSystemUnit(ToVec3(softBody->getWorldTransform().getOrigin()));
    return worldCentroid - transformedCentroid;
}

void PhysSoftBody::SetOrigin(const Vec3 &origin) {
    btSoftBody *softBody = GetSoftBody();

    btTransform worldTransform = softBody->getWorldTransform();
    worldTransform.setOrigin(ToBtVector3(SystemUnitToPhysicsUnit(origin + GetAxis() * centroid)));
    softBody->setWorldTransform(worldTransform);

    btTransform interpTransform = softBody->getInterpolationWorldTransform();
    interpTransform.setOrigin(worldTransform.getOrigin());
    softBody->setInterpolationWorldTransform(interpTransform);
}

const Mat3 PhysSoftBody::GetAxis() const {
    const btSoftBody *softBody = GetSoftBody();

    Mat3 axis = ToMat3(softBody->getWorldTransform().getBasis());
    return axis;
}

void PhysSoftBody::SetAxis(const Mat3 &axis) {
    btSoftBody *softBody = GetSoftBody();

    btTransform worldTransform = softBody->getWorldTransform();
    worldTransform.setBasis(ToBtMatrix3x3(axis));
    softBody->setWorldTransform(worldTransform);

    btTransform interpTransform = softBody->getInterpolationWorldTransform();
    interpTransform.setBasis(worldTransform.getBasis());
    softBody->setInterpolationWorldTransform(interpTransform);
}

void PhysSoftBody::SetTransform(const Mat3x4 &transform) {
    btSoftBody *softBody = GetSoftBody();

    btTransform worldTransform = softBody->getWorldTransform();
    worldTransform.setOrigin(ToBtVector3(SystemUnitToPhysicsUnit(transform.TransformPos(centroid))));
    worldTransform.setBasis(btMatrix3x3(
        transform[0][0], transform[0][1], transform[0][2],
        transform[1][0], transform[1][1], transform[1][2],
        transform[2][0], transform[2][1], transform[2][2]));
    softBody->setWorldTransform(worldTransform);

    btTransform interpTransform = softBody->getInterpolationWorldTransform();
    interpTransform.setOrigin(worldTransform.getOrigin());
    interpTransform.setBasis(worldTransform.getBasis());
    softBody->setInterpolationWorldTransform(interpTransform);
}

void PhysSoftBody::AddToWorld(PhysicsWorld *physicsWorld) {
    if (IsInWorld()) {
        BE_WARNLOG("PhysSoftBody::AddToWorld: already added\n");
        return;
    }

    btSoftBody *softBody = static_cast<btSoftBody *>(collisionObject);

    int internalGroup;
    if (collisionFilterBit == 0) {
        internalGroup = btBroadphaseProxy::DefaultFilter;
    } else {
        internalGroup = (BIT(collisionFilterBit) & ~1) << 5;
    }

    int collisionFilterMask = physicsWorld->GetCollisionFilterMask(collisionFilterBit);
    int internalMask = (collisionFilterMask & ~1) << 5;

    if (collisionFilterMask & BIT(0)) {
        internalMask |= (btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::StaticFilter | btBroadphaseProxy::KinematicFilter | btBroadphaseProxy::SensorTrigger);
    }

    btSoftRigidDynamicsWorld *softDynamicsWorld = (btSoftRigidDynamicsWorld *)physicsWorld->dynamicsWorld;
    btSoftBodyWorldInfo &worldInfo = softDynamicsWorld->getWorldInfo();
    softBody->m_worldInfo = &worldInfo;
    softDynamicsWorld->addSoftBody(softBody, internalGroup, internalMask);
    this->physicsWorld = physicsWorld;
}

void PhysSoftBody::RemoveFromWorld() {
    if (!IsInWorld()) {
        BE_WARNLOG("PhysSoftBody::RemoveFromWorld: already removed\n");
        return;
    }

    btSoftRigidDynamicsWorld *softDynamicsWorld = (btSoftRigidDynamicsWorld *)physicsWorld->dynamicsWorld;
    softDynamicsWorld->removeSoftBody(static_cast<btSoftBody *>(collisionObject));
    physicsWorld = nullptr;
}

float PhysSoftBody::GetFriction() const {
    return GetSoftBody()->getFriction();
}

void PhysSoftBody::SetFriction(float friction) {
    GetSoftBody()->setFriction(friction);
    GetSoftBody()->m_cfg.kDF = friction;
}

int PhysSoftBody::GetBendingConstraintDistance() const {
    return bendingConstraintDistance;
}

void PhysSoftBody::SetBendingConstraintDistance(int distance) {
    bendingConstraintDistance = distance;

    GetSoftBody()->generateBendingConstraints(distance, GetSoftBody()->m_materials[0]);
    GetSoftBody()->randomizeConstraints();
}

float PhysSoftBody::GetStiffness() const {
    return GetSoftBody()->m_materials[0]->m_kLST;
}

void PhysSoftBody::SetStiffness(float stiffness) {
    btSoftBody::Material *softBodyMaterial = GetSoftBody()->m_materials[0];
    softBodyMaterial->m_kLST = stiffness;
    softBodyMaterial->m_kAST = stiffness;
    softBodyMaterial->m_kVST = stiffness;
}

float PhysSoftBody::GetMass() const {
    return GetSoftBody()->getTotalMass();
}

void PhysSoftBody::SetMass(float mass) {
    GetSoftBody()->setTotalMass(mass, true);
}

const Vec3 PhysSoftBody::GetGravity() const {
    btVector3 gravityAcceleration = GetSoftBody()->m_worldInfo->m_gravity;
    return PhysicsUnitToSystemUnit(ToVec3(gravityAcceleration));
}

void PhysSoftBody::SetGravity(const Vec3 &gravityAcceleration) {
    GetSoftBody()->m_worldInfo->m_gravity = ToBtVector3(SystemUnitToPhysicsUnit(gravityAcceleration));
}

bool PhysSoftBody::IsContinuousCollisionDetectionEnabled() const {
    return GetSoftBody()->getCcdMotionThreshold() > 0.0f ? true : false;
}

void PhysSoftBody::SetContinuousCollisionDetectionEnabled(bool enabled) {
    if (enabled) {
        btVector3 sphereCenter;
        btScalar sphereRadius;

        GetSoftBody()->getCollisionShape()->getBoundingSphere(sphereCenter, sphereRadius);
        // TODO:
        GetSoftBody()->setCcdMotionThreshold(sphereRadius * 0.5f);
        GetSoftBody()->setCcdSweptSphereRadius(sphereRadius * 0.25f);
    } else {
        GetSoftBody()->setCcdMotionThreshold(0.0f);
    }
}

bool PhysSoftBody::IsSelfCollisionEnabled() const {
    return !!(GetSoftBody()->m_cfg.collisions & btSoftBody::fCollision::CL_SELF);
}

void PhysSoftBody::SetSelfCollisionEnabled(bool enabled) {
    if (enabled) {
        GetSoftBody()->m_cfg.collisions |= btSoftBody::fCollision::CL_SELF;
    } else {
        GetSoftBody()->m_cfg.collisions &= ~btSoftBody::fCollision::CL_SELF;
    }
}

void PhysSoftBody::AddForce(const Vec3 &force) {
    GetSoftBody()->addForce(ToBtVector3(force));
}

void PhysSoftBody::AddVelocity(const Vec3 &velocity) {
    GetSoftBody()->addVelocity(ToBtVector3(velocity));
}

const Vec3 PhysSoftBody::GetWindVelocity() const {
    return ToVec3(GetSoftBody()->m_windVelocity);
}

void PhysSoftBody::SetWindVelocity(const Vec3 &windVelocity) {
    GetSoftBody()->m_windVelocity = ToBtVector3(windVelocity);
}

int PhysSoftBody::GetPositionSolverIterationCount() const {
    return GetSoftBody()->m_cfg.piterations;
}

void PhysSoftBody::SetPositionSolverIterationCount(int iterationCount) {
    GetSoftBody()->m_cfg.piterations = iterationCount;
}

int PhysSoftBody::GetNodeCount() const {
    return GetSoftBody()->m_nodes.size();
}

float PhysSoftBody::GetNodeMass(int nodeIndex) const {
    return GetSoftBody()->getMass(nodeIndex);
}

void PhysSoftBody::SetNodeMass(int nodeIndex, float mass) {
    GetSoftBody()->setMass(nodeIndex, mass);
}

void PhysSoftBody::AddNodeForce(int nodeIndex, const Vec3 &force) {
    GetSoftBody()->addForce(ToBtVector3(force), nodeIndex);
}

void PhysSoftBody::AddNodeVelocity(int nodeIndex, const Vec3 &velocity) {
    GetSoftBody()->addVelocity(ToBtVector3(velocity), nodeIndex);
}

const Vec3 PhysSoftBody::GetNodePosition(int nodeIndex) const {
    const btAlignedObjectArray<btSoftBody::Node> &nodes = GetSoftBody()->m_nodes;
    Vec3 position = PhysicsUnitToSystemUnit(ToVec3(nodes[nodeIndex].m_x));
    return position;
}

const Vec3 PhysSoftBody::GetNodeNormal(int nodeIndex) const {
    const btAlignedObjectArray<btSoftBody::Node> &nodes = GetSoftBody()->m_nodes;
    Vec3 normal = ToVec3(nodes[nodeIndex].m_n);
    return normal;
}

void PhysSoftBody::GetWorldAABB(AABB &worldAabb) const {
    btVector3 aabb_min;
    btVector3 aabb_max;
    GetSoftBody()->getAabb(aabb_min, aabb_max);

    worldAabb.b[0] = PhysicsUnitToSystemUnit(ToVec3(aabb_min));
    worldAabb.b[1] = PhysicsUnitToSystemUnit(ToVec3(aabb_max));
}

BE_NAMESPACE_END
