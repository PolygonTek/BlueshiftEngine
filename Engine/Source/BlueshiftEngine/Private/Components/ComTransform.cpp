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
    REGISTER_ACCESSOR_PROPERTY("origin", "Origin", Vec3, GetLocalOrigin, SetLocalOrigin, Vec3::zero, "xyz position in local space", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Angles::zero, "yaw, pitch, roll in degree in local space", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("scale", "Scale", Vec3, GetLocalScale, SetLocalScale, Vec3::one, "xyz scale in local space", PropertyInfo::EditorFlag);
}

ComTransform::ComTransform() {
    localOrigin = Vec3::zero;
    localScale = Vec3::one;
    localAxis = Mat3::identity;
    worldMatrix = Mat3x4::identity;
    worldMatrixInvalidated = false;
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

    UpdateWorldMatrix();

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
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalScale(const Vec3 &scale) {
    this->localScale = scale;

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalAxis(const Mat3 &axis) {
    this->localAxis = axis;

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalTransform(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    this->localOrigin = origin;
    this->localAxis = axis;
    this->localScale = scale;

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

Vec3 ComTransform::GetOrigin() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    return worldMatrix.ToTranslationVec3();
}

Mat3 ComTransform::GetAxis() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    Mat3 axis = worldMatrix.ToMat3();
    axis.OrthoNormalizeSelf();
    return axis;
}

Vec3 ComTransform::GetScale() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    return worldMatrix.ToScaleVec3();
}

const Mat3x4 &ComTransform::GetTransform() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
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
    SetScale(scale);
    SetAxis(axis);
    SetOrigin(origin);
}

Vec3 ComTransform::Forward(TransformSpace space) const {
    if (space == LocalSpace) {
        return localAxis[0];
    } else {
        return GetAxis()[0];
    }
}

Vec3 ComTransform::Right(TransformSpace space) const {
    if (space == LocalSpace) {
        return localAxis[1];
    } else {
        return GetAxis()[1];
    }
}

Vec3 ComTransform::Up(TransformSpace space) const { 
    if (space == LocalSpace) {
        return localAxis[2];
    } else {
        return GetAxis()[2];
    }
}

void ComTransform::Translate(const Vec3 &translation, TransformSpace space) {
    if (space == LocalSpace) {
        SetLocalOrigin(GetLocalOrigin() + translation);
    } else {
        SetOrigin(GetOrigin() + translation);
    }
}

void ComTransform::Rotate(const Vec3 &rotVec, float angle, TransformSpace space) {
    Mat3 rotation = Rotation(Vec3::zero, rotVec, angle).ToMat3();

    if (space == LocalSpace) {
        SetLocalAxis(rotation * GetLocalAxis());
    } else {
        SetAxis(rotation * GetAxis());
    }
}

void ComTransform::InvalidateWorldMatrix() {
    // Precondition:
    // a) whenever a transform is marked worldMatrixInvalidated, all its children are marked worldMatrixInvalidated as well.
    // b) whenever a transform is cleared from being worldMatrixInvalidated, all its parents must have been
    //    cleared as well.
    if (worldMatrixInvalidated) {
        return;
    }
    worldMatrixInvalidated = true;

    // World matrix should be updated so we emit this signal
    EmitSignal(&SIG_TransformUpdated, this);

    if (physicsUpdating) {
        for (Entity *childEntity = GetEntity()->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            if (childEntity->GetComponent(&ComRigidBody::metaObject)) {
                continue;
            }

            childEntity->GetTransform()->InvalidateWorldMatrix();
        }
    } else {
        for (Entity *childEntity = GetEntity()->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            childEntity->GetTransform()->InvalidateWorldMatrix();
        }
    }
}

void ComTransform::UpdateWorldMatrix() const {
    Mat3x4 localTransform = GetLocalTransform();

    ComTransform *parent = GetParent();

    if (parent) {
        worldMatrix = parent->GetTransform() * localTransform;
    } else {
        worldMatrix = localTransform;
    }

    worldMatrixInvalidated = false;
}

// Called by ComRigidBody::Update()
void ComTransform::PhysicsUpdated(const PhysRigidBody *body) {
    physicsUpdating = true;

    SetAxis(body->GetAxis());
    SetOrigin(body->GetOrigin());

    physicsUpdating = false;
}

BE_NAMESPACE_END
