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

class ComSpringJoint : public ComJoint {
public:
    OBJECT_PROTOTYPE(ComSpringJoint);

    ComSpringJoint();
    virtual ~ComSpringJoint();

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

    bool                    GetEnableLimitDistances() const { return enableLimitDistances; }
    void                    SetEnableLimitDistances(bool enable);

    float                   GetMinimumDistance() const { return minDist; }
    void                    SetMinimumDistance(float minDist);
    float                   GetMaximumDistance() const { return maxDist; }
    void                    SetMaximumDistance(float maxDist);

    float                   GetStiffness() const { return stiffness; }
    void                    SetStiffness(float stiffness);

    float                   GetDamping() const { return damping; }
    void                    SetDamping(float damping);

protected:
    virtual void            CreateConstraint() override;

    Mat3                    axis = Mat3::identity;
    Vec3                    anchor = Vec3::zero;
    Mat3                    connectedAxis;
    Vec3                    connectedAnchor;
    bool                    enableLimitDistances = false;
    float                   minDist = 0.0f;
    float                   maxDist = 0.0f;
    float                   stiffness = 30.0f;
    float                   damping = 0.2f;
};

BE_NAMESPACE_END