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

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComRectTransform;

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

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Transform component cannot be disabled.
    virtual bool            CanDisable() const override { return false; }

                            /// Returns parent transform component.
    ComTransform *          GetParent() const;

                            /// Returns position in local space.
    const Vec3 &            GetLocalOrigin() const { return localOrigin; }
                            /// Returns scale in local space.
    const Vec3 &            GetLocalScale() const { return localScale; }
                            /// Returns rotation axis in local space.
    const Quat &            GetLocalRotation() const { return localRotation; }
                            /// Returns rotation axis in local space.
    Mat3                    GetLocalAxis() const { return localRotation.ToMat3(); }
                            /// Returns Euler angles in local space.
    Angles                  GetLocalAngles() const;
                            /// Returns local space transform matrix.
    Mat3x4                  GetLocalMatrix() const { return Mat3x4(localScale, localRotation.ToMat3(), localOrigin); }
                            /// Returns local space transform matrix without scaling.
    Mat3x4                  GetLocalMatrixNoScale() const { return Mat3x4(localRotation.ToMat3(), localOrigin); }

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
    const Mat3x4 &          GetMatrix() const;
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

    static Angles           CalculateClosestEulerAnglesFromQuaternion(const Angles& currentEulerAnglesHint, const Quat &newRotation);

    static const SignalDef  SIG_TransformUpdated;

protected:
                            /// Mark this component and children to need world transform recalculation.
    void                    InvalidateWorldMatrix();
                            /// Recalculate world matrix.
    void                    UpdateWorldMatrix() const;
                            /// Marks this component and children to need cached rect recalculation.
    virtual void            InvalidateCachedRect();

    Quat                    localRotation;          ///< Rotation in local space.
    Vec3                    localOrigin;            ///< Position in local space.
    Vec3                    localScale;             ///< Scale in local space.

#if WITH_EDITOR
    Angles                  localEulerAnglesHint;
#endif

    mutable Mat3x4          worldMatrix = Mat3x4::identity;
    mutable bool            worldMatrixInvalidated = true;

    bool                    physicsUpdating = false;
};

BE_INLINE Angles ComTransform::GetLocalAngles() const {
#if WITH_EDITOR
    return localEulerAnglesHint;
#else
    return localRotation.ToAngles();
#endif
}

BE_INLINE void ComTransform::SetLocalOrigin(const Vec3 &origin) {
    this->localOrigin = origin;

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

BE_INLINE void ComTransform::SetLocalScale(const Vec3 &scale) {
    this->localScale = scale;

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

BE_INLINE void ComTransform::SetLocalRotation(const Quat &rotation) {
    this->localRotation = rotation;

#if WITH_EDITOR
    this->localEulerAnglesHint = CalculateClosestEulerAnglesFromQuaternion(localEulerAnglesHint, localRotation);
#endif

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

BE_INLINE void ComTransform::SetLocalOriginRotation(const Vec3 &origin, const Quat &rotation) {
    this->localOrigin = origin;

    SetLocalRotation(rotation);
}

BE_INLINE void ComTransform::SetLocalOriginRotationScale(const Vec3 &origin, const Quat &rotation, const Vec3 &scale) {
    this->localOrigin = origin;
    this->localScale = scale;

    SetLocalRotation(rotation);
}

BE_INLINE void ComTransform::SetLocalAngles(const Angles &localAngles) {
#if WITH_EDITOR
    this->localEulerAnglesHint = localAngles;
#endif

    this->localRotation = localAngles.ToQuat();

    if (IsInitialized()) {
        InvalidateWorldMatrix();
    }
}

BE_INLINE void ComTransform::SetLocalOriginAxis(const Vec3 &origin, const Mat3 &axis) {
    this->localOrigin = origin;

    SetLocalRotation(axis.ToQuat());
}

BE_INLINE void ComTransform::SetLocalOriginAxisScale(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale) {
    this->localOrigin = origin;
    this->localScale = scale;

    SetLocalRotation(axis.ToQuat());
}

BE_INLINE Vec3 ComTransform::GetOrigin() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    return worldMatrix.ToTranslationVec3();
}

BE_INLINE Vec3 ComTransform::GetScale() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    return worldMatrix.ToScaleVec3();
}

BE_INLINE Quat ComTransform::GetRotation() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    Mat3 rotation = worldMatrix.ToMat3();
    rotation.OrthoNormalizeSelf();
    return rotation.ToQuat();
}

BE_INLINE Mat3 ComTransform::GetAxis() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    Mat3 axis = worldMatrix.ToMat3();
    axis.OrthoNormalizeSelf();
    return axis;
}

BE_INLINE const Mat3x4 &ComTransform::GetMatrix() const {
    if (worldMatrixInvalidated) {
        UpdateWorldMatrix();
    }
    return worldMatrix;
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
        return localRotation.ToMat3()[0];
    } else {
        return GetAxis()[0];
    }
}

BE_INLINE Vec3 ComTransform::Left(TransformSpace space) const {
    if (space == LocalSpace) {
        return localRotation.ToMat3()[1];
    } else {
        return GetAxis()[1];
    }
}

BE_INLINE Vec3 ComTransform::Up(TransformSpace space) const { 
    if (space == LocalSpace) {
        return localRotation.ToMat3()[2];
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
