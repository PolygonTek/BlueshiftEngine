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
#include "Components/ComVehicleWheel.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

const SignalDef ComTransform::SIG_TransformUpdated("ComTransform::TransformUpdated", "a");

OBJECT_DECLARATION("Transform", ComTransform, Component)
BEGIN_EVENTS(ComTransform)
END_EVENTS

void ComTransform::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("origin", "Origin", Vec3, GetLocalOrigin, SetLocalOrigin, Vec3::zero, 
        "xyz position in local space", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Angles::zero, 
        "roll, pitch, yaw in degree in local space", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("scale", "Scale", Vec3, GetLocalScale, SetLocalScale, Vec3::one, 
        "xyz scale in local space", PropertyInfo::EditorFlag);
}

ComTransform::ComTransform() {
    localOrigin = Vec3::zero;
    localScale = Vec3::one;
    localRotation = Quat::identity;
    worldMatrix = Mat3x4::identity;
    worldMatrixInvalidated = false;
    physicsUpdating = false;
}

ComTransform::~ComTransform() {
}

ComTransform *ComTransform::GetParent() const { 
    const Entity *parent = GetEntity()->GetNode().GetParent();
    if (!parent) {
        return nullptr;
    }
    return parent->GetTransform();
}

void ComTransform::Init() {
    Component::Init();

    UpdateWorldMatrix();

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

void ComTransform::SetLocalRotation(const Quat &rotation) {
    this->localRotation = rotation;

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalOriginRotation(const Vec3 &origin, const Quat &rotation) {
    this->localOrigin = origin;
    this->localRotation = rotation;

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalOriginRotationScale(const Vec3 &origin, const Quat &rotation, const Vec3 &scale) {
    this->localOrigin = origin;
    this->localRotation = rotation;
    this->localScale = scale;

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalAxis(const Mat3 &axis) {
    this->localRotation = axis.ToQuat();

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalOriginAxis(const Vec3 &origin, const Mat3 &axis) {
    this->localOrigin = origin;
    this->localRotation = axis.ToQuat();

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalOriginAxisScale(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    this->localOrigin = origin;
    this->localRotation = axis.ToQuat();
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

Vec3 ComTransform::GetScale() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    return worldMatrix.ToScaleVec3();
}

Quat ComTransform::GetRotation() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    Mat3 rotation = worldMatrix.ToMat3();
    rotation.OrthoNormalizeSelf();
    return rotation.ToQuat();
}

Mat3 ComTransform::GetAxis() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    Mat3 axis = worldMatrix.ToMat3();
    axis.OrthoNormalizeSelf();
    return axis;
}

const Mat3x4 &ComTransform::GetMatrix() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    return worldMatrix;
}

Mat3x4 ComTransform::GetMatrixNoScale() const {
    Mat3x4 worldMatrixNoScale;
    Mat3x4 localTransform = GetLocalMatrixNoScale();
    
    const ComTransform *parent = GetParent();
    if (parent) {
        worldMatrixNoScale = parent->GetMatrixNoScale() * localTransform;
    } else {
        worldMatrixNoScale = localTransform;
    }
    return worldMatrixNoScale;
}

void ComTransform::SetOrigin(const Vec3 &origin) {
    const ComTransform *parent = GetParent();
    SetLocalOrigin(parent ? parent->GetMatrix().Inverse() * origin : origin);
}

void ComTransform::SetScale(const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    SetLocalScale(parent ? scale / parent->GetScale() : scale);
}

void ComTransform::SetRotation(const Quat &rotation) {
    const ComTransform *parent = GetParent();
    SetLocalRotation(parent ? parent->GetRotation().Inverse() * rotation : rotation);
}

void ComTransform::SetOriginRotation(const Vec3 &origin, const Quat &rotation) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginRotation(parent->GetMatrix().Inverse() * origin, parent->GetRotation().Inverse() * rotation);
    } else {
        SetLocalOriginRotation(origin, rotation);
    }
}

void ComTransform::SetOriginRotationScale(const Vec3 &origin, const Quat &rotation, const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginRotationScale(parent->GetMatrix().Inverse() * origin, parent->GetRotation().Inverse() * rotation, scale / parent->GetScale());
    } else {
        SetLocalOriginRotationScale(origin, rotation, scale);
    }
}

void ComTransform::SetAxis(const Mat3 &axis) {
    const ComTransform *parent = GetParent();
    SetLocalAxis(parent ? parent->GetAxis().Transpose() * axis : axis);
}

void ComTransform::SetOriginAxis(const Vec3 &origin, const Mat3 &axis) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginAxis(parent->GetMatrix().Inverse() * origin, parent->GetAxis().Transpose() * axis);
    } else {
        SetLocalOriginAxis(origin, axis);
    }
}

void ComTransform::SetOriginAxisScale(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginAxisScale(parent->GetMatrix().Inverse() * origin, parent->GetAxis().Transpose() * axis, scale / parent->GetScale());
    } else {
        SetLocalOriginAxisScale(origin, axis, scale);
    }
}

void ComTransform::LookAt(const Vec3 &targetPosition, const Vec3 &worldUp) {
    Vec3 forward = targetPosition - GetOrigin();
    Mat3 axis;
    if (axis.SetFromLookAt(forward, worldUp)) {
        SetAxis(axis);
    } else {
        if (forward.Normalize() > VECTOR_EPSILON) {
            Quat q = Quat::FromTwoVectors(Vec3::unitX, forward);
            SetAxis(q.ToMat3());
        }
    }
}

Vec3 ComTransform::Forward(TransformSpace space) const {
    if (space == LocalSpace) {
        return localRotation.ToMat3()[0];
    } else {
        return GetAxis()[0];
    }
}

Vec3 ComTransform::Right(TransformSpace space) const {
    if (space == LocalSpace) {
        return localRotation.ToMat3()[1];
    } else {
        return GetAxis()[1];
    }
}

Vec3 ComTransform::Up(TransformSpace space) const { 
    if (space == LocalSpace) {
        return localRotation.ToMat3()[2];
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
    Quat rotation = Quat::FromAngleAxis(angle, rotVec);

    if (space == LocalSpace) {
        SetLocalRotation(GetLocalRotation() * rotation);
    } else {
        SetRotation(rotation * GetRotation());
    }
}

void ComTransform::InvalidateWorldMatrix() {
    // Precondition:
    // a) whenever a transform is marked worldMatrixInvalidated, all its children are marked worldMatrixInvalidated as well.
    // b) whenever a transform is cleared from being worldMatrixInvalidated, all its parents must have been cleared as well.
    if (worldMatrixInvalidated) {
        return;
    }
    worldMatrixInvalidated = true;

    // World matrix should be updated so we emit this signal
    EmitSignal(&SIG_TransformUpdated, this);

    if (physicsUpdating) {
        for (Entity *childEntity = GetEntity()->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            // Don't update children that has rigid body. they will be updated by own.
            if (childEntity->GetComponent(&ComRigidBody::metaObject) || childEntity->GetComponent(&ComVehicleWheel::metaObject)) {
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
    Mat3x4 localTransform = GetLocalMatrix();

    const ComTransform *parent = GetParent();
    if (parent) {
        worldMatrix = parent->GetMatrix() * localTransform;
    } else {
        worldMatrix = localTransform;
    }

    worldMatrixInvalidated = false;
}

BE_NAMESPACE_END
