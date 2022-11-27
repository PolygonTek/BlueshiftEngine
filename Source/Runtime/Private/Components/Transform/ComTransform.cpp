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
#include "Render/Render.h"
#include "Components/Transform/ComTransform.h"
#include "Components/Transform/ComRectTransform.h"
#include "Components/Transform/JointHierarchy.h"
#include "Components/ComRigidBody.h"
#include "Components/ComVehicleWheel.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

const SignalDef ComTransform::SIG_TransformUpdated("ComTransform::TransformUpdated", "a");

OBJECT_DECLARATION("Transform", ComTransform, Component)
BEGIN_EVENTS(ComTransform)
END_EVENTS

void ComTransform::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("origin", "Origin", Vec3, GetLocalOrigin, SetLocalOrigin, Vec3::zero,
        "xyz position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
#if WITH_EDITOR
    REGISTER_PROPERTY("localEulerAnglesHint", "LocalEulerAnglesHint", Angles, localEulerAnglesHint, Angles::zero,
        "", PropertyInfo::Flag::Empty);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Angles::zero,
        "roll, pitch, yaw in degree in local space", PropertyInfo::Flag::Editor | PropertyInfo::Flag::SkipSerialization);
#endif
    REGISTER_MIXED_ACCESSOR_PROPERTY("rotation", "Rotation", Quat, GetLocalRotation, SetLocalRotation, Quat::identity,
        "Rotation in local space", PropertyInfo::Flag::Empty);
    REGISTER_MIXED_ACCESSOR_PROPERTY("scale", "Scale", Vec3, GetLocalScale, SetLocalScale, Vec3::one,
        "xyz scale in local space", PropertyInfo::Flag::Editor);
}

ComTransform::ComTransform() {
}

ComTransform::~ComTransform() {
    Purge();
}

void ComTransform::Purge(bool chainPurge) {
    ReleaseJointHierarchy(false);

    if (chainPurge) {
        Component::Purge();
    }
}

ComTransform *ComTransform::GetParent() const {
    const Entity *parent = entity->GetNode().GetParent();
    if (!parent) {
        return nullptr;
    }
    return parent->GetTransform();
}

void ComTransform::Init() {
    Component::Init();

    entity->Connect(&Entity::SIG_ParentChanged, this, (SignalCallback)&ComTransform::ParentChanged, SignalObject::ConnectionType::Unique);

    UpdateWorldMatrix();

    // Mark as initialized
    SetInitialized(true);
}

void ComTransform::SetLocalOrigin(const Vec3 &inLocalOrigin) {
    if (jointHierarchy) {
        Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrix.SetTranslation(inLocalOrigin);
    } else {
        localOrigin = inLocalOrigin;
    }

    if (IsInitialized()) {
        InvalidateWorldMatrix(this);
    }
}

void ComTransform::SetLocalScale(const Vec3 &inLocalScale) {
    if (jointHierarchy) {
        Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrix.SetScale(inLocalScale, true);
    } else {
        localScale = inLocalScale;
    }

    if (IsInitialized()) {
        InvalidateWorldMatrix(this);
    }
}

void ComTransform::SetLocalRotation(const Quat &inLocalRotation) {
    if (jointHierarchy) {
        Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrix.SetRotation(inLocalRotation.ToMat3(), true);
    } else {
        localRotation = inLocalRotation;
    }

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, inLocalRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix(this);
    }
}

void ComTransform::SetLocalOriginRotation(const Vec3 &inLocalOrigin, const Quat &inLocalRotation) {
    if (jointHierarchy) {
        Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrix.SetTranslationRotation(inLocalOrigin, inLocalRotation.ToMat3(), true);
    } else {
        localOrigin = inLocalOrigin;
        localRotation = inLocalRotation;
    }

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, inLocalRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix(this);
    }
}

void ComTransform::SetLocalOriginAxis(const Vec3 &inLocalOrigin, const Mat3 &inLocalAxis) {
    if (jointHierarchy) {
        Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrix.SetTranslationRotation(inLocalOrigin, inLocalAxis, true);
    } else {
        localOrigin = inLocalOrigin;
        localRotation = inLocalAxis.ToQuat();
    }

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix(this);
    }
}

void ComTransform::SetLocalOriginAxisScale(const Vec3 &inLocalOrigin, const Mat3 &inLocalAxis, const Vec3 &inLocalScale) {
    if (jointHierarchy) {
        Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrix.SetTRS(inLocalOrigin, inLocalAxis, inLocalScale);
    } else {
        localOrigin = inLocalOrigin;
        localRotation = inLocalAxis.ToQuat();
        localScale = inLocalScale;
    }

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix(this);
    }
}

