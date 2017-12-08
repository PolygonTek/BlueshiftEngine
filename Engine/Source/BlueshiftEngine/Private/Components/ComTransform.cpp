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
#include "Components/ComRigidBody.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

const SignalDef ComTransform::SIG_TransformUpdated("ComTransform::TransformUpdated", "a");

OBJECT_DECLARATION("Transform", ComTransform, Component)
BEGIN_EVENTS(ComTransform)
END_EVENTS

void ComTransform::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("origin", "Origin", Vec3, GetLocalOrigin, SetLocalOrigin, Vec3::zero, "xyz position in local space", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Angles::zero, "yaw, pitch, roll in degree in local space", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("scale", "Scale", Vec3, GetLocalScale, SetLocalScale, Vec3::one, "xyz scale in local space", PropertyInfo::EditorFlag);
}

ComTransform::ComTransform() {
    localOrigin = Vec3::zero;
    localScale = Vec3::one;
    localAxis = Mat3::identity;
    worldMatrix = Mat3x4::identity;
    dirty = false;
    physicsUpdating = false;
}

ComTransform::~ComTransform() {
}

ComTransform *ComTransform::GetParent() const { 
    Entity *parent = GetEntity()->GetNode().GetParent();
    if (!parent) {
        return nullptr;
    }
    return parent->GetTransform();
}

void ComTransform::Init() {
    Component::Init();

    UpdateWorldTransform();

    // If this entity has a rigid body component, connect it to update the transform due to the physics simulation.
    ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    if (rigidBody) {
        rigidBody->Connect(&ComRigidBody::SIG_PhysicsUpdated, this, (SignalCallback)&ComTransform::PhysicsUpdated, SignalObject::Unique);
    }

    // Mark as initialized
    SetInitialized(true);
}

void ComTransform::SetLocalOrigin(const Vec3 &origin) {
    this->localOrigin = origin;

    if (IsInitialized()) {
        MarkDirty();
    }
}

void ComTransform::SetLocalScale(const Vec3 &scale) {
    this->localScale = scale;

    if (IsInitialized()) {
        MarkDirty();
    }
}

void ComTransform::SetLocalAxis(const Mat3 &axis) {
    this->localAxis = axis;

    if (IsInitialized()) {
        MarkDirty();
    }
}

void ComTransform::SetLocalTransform(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    this->localOrigin = origin;
    this->localAxis = axis;
    this->localScale = scale;

    if (IsInitialized()) {
        MarkDirty();
    }
}

Vec3 ComTransform::GetOrigin() const {
    if (dirty) {
        UpdateWorldTransform();
    }
    return worldMatrix.ToTranslationVec3();
}

Mat3 ComTransform::GetAxis() const {
    if (dirty) {
        UpdateWorldTransform();
    }
    Mat3 axis = worldMatrix.ToMat3();
    axis.OrthoNormalizeSelf();
    return axis;
}

Vec3 ComTransform::GetScale() const {
    if (dirty) {
        UpdateWorldTransform();
    }
    return worldMatrix.ToScaleVec3();
}

const Mat3x4 &ComTransform::GetTransform() const {
    if (dirty) {
        UpdateWorldTransform();
    }
    return worldMatrix;
}

void ComTransform::SetOrigin(const Vec3 &origin) {
    const ComTransform *parent = GetParent();
    SetLocalOrigin(parent ? parent->GetTransform().Inverse() * origin : origin);
}

void ComTransform::SetAxis(const Mat3 &axis) {
    const ComTransform *parent = GetParent();
    SetLocalAxis(parent ? parent->GetAxis().Transpose() * axis : axis);
}

void ComTransform::SetScale(const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    SetLocalScale(parent ? scale / parent->GetScale() : scale);
}

void ComTransform::SetTransform(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    SetOrigin(origin);
    SetScale(scale);
    SetAxis(axis);
}

void ComTransform::TranslateLocal(const Vec3 &translation) {
    SetLocalOrigin(GetLocalOrigin() + translation);
}

void ComTransform::RotateLocal(const Vec3 &axis, float angle) {
    Mat3 rotation = Rotation(Vec3::zero, axis, angle).ToMat3();
    SetLocalAxis(rotation * GetLocalAxis());
}

void ComTransform::Translate(const Vec3 &translation) {
    SetOrigin(GetOrigin() + translation);
}

void ComTransform::Rotate(const Vec3 &axis, float angle) { 
    Mat3 rotation = Rotation(Vec3::zero, axis, angle).ToMat3();
    SetAxis(rotation * GetAxis()); 
}

void ComTransform::MarkDirty() {
    // Precondition:
    // a) whenever a transform is marked dirty, all its children are marked dirty as well.
    // b) whenever a transform is cleared from being dirty, all its parents must have been
    //    cleared as well.
    if (dirty) {
        return;
    }
    dirty = true;

    // World matrix should be updated so we emit this signal
    EmitSignal(&SIG_TransformUpdated, this);

    if (physicsUpdating) {
        for (Entity *childEntity = GetEntity()->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            if (childEntity->GetComponent(&ComRigidBody::metaObject)) {
                continue;
            }

            childEntity->GetTransform()->MarkDirty();
        }
    } else {
        for (Entity *childEntity = GetEntity()->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            childEntity->GetTransform()->MarkDirty();
        }
    }
}

void ComTransform::UpdateWorldTransform() const {
    Mat3x4 localTransform = GetLocalTransform();

    ComTransform *parent = GetParent();

    if (parent) {
        worldMatrix = parent->GetTransform() * localTransform;
    } else {
        worldMatrix = localTransform;
    }

    dirty = false;
}

// Called by ComRigidBody::Update()
void ComTransform::PhysicsUpdated(const PhysRigidBody *body) {
    physicsUpdating = true;

    SetTransform(body->GetOrigin(), body->GetAxis(), GetScale());

    physicsUpdating = false;
}

BE_NAMESPACE_END
