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

class MyVehicleRaycaster : public btVehicleRaycaster {
public:
    MyVehicleRaycaster(btDynamicsWorld *world) : dynamicsWorld(world) {}

    virtual void *castRay(const btVector3 &from, const btVector3 &to, btVehicleRaycasterResult &result) {
        btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
        dynamicsWorld->rayTest(from, to, rayCallback);

        if (rayCallback.hasHit()) {
            const btRigidBody *body = btRigidBody::upcast(rayCallback.m_collisionObject);
            if (body && body->hasContactResponse()) {
                result.m_hitPointInWorld = rayCallback.m_hitPointWorld;
                result.m_hitNormalInWorld = rayCallback.m_hitNormalWorld;
                result.m_hitNormalInWorld.normalize();
                result.m_distFraction = rayCallback.m_closestHitFraction;
                return (void *)body;
            }
        }
        return nullptr;
    }

    btDynamicsWorld *dynamicsWorld;
};

PhysVehicle::PhysVehicle(PhysRigidBody *chassisBody) {
    this->chassisBody = chassisBody;
    this->vehicleRayCaster = nullptr;
    this->vehicle = nullptr;
    this->physicsWorld = nullptr;

    vehicleRayCaster = new MyVehicleRaycaster(nullptr);
    vehicle = new btRaycastVehicle(btRaycastVehicle::btVehicleTuning(), chassisBody->GetRigidBody(), vehicleRayCaster);
    vehicle->setCoordinateSystem(1, 2, 0);
}

PhysVehicle::~PhysVehicle() {
    SAFE_DELETE(vehicleRayCaster);
    SAFE_DELETE(vehicle);
}

bool PhysVehicle::IsInWorld() const {
    return physicsWorld != nullptr ? true : false;
}

void PhysVehicle::AddToWorld(PhysicsWorld *physicsWorld) {
    if (IsInWorld()) {
        BE_WARNLOG(L"PhysVehicle::AddToWorld: already added\n");
        return;
    }

    ((MyVehicleRaycaster *)vehicleRayCaster)->dynamicsWorld = physicsWorld->dynamicsWorld;

    chassisBody->vehiclePtr = this;

    physicsWorld->dynamicsWorld->addVehicle(vehicle);

    this->physicsWorld = physicsWorld;
}

void PhysVehicle::RemoveFromWorld() {
    if (!IsInWorld()) {
        BE_WARNLOG(L"PhysVehicle::RemoveFromWorld: already removed\n");
        return;
    }

    chassisBody->vehiclePtr = nullptr;

    physicsWorld->dynamicsWorld->removeVehicle(vehicle);
    physicsWorld = nullptr;
}

void PhysVehicle::Reset() {
    vehicle->resetSuspension();
}

int PhysVehicle::NumWheels() const {
    return vehicle->getNumWheels();
}

int PhysVehicle::AddWheel(const Vec3 &chassisLocalOrigin, const Mat3 &chassisLocalAxis, float wheelRadius,
    float suspensionRestLength, float suspensionMaxDistance, float suspensionMaxForce, 
    float suspensionStiffness, float suspensionDampingRelaxation, float suspensionDampingCompression,
    float rollingFriction, float rollingInfluence) {
    // wheel tunning options
    btRaycastVehicle::btVehicleTuning wheelTunning;
    wheelTunning.m_maxSuspensionTravelCm = SystemUnitToPhysicsUnit(suspensionMaxDistance) * 100;
    wheelTunning.m_maxSuspensionForce = SystemUnitToPhysicsUnit(suspensionMaxForce);
    wheelTunning.m_suspensionStiffness = suspensionStiffness;
    wheelTunning.m_suspensionDamping = suspensionDampingRelaxation;
    wheelTunning.m_suspensionCompression = suspensionDampingCompression;
    wheelTunning.m_frictionSlip = rollingFriction;

    int wheelIndex = vehicle->getNumWheels();

    // connection point in chassis local space
    Vec3 connectionPoint = SystemUnitToPhysicsUnit(chassisLocalOrigin - chassisBody->centroid); 

    // ray cast direction in chassis local space
    Vec3 wheelDirection = -chassisLocalAxis[2];

    // vehicle right direction in chassis local space
    Vec3 axleDirection = -chassisLocalAxis[1];

    btWheelInfo &wheelInfo = vehicle->addWheel(
        ToBtVector3(connectionPoint), ToBtVector3(wheelDirection), ToBtVector3(axleDirection),
        SystemUnitToPhysicsUnit(suspensionRestLength), SystemUnitToPhysicsUnit(wheelRadius), wheelTunning, false);

    wheelInfo.m_rollInfluence = rollingInfluence;

    return wheelIndex;
}

float PhysVehicle::GetTorque(int wheelIndex) const {
    btWheelInfo &wheelInfo = vehicle->getWheelInfo(wheelIndex);
    return PhysicsUnitToSystemUnit(wheelInfo.m_engineForce);
}

void PhysVehicle::SetTorque(int wheelIndex, float torque) {
    vehicle->applyEngineForce(SystemUnitToPhysicsUnit(torque), wheelIndex);
}

float PhysVehicle::GetBrakingTorque(int wheelIndex) const {
    btWheelInfo &wheelInfo = vehicle->getWheelInfo(wheelIndex);

    return PhysicsUnitToSystemUnit(wheelInfo.m_brake);
}

void PhysVehicle::SetBrakingTorque(int wheelIndex, float torque) {
    vehicle->setBrake(SystemUnitToPhysicsUnit(torque), wheelIndex);
}

float PhysVehicle::GetSteeringAngle(int wheelIndex) const {
    return vehicle->getSteeringValue(wheelIndex);
}

void PhysVehicle::SetSteeringAngle(int wheelIndex, float steeringAngle) {
    vehicle->setSteeringValue(steeringAngle, wheelIndex);
}

float PhysVehicle::GetSkidInfo(int wheelIndex) const {
    btWheelInfo &wheelInfo = vehicle->getWheelInfo(wheelIndex);
    return wheelInfo.m_skidInfo;
}

float PhysVehicle::GetSuspensionRelativeVelocity(int wheelIndex) const {
    btWheelInfo &wheelInfo = vehicle->getWheelInfo(wheelIndex);
    return wheelInfo.m_suspensionRelativeVelocity;
}

float PhysVehicle::GetCurrentSpeed() const {
    return vehicle->getCurrentSpeedKmHour();
}

void PhysVehicle::UpdateWheelTransform(int wheelIndex) {
    vehicle->updateWheelTransform(wheelIndex, true);
}

Mat3x4 PhysVehicle::GetWheelTransform(int wheelIndex) const {
    const btTransform &worldTransform = vehicle->getWheelTransformWS(wheelIndex);
    const btMatrix3x3 &basis = worldTransform.getBasis();
    const btVector3 origin = PhysicsUnitToSystemUnit(worldTransform.getOrigin());

    return Mat3x4(
        basis[0][0], basis[0][1], basis[0][2], origin.x(),
        basis[1][0], basis[1][1], basis[1][2], origin.y(),
        basis[2][0], basis[2][1], basis[2][2], origin.z());
}

BE_NAMESPACE_END
