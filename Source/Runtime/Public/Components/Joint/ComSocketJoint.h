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

class ComSocketJoint : public ComJoint {
public:
    OBJECT_PROTOTYPE(ComSocketJoint);

    ComSocketJoint();
    virtual ~ComSocketJoint();

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

#if WITH_EDITOR
                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    const Vec3 &            GetAnchor() const { return anchor; }
    void                    SetAnchor(const Vec3 &anchor);

    const Vec3 &            GetConnectedAnchor() const;
    void                    SetConnectedAnchor(const Vec3 &anchor);

    float                   GetImpulseClamp() const;
    void                    SetImpulseClamp(float impulseClamp);

protected:
    virtual void            CreateConstraint() override;

    Vec3                    anchor = Vec3::zero;
    Vec3                    connectedAnchor;
    float                   impulseClamp = 0.0f;
};

BE_NAMESPACE_END
