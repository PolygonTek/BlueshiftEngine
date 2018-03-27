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

class PhysConstraint;
class ComTransform;
class ComRigidBody;

class ComJoint : public Component {
public:
    ABSTRACT_PROTOTYPE(ComJoint);

    ComJoint();
    virtual ~ComJoint() = 0;

                            /// Returns true if the same component is allowed
    virtual bool            AllowSameComponent() const override { return true; }

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual void            Awake() override;

    Guid                    GetConnectedBodyGuid() const { return connectedBodyGuid; }
    void                    SetConnectedBodyGuid(const Guid &connectedBodyGuid);

    ComRigidBody *          GetConnectedBody() const;
    void                    SetConnectedBody(const ComRigidBody *connectedBody);

    bool                    IsCollisionEnabled() const { return collisionEnabled; }
    void                    SetCollisionEnabled(bool enabled);

    float                   GetBreakImpulse() const { return breakImpulse; }
    void                    SetBreakImpulse(float breakImpulse);

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    virtual void            CreateConstraint() = 0;

    void                    TransformUpdated(const ComTransform *transform);

    Guid                    connectedBodyGuid;
    mutable ComRigidBody *  connectedBody;
    PhysConstraint *        constraint;
    bool                    collisionEnabled;
    float                   breakImpulse;
};

BE_NAMESPACE_END
