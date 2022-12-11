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

class btRigidBody;

BE_NAMESPACE_BEGIN

class PhysVehicle;

class PhysRigidBody : public PhysCollidable {
    friend class PhysicsSystem;
    friend class PhysConstraint;
    friend class PhysGenericConstraint;
    friend class PhysGenericSpringConstraint;
    friend class PhysP2PConstraint;
    friend class PhysHingeConstraint;
    friend class PhysSliderConstraint;
    friend class PhysSwingTwistConstraint;
    friend class PhysVehicle;

public:
    PhysRigidBody(btRigidBody *rididBody, const Vec3 &centroid);
    virtual ~PhysRigidBody();

    virtual const Vec3      GetOrigin() const override;
    virtual void            SetOrigin(const Vec3 &origin) override;

    virtual const Mat3      GetAxis() const override;
    virtual void            SetAxis(const Mat3 &axis) override;

    virtual void            SetTransform(const Mat3x4 &transform) override;

    virtual void            AddToWorld(PhysicsWorld *physicsWorld) override;
    virtual void            RemoveFromWorld() override;

    float                   GetMass() const;
    void                    SetMass(float mass);

    const Vec3              GetGravity() const;
    void                    SetGravity(const Vec3 &gravityAcceleration);

    float                   GetLinearDamping() const;
    void                    SetLinearDamping(float linearDamping);

    float                   GetAngularDamping() const;
    void                    SetAngularDamping(float angularDamping);

    const Vec3              GetLinearVelocity() const;
    void                    SetLinearVelocity(const Vec3 &linearVelocity);

    const Vec3              GetAngularVelocity() const;
    void                    SetAngularVelocity(const Vec3 &angularVelocity);

    const Vec3              GetLinearFactor() const;
    void                    SetLinearFactor(const Vec3 &linearFactor);

    const Vec3              GetAngularFactor() const;
    void                    SetAngularFactor(const Vec3 &angularFactor);

    const Vec3              GetTotalForce() const;
    const Vec3              GetTotalTorque() const;

    bool                    IsCCD() const;
    void                    SetCCD(bool enableCcd);

    void                    ClearForces();
    void                    ClearVelocities();

    void                    ApplyCentralForce(const Vec3 &force);
    void                    ApplyForce(const Vec3 &force, const Vec3 &worldPos);
    void                    ApplyTorque(const Vec3 &torque);

    void                    ApplyCentralImpulse(const Vec3 &impulse);
    void                    ApplyImpulse(const Vec3 &impulse, const Vec3 &worldPos);
    void                    ApplyAngularImpulse(const Vec3 &impulse);

private:
    btRigidBody *           GetRigidBody();
    const btRigidBody *     GetRigidBody() const;

    PhysVehicle *           vehiclePtr = nullptr;
};

BE_NAMESPACE_END
