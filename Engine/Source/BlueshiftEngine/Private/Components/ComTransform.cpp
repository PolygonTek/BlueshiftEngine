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
}

ComTransform::~ComTransform() {
}

bool ComTransform::CanDisable() const {
    return false;
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

    localMatrix.SetLinearTransform(localAxis, localScale, localOrigin);

    // Re-calculate world matrix from the hierarchy
    // For the correct world matrix calculation, this should be called in depth first order
    RecalcWorldMatrix();

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
        // Update localMatrix -> Update worldMatrix -> Update children
        localMatrix.SetLinearTransform(localAxis, localScale, localOrigin);

        RecalcWorldMatrix();

        EmitSignal(&SIG_TransformUpdated, this);

        UpdateChildren();
    }
}

void ComTransform::SetLocalScale(const Vec3 &scale) {
    this->localScale = scale;

    if (IsInitialized()) {
        // Update localMatrix -> Update worldMatrix -> Update children
        localMatrix.SetLinearTransform(localAxis, localScale, localOrigin);

        RecalcWorldMatrix();

        EmitSignal(&SIG_TransformUpdated, this);

        UpdateChildren();
    }
}

void ComTransform::SetLocalAxis(const Mat3 &axis) {
    this->localAxis = axis;
    this->localAxis.FixDegeneracies();

    if (IsInitialized()) {
        // Update localMatrix -> Update worldMatrix -> Update children
        localMatrix.SetLinearTransform(localAxis, localScale, localOrigin);

        RecalcWorldMatrix();

        EmitSignal(&SIG_TransformUpdated, this);

        UpdateChildren();
    }
}

void ComTransform::SetLocalTransform(const Vec3 &origin, const Vec3 &scale, const Mat3 &axis) {
    this->localScale = scale;
    this->localAxis = axis;
    this->localAxis.FixDegeneracies();
    this->localOrigin = origin;

    // Update localMatrix -> Update worldMatrix -> Update children
    localMatrix.SetLinearTransform(localAxis, localScale, localOrigin);

    RecalcWorldMatrix();

    EmitSignal(&SIG_TransformUpdated, this);

    UpdateChildren();
}

Vec3 ComTransform::GetOrigin() const {
    return worldMatrix.ToTranslationVec3();
}

Mat3 ComTransform::GetAxis() const {
    Mat3 axis = worldMatrix.ToMat3();
    axis.OrthoNormalizeSelf();
    return axis;
}

Vec3 ComTransform::GetScale() const {
    Mat3 axis = worldMatrix.ToMat3();
    Vec3 scale;
    scale.x = axis[0].Length();
    scale.y = axis[1].Length();
    scale.z = axis[2].Length();
    return scale;
}

void ComTransform::SetOrigin(const Vec3 &origin) {
    worldMatrix.SetLinearTransform(GetAxis(), GetScale(), origin);

    // Update worldMatrix -> recalc localMatrix -> Update children
    RecalcLocalMatrix();

    localOrigin = localMatrix.ToTranslationVec3();

    EmitSignal(&SIG_TransformUpdated, this);

    UpdateChildren();
}

void ComTransform::SetAxis(const Mat3 &axis) {
    worldMatrix.SetLinearTransform(axis, GetScale(), GetOrigin());

    // Update worldMatrix -> recalc localMatrix -> Update children
    RecalcLocalMatrix();

    localAxis = localMatrix.ToMat3();
    localAxis.OrthoNormalizeSelf();

    EmitSignal(&SIG_TransformUpdated, this);

    UpdateChildren();
}

void ComTransform::Translate(const Vec3 &translation) {
    SetOrigin(GetOrigin() + translation);
}

void ComTransform::Rotate(const Vec3 &axis, float angle) {
    SetAxis(Rotation(Vec3::zero, axis, angle).ToMat3() * GetAxis());
}

void ComTransform::RecalcWorldMatrix() {
    const ComTransform *parent = GetParent();
    if (parent) {
        worldMatrix = parent->worldMatrix * localMatrix;
    } else {
        worldMatrix = localMatrix;
    }
}

void ComTransform::RecalcLocalMatrix() {
    const ComTransform *parent = GetParent();
    if (parent) {
        localMatrix = parent->worldMatrix.AffineInverse() * worldMatrix;
    } else {
        localMatrix = worldMatrix;
    }
}

void ComTransform::UpdateChildren(bool ignorePhysicsEntity) {
    for (Entity *childEntity = GetEntity()->GetNode().GetChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        if (ignorePhysicsEntity && childEntity->GetComponent(&ComRigidBody::metaObject)) {
            continue;
        }

        ComTransform *childTransform = childEntity->GetTransform();

        childTransform->worldMatrix = worldMatrix * childTransform->localMatrix;
        childTransform->EmitSignal(&SIG_TransformUpdated, childTransform);

        childTransform->UpdateChildren();
    }
}

void ComTransform::PhysicsUpdated(const PhysRigidBody *body) {
    worldMatrix.SetLinearTransform(body->GetAxis(), GetScale(), body->GetOrigin());

    RecalcLocalMatrix();

    localOrigin = localMatrix.ToTranslationVec3();
    localAxis = localMatrix.ToMat3();

    EmitSignal(&SIG_TransformUpdated, this);

    UpdateChildren(true);
}

BE_NAMESPACE_END
