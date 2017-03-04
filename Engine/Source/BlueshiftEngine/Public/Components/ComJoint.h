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

    virtual bool            AllowSameComponent() const override { return true; }

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Start() override;

    virtual void            Enable(bool enable) override;

protected:
    const Guid              GetConnectedBody() const;
    void                    SetConnectedBody(const Guid &guid);

    const bool              IsCollisionEnabled() const;
    void                    SetCollisionEnabled(const bool enabled);

    const float             GetBreakImpulse() const;
    void                    SetBreakImpulse(const float breakImpulse);

    void                    PropertyChanged(const char *classname, const char *propName);

    const ComRigidBody *    connectedBody;
    PhysConstraint *        constraint;
    bool                    collisionEnabled;
    float                   breakImpulse;
};

BE_NAMESPACE_END
