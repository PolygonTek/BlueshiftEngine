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

class btCollisionObject;

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class PhysCollidable;
class PhysCollisionListener;
class PhysicsWorld;
class PhysicsSystem;
class CollisionFilterCallback;

struct CastResult {
    PhysCollidable *        hitObject;
    Vec3                    point;
    Vec3                    normal;
    Vec3                    endPos;
    float                   fraction;
    int                     surfaceFlags;
};

struct Contact {
    PhysCollidable *        object;
    Vec3                    point;
    Vec3                    normal;
    float                   dist;
    float                   impulse;
};

class PhysCollidable {
    friend class PhysicsSystem;
    friend class PhysicsWorld;
    friend class CollisionFilterCallback;

public:
    struct Type {
        enum Enum {
            RigidBody,
            SoftBody,
            Sensor
        };
    };

    PhysCollidable(Type::Enum type, btCollisionObject *collisionObject, const Vec3 &centroid);
    virtual ~PhysCollidable() = 0;

    virtual const Vec3      GetOrigin() const;
    virtual void            SetOrigin(const Vec3 &origin);

    virtual const Mat3      GetAxis() const;
    virtual void            SetAxis(const Mat3 &axis);

    virtual void            SetTransform(const Mat3x4 &transform);

    virtual void            AddToWorld(PhysicsWorld *physicsWorld) = 0;
    virtual void            RemoveFromWorld() = 0;

    float                   GetRestitution() const;
    void                    SetRestitution(float rest);

    float                   GetFriction() const;
    void                    SetFriction(float friction);

    float                   GetRollingFriction() const;
    void                    SetRollingFriction(float friction);

    float                   GetSpinningFriction() const;
    void                    SetSpinningFriction(float friction);

    void *                  GetUserPointer() const { return userPointer; }
    void                    SetUserPointer(void *ptr) { userPointer = ptr; }

    void                    Activate();

    bool                    IsStatic() const;
    bool                    IsKinematic() const;
    bool                    IsCharacter() const;
    
    void                    SetKinematic(bool kinematic);
    void                    SetCharacter(bool character);

    bool                    IsActive() const;

    bool                    IsInWorld() const;

    void                    SetIgnoreCollisionCheck(const PhysCollidable &collidable, bool ignoreCollisionCheck);

    unsigned int            GetCollisionFilterBit() const { return collisionFilterBit; }
    void                    SetCollisionFilterBit(int bit);

    void                    SetDebugDraw(bool draw);

    PhysCollisionListener * GetCollisionListener() const { return collisionListener; }
    void                    SetCollisionListener(PhysCollisionListener *listener) { collisionListener = listener; }

protected:
    Type::Enum              type;                       ///< Collidable type
    Vec3                    centroid = Vec3::zero;      ///< Position of the center of mass in system units
    int                     collisionFilterBit = 0;
    int                     collisionFilterMask = BIT(0);
    btCollisionObject *     collisionObject = nullptr;
    PhysCollisionListener * collisionListener = nullptr;
    void *                  userPointer = nullptr;
    PhysicsWorld *          physicsWorld = nullptr;
};

BE_NAMESPACE_END
