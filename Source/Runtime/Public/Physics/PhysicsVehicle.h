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

#pragma once

struct btVehicleRaycaster;
class btRaycastVehicle;

BE_NAMESPACE_BEGIN

class PhysRigidBody;
class PhysicsWorld;
class PhysicsSystem;

class PhysVehicle {
    friend class PhysicsSystem;

public:
    PhysVehicle() {}
    PhysVehicle(PhysRigidBody *body);
    ~PhysVehicle();

    const PhysRigidBody *   ChassisRigidBody() const { return chassisBody; }
    PhysRigidBody *         ChassisRigidBody() { return chassisBody; }

    bool                    IsInWorld() const;
    void                    AddToWorld(PhysicsWorld *physicsWorld);
    void                    RemoveFromWorld();

    void                    Reset();

    int                     NumWheels() const;

    int                     AddWheel(const Vec3 &chassisLocalOrigin, const Mat3 &chassisLocalAxis, float wheelRadius,
                                float suspensionRestLength, float suspensionMaxDistance, float suspensionMaxForce, 
                                float suspensionStiffness, float suspensionDampingRelaxation, float suspensionDampingCompression, 
                                float rollingFriction, float rollingInfluence);

                            /// Gets torque to provide vehicle acceleration.
    float                   GetTorque(int wheelIndex) const;
                            /// Sets torque to provide vehicle acceleration.
    void                    SetTorque(int wheelIndex, float torque);
                            
                            /// Gets braking torque.
    float                   GetBrakingTorque(int wheelIndex) const;
                            /// Sets braking torque.
    void                    SetBrakingTorque(int wheelIndex, float torque);

                            /// Gets steering angle in radian.
    float                   GetSteeringAngle(int wheelIndex) const;
                            /// Sets steering angle in radian.
    void                    SetSteeringAngle(int wheelIndex, float steeringAngle);

    float                   GetSkidInfo(int wheelIndex) const;

    float                   GetSuspensionRelativeVelocity(int wheelIndex) const;

    float                   GetCurrentSpeed() const;

    void                    UpdateWheelTransform(int wheelIndex);

                            /// Returns world transform with the given wheel index.
    Mat3x4                  GetWheelTransform(int wheelIndex) const;

protected:
    PhysRigidBody *         chassisBody;
    btVehicleRaycaster *    vehicleRayCaster;
    btRaycastVehicle *      vehicle;
    PhysicsWorld *          physicsWorld;
};

BE_NAMESPACE_END
