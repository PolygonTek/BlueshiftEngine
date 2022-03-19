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
        "xyz position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
#if WITH_EDITOR
    REGISTER_PROPERTY("localEulerAnglesHint", "LocalEulerAnglesHint", Angles, localEulerAnglesHint, Angles::zero,
        "", PropertyInfo::Flag::Empty);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Angles::zero,
        "roll, pitch, yaw in degree in local space", PropertyInfo::Flag::Editor | PropertyInfo::Flag::SkipSerialization);
#endif
    REGISTER_ACCESSOR_PROPERTY("rotation", "Rotation", Quat, GetLocalRotation, SetLocalRotation, Quat::identity,
        "", PropertyInfo::Flag::Empty);
    REGISTER_ACCESSOR_PROPERTY("scale", "Scale", Vec3, GetLocalScale, SetLocalScale, Vec3::one, 
        "xyz scale in local space", PropertyInfo::Flag::Editor);
}

ComTransform::ComTransform() {
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

Angles ComTransform::GetLocalAngles() const {
#if WITH_EDITOR
    return localEulerAnglesHint;
#else
    return localRotation.ToAngles();
#endif
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

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalOriginRotation(const Vec3 &origin, const Quat &rotation) {
    this->localOrigin = origin;
    this->localRotation = rotation;

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalOriginRotationScale(const Vec3 &origin, const Quat &rotation, const Vec3 &scale) {
    this->localOrigin = origin;
    this->localRotation = rotation;
    this->localScale = scale;

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalAxis(const Mat3 &axis) {
    this->localRotation = axis.ToQuat();

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalAngles(const Angles &localAngles) {
#if WITH_EDITOR
    localEulerAnglesHint = localAngles;
#endif

    SetLocalRotation(localAngles.ToQuat());
}

void ComTransform::SetLocalOriginAxis(const Vec3 &origin, const Mat3 &axis) {
    this->localOrigin = origin;
    this->localRotation = axis.ToQuat();

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

void ComTransform::SetLocalOriginAxisScale(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    this->localOrigin = origin;
    this->localRotation = axis.ToQuat();
    this->localScale = scale;

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

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
    ALIGN_AS32 Mat3x4 worldMatrixNoScale;
    ALIGN_AS32 Mat3x4 localMatrix = GetLocalMatrixNoScale();
    
    const ComTransform *parent = GetParent();
    if (parent) {
        worldMatrixNoScale = parent->GetMatrixNoScale() * localMatrix;
    } else {
        worldMatrixNoScale = localMatrix;
    }
    return worldMatrixNoScale;
}

void ComTransform::SetOrigin(const Vec3 &origin) {
    const ComTransform *parent = GetParent();
    SetLocalOrigin(parent ? parent->GetMatrix().InverseOrthogonal() * origin : origin);
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
        SetLocalOriginRotation(parent->GetMatrix().InverseOrthogonal() * origin, parent->GetRotation().Inverse() * rotation);
    } else {
        SetLocalOriginRotation(origin, rotation);
    }
}

void ComTransform::SetOriginRotationScale(const Vec3 &origin, const Quat &rotation, const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginRotationScale(parent->GetMatrix().InverseOrthogonal() * origin, parent->GetRotation().Inverse() * rotation, scale / parent->GetScale());
    } else {
        SetLocalOriginRotationScale(origin, rotation, scale);
    }
}

void ComTransform::SetAxis(const Mat3 &axis) {
    const ComTransform *parent = GetParent();
    SetLocalAxis(parent ? parent->GetAxis().Transpose() * axis : axis);
}

void ComTransform::SetAngles(const Angles &angles) {
    SetRotation(angles.ToQuat());
}

void ComTransform::SetOriginAxis(const Vec3 &origin, const Mat3 &axis) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginAxis(parent->GetMatrix().InverseOrthogonal() * origin, parent->GetAxis().Transpose() * axis);
    } else {
        SetLocalOriginAxis(origin, axis);
    }
}

void ComTransform::SetOriginAxisScale(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginAxisScale(parent->GetMatrix().InverseOrthogonal() * origin, parent->GetAxis().Transpose() * axis, scale / parent->GetScale());
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

Vec3 ComTransform::Left(TransformSpace space) const {
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
    Quat rotation = Quat::FromAngleAxis(DEG2RAD(angle), rotVec);

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

    // It is safe to emit this signal as the world matrix will be updated on demand.
    EmitSignal(&SIG_TransformUpdated, this);

    if (physicsUpdating) {
        for (Entity *childEntity = GetEntity()->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            // Don't update children that has rigid body. they will be updated by own.
            if (childEntity->GetComponent(&ComRigidBody::metaObject) || childEntity->GetComponent(&ComVehicleWheel::metaObject)) {
                continue;
            }

            childEntity->GetTransform()->InvalidateWorldMatrix();
        }
    } else {
        for (Entity *childEntity = GetEntity()->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
            childEntity->GetTransform()->InvalidateWorldMatrix();
        }
    }
}

void ComTransform::UpdateWorldMatrix() const {
    ALIGN_AS32 Mat3x4 localMatrix = GetLocalMatrix();

    const ComTransform *parent = GetParent();
    if (parent) {
        worldMatrix = parent->GetMatrix() * localMatrix;
    } else {
        worldMatrix = localMatrix;
    }

    worldMatrixInvalidated = false;
}

void ComTransform::InvalidateCachedRect() {
    for (Entity *childEntity = GetEntity()->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        ComTransform *transform = childEntity->GetTransform();

        if (transform) {
            transform->InvalidateCachedRect();
        }
    }
}

// newRotation 으로 currentEulerAngles 와 수치적으로 [-180, +180] 범위 내에서 가장 가까운 Euler angles 를 구한다.
Angles ComTransform::CalculateClosestEulerAnglesFromQuaternion(const Angles &currentEulerAngles, const Quat &newRotation) {
    // newRotation 과 각도 차이가 0.001 보다 작다면 currentEulerAngles 를 유지한다.
    float angleDiff = newRotation.AngleBetween(currentEulerAngles.ToQuat());
    if (angleDiff < 1e-3f) {
        return currentEulerAngles;
    }

    // Converting the quaternion to a rotation matrix R = Rz * Ry * Rx and then to
    // Euler angles gives the range ([-180, +180], [-90, +90], [-180, +180]).
    Angles e1 = newRotation.ToAngles();

    // Round off degrees to the fourth decimal place.
    e1[0] = Math::Round(e1[0] / 1e-3f) * 1e-3f;
    e1[1] = Math::Round(e1[1] / 1e-3f) * 1e-3f;
    e1[2] = Math::Round(e1[2] / 1e-3f) * 1e-3f;

    // Make alternative one but represents the same rotation.
    Angles e2;
    e2 = e1.Alternate();

    // Synchronize Euler angles e1 and e2 using base (current).
    Angles eulerAnglesSynced1 = e1.SyncAngles(currentEulerAngles);
    Angles eulerAnglesSynced2 = e2.SyncAngles(currentEulerAngles);

    // Calculate differences between current Euler angles and others.
    Vec3 deltaAngles1 = Vec3(eulerAnglesSynced1 - currentEulerAngles);
    Vec3 deltaAngles2 = Vec3(eulerAnglesSynced2 - currentEulerAngles);

    // Returns synchronized Euler angles which have smaller angle difference.
    float diff1 = deltaAngles1.Dot(deltaAngles1);
    float diff2 = deltaAngles2.Dot(deltaAngles2);
    return diff1 < diff2 ? eulerAnglesSynced1 : eulerAnglesSynced2;
}

BE_NAMESPACE_END
