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

#pragma once

#include "Components/Component.h"
#include "JointHierarchy.h"

BE_NAMESPACE_BEGIN

class ComTransform;
class ComRectTransform;
class JointHierarchy;

class ComTransform : public Component {
    friend class Entity;
    friend class ComRectTransform;

public:
    OBJECT_PROTOTYPE(ComTransform);

    enum TransformSpace {
        LocalSpace,
        WorldSpace
    };
    
    ComTransform();
    virtual ~ComTransform();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Transform component cannot be disabled.
    virtual bool            CanDisable() const override { return false; }

#if WITH_EDITOR
                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

                            /// Returns parent transform component.
    ComTransform *          GetParent() const;

                            /// Returns position in local space.
    Vec3                    GetLocalOrigin() const;
                            /// Returns scale in local space.
    Vec3                    GetLocalScale() const;
                            /// Returns rotation axis in local space.
    Quat                    GetLocalRotation() const;
                            /// Returns rotation axis in local space.
    Mat3                    GetLocalAxis() const { return GetLocalRotation().ToMat3(); }
                            /// Returns Euler angles in local space.
    Angles                  GetLocalAngles() const;
                            /// Returns local space transform matrix.
    Mat3x4                  GetLocalMatrix() const;
                            /// Returns local space transform matrix without scaling.
    Mat3x4                  GetLocalMatrixNoScale() const;

                            /// Sets position in local space.
    void                    SetLocalOrigin(const Vec3 &origin);
                            /// Sets scale in local space.
    void                    SetLocalScale(const Vec3 &scale);
                            /// Sets rotation in local space.
    void                    SetLocalRotation(const Quat &rotation);
                            /// Sets position, rotation in local space.
    void                    SetLocalOriginRotation(const Vec3 &origin, const Quat &rotation);
                            /// Sets position, rotation and scale in local space as an atomic operation.
    void                    SetLocalOriginRotationScale(const Vec3 &origin, const Quat &rotation, const Vec3 &scale = Vec3::one);
                            /// Sets rotation axis in local space.
    void                    SetLocalAxis(const Mat3 &axis) { SetLocalRotation(axis.ToQuat()); }
                            /// Sets Euler angles in local space.
    void                    SetLocalAngles(const Angles &localAngles);
                            /// Sets position, rotation axis in local space.
    void                    SetLocalOriginAxis(const Vec3 &origin, const Mat3 &axis);
                            /// Sets position, rotation axis and scale in local space as an atomic operation.
    void                    SetLocalOriginAxisScale(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale = Vec3::one);

                            /// Returns position in world space.
    Vec3                    GetOrigin() const;
                            /// Returns scale in world space.
    Vec3                    GetScale() const;
                            /// Returns rotation in world space.
    Quat                    GetRotation() const;
                            /// Returns rotation axis in world space.
    Mat3                    GetAxis() const;
                            /// Returns rotation angles in world space.
    Angles                  GetAngles() const { return GetAxis().ToAngles(); }
                            /// Returns world space transform matrix.
    Mat3x4                  GetMatrix() const;
                            /// Returns world space transform matrix without scaling.
    Mat3x4                  GetMatrixNoScale() const;

                            /// Sets position in world space.
    void                    SetOrigin(const Vec3 &origin);
                            /// Sets scale in world space.
    void                    SetScale(const Vec3 &scale);
                            /// Sets rotation in world space.
    void                    SetRotation(const Quat &rotation);
                            /// Sets position, rotation in world space.
    void                    SetOriginRotation(const Vec3 &origin, const Quat &rotation);
                            /// Sets position, rotation and scale in world space as an atomic operation.
    void                    SetOriginRotationScale(const Vec3 &origin, const Quat &rotation, const Vec3 &scale = Vec3::one);
                            /// Sets rotation axis in world space.
    void                    SetAxis(const Mat3 &axis);
                            /// Sets position angles in world space.
    void                    SetAngles(const Angles &angles) { SetRotation(angles.ToQuat()); }
                            /// Sets position, rotation in world space.
    void                    SetOriginAxis(const Vec3 &origin, const Mat3 &axis);
                            /// Sets position, rotation and scale in world space as an atomic operation.
    void                    SetOriginAxisScale(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale = Vec3::one);

                            /// Rotates the transform so the forward vector points at targetPosition.
    void                    LookAt(const Vec3 &targetPosition, const Vec3 &worldUp = Vec3::unitZ);

