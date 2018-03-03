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
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

const SignalDef ComRigidBody::SIG_PhysicsUpdated("ComRigidBody::PhysicsUpdated", "a");

OBJECT_DECLARATION("Rigid Body", ComRigidBody, Component)
BEGIN_EVENTS(ComRigidBody)
END_EVENTS

class ComRigidBody::CollisionListener : public PhysCollisionListener {
public:
    CollisionListener(ComRigidBody *body) { this->body = body; }
    virtual ~CollisionListener() {}

    virtual void Collide(const PhysCollidable *objectA, const PhysCollidable *objectB, const Vec3 &point, const Vec3 &normal, float distance, float impulse) override;

    ComRigidBody *body;
};

void ComRigidBody::CollisionListener::Collide(const PhysCollidable *objectA, const PhysCollidable *objectB, const Vec3 &point, const Vec3 &normal, float distance, float impulse) {
    Component *collisionComponent = reinterpret_cast<Component *>(objectB->GetUserPointer());
    if (!collisionComponent) {
        return;
    }

    ComRigidBody *collisionRigidBody = collisionComponent->Cast<ComRigidBody>();
    ComCharacterController *collisionController = collisionComponent->Cast<ComCharacterController>();

    if (collisionRigidBody || collisionController) {
        Collision collision;
        collision.entityGuid = collisionComponent->GetEntity()->GetGuid();
        collision.entity = collisionComponent->GetEntity();
        collision.body = collisionRigidBody;
        collision.controller = collisionController;
        collision.point = point;
        collision.normal = normal;
        collision.distance = distance;
        collision.impulse = impulse;

        Collision *existingCollision = body->collisions.Find(collision);
        if (!existingCollision) {
            body->collisions.Append(collision);
        } else if (impulse > existingCollision->impulse) {
            existingCollision->impulse = impulse;
            //BE_LOG(L"Collide with %hs, n = (%.2f %.2f %.2f), impulse = %.2f\n", collision.entity->GetName(), normal.x, normal.y, normal.z, impulse);
        }
    }
}

