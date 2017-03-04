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
#include "Components/ComTransform.h"
#include "Components/ComCollider.h"
#include "Components/ComRigidBody.h"
#include "Components/ComCharacterController.h"
#include "Components/ComSensor.h"
#include "Components/ComScript.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

const SignalDef     SIG_PhysicsUpdated("physicsUpdated", "a");

OBJECT_DECLARATION("Rigid Body", ComRigidBody, Component)
BEGIN_EVENTS(ComRigidBody)
END_EVENTS
BEGIN_PROPERTIES(ComRigidBody)
    PROPERTY_RANGED_FLOAT("mass", "Mass", "kg", Rangef(0, 100, 0.01f), "1", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("restitution", "Restitution", "parameter for make objects bounce", Rangef(0, 1, 0.01f), "0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("friction", "Friction", "parameter for make objects slide along each other realistically", Rangef(0, 1, 0.01f), "1", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("rollingFriction", "Rolling Friction", "", Rangef(0, 1, 0.01f), "1", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("linearDamping", "Linear Damping", "reduced amount of linear velocity", Rangef(0, 1, 0.01f), "0.05", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("angularDamping", "Angular Damping", "reduced amount of angular velocity", Rangef(0, 1, 0.01f), "0.01", PropertySpec::ReadWrite),
    PROPERTY_BOOL("kinematic", "Kinematic", "", "false", PropertySpec::ReadWrite),
    PROPERTY_BOOL("ccd", "CCD", "continuous collision detection", "false", PropertySpec::ReadWrite),
END_PROPERTIES

void ComRigidBody::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Mass", float, GetMass, SetMass, "1", PropertySpec::ReadWrite).SetRange(0, 200, 0.01f);
    //REGISTER_ACCESSOR_PROPERTY("Restitution", float, GetRestitution, SetRestitution, "0", PropertySpec::ReadWrite).SetRange(0, 1, 0.01f);
    //REGISTER_ACCESSOR_PROPERTY("Friction", float, GetFriction, SetFriction, "1", PropertySpec::ReadWrite).SetRange(0, 1, 0.01f);
    //REGISTER_ACCESSOR_PROPERTY("Rolling Friction", float, GetRollingFriction, SetRollingFriction, "1", PropertySpec::ReadWrite).SetRange(0, 1, 0.01f);
    //REGISTER_ACCESSOR_PROPERTY("Linear Damping", float, GetLinearDamping, SetLinearDamping, "0.05", PropertySpec::ReadWrite).SetRange(0, 1, 0.01f);
    //REGISTER_ACCESSOR_PROPERTY("Angular Damping", float, GetAngularDamping, SetAngularDamping, "0.01", PropertySpec::ReadWrite).SetRange(0, 1, 0.01f);
    //REGISTER_ACCESSOR_PROPERTY("Kinematic", bool, IsKinematic, SetKinematic, "false", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("CCD", bool, IsCCD, SetCCD, "false", PropertySpec::ReadWrite);
}

ComRigidBody::ComRigidBody() {
    body = nullptr;
    collisionListener = nullptr;
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComRigidBody::PropertyChanged);
}

ComRigidBody::~ComRigidBody() {
    Purge(false);
}

