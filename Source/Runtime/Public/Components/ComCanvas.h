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

#include "Containers/HashTable.h"
#include "Component.h"
#include "Game/InputUtils.h"

BE_NAMESPACE_BEGIN

class ComRectTransform;

class ComCanvas : public Component {
    friend class GameWorld;

public:
    OBJECT_PROTOTYPE(ComCanvas);

    ComCanvas();
    virtual ~ComCanvas();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

#if WITH_EDITOR
                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    virtual const AABB      GetAABB() const override;

                            /// Converts position in world space to screen space.
    const Point             WorldToScreen(const Vec3 &worldPos) const;

                            /// Makes world space ray from screen space point.
    const Ray               ScreenPointToRay(const Point &screenPoint);

protected:
    virtual void            OnInactive() override;

    RenderCamera *          renderCamera;
    RenderCamera::State     renderCameraDef;

private:
    bool                    ProcessMousePointerInput();
    bool                    ProcessTouchPointerInput();

    void                    Render();

    InputUtils::PointerState mousePointerState;
    HashTable<int32_t, InputUtils::PointerState> touchPointerStateTable;
};

BE_NAMESPACE_END
