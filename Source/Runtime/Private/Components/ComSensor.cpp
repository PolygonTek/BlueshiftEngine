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
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Sensor", ComSensor, Component)
BEGIN_EVENTS(ComSensor)
END_EVENTS

void ComSensor::RegisterProperties() {
}

ComSensor::ComSensor() {
    memset(&physicsDesc, 0, sizeof(physicsDesc));
    sensor = nullptr;
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
    Component::Init();

    physicsDesc.type = PhysCollidable::Sensor;
    physicsDesc.shapes.Clear();

    // Mark as initialized
    SetInitialized(true);
}

void ComSensor::AddChildShapeRecursive(const Mat3x4 &parentWorldMatrixInverse, const Entity *entity, Array<PhysShapeDesc> &shapes) {
    if (entity->GetComponent<ComRigidBody>() || entity->GetComponent<ComSensor>()) {
        return;
    }

    ComCollider *collider = entity->GetComponent<ComCollider>();
    if (!collider) {
        return;
    }

    collider->CreateCollider();

    const ComTransform *transform = entity->GetTransform();

    Mat3x4 localTransform = parentWorldMatrixInverse * Mat3x4(transform->GetAxis(), transform->GetOrigin());
    localTransform.FixDegeneracies();

    PhysShapeDesc &shapeDesc = shapes.Alloc();
    shapeDesc.collider = collider->GetCollider();
    shapeDesc.localOrigin = localTransform.ToTranslationVec3();
    shapeDesc.localAxis = localTransform.ToMat3();

    for (Entity *childEntity = entity->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        AddChildShapeRecursive(parentWorldMatrixInverse, childEntity, shapes);
    }
}

void ComSensor::Awake() {
    if (!sensor) {
        CreateSensor();

        if (sensor) {
            if (IsActiveInHierarchy()) {
                sensor->AddToWorld(GetGameWorld()->GetPhysicsWorld());
            }

            ComTransform *transform = GetEntity()->GetTransform();
            transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComSensor::TransformUpdated, SignalObject::Unique);
        }
    }

    oldColliders.Clear();
}

void ComSensor::CreateSensor() {
    ComTransform *transform = GetEntity()->GetTransform();

    physicsDesc.origin = transform->GetOrigin();
    physicsDesc.axis = transform->GetAxis();

    // Collect collider shadpes in this entity
    ComponentPtrArray colliders = entity->GetComponents(&ComCollider::metaObject);
    if (colliders.Count() > 0) {
        for (int i = 0; i < colliders.Count(); i++) {
            ComCollider *collider = colliders[i]->Cast<ComCollider>();
            collider->CreateCollider();

            PhysShapeDesc shapeDesc;
            shapeDesc.localOrigin = Vec3::zero;
            shapeDesc.localAxis.SetIdentity();
            shapeDesc.collider = collider->GetCollider();

            physicsDesc.shapes.Append(shapeDesc);
        }
    }

    // Collect collider shadpes in children recursively
    for (Entity *childEntity = entity->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        AddChildShapeRecursive(transform->GetMatrixNoScale().Inverse(), childEntity, physicsDesc.shapes);
    }

    if (physicsDesc.shapes.Count() == 0) {
        BE_WARNLOG(L"Entity %hs has sensor but no associated colliders in its hierarchy\n", GetEntity()->GetName().c_str());
    }

    sensor = static_cast<PhysSensor *>(physicsSystem.CreateCollidable(physicsDesc));
    sensor->SetUserPointer(this);
    sensor->SetCollisionFilterBit(entity->GetLayer());
}

void ComSensor::Update() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    ProcessScriptCallback();
}

void ComSensor::ProcessScriptCallback() {
    ComponentPtrArray scriptComponents = GetEntity()->GetComponents(&ComScript::metaObject);
    if (scriptComponents.Count() == 0) {
        return;
    }

    Array<PhysCollidable *> colliders;
    sensor->GetOverlaps(colliders);

    Array<Guid> newColliders;
    newColliders.SetCount(colliders.Count());

    for (int index = 0; index < colliders.Count(); index++) {
        const Entity *entity = reinterpret_cast<Component *>(colliders[index]->GetUserPointer())->GetEntity();

        newColliders.Append(entity->GetGuid());
    }

    for (int newIndex = 0; newIndex < newColliders.Count(); newIndex++) {
        const Guid &collider = newColliders[newIndex];

        bool stay = false;
        for (int oldIndex = 0; oldIndex < oldColliders.Count(); oldIndex++) {
            if (oldColliders[oldIndex] == collider) {
                oldColliders.RemoveIndexFast(oldIndex);
                stay = true;
                break;
            }
        }

        // Get the collided entity
        const Entity *entity = (Entity *)Entity::FindInstance(collider);
        if (!entity || entity == GetEntity()) {
            continue;
        }

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (stay) {
                scriptComponent->OnSensorStay(entity);
            } else {
                scriptComponent->OnSensorEnter(entity);
            }
        }
    }

    for (int oldIndex = 0; oldIndex < oldColliders.Count(); oldIndex++) {
        const Guid &collider = oldColliders[oldIndex];

        // Get the collided entity in past
        const Entity *entity = (Entity *)Entity::FindInstance(collider);
        if (!entity || entity == GetEntity()) {
            continue;
        }

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            scriptComponent->OnSensorExit(entity);
        }
    }

    oldColliders.Swap(newColliders);
}

void ComSensor::OnActive() {
    if (sensor) {
        sensor->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComSensor::OnInactive() {
    if (sensor) {
        sensor->RemoveFromWorld();
    }
}

void ComSensor::DrawGizmos(const RenderView::State &viewState, bool selected) {
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

BE_NAMESPACE_END