void ComTransform::SetLocalOriginRotationScale(const Vec3 &inLocalOrigin, const Quat &inLocalRotation, const Vec3 &inLocalScale) {
    if (jointHierarchy) {
        Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrix.SetTRS(inLocalOrigin, inLocalRotation.ToMat3(), inLocalScale);
    } else {
        localOrigin = inLocalOrigin;
        localRotation = inLocalRotation;
        localScale = inLocalScale;
    }

#if WITH_EDITOR
    localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, inLocalRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix(this);
    }
}

void ComTransform::SetLocalAngles(const Angles &inLocalAngles) {
    if (jointHierarchy) {
        Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrix.SetRotation(inLocalAngles.ToMat3(), true);
    } else {
        localRotation = inLocalAngles.ToQuat();
    }

#if WITH_EDITOR
    localEulerAnglesHint = inLocalAngles;
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix(this);
    }
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

Mat3x4 ComTransform::GetWorldMatrix() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }

    if (jointHierarchy) {
        const Mat3x4 &worldJointMatrix = jointHierarchy->GetWorldJointMatrix(jointHierarchyIndex);
        const ComTransform *jointRootParent = jointHierarchy->GetRootTransform()->GetParent();
        if (!jointRootParent) {
            return worldJointMatrix;
        }
        return jointRootParent->GetWorldMatrix() * worldJointMatrix;
    }
    return worldMatrix;
}

Mat3x4 ComTransform::GetWorldJointMatrix() const {
    if (!jointHierarchy) {
        assert(0);
        return Mat3x4::identity;
    }

    if (worldMatrixInvalidated) {
        UpdateWorldJointMatrix();
    }

    const Mat3x4 &worldJointMatrix = jointHierarchy->GetWorldJointMatrix(jointHierarchyIndex);
    return worldJointMatrix;
}

void ComTransform::InvalidateWorldMatrix(const ComTransform *instigatedBy) {
    // Precondition:
    // a) whenever a transform is marked worldMatrixInvalidated, all its children are marked worldMatrixInvalidated as well.
    // b) whenever a transform is cleared from being worldMatrixInvalidated, all its parents must have been cleared as well.
    if (worldMatrixInvalidated) {
        return;
    }
    worldMatrixInvalidated = true;

    // It is safe to emit this signal as the world matrix will be updated on demand.
    if (entity->NumComponents() > 1) {
        EmitSignal(&SIG_TransformUpdated, this);
    }

    // FIXME: joint transform 일 경우 모든 children 의 joint world matrix 를 업데이트한다. 필요한 상황에서만 업데이트하도록 수정할 것
    if (jointHierarchy) {
        UpdateWorldJointMatrix();
    }

    for (Entity *childEntity = entity->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        // Don't update children that has rigid body. they will be updated by own.
        if (instigatedBy && instigatedBy->physicsUpdating) {
            if (childEntity->GetComponent(&ComRigidBody::metaObject) || childEntity->GetComponent(&ComVehicleWheel::metaObject)) {
                continue;
            }
        }

        ComTransform *childTransform = childEntity->GetTransform();

        if (childTransform->jointHierarchy == nullptr || childTransform->jointHierarchy == jointHierarchy) {
            childTransform->InvalidateWorldMatrix(instigatedBy);
        }
    }
}

void ComTransform::UpdateJointHierarchy(const int *jointParentIndexes) {
    assert(jointHierarchy);
    if (!jointHierarchy) {
        return;
    }

    const Mat3x4 *localJointMats = jointHierarchy->GetLocalJointMatrices();
    Mat3x4 *worldJointMats = jointHierarchy->GetWorldJointMatrices();

    // Convert joint matrices from local space to world space.
    simdProcessor->TransformJoints(localJointMats, worldJointMats, jointParentIndexes, 0, jointHierarchy->NumJoints() - 1);

    // FIXME: Slow !!
    WorldJointMatrixUpdated();
}

void ComTransform::WorldJointMatrixUpdated() {
    worldMatrixInvalidated = false;

    if (jointHierarchy && entity->NumComponents() > 1) {
        EmitSignal(&SIG_TransformUpdated, this);
    }

    for (Entity *childEntity = entity->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        ComTransform *childTransform = childEntity->GetTransform();

        if (childTransform->jointHierarchy) {
            childTransform->WorldJointMatrixUpdated();
        } else {
            childTransform->InvalidateWorldMatrix(this);
        }
    }
}

void ComTransform::UpdateWorldMatrix() const {
    if (jointHierarchy) {
        UpdateWorldJointMatrix();
    } else {
        ALIGN_AS32 Mat3x4 localMatrix = GetLocalMatrix();
        const ComTransform *parent = GetParent();

        if (parent) {
            worldMatrix = parent->GetWorldMatrix() * localMatrix;
        } else {
            worldMatrix = localMatrix;
        }
        worldMatrixInvalidated = false;
    }
}

