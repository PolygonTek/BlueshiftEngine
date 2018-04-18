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

#include "ComCollider.h"

BE_NAMESPACE_BEGIN

class LuaVM;

class ComCylinderCollider : public ComCollider {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComCylinderCollider);

    ComCylinderCollider();
    virtual ~ComCylinderCollider();

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderView::State &viewState, bool selected) override;

    Vec3                    GetCenter() const { return center; }
    void                    SetCenter(const Vec3 &center);

    float                   GetRadius() const { return radius; }
    void                    SetRadius(float radius);

    float                   GetHeight() const { return height; }
    void                    SetHeight(float height);

protected:
    virtual void            CreateCollider() override;

    Vec3                    center;     ///< Local center position in system units
    float                   radius;     ///< Radius in system units
    float                   height;     ///< Height in system units
};

BE_NAMESPACE_END
