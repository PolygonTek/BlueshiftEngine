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

class PhysRigidBody;

class ComTransform : public Component {
    friend class Entity;

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
    const Mat3 &            GetLocalAxis() const { return localAxis; }
                            /// Returns rotation angles in local space.
    Angles                  GetLocalAngles() const { return GetLocalAxis().ToAngles(); }
                            /// Returns local space transform matrix.
    Mat3x4                  GetLocalTransform() const { return Mat3x4(localScale, localAxis, localOrigin); }

                            /// Sets position in local space.
    void                    SetLocalOrigin(const Vec3 &origin);
                            /// Sets scale in local space.
    void                    SetLocalScale(const Vec3 &scale);
                            /// Sets rotation axis in local space.
    void                    SetLocalAxis(const Mat3 &axis);
                            /// Sets rotation angles in local space.
    void                    SetLocalAngles(const Angles &localAngles) { SetLocalAxis(localAngles.ToMat3()); }
                            /// Sets position, rotation and scale in local space as an atomic operation.
    void                    SetLocalTransform(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale = Vec3::one);

                            /// Returns position in world space.
    Vec3                    GetOrigin() const;
                            /// Returns scale in world space.
    Vec3                    GetScale() const;
                            /// Returns rotation axis in world space.
    Mat3                    GetAxis() const;
                            /// Returns rotation angles in world space.
    Angles                  GetAngles() const { return GetAxis().ToAngles(); }
                            /// Returns world space transform matrix.
    const Mat3x4 &          GetTransform() const;

                            /// Sets position in world space.
    void                    SetOrigin(const Vec3 &origin);
                            /// Sets scale in world space.
    void                    SetScale(const Vec3 &scale);
                            /// Sets rotation axis in world space.
    void                    SetAxis(const Mat3 &axis);
                            /// Sets position angles in world space.
    void                    SetAngles(const Angles &angles) { SetAxis(angles.ToMat3()); }
                            /// Sets position, rotation and scale in world space as an atomic operation.
    void                    SetTransform(const Vec3 &origin, const Mat3 &axis, const Vec3 &scale = Vec3::one);

                            /// Returns forward direction in the choosen transform space.
    Vec3                    Forward(TransformSpace space = WorldSpace) const;
                            /// Returns right direction in the choosen transform space.
    Vec3                    Right(TransformSpace space = WorldSpace) const;
                            /// Returns up direction in the choosen transform space.
    Vec3                    Up(TransformSpace space = WorldSpace) const;

                            /// Move in the chosen transform space.
    void                    Translate(const Vec3 &translation, TransformSpace space = WorldSpace);
                            /// Rotate with given vector and angle in the choosen transform space.
    void                    Rotate(const Vec3 &rotVec, float angle, TransformSpace space = WorldSpace);

    static const SignalDef  SIG_TransformUpdated;

protected:
                            /// Mark this component and children to need world transform recalculation.
    void                    InvalidateWorldMatrix();
                            /// Recalculate world matrix
    void                    UpdateWorldMatrix() const;
    void                    PhysicsUpdated(const PhysRigidBody *body);

    Vec3                    localOrigin;            ///< Position in local space.
    Vec3                    localScale;             ///< Scale in local space.
    Mat3                    localAxis;              ///< Rotation axis in local space.

    mutable Mat3x4          worldMatrix;
    mutable bool            worldMatrixInvalidated;
    bool                    physicsUpdating;
};

BE_NAMESPACE_END