                            /// Returns forward direction in the chosen transform space.
    Vec3                    Forward(TransformSpace space = WorldSpace) const;
                            /// Returns left direction in the chosen transform space.
    Vec3                    Left(TransformSpace space = WorldSpace) const;
                            /// Returns up direction in the chosen transform space.
    Vec3                    Up(TransformSpace space = WorldSpace) const;

                            /// Move in the chosen transform space.
    void                    Translate(const Vec3 &translation, TransformSpace space = WorldSpace);
                            /// Rotate with given vector and angle in the chosen transform space.
    void                    Rotate(const Vec3 &rotVec, float angle, TransformSpace space = WorldSpace);

    void                    SetPhysicsUpdating(bool updating) { physicsUpdating = updating; }

                            /// Returns joint hierarchy.
    JointHierarchy *        GetJointHierarchy() const { return jointHierarchy; }

    bool                    ConstructJointHierarchy();
    void                    ReleaseJointHierarchyRecursive(bool copyTransformFromJointHierarchyMatrix);

    void                    UpdateJointHierarchy(const int *jointParentIndexes);

    static Angles           CalculateClosestEulerAnglesFromQuaternion(const Angles& currentEulerAnglesHint, const Quat &newRotation);

    static const SignalDef  SIG_TransformUpdated;

protected:
                            /// Mark this component and children to need world transform recalculation.
    void                    InvalidateWorldMatrix(const ComTransform *instigatedBy);
                            /// Recalculate world matrix.
    void                    UpdateWorldMatrix() const;
                            /// Marks this component and children to need cached rect recalculation.
    virtual void            InvalidateCachedRect();

    void                    ReleaseJointHierarchy(bool copyTransformFromJointHierarchyMatrix);

    void                    ParentChanged(const Entity *entity, const Entity *oldParentEntity, const Entity *newParentEntity);

private:
    Mat3x4                  GetWorldJointMatrix() const;
    void                    UpdateWorldJointMatrix() const;
    void                    WorldJointMatrixUpdated();

    Quat                    localRotation = Quat::identity; ///< Rotation in local space.
    Vec3                    localOrigin = Vec3::zero;       ///< Position in local space.
    Vec3                    localScale = Vec3::one;         ///< Scale in local space.

#if WITH_EDITOR
    Angles                  localEulerAnglesHint = Angles::zero;
#endif

    JointHierarchy *        jointHierarchy = nullptr;
    int32_t                 jointHierarchyIndex = -1;

    mutable Mat3x4          worldMatrix = Mat3x4::identity;
    mutable bool            worldMatrixInvalidated = true;

    bool                    physicsUpdating = false;
};

BE_INLINE Vec3 ComTransform::GetLocalOrigin() const {
    if (jointHierarchy) {
        const Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        return localMatrix.ToTranslationVec3();
    }
    return localOrigin;
    }

BE_INLINE Vec3 ComTransform::GetLocalScale() const {
    if (jointHierarchy) {
        const Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        return localMatrix.ToScaleVec3();
    }
    return localScale;
}

BE_INLINE Quat ComTransform::GetLocalRotation() const {
    if (jointHierarchy) {
        const Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        return localMatrix.ToRotationMat3().ToQuat();
    }
    return localRotation;
}

BE_INLINE Angles ComTransform::GetLocalAngles() const {
#if WITH_EDITOR
    if (jointHierarchy) {
        return GetLocalRotation().ToAngles();
    }
    return localEulerAnglesHint;
#else
    return GetLocalRotation().ToAngles();
#endif
}

BE_INLINE Mat3x4 ComTransform::GetLocalMatrix() const {
    if (jointHierarchy) {
        const Mat3x4 &localMatrix = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        return localMatrix;
    }
    return Mat3x4(localScale, localRotation.ToMat3(), localOrigin);
}

BE_INLINE Mat3x4 ComTransform::GetLocalMatrixNoScale() const {
    if (jointHierarchy) {
        Mat3x4 localMatrixNoScale = jointHierarchy->GetLocalJointMatrix(jointHierarchyIndex);
        localMatrixNoScale.RemoveScale();
        return localMatrixNoScale;
    }
    return Mat3x4(localRotation.ToMat3(), localOrigin);
}

