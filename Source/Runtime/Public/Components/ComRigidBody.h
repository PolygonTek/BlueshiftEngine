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

#include "Physics/Physics.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class ComJoint;
class ComTransform;
class ComRigidBody;
class ComCharacterController;
class ComVehicleWheel;

class Collision {
public:
    bool                    operator==(const Collision &rhs) const { return entityGuid == rhs.entityGuid && body == rhs.body && characterController == rhs.characterController; }

    Guid                    entityGuid;
    Entity *                entity;
    ComRigidBody *          body;
    ComCharacterController *characterController;
    Vec3                    point;
    Vec3                    normal;
    float                   distance;
    float                   impulse;
};

class ComRigidBody : public Component {
    friend class ComJoint;

public:
    OBJECT_PROTOTYPE(ComRigidBody);

    ComRigidBody();
    virtual ~ComRigidBody();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

    float                   GetMass() const;
    void                    SetMass(float mass);

    float                   GetRestitution() const;
    void                    SetRestitution(float restitution);

    float                   GetFriction() const;
    void                    SetFriction(float friction);

    float                   GetRollingFriction() const;
    void                    SetRollingFriction(float rollingFriction);

    float                   GetSpinningFriction() const;
    void                    SetSpinningFriction(float spinningFriction);

    float                   GetLinearDamping() const;
    void                    SetLinearDamping(float linearDamping);

    float                   GetAngularDamping() const;
    void                    SetAngularDamping(float angularDamping);

    bool                    IsKinematic() const;
    void                    SetKinematic(bool kinematic);

    bool                    IsCCD() const;
    void                    SetCCD(bool enableCcd);

    const Vec3              GetGravity() const { return body ? body->GetGravity() : Vec3::zero; }
    void                    SetGravity(const Vec3 &gravityAcceleration) { if (body) body->SetGravity(gravityAcceleration); }

    const Vec3              GetLinearVelocity() const { return body ? body->GetLinearVelocity() : Vec3::zero; }
    void                    SetLinearVelocity(const Vec3 &linearVelocity) { if (body) body->SetLinearVelocity(linearVelocity); }

    const Vec3              GetAngularVelocity() const { return body ? body->GetAngularVelocity() : Vec3::zero; }
    void                    SetAngularVelocity(const Vec3 &angularVelocity) { if (body) body->SetAngularVelocity(angularVelocity); }

    const Vec3              GetLinearFactor() const { return body ? body->GetLinearFactor() : Vec3::zero; }
    void                    SetLinearFactor(const Vec3 &linearFactor) { if (body) body->SetLinearFactor(linearFactor); }

    const Vec3              GetAngularFactor() const { return body ? body->GetAngularFactor() : Vec3::zero; }
    void                    SetAngularFactor(const Vec3 &angularFactor) { if (body) body->SetAngularFactor(angularFactor); }

    const Vec3              GetTotalForce() const { return body ? body->GetTotalForce() : Vec3::zero; }
    const Vec3              GetTotalTorque() const { return body ? body->GetTotalTorque() : Vec3::zero; }

    void                    ClearForces() { body->ClearForces(); }
    void                    ClearVelocities() { body->ClearVelocities(); }

    void                    ApplyCentralForce(const Vec3 &force) { body->ApplyCentralForce(force); }
    void                    ApplyForce(const Vec3 &force, const Vec3 &relativePos) { body->ApplyForce(force, relativePos); }
    void                    ApplyTorque(const Vec3 &torque) { body->ApplyTorque(torque); }

    void                    ApplyCentralImpulse(const Vec3 &impulse) { body->ApplyCentralImpulse(impulse); }
    void                    ApplyImpulse(const Vec3 &impulse, const Vec3 &relativePos) { body->ApplyImpulse(impulse, relativePos); }
    void                    ApplyAngularImpulse(const Vec3 &impulse) { body->ApplyAngularImpulse(impulse); }

                            /// Returns internal rigid body pointer.
    PhysRigidBody *         GetBody() const { return body; }

    void                    Activate() { body->Activate(); }

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    CreateBody();
    void                    AddChildShapeRecursive(const Mat3x4 &parentWorldMatrixInverse, const Entity *entity, Array<PhysShapeDesc> &shapeDescs);
    void                    AddChildWheelRecursive(const Mat3x4 &parentWorldMatrixInverse, const Entity *entity, Array<PhysWheelDesc> &wheelDescs, Array<ComVehicleWheel *> &vehicleWheels);
    void                    ProcessScriptCallback();
    void                    TransformUpdated(const ComTransform *transform);

    class CollisionListener;

    PhysRigidBody *         body;
    PhysVehicle *           vehicle;
    PhysCollidableDesc      physicsDesc;
    CollisionListener *     collisionListener;
    Array<Collision>        collisions;
    Array<Collision>        oldCollisions;
    bool                    physicsUpdating;
};

BE_NAMESPACE_END
