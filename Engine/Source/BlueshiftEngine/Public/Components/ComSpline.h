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

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;

class ComSpline : public Component {
public:
    OBJECT_PROTOTYPE(ComSpline);

    ComSpline();
    virtual ~ComSpline();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    float                   Length();

    Vec3                    GetCurrentOrigin(float time) const;
    Mat3                    GetCurrentAxis(float time) const;

    int                     GetPointCount() const;
    void                    SetPointCount(int count);
    Guid                    GetPointGuid(int index) const;
    void                    SetPointGuid(int index, const Guid &pointGuid);

    bool                    IsLoop() const;
    void                    SetLoop(bool loop);

protected:
    void                    UpdateCurve();
    void                    PointTransformUpdated(const ComTransform *transform);

    Array<Guid>             pointGuids;
    Curve_Spline<Vec3> *    originCurve;
    Curve_Spline<Angles> *  anglesCurve;
    bool                    curveUpdated;
    bool                    loop;
};

BE_NAMESPACE_END