BE_INLINE Vec3 ComTransform::GetOrigin() const {
    return GetMatrix().ToTranslationVec3();
}

BE_INLINE Vec3 ComTransform::GetScale() const {
    return GetMatrix().ToScaleVec3();
}

BE_INLINE Quat ComTransform::GetRotation() const {
    Mat3 rotation = GetMatrix().ToMat3();
    rotation.OrthoNormalizeSelf();
    return rotation.ToQuat();
}

BE_INLINE Mat3 ComTransform::GetAxis() const {
    Mat3 axis = GetMatrix().ToMat3();
    axis.OrthoNormalizeSelf();
    return axis;
}

BE_INLINE void ComTransform::SetOrigin(const Vec3 &origin) {
    const ComTransform *parent = GetParent();
    SetLocalOrigin(parent ? parent->GetMatrix().InverseOrthogonal().TransformPos(origin) : origin);
}

BE_INLINE void ComTransform::SetScale(const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    SetLocalScale(parent ? scale / parent->GetScale() : scale);
}

BE_INLINE void ComTransform::SetRotation(const Quat &rotation) {
    const ComTransform *parent = GetParent();
    SetLocalRotation(parent ? parent->GetRotation().Inverse() * rotation : rotation);
}

BE_INLINE void ComTransform::SetOriginRotation(const Vec3 &origin, const Quat &rotation) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginRotation(parent->GetMatrix().InverseOrthogonal().TransformPos(origin), parent->GetRotation().Inverse() * rotation);
    } else {
        SetLocalOriginRotation(origin, rotation);
    }
}

BE_INLINE void ComTransform::SetOriginRotationScale(const Vec3 &origin, const Quat &rotation, const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginRotationScale(parent->GetMatrix().InverseOrthogonal().TransformPos(origin), parent->GetRotation().Inverse() * rotation, scale / parent->GetScale());
    } else {
        SetLocalOriginRotationScale(origin, rotation, scale);
    }
}

BE_INLINE void ComTransform::SetAxis(const Mat3 &axis) {
    const ComTransform *parent = GetParent();
    SetLocalAxis(parent ? parent->GetAxis().Transpose() * axis : axis);
}

BE_INLINE void ComTransform::SetOriginAxis(const Vec3 &origin, const Mat3 &axis) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginAxis(parent->GetMatrix().InverseOrthogonal().TransformPos(origin), parent->GetAxis().Transpose() * axis);
    } else {
        SetLocalOriginAxis(origin, axis);
    }
}

BE_INLINE void ComTransform::SetOriginAxisScale(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    const ComTransform *parent = GetParent();
    if (parent) {
        SetLocalOriginAxisScale(parent->GetMatrix().InverseOrthogonal().TransformPos(origin), parent->GetAxis().Transpose() * axis, scale / parent->GetScale());
    } else {
        SetLocalOriginAxisScale(origin, axis, scale);
    }
}

BE_INLINE void ComTransform::LookAt(const Vec3 &targetPosition, const Vec3 &worldUp) {
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

BE_INLINE Vec3 ComTransform::Forward(TransformSpace space) const {
    if (space == LocalSpace) {
        return GetLocalRotation().ToMat3()[0];
    } else {
        return GetAxis()[0];
    }
}

BE_INLINE Vec3 ComTransform::Left(TransformSpace space) const {
    if (space == LocalSpace) {
        return GetLocalRotation().ToMat3()[1];
    } else {
        return GetAxis()[1];
    }
}

BE_INLINE Vec3 ComTransform::Up(TransformSpace space) const { 
    if (space == LocalSpace) {
        return GetLocalRotation().ToMat3()[2];
    } else {
        return GetAxis()[2];
    }
}

BE_INLINE void ComTransform::Translate(const Vec3 &translation, TransformSpace space) {
    if (space == LocalSpace) {
        SetLocalOrigin(GetLocalOrigin() + translation);
    } else {
        SetOrigin(GetOrigin() + translation);
    }
}

BE_INLINE void ComTransform::Rotate(const Vec3 &rotVec, float angle, TransformSpace space) {
    Quat rotation = Quat::FromAngleAxis(DEG2RAD(angle), rotVec);

    if (space == LocalSpace) {
        SetLocalRotation(GetLocalRotation() * rotation);
    } else {
        SetRotation(rotation * GetRotation());
    }
}

BE_NAMESPACE_END