void ComRigidBody::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("mass", "Mass", float, GetMass, SetMass, 1.f, "Mass in kg", PropertyInfo::EditorFlag)
        .SetRange(0, 200, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("restitution", "Restitution", float, GetRestitution, SetRestitution, 0.f, "Parameter for make objects bounce", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("friction", "Friction", float, GetFriction, SetFriction, 1.f, "Parameter for make objects slide along each other realistically", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("rollingFriction", "Rolling Friction", float, GetRollingFriction, SetRollingFriction, 1.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("linearDamping", "Linear Damping", float, GetLinearDamping, SetLinearDamping, 0.05f, "Reduced amount of linear velocity", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("angularDamping", "Angular Damping", float, GetAngularDamping, SetAngularDamping, 0.01f, "Reduced amount of angular velocity", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("kinematic", "Kinematic", bool, IsKinematic, SetKinematic, false, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("ccd", "CCD", bool, IsCCD, SetCCD, false, "Continuous collision detection", PropertyInfo::EditorFlag);
}

ComRigidBody::ComRigidBody() {
    body = nullptr;
    collisionListener = nullptr;
    physicsUpdating = false;
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
    Component::Init();

    physicsDesc.type = PhysCollidable::Type::RigidBody;
    physicsDesc.shapes.Clear();

    // static rigid body can't have collision listener
    if (physicsDesc.mass > 0) {
        collisionListener = new CollisionListener(this);
    }

    // Mark as initialized
    SetInitialized(true);
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

        ComponentPtrArray colliders = entity->GetComponents(&ComCollider::metaObject);
        if (colliders.Count() > 0) {
            for (int i = 0; i < colliders.Count(); i++) {
                ComCollider *collider = colliders[i]->Cast<ComCollider>();

                PhysShapeDesc shapeDesc;
                shapeDesc.localOrigin = Vec3::zero;
                shapeDesc.localAxis.SetIdentity();
                shapeDesc.collider = collider->GetCollider();

                physicsDesc.shapes.Append(shapeDesc);
            }
        } 

        for (Entity *childEntity = entity->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            AddChildShapeRecursive(childEntity, physicsDesc.shapes);
        }

        if (physicsDesc.shapes.Count() == 0) {
            BE_WARNLOG(L"Entity %hs has rigid body but no associated colliders in its hierarchy\n", GetEntity()->GetName().c_str());
            return;
        }

        body = static_cast<PhysRigidBody *>(physicsSystem.CreateCollidable(&physicsDesc));
        body->SetUserPointer(this);
        body->SetCustomCollisionFilterIndex(entity->GetLayer());
        body->SetCollisionListener(collisionListener);

        if (IsActiveInHierarchy()) {
            body->AddToWorld(GetGameWorld()->GetPhysicsWorld());
        }

        transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComRigidBody::TransformUpdated, SignalObject::Unique);
    }

    collisions.Clear();
    oldCollisions.Clear();
}

void ComRigidBody::Update() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    if (!body) {
        return;
    }

    if (!body->IsStatic() && !body->IsKinematic() && body->IsActive()) {
        // Block SIG_TransformUpdated during SIG_PhysicsUpdated
        physicsUpdating = true;

        EmitSignal(&SIG_PhysicsUpdated, body);

        physicsUpdating = false;
    }

    ProcessScriptCallback();
}

void ComRigidBody::ProcessScriptCallback() {
    ComponentPtrArray scriptComponents = GetEntity()->GetComponents(&ComScript::metaObject);
    if (scriptComponents.Count() == 0) {
        return;
    }

    for (int newIndex = 0; newIndex < collisions.Count(); newIndex++) {
        const Collision &collision = collisions[newIndex];

        bool stay = false;
        for (int oldIndex = 0; oldIndex < oldCollisions.Count(); oldIndex++) {
            if (oldCollisions[oldIndex] == collision) {
                oldCollisions.RemoveIndexFast(oldIndex);
                stay = true;
                break;
            }
        }

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (stay) {
                scriptComponent->OnCollisionStay(collision);
            } else {
                scriptComponent->OnCollisionEnter(collision);
            }
        }
    }

    for (int oldIndex = 0; oldIndex < oldCollisions.Count(); oldIndex++) {
        const Collision &collision = oldCollisions[oldIndex];

        // Check if old collision entity is removed
        const Entity *entity = (Entity *)Entity::FindInstance(collision.entityGuid);
        if (!entity) {
            continue;
        }

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            scriptComponent->OnCollisionExit(collision);
        }
    }

    oldCollisions = collisions;
    collisions.Clear();
}

void ComRigidBody::OnActive() {
    if (body) {
        body->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComRigidBody::OnInactive() {
    if (body) {
        body->RemoveFromWorld();
    }
    collisions.Clear();
    oldCollisions.Clear();
}

void ComRigidBody::TransformUpdated(const ComTransform *transform) {
    if (physicsUpdating) {
        return;
    }

    if (body) {
        body->SetTransform(transform->GetAxis(), transform->GetOrigin());
        body->ClearForces();
        body->ClearVelocities();
        body->Activate();
    }
}

float ComRigidBody::GetMass() const { 
    return body ? body->GetMass() : physicsDesc.mass;
}

void ComRigidBody::SetMass(float mass) { 
    if (body) {
        body->SetMass(mass);
    } else {
        physicsDesc.mass = mass;
    }
}

float ComRigidBody::GetRestitution() const { 
    return body ? body->GetRestitution() : physicsDesc.restitution;
}

void ComRigidBody::SetRestitution(float restitution) {
    if (body) {
        body->SetRestitution(restitution);
    } else {
        physicsDesc.restitution = restitution;
    }
}

float ComRigidBody::GetFriction() const { 
    return body ? body->GetFriction() : physicsDesc.friction;
}

void ComRigidBody::SetFriction(float friction) { 
    if (body) {
        body->SetFriction(friction);
    } else {
        physicsDesc.friction = friction;
    }
}

float ComRigidBody::GetRollingFriction() const {
    return body ? body->GetRollingFriction() : physicsDesc.rollingFriction;
}

void ComRigidBody::SetRollingFriction(float rollingFriction) {
    if (body) {
        body->SetRollingFriction(rollingFriction);
    } else {
        physicsDesc.rollingFriction = rollingFriction;
    }
}

float ComRigidBody::GetLinearDamping() const { 
    return body ? body->GetLinearDamping() : physicsDesc.linearDamping;
}

void ComRigidBody::SetLinearDamping(float linearDamping) { 
    if (body) {
        body->SetLinearDamping(linearDamping);
    } else {
        physicsDesc.linearDamping = linearDamping;
    }
}

float ComRigidBody::GetAngularDamping() const { 
    return body ? body->GetAngularDamping() : physicsDesc.angularDamping;
}

void ComRigidBody::SetAngularDamping(float angularDamping) { 
    if (body) {
        body->SetAngularDamping(angularDamping);
    } else {
        physicsDesc.angularDamping = angularDamping;
    }
}

bool ComRigidBody::IsKinematic() const { 
    return body ? body->IsKinematic() : physicsDesc.kinematic;
}

void ComRigidBody::SetKinematic(bool kinematic) {
    if (body) {
        body->SetKinematic(kinematic);
    } else {
        physicsDesc.kinematic = kinematic;
    }
}

bool ComRigidBody::IsCCD() const { 
    return body ? body->IsCCD() : physicsDesc.ccd;
}

void ComRigidBody::SetCCD(bool enableCcd) { 
    if (body) {
        body->SetCCD(enableCcd);
    } else {
        physicsDesc.ccd = enableCcd;
    }
}

BE_NAMESPACE_END