void ComRigidBody::Purge(bool chainPurge) {
    if (body) {
        physicsSystem.DestroyCollidable(body);
        body = nullptr;
    }

    if (collisionListener) {
        delete collisionListener;
        collisionListener = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComRigidBody::Init() {
    Purge();

    Component::Init();

    physicsDesc.type            = PhysCollidable::Type::RigidBody;
    physicsDesc.mass            = props->Get("mass").As<float>();
    physicsDesc.restitution     = props->Get("restitution").As<float>();
    physicsDesc.friction        = props->Get("friction").As<float>();
    physicsDesc.rollingFriction = props->Get("rollingFriction").As<float>();
    physicsDesc.linearDamping   = props->Get("linearDamping").As<float>();
    physicsDesc.angularDamping  = props->Get("angularDamping").As<float>();
    physicsDesc.kinematic       = props->Get("kinematic").As<bool>();
    physicsDesc.ccd             = props->Get("ccd").As<bool>();

    physicsDesc.shapes.Clear();

    // static rigid body can't have collision listener
    if (physicsDesc.mass > 0) {
        collisionListener = new CollisionListener(this);
    }
}

static void AddChildShapeRecursive(const Entity *entity, Array<PhysShapeDesc> &shapes) {
    if (entity->GetComponent<ComRigidBody>() || entity->GetComponent<ComSensor>()) {
        return;
    }

    const ComCollider *collider = entity->GetComponent<ComCollider>();
    if (!collider) {
        return;
    }

    ComTransform *transform = entity->GetTransform();

    PhysShapeDesc &shapeDesc = shapes.Alloc();
    shapeDesc.collider = collider->GetCollider();
    shapeDesc.localOrigin = transform->GetScale() * transform->GetLocalOrigin();
    shapeDesc.localAxis = transform->GetLocalAxis();
    shapeDesc.localAxis.FixDegeneracies();
    
    for (Entity *childEntity = entity->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        AddChildShapeRecursive(childEntity, shapes);
    }
}

void ComRigidBody::Awake() {
    if (!body) {
        ComTransform *transform = GetEntity()->GetTransform();

        physicsDesc.origin = transform->GetOrigin();
        physicsDesc.axis = transform->GetAxis();

        ComponentPtrArray colliders = entity->GetComponents(ComCollider::metaObject);
        if (colliders.Count() > 0) {
            for (int i = 0; i < colliders.Count(); i++) {
                ComCollider *collider = colliders[i]->Cast<ComCollider>();

                PhysShapeDesc shapeDesc;
                shapeDesc.localOrigin = Vec3::zero;
                shapeDesc.localAxis.SetIdentity();
                shapeDesc.collider = collider->GetCollider();

                physicsDesc.shapes.Append(shapeDesc);
            }
        } else {
            return;
        }

        for (Entity *childEntity = entity->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            AddChildShapeRecursive(childEntity, physicsDesc.shapes);
        }

        body = static_cast<PhysRigidBody *>(physicsSystem.CreateCollidable(&physicsDesc));
        body->SetUserPointer(this);
        body->SetCollisionListener(collisionListener);        

        if (IsEnabled()) {
            body->AddToWorld(GetGameWorld()->GetPhysicsWorld());
        }

        transform->Connect(&SIG_TransformUpdated, this, (SignalCallback)&ComRigidBody::TransformUpdated, SignalObject::Unique);
    }

    collisionArray.Clear();
    oldCollisionArray.Clear();
}

void ComRigidBody::CollisionListener::Collide(const PhysCollidable *objectA, const PhysCollidable *objectB, const Vec3 &point, const Vec3 &normal, float distance, float impulse) {
    Component *collisionComponent = reinterpret_cast<Component *>(objectB->GetUserPointer());
    if (!collisionComponent) {
        return;
    }

    ComRigidBody *collisionRigidBody = collisionComponent->Cast<ComRigidBody>();
    ComCharacterController *collisionController = collisionComponent->Cast<ComCharacterController>();

    if (collisionRigidBody || collisionController) {
        Collision collision;
        collision.entity = collisionComponent->GetEntity();
        collision.body = collisionRigidBody;
        collision.controller = collisionController;
        collision.point = point;
        collision.normal = normal;
        collision.distance = distance;
        collision.impulse = impulse;

        Collision *existingCollision = body->collisionArray.Find(collision);
        if (!existingCollision) {
            body->collisionArray.Append(collision);
        } else if (impulse > existingCollision->impulse) {
            existingCollision->impulse = impulse;
            //BE_LOG(L"Collide with %hs, n = (%.2f %.2f %.2f), impulse = %.2f\n", collision.entity->GetName(), normal.x, normal.y, normal.z, impulse);
        }

    }
}

void ComRigidBody::Update() {
    if (!IsEnabled()) {
        return;
    }

    if (body) {
        EmitSignal(&SIG_PhysicsUpdated, body);
    }

    for (int newIndex = 0; newIndex < collisionArray.Count(); newIndex++) {
        const Collision &collision = collisionArray[newIndex];

        bool stay = false;
        for (int oldIndex = 0; oldIndex < oldCollisionArray.Count(); oldIndex++) {
            if (oldCollisionArray[oldIndex] == collision) {
                oldCollisionArray.RemoveIndexFast(oldIndex);
                stay = true;
                break;
            }
        }

        ComponentPtrArray scriptComponents = GetEntity()->GetComponents(ComScript::metaObject);
        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (stay) {
                scriptComponent->OnCollisionStay(collision);
            } else {
                scriptComponent->OnCollisionEnter(collision);
            }
        }
    }

    for (int oldIndex = 0; oldIndex < oldCollisionArray.Count(); oldIndex++) {
        const Collision &collision = oldCollisionArray[oldIndex];

        ComponentPtrArray scriptComponents = GetEntity()->GetComponents(ComScript::metaObject);
        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            scriptComponent->OnCollisionExit(collision);
        }
    }

    oldCollisionArray = collisionArray;
    collisionArray.Clear();
}

void ComRigidBody::Enable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            if (body) {
                body->AddToWorld(GetGameWorld()->GetPhysicsWorld());
            }
            Component::Enable(true);
        }
    } else {
        if (IsEnabled()) {
            if (body) {
                body->RemoveFromWorld();
            }
            Component::Enable(false);
        }
    }
}

void ComRigidBody::TransformUpdated(const ComTransform *transform) {
    if (body) {
        body->SetOrigin(transform->GetOrigin());
        body->SetAxis(transform->GetAxis());
        body->ClearForces();
        body->ClearVelocities();
        body->Activate();
    }
}

void ComRigidBody::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }
    
    if (!Str::Cmp(propName, "mass")) {
        SetMass(props->Get("mass").As<float>());
        return;
    } 
    
    if (!Str::Cmp(propName, "restitution")) {
        SetRestitution(props->Get("restitution").As<float>());
        return;
    } 
    
    if (!Str::Cmp(propName, "friction")) {
        SetFriction(props->Get("friction").As<float>());
        return;
    } 
    
    if (!Str::Cmp(propName, "rollingFriction")) {
        SetRollingFriction(props->Get("rollingFriction").As<float>());
        return;
    } 
    
    if (!Str::Cmp(propName, "linearDamping")) {
        SetLinearDamping(props->Get("linearDamping").As<float>());
        return;
    }
    
    if (!Str::Cmp(propName, "angularDamping")) {
        SetAngularDamping(props->Get("angularDamping").As<float>());
        return;
    }
    
    if (!Str::Cmp(propName, "kinematic")) {
        SetKinematic(props->Get("kinematic").As<bool>());
        return;
    }
    
    if (!Str::Cmp(propName, "ccd")) {
        SetCCD(props->Get("ccd").As<bool>());
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END

