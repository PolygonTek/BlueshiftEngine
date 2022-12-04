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

class ComHingeJoint : public ComJoint {
public:
    OBJECT_PROTOTYPE(ComHingeJoint);

    ComHingeJoint();
    virtual ~ComHingeJoint();

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual void            Awake() override;

#if WITH_EDITOR
                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    const Vec3 &            GetAnchor() const { return anchor; }
    void                    SetAnchor(const Vec3 &anchor);

    Angles                  GetAngles() const { return axis.ToAngles(); }
    void                    SetAngles(const Angles &angles);

    const Vec3 &            GetConnectedAnchor() const { return connectedAnchor; }
    void                    SetConnectedAnchor(const Vec3 &anchor);

    const Mat3 &            GetConnectedAxis() const { return connectedAxis; }
    void                    SetConnectedAxis(const Mat3 &axis);

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

    Mat3                    axis = Mat3::identity;
    Vec3                    anchor = Vec3::zero;
    Mat3                    connectedAxis;
    Vec3                    connectedAnchor;
    float                   minAngle = 0.0f;
    float                   maxAngle = 0.0f;
    bool                    enableLimitAngles = false;
    float                   motorTargetVelocity = 0.0f;
    float                   maxMotorImpulse = 0.0f;

#if WITH_EDITOR
    Mat3                    startLocalAxisInConnectedBody;
#endif
};

BE_NAMESPACE_END
