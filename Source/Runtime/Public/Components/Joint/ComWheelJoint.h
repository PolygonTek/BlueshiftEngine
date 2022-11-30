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

class ComWheelJoint : public ComJoint {
public:
    OBJECT_PROTOTYPE(ComWheelJoint);

    ComWheelJoint();
    virtual ~ComWheelJoint();

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

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

    bool                    GetEnableSuspensionLimit() const { return enableSusLimit; }
    void                    SetEnableSuspensionLimit(bool enable);

    float                   GetMinimumSuspensionDistance() const { return minSusDist; }
    void                    SetMinimumSuspensionDistance(float dist);
    float                   GetMaximumSuspensionDistance() const { return maxSusDist; }
    void                    SetMaximumSuspensionDistance(float dist);

    float                   GetSuspensionStiffness() const { return susStiffness; }
    void                    SetSuspensionStiffness(float stiffness);

    float                   GetSuspensionDamping() const { return susDamping; }
    void                    SetSuspensionDamping(float damping);

    bool                    GetEnableSteeringLimit() const { return enableSteeringLimit; }
    void                    SetEnableSteeringLimit(bool enable);

    float                   GetMinimumSteeringAngle() const { return minSteeringAngle; }
    void                    SetMinimumSteeringAngle(float angle);
    float                   GetMaximumSteeringAngle() const { return maxSteeringAngle; }
    void                    SetMaximumSteeringAngle(float angle);

    float                   GetMotorTargetVelocity() const { return motorTargetVelocity; }
    void                    SetMotorTargetVelocity(float motorTargetVelocity);

    float                   GetMaxMotorImpulse() const { return maxMotorImpulse; }
    void                    SetMaxMotorImpulse(float maxMotorImpulse);

protected:
    virtual void            CreateConstraint() override;

    Mat3                    axis;
    Vec3                    anchor;
    Mat3                    connectedAxis;
    Vec3                    connectedAnchor;
    bool                    enableSusLimit;
    float                   minSusDist;
    float                   maxSusDist;
    float                   susStiffness;
    float                   susDamping;
    bool                    enableSteeringLimit;
    float                   minSteeringAngle;
    float                   maxSteeringAngle;
    float                   motorTargetVelocity;
    float                   maxMotorImpulse;
};

BE_NAMESPACE_END