void ComTransform::UpdateWorldJointMatrix() const {
    assert(jointHierarchy);
    if (!jointHierarchy) {
        return;
    }

    Mat3x4 &worldJointMatrix = jointHierarchy->GetWorldJointMatrix(jointHierarchyIndex);
    ALIGN_AS32 Mat3x4 localMatrix = GetLocalMatrix();
    const ComTransform *parent = GetParent();

    if (parent && parent->jointHierarchy) {
        worldJointMatrix = parent->GetWorldJointMatrix() * localMatrix;
    } else {
        worldJointMatrix = localMatrix;
    }

    worldMatrixInvalidated = false;
}

void ComTransform::InvalidateCachedRect() {
    for (Entity *childEntity = entity->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
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

void ComTransform::ReleaseJointHierarchy(bool copyTransformFromJointHierarchyMatrix) {
    if (jointHierarchy) {
        if (copyTransformFromJointHierarchyMatrix) {
            const Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
            localMatrix.GetTQS(localOrigin, localRotation, localScale);
        }

        JointHierarchy::Release(jointHierarchy);
        jointHierarchy = nullptr;
    }
}

void ComTransform::ReleaseJointHierarchyRecursive(bool copyTransformFromJointHierarchyMatrix) {
    if (!jointHierarchy) {
        return;
    }

    ReleaseJointHierarchy(copyTransformFromJointHierarchyMatrix);

    Entity *ownerEntity = GetEntity();

    for (Entity *childEntity = ownerEntity->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextIn(ownerEntity->GetNode())) {
        ComTransform *childTransform = childEntity->GetTransform();

        if (childTransform) {
            childTransform->ReleaseJointHierarchy(copyTransformFromJointHierarchyMatrix);
        }
    }
}

bool ComTransform::ConstructJointHierarchy() {
    ReleaseJointHierarchyRecursive(false);

    Entity *ownerEntity = GetEntity();

    int matrixCount = 1;
    for (Entity *childEntity = ownerEntity->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextIn(ownerEntity->GetNode())) {
        if (!childEntity->IsLockedInHierarchy()) {
            continue;
        }

        // Joint hierarchy requires only transform component.
        ComTransform *childTransform = childEntity->GetTransform();
        if (!childTransform->IsInstanceOf<ComTransform>()) {
            continue;
        }

        matrixCount++;
    }

    if (matrixCount == 1) {
        // Don't create joint hierarchy if this entity have no children.
        return false;
    }

    jointHierarchy = new JointHierarchy(this, matrixCount);
    jointHierarchy->AddRefCount();

    int32_t currentIndex = 0;

    jointHierarchyIndex = currentIndex++;

    Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
    localMatrix.SetTQS(localOrigin, localRotation, localScale);

    UpdateWorldJointMatrix();

    for (Entity *childEntity = ownerEntity->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextIn(ownerEntity->GetNode())) {
        if (!childEntity->IsLockedInHierarchy()) {
            continue;
        }

        ComTransform *childTransform = childEntity->GetTransform();
        if (!childTransform->IsInstanceOf<ComTransform>()) {
            continue;
        }

        childTransform->jointHierarchy = jointHierarchy->AddRefCount();
        childTransform->jointHierarchyIndex = currentIndex++;

        Mat3x4 &localMatrix = childTransform->jointHierarchy->GetLocalJointMatrix(childTransform->jointHierarchyIndex);
        localMatrix.SetTQS(childTransform->localOrigin, childTransform->localRotation, childTransform->localScale);

        childTransform->UpdateWorldJointMatrix();
    }
    return true;
}

void ComTransform::ParentChanged(const Entity *entity, const Entity *oldParentEntity, const Entity *newParentEntity) {
    const char *oldParentName = oldParentEntity ? oldParentEntity->GetName().c_str() : "root";
    const char *newParentName = newParentEntity ? newParentEntity->GetName().c_str() : "root";

    BE_DLOG("%s's parent changed from %s to %s\n", entity->GetName().c_str(), oldParentName, newParentName);
}

#if WITH_EDITOR
void ComTransform::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    if (!jointHierarchy) {
        return;
    }

    if (selectedByParent) {
        const ComTransform *parent = GetParent();
        if (parent && parent->jointHierarchy) {
            Mat3x4 worldMatrix = GetWorldMatrix();
            Mat3x4 parentWorldMatrix = parent->GetWorldMatrix();

            RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();
            renderWorld->SetDebugColor(selected ? Color4::purple : Color4::mediumPurple, Color4::zero);
            renderWorld->DebugLine(parentWorldMatrix.ToTranslationVec3(), worldMatrix.ToTranslationVec3(), 1.25f);
        }
    }
}
#endif

BE_NAMESPACE_END
