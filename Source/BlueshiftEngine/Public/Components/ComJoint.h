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

class LuaVM;
class PhysConstraint;
class ComRigidBody;

class ComJoint : public Component {
    friend class LuaVM;

public:
    ABSTRACT_PROTOTYPE(ComJoint);

    ComJoint();
    virtual ~ComJoint() = 0;

                            /// Returns true if same component is allowed
    virtual bool            AllowSameComponent() const override { return true; }

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started.
                            /// When game already started, called immediately after spawned
    virtual void            Start() override;

    bool                    IsCollisionEnabled() const;
    void                    SetCollisionEnabled(bool enabled);

    float                   GetBreakImpulse() const;
    void                    SetBreakImpulse(float breakImpulse);

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    Guid                    connectedBodyGuid;
    const ComRigidBody *    connectedBody;
    PhysConstraint *        constraint;
    bool                    collisionEnabled;
    float                   breakImpulse;
};

BE_NAMESPACE_END
