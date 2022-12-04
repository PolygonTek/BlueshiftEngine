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

class ComCharacterJoint : public ComJoint {
public:
    OBJECT_PROTOTYPE(ComCharacterJoint);

    ComCharacterJoint();
    virtual ~ComCharacterJoint();

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual void            Awake() override;

#if WITH_EDITOR
                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    const Vec3 &            GetAnchor() const { return anchor; }
    void                    SetAnchor(const Vec3 &anchor);

    const Mat3 &            GetAxis() const { return axis; }
    void                    SetAxis(const Mat3 &axis);

    const Vec3 &            GetConnectedAnchor() const { return connectedAnchor; }
    void                    SetConnectedAnchor(const Vec3 &anchor);

    const Mat3 &            GetConnectedAxis() const { return connectedAxis; }
    void                    SetConnectedAxis(const Mat3 &axis);

    const Vec3 &            GetSwing1Axis() const { return swing1Axis; }
    void                    SetSwing1Axis(const Vec3 &swing1Axis);

    const Vec3 &            GetSwing2Axis() const { return swing2Axis; }
    void                    SetSwing2Axis(const Vec3 &swing2Axis);

    float                   GetSwing1Limit() const { return angleLimits.x; }
    void                    SetSwing1Limit(const float limit);
    float                   GetSwing1Stiffness() const { return stiffness.x; }
    void                    SetSwing1Stiffness(const float stiffness);
    float                   GetSwing1Damping() const { return damping.x; }
    void                    SetSwing1Damping(const float damping);

    float                   GetSwing2Limit() const { return angleLimits.y; }
    void                    SetSwing2Limit(const float limit);
    float                   GetSwing2Stiffness() const { return stiffness.y; }
    void                    SetSwing2Stiffness(float stiffness);
    float                   GetSwing2Damping() const { return damping.y; }
    void                    SetSwing2Damping(float damping);

    float                   GetTwistLimit() const { return angleLimits.z; }
    void                    SetTwistLimit(float limit);
    float                   GetTwistStiffness() const { return stiffness.z; }
    void                    SetTwistStiffness(float stiffness);
    float                   GetTwistDamping() const { return damping.z; }
    void                    SetTwistDamping(float damping);

protected:
    void                    ApplyAngleLimits();
    virtual void            CreateConstraint() override;

    Mat3                    axis = Mat3::identity;
    Vec3                    anchor = Vec3::zero;
    Mat3                    connectedAxis;
    Vec3                    connectedAnchor;
    Angles                  baseAngles = Angles::zero;
    Vec3                    swing1Axis = Vec3::unitX;
    Vec3                    swing2Axis = Vec3::unitY;
    Vec3                    angleLimits = Vec3::zero;
    Vec3                    stiffness = Vec3::zero;
    Vec3                    damping = Vec3(0.2f, 0.2f, 0.2f);

#if WITH_EDITOR
    Mat3                    startLocalAxisInConnectedBody;
#endif
};

BE_NAMESPACE_END
