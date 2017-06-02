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
#include "Components/ComSensor.h"
#include "Components/ComRigidBody.h"
#include "Components/ComScript.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Sensor", ComSensor, Component)
BEGIN_EVENTS(ComSensor)
END_EVENTS
BEGIN_PROPERTIES(ComSensor)
END_PROPERTIES

void ComSensor::RegisterProperties() {
}

ComSensor::ComSensor() {
    sensor = nullptr;

    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComSensor::PropertyChanged);
}

ComSensor::~ComSensor() {
    Purge(false);
}

void ComSensor::Purge(bool chainPurge) {
    if (sensor) {
        physicsSystem.DestroyCollidable(sensor);
        sensor = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComSensor::Init() {
    Purge();

    Component::Init();

    memset(&physicsDesc, 0, sizeof(physicsDesc));
    physicsDesc.type = PhysCollidable::Sensor;

    physicsDesc.shapes.Clear();
}

static void AddChildShapeRecursive(const Entity *entity, Array<PhysShapeDesc> &shapes) {
    const ComCollider *collider = entity->GetComponent<ComCollider>();
    if (!collider) {
        return;
    }

    const ComSensor *sensor = entity->GetComponent<ComSensor>();
    if (sensor) {
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

void ComSensor::Awake() {
    if (!sensor) {
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

        sensor = static_cast<PhysSensor *>(physicsSystem.CreateCollidable(&physicsDesc));
        sensor->SetUserPointer(this);
        sensor->SetCollisionFilterMask(PhysCollidable::DefaultGroup | PhysCollidable::CharacterGroup | PhysCollidable::KinematicGroup | PhysCollidable::StaticGroup);

        if (IsEnabled()) {
            sensor->AddToWorld(GetGameWorld()->GetPhysicsWorld());
        }

        transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComSensor::TransformUpdated, SignalObject::Unique);

        ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
        if (rigidBody) {
            rigidBody->Connect(&ComRigidBody::SIG_PhysicsUpdated, this, (SignalCallback)&ComSensor::PhysicsUpdated, SignalObject::Unique);
        }
    }

    oldColliderArray.Clear();
}

void ComSensor::Update() {
    if (!IsEnabled()) {
        return;
    }

    Array<PhysCollidable *> newColliders;
    sensor->GetOverlaps(newColliders);

    for (int newIndex = 0; newIndex < newColliders.Count(); newIndex++) {
        const PhysCollidable *collider = newColliders[newIndex];

        bool stay = false;
        for (int oldIndex = 0; oldIndex < oldColliderArray.Count(); oldIndex++) {
            if (oldColliderArray[oldIndex] == collider) {
                oldColliderArray.RemoveIndexFast(oldIndex);
                stay = true;
                break;
            }
        }

        // Get the collided entity
        const Entity *entity = reinterpret_cast<Component *>(collider->GetUserPointer())->GetEntity();
        if (entity == GetEntity()) {
            // Ignore self
            continue;
        }

        ComponentPtrArray scriptComponents = GetEntity()->GetComponents(ComScript::metaObject);
        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (stay) {
                scriptComponent->OnSensorStay(entity);
            } else {
                scriptComponent->OnSensorEnter(entity);
            }
        }
    }

    for (int oldIndex = 0; oldIndex < oldColliderArray.Count(); oldIndex++) {
        const PhysCollidable *collider = oldColliderArray[oldIndex];

        // Get the collided entity in past
        const Entity *entity = reinterpret_cast<Component *>(collider->GetUserPointer())->GetEntity();
        if (entity == GetEntity()) {
            // Ignore self
            continue;
        }

        ComponentPtrArray scriptComponents = GetEntity()->GetComponents(ComScript::metaObject);
        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            scriptComponent->OnSensorExit(entity);
        }
    }

    oldColliderArray = newColliders;
}

void ComSensor::Enable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            if (sensor) {
                sensor->AddToWorld(GetGameWorld()->GetPhysicsWorld());
            }
            Component::Enable(true);
        }
    } else {
        if (IsEnabled()) {
            if (sensor) {
                sensor->RemoveFromWorld();
            }
            Component::Enable(false);
        }
    }
}

void ComSensor::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    // TODO: draw blend mesh
    /*ComponentPtrArray colliderComponents = GetEntity()->GetComponents(ComCollider::metaObject);

    for (int i = 0; i < colliderComponents.Count(); i++) {
        colliderComponents[i]->DrawGizmos(sceneView, true);
    }*/
}

void ComSensor::TransformUpdated(const ComTransform *transform) {
    if (sensor) {
        sensor->SetOrigin(transform->GetOrigin());
        sensor->SetAxis(transform->GetAxis());
        sensor->Activate();
    }
}

void ComSensor::PhysicsUpdated(const PhysRigidBody *body) {
    if (sensor) {
        sensor->SetOrigin(body->GetOrigin());
        sensor->SetAxis(body->GetAxis());
        sensor->Activate();
    }
}

void ComSensor::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
