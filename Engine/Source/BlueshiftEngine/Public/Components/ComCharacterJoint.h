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

                            /// Called once when game started.
                            /// When game already started, called immediately after spawned
    virtual void            Start() override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    const Vec3 &            GetAnchor() const;
    void                    SetAnchor(const Vec3 &anchor);

    Angles                  GetAngles() const;
    void                    SetAngles(const Angles &angles);

    float                   GetSwing1LowerLimit() const;
    void                    SetSwing1LowerLimit(const float limit);

    float                   GetSwing1UpperLimit() const;
    void                    SetSwing1UpperLimit(const float limit);

    float                   GetSwing1Stiffness() const;
    void                    SetSwing1Stiffness(const float stiffness);

    float                   GetSwing1Damping() const;
    void                    SetSwing1Damping(const float damping);

    float                   GetSwing2LowerLimit() const;
    void                    SetSwing2LowerLimit(const float limit);

    float                   GetSwing2UpperLimit() const;
    void                    SetSwing2UpperLimit(const float limit);

    float                   GetSwing2Stiffness() const;
    void                    SetSwing2Stiffness(float stiffness);

    float                   GetSwing2Damping() const;
    void                    SetSwing2Damping(float damping);

    float                   GetTwistLowerLimit() const;
    void                    SetTwistLowerLimit(float limit);

    float                   GetTwistUpperLimit() const;
    void                    SetTwistUpperLimit(float limit);

    float                   GetTwistStiffness() const;
    void                    SetTwistStiffness(float stiffness);

    float                   GetTwistDamping() const;
    void                    SetTwistDamping(float damping);

protected:
    Vec3                    anchor;
    Mat3                    axis;
    Vec3                    lowerLimit;
    Vec3                    upperLimit;
    Vec3                    stiffness;
    Vec3                    damping;
};

BE_NAMESPACE_END
