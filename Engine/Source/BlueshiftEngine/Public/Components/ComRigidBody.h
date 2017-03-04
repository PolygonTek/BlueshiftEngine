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

class ComTransform;
class ComRigidBody;
class ComCharacterController;

class Collision {
public:
    bool                    operator==(const Collision &rhs) const { return entity == rhs.entity && body == rhs.body && controller == rhs.controller; }

    Entity *                entity;
    ComRigidBody *          body;
    ComCharacterController *controller;
    Vec3                    point;
    Vec3                    normal;
    float                   distance;
    float                   impulse;
};
    
class ComRigidBody : public Component {
public:
    OBJECT_PROTOTYPE(ComRigidBody);

    class CollisionListener : public PhysCollisionListener {
    public:
        CollisionListener(ComRigidBody *body) { this->body = body; }
        virtual ~CollisionListener() {}

        virtual void        Collide(const PhysCollidable *objectA, const PhysCollidable *objectB, const Vec3 &point, const Vec3 &normal, float distance, float impulse) override;

        ComRigidBody *      body;
    };

    ComRigidBody();
    virtual ~ComRigidBody();

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            Enable(bool enable) override;

    virtual void            Update() override;

    const Vec3              GetOrigin() const { return body ? body->GetOrigin() : Vec3::origin; }
    void                    SetOrigin(const Vec3 &origin) { if (body) body->SetOrigin(origin); }

    const Mat3              GetAxis() const { return body ? body->GetAxis() : Mat3::identity; }
    void                    SetAxis(const Mat3 &axis) { if (body) body->SetAxis(axis); }

    float                   GetMass() const { return body ? body->GetMass() : 0; }
    void                    SetMass(float mass) { if (body) body->SetMass(mass); }

    float                   GetRestitution() const { return body ? body->GetRestitution() : 0; }
    void                    SetRestitution(float rest) { if (body) body->SetRestitution(rest); }

    float                   GetFriction() const { return body ? body->GetFriction() : 0; }
    void                    SetFriction(float friction) { if (body) body->SetFriction(friction); }

    float                   GetRollingFriction() const { return body ? body->GetRollingFriction() : 0; }
    void                    SetRollingFriction(float friction) { if (body) body->SetRollingFriction(friction); }

    bool                    IsKinematic() const { return body ? body->IsKinematic() : false; }
    void                    SetKinematic(bool kinematic) { if (body) body->SetKinematic(kinematic); }

    const Vec3              GetGravity() const { return body ? body->GetGravity() : Vec3::zero; }
    void                    SetGravity(const Vec3 &gravityAcceleration) { if (body) body->SetGravity(gravityAcceleration); }

    float                   GetLinearDamping() const { return body ? body->GetLinearDamping() : 0; }
    void                    SetLinearDamping(float linearDamping) { if (body) body->SetLinearDamping(linearDamping); }

    float                   GetAngularDamping() const { return body ? body->GetAngularDamping() : 0; }
    void                    SetAngularDamping(float angularDamping) { if (body) body->SetAngularDamping(angularDamping); }

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

    bool                    IsCCD() const { return body ? body->IsCCD() : false; }
    void                    SetCCD(bool enableCcd) { if (body) body->SetCCD(enableCcd); }

    void                    ClearForces() { body->ClearForces(); }
    void                    ClearVelocities() { body->ClearVelocities(); }

    void                    ApplyCentralForce(const Vec3 &force) { body->ApplyCentralForce(force); }
    void                    ApplyForce(const Vec3 &force, const Vec3 &relativePos) { body->ApplyForce(force, relativePos); }
    void                    ApplyTorque(const Vec3 &torque) { body->ApplyTorque(torque); }

    void                    ApplyCentralImpulse(const Vec3 &impulse) { body->ApplyCentralImpulse(impulse); }
    void                    ApplyImpulse(const Vec3 &impulse, const Vec3 &relativePos) { body->ApplyImpulse(impulse, relativePos); }
    void                    ApplyAngularImpulse(const Vec3 &impulse) { body->ApplyAngularImpulse(impulse); }

    PhysRigidBody *         GetBody() const { return body; }

protected:
    void                    PropertyChanged(const char *classname, const char *propName);
    void                    TransformUpdated(const ComTransform *transform);

    PhysRigidBody *         body;
    PhysCollidableDesc      physicsDesc;
    CollisionListener *     collisionListener;
    Array<Collision>        collisionArray;
    Array<Collision>        oldCollisionArray;
};

extern const SignalDef      SIG_PhysicsUpdated;

BE_NAMESPACE_END
