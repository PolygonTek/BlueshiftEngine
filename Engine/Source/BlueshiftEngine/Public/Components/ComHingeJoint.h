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

#include "ComJoint.h"

BE_NAMESPACE_BEGIN

class LuaVM;

class ComHingeJoint : public ComJoint {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComHingeJoint);

    ComHingeJoint();
    virtual ~ComHingeJoint();

    virtual void            Init() override;

    virtual void            Start() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    const Vec3 &            GetAnchor() const;
    void                    SetAnchor(const Vec3 &anchor);

    const Angles            GetAngles() const;
    void                    SetAngles(const Angles &angles);

    const float             GetMotorSpeed() const;
    void                    SetMotorSpeed(const float motorSpeed);

    const float             GetMaxMotorImpulse() const;
    void                    SetMaxMotorImpulse(const float maxMotorImpulse);

    void                    PropertyChanged(const char *classname, const char *propName);

    Vec3                    anchor;
    Mat3                    axis;
    float                   motorSpeed;
    float                   maxMotorImpulse;
};

BE_NAMESPACE_END
