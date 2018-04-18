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

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderView::State &viewState, bool selected) override;

    const Vec3 &            GetLocalAnchor() const;
    void                    SetLocalAnchor(const Vec3 &anchor);

    Angles                  GetLocalAngles() const;
    void                    SetLocalAngles(const Angles &angles);

    const Vec3 &            GetConnectedAnchor() const;
    void                    SetConnectedAnchor(const Vec3 &anchor);

    Angles                  GetConnectedAngles() const;
    void                    SetConnectedAngles(const Angles &angles);

    bool                    GetEnableLimitAngles() const { return enableLimitAngles; }
    void                    SetEnableLimitAngles(bool enable);

    float                   GetMinimumAngle() const { return minAngle; }
    void                    SetMinimumAngle(float minAngle);
    float                   GetMaximumAngle() const { return maxAngle; }
    void                    SetMaximumAngle(float maxAngle);

    float                   GetMotorTargetVelocity() const { return motorTargetVelocity; }
    void                    SetMotorTargetVelocity(float motorTargetVelocity);

    float                   GetMaxMotorImpulse() const { return maxMotorImpulse; }
    void                    SetMaxMotorImpulse(float maxMotorImpulse);

protected:
    virtual void            CreateConstraint() override;

    Mat3                    localAxis;
    Vec3                    localAnchor;
    Mat3                    connectedAxis;
    Vec3                    connectedAnchor;
    float                   minAngle;
    float                   maxAngle;
    bool                    enableLimitAngles;
    float                   motorTargetVelocity;
    float                   maxMotorImpulse;
};

BE_NAMESPACE_END
