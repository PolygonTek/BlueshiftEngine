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
    struct Direction {
        enum Enum {
            XAxis,
            YAxis,
            ZAxis
        };
    };

    OBJECT_PROTOTYPE(ComCylinderCollider);

    ComCylinderCollider();
    virtual ~ComCylinderCollider();

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

#if WITH_EDITOR
                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;

    virtual bool            GetHandlePosition(int handleIndex, Vec3 &handlePosition) const override;
#endif

    virtual void            CreateCollider() override;

    virtual Vec3            GetColliderCenter() const override { return center; }
    virtual Mat3            GetColliderAxis() const override;

    Vec3                    GetCenter() const { return center; }
    void                    SetCenter(const Vec3 &center);

    float                   GetRadius() const { return radius; }
    void                    SetRadius(float radius);

    float                   GetHeight() const { return height; }
    void                    SetHeight(float height);

protected:
    Vec3                    center = Vec3::zero;        ///< Local center position in system units
    float                   radius = MeterToUnit(0.5f); ///< Radius in system units
    float                   height = MeterToUnit(1.0f); ///< Height in system units
    Direction::Enum         direction = Direction::Enum::ZAxis;
};

BE_NAMESPACE_END
