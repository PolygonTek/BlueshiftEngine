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
    Vec3                    endpos;
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
    // same order with btCollisionObject::CollisionFilterGroups
    enum CollisionFilterGroup {
        DefaultGroup        = BIT(0),
        StaticGroup         = BIT(1),
        KinematicGroup      = BIT(2),
        //DebrisGroup         = BIT(3),
        SensorGroup         = BIT(4),
        CharacterGroup      = BIT(5),
        AllGroup            = -1
    };

    enum Type {
        RigidBody,
        Character,
        Sensor
    };

    PhysCollidable(Type type, btCollisionObject *collisionObject, const Vec3 &centroid);
    virtual ~PhysCollidable();

    virtual const Vec3      GetOrigin() const;
    virtual void            SetOrigin(const Vec3 &origin);

    virtual const Mat3      GetAxis() const;
    virtual void            SetAxis(const Mat3 &axis);

    virtual void            SetTransform(const Mat3 &axis, const Vec3 &origin);

    float                   GetRestitution() const;
    void                    SetRestitution(float rest);

    float                   GetFriction() const;
    void                    SetFriction(float friction);

    float                   GetRollingFriction() const;
    void                    SetRollingFriction(float friction);

    void *                  GetUserPointer() const;
    void                    SetUserPointer(void *ptr);

    void                    Activate();

    bool                    IsStatic() const;
    bool                    IsKinematic() const;
    bool                    IsCharacter() const;
    
    void                    SetKinematic(bool kinematic);
    void                    SetCharacter(bool character);

    bool                    IsActive() const;

    bool                    IsInWorld() const;
    void                    AddToWorld(PhysicsWorld *physicsWorld);
    void                    RemoveFromWorld();

    void                    SetIgnoreCollisionCheck(const PhysCollidable &collidable, bool ignoreCollisionCheck);

    unsigned int            GetCustomCollisionFilterIndex() const { return customFilterIndex; }
    void                    SetCustomCollisionFilterIndex(unsigned int index);

    void                    SetDebugDraw(bool draw);

    PhysCollisionListener * GetCollisionListener() const;
    void                    SetCollisionListener(PhysCollisionListener *listener);

protected:
    Type                    type;
    Vec3                    centroid;
    unsigned int            customFilterIndex;
    btCollisionObject *     collisionObject;
    PhysCollisionListener * collisionListener;
    void *                  userPointer;
    PhysicsWorld *          physicsWorld;
};

BE_NAMESPACE_END
