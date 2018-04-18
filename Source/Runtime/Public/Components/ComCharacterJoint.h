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

class ComCharacterJoint : public ComJoint {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComCharacterJoint);

    ComCharacterJoint();
    virtual ~ComCharacterJoint();

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

    float                   GetSwing1LowerLimit() const { return lowerLimit.x; }
    void                    SetSwing1LowerLimit(const float limit);
    float                   GetSwing1UpperLimit() const { return upperLimit.x; }
    void                    SetSwing1UpperLimit(const float limit);
    float                   GetSwing1Stiffness() const { return stiffness.x; }
    void                    SetSwing1Stiffness(const float stiffness);
    float                   GetSwing1Damping() const { return damping.x; }
    void                    SetSwing1Damping(const float damping);

    float                   GetSwing2LowerLimit() const { return lowerLimit.y; }
    void                    SetSwing2LowerLimit(const float limit);
    float                   GetSwing2UpperLimit() const { return upperLimit.y; }
    void                    SetSwing2UpperLimit(const float limit);
    float                   GetSwing2Stiffness() const { return stiffness.y; }
    void                    SetSwing2Stiffness(float stiffness);
    float                   GetSwing2Damping() const { return damping.y; }
    void                    SetSwing2Damping(float damping);

    float                   GetTwistLowerLimit() const { return lowerLimit.z; }
    void                    SetTwistLowerLimit(float limit);
    float                   GetTwistUpperLimit() const { return upperLimit.z; }
    void                    SetTwistUpperLimit(float limit);
    float                   GetTwistStiffness() const { return stiffness.z; }
    void                    SetTwistStiffness(float stiffness);
    float                   GetTwistDamping() const { return damping.z; }
    void                    SetTwistDamping(float damping);

protected:
    virtual void            CreateConstraint() override;

    Mat3                    localAxis;
    Vec3                    localAnchor;
    Mat3                    connectedAxis;
    Vec3                    connectedAnchor;
    Vec3                    lowerLimit;
    Vec3                    upperLimit;
    Vec3                    stiffness;
    Vec3                    damping;
};

BE_NAMESPACE_END
