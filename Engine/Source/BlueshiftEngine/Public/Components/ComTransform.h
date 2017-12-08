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
    
    ComTransform();
    virtual ~ComTransform();

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Transform component cannot be disabled.
    virtual bool            CanDisable() const override { return false; }

                            /// Returns parent transform component.
    ComTransform *          GetParent() const;

                            /// Returns position in local space.
    Vec3                    GetLocalOrigin() const { return localOrigin; }
                            /// Returns scale in local space.
    Vec3                    GetLocalScale() const { return localScale; }
                            /// Returns rotation axis in local space.
    Mat3                    GetLocalAxis() const { return localAxis; }
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
                            /// Returns forward direction in world space.
    Vec3                    Forward() const { return GetAxis()[0]; }
                            /// Returns right direction in world space.
    Vec3                    Right() const { return GetAxis()[1]; }
                            /// Returns up direction in world space.
    Vec3                    Up() const { return GetAxis()[2]; }
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

                            /// Move in local space.
    void                    TranslateLocal(const Vec3 &translation);
                            /// Rotate around axis with the given angle in local space.
    void                    RotateLocal(const Vec3 &axis, float angle);

                            /// Move in world space.
    void                    Translate(const Vec3 &translation);
                            /// Rotate around axis with the given angle in world space.
    void                    Rotate(const Vec3 &axis, float angle);

    static const SignalDef  SIG_TransformUpdated;

protected:
                            /// Mark this component and children to need world transform recalculation.
    void                    MarkDirty();
    void                    UpdateWorldTransform() const;
    void                    PhysicsUpdated(const PhysRigidBody *body);

    Vec3                    localOrigin;
    Vec3                    localScale;
    Mat3                    localAxis;

    mutable Mat3x4          worldMatrix;
    mutable bool            dirty;
    bool                    physicsUpdating;
};

BE_NAMESPACE_END
