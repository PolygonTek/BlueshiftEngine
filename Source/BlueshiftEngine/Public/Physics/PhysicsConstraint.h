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

class btTypedConstraint;

BE_NAMESPACE_BEGIN

class PhysRigidBody;
class PhysicsWorld;
class PhysicsSystem;

class PhysConstraint {
    friend class PhysicsSystem;

public:
    enum Type {
        Generic,
        GenericSpring,
        Point2Point,
        Hinge,
        Slider
    };

    PhysConstraint() {}
    PhysConstraint(PhysRigidBody *bodyA, PhysRigidBody *bodyB);
    virtual ~PhysConstraint() = 0;

    const PhysRigidBody *   RigidBodyA() const { return bodyA; } 
    PhysRigidBody *         RigidBodyA() { return bodyA; }

    const PhysRigidBody *   RigidBodyB() const { return bodyB; } 
    PhysRigidBody *         RigidBodyB() { return bodyB; }

    bool                    IsCollisionEnabled() const { return collisionEnabled; }
    void                    EnableCollision(bool enable);
    
    float                   GetBreakImpulse() const;
    void                    SetBreakImpulse(float impulse);

    bool                    IsEnabled() const;
    void                    SetEnabled(bool enabled);

    bool                    IsInWorld() const;
    void                    AddToWorld(PhysicsWorld *physicsWorld);
    void                    RemoveFromWorld();

    void                    Reset();

protected:
    PhysRigidBody *         bodyA;
    PhysRigidBody *         bodyB;
    btTypedConstraint *     constraint;
    bool                    collisionEnabled;
    PhysicsWorld *          physicsWorld;
};

BE_NAMESPACE_END
