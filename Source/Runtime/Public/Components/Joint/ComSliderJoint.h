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

class ComSliderJoint : public ComJoint {
public:
    OBJECT_PROTOTYPE(ComSliderJoint);

    ComSliderJoint();
    virtual ~ComSliderJoint();

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

    bool                    GetEnableLimitDistances() const { return enableLimitDistances; }
    void                    SetEnableLimitDistances(bool enable);

    float                   GetMinimumDistance() const { return minDist; }
    void                    SetMinimumDistance(float minDist);
    float                   GetMaximumDistance() const { return maxDist; }
    void                    SetMaximumDistance(float maxDist);

    bool                    GetEnableLimitAngles() const { return enableLimitAngles; }
    void                    SetEnableLimitAngles(bool enable);

    float                   GetMinimumAngle() const { return minAngle; }
    void                    SetMinimumAngle(float angle);
    float                   GetMaximumAngle() const { return maxAngle; }
    void                    SetMaximumAngle(float angle);

    float                   GetLinearMotorTargetVelocity() const { return linearMotorTargetVelocity; }
    void                    SetLinearMotorTargetVelocity(float motorTargetVelocity);

    float                   GetMaxLinearMotorImpulse() const { return maxLinearMotorImpulse; }
    void                    SetMaxLinearMotorImpulse(float maxMotorImpulse);

    float                   GetAngularMotorTargetVelocity() const { return angularMotorTargetVelocity; }
    void                    SetAngularMotorTargetVelocity(float motorTargetVelocity);

    float                   GetMaxAngularMotorImpulse() const { return maxAngularMotorImpulse; }
    void                    SetMaxAngularMotorImpulse(float maxMotorImpulse);

protected:
    virtual void            CreateConstraint() override;

    Mat3                    axis;
    Vec3                    anchor;
    Mat3                    connectedAxis;
    Vec3                    connectedAnchor;
    bool                    enableLimitDistances;
    float                   minDist;
    float                   maxDist;
    bool                    enableLimitAngles;
    float                   minAngle;
    float                   maxAngle;
    float                   linearMotorTargetVelocity;
    float                   maxLinearMotorImpulse;
    float                   angularMotorTargetVelocity;
    float                   maxAngularMotorImpulse;

#if WITH_EDITOR
    Mat3                    startLocalAxisInConnectedBody;
#endif
};

BE_NAMESPACE_END
