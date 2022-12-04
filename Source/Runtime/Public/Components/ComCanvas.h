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
#include "Render/RenderCamera.h"
#include "Component.h"
#include "Game/InputUtils.h"

BE_NAMESPACE_BEGIN

class ComTransform;
class ComRectTransform;

class ComCanvas : public Component {
    friend class GameWorld;

public:
    OBJECT_PROTOTYPE(ComCanvas);

    struct ScaleMode {
        enum Enum {
            ConstantPixelSize,
            ScaleWithScreenSize
        };
    };

    struct MatchMode {
        enum Enum {
            MatchWidthOrHeight,
            Expand,
            Shrink
        };
    };

    ComCanvas();
    virtual ~ComCanvas();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned.
    virtual void            Awake() override;

#if WITH_EDITOR
                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    virtual const AABB      GetAABB() const override;

    ScaleMode::Enum         GetScaleMode() const;
    void                    SetScaleMode(ScaleMode::Enum scaleMode);

    bool                    GetClearDepth() const;
    void                    SetClearDepth(bool clearDepth);

                            /// Converts position in world space to screen space.
    const Point             WorldToScreenPoint(const Vec3 &worldPos) const;

                            /// Converts position in world space to canvas space.
    const Point             WorldToCanvasPoint(const Vec3 &worldPos) const;

                            /// Converts point in screen space to canvas space.
    const Point             ScreenToCanvasPoint(const Point screenPoint) const;

                            /// Converts point in canvas space to screen space.
    const Point             CanvasToScreenPoint(const Point canvasPoint) const;

                            /// Makes world space ray from screen space point.
    const Ray               ScreenPointToRay(const Point &screenPoint);

                            /// Makes world space ray from canvas space point.
    const Ray               CanvasPointToRay(const Point &canvasPoint);

                            /// Returns true if the given screen point is in child rect transform.
    bool                    IsScreenPointOverChildRect(const Point &screenPoint);

    void                    UpdateRenderingOrderForCanvasElements() const;

protected:
    virtual void            OnInactive() override;

#if WITH_EDITOR
    void                    RectTransformUpdated(ComRectTransform *rectTransform);
#endif

    RenderCamera *          renderCamera = nullptr;
    RenderCamera::State     renderCameraDef;

private:
    Size                    GetOrthoSize() const;
    void                    UpdateRenderingOrderRecursive(Entity *entity, int sceneNum, int &order) const;
    bool                    ProcessMousePointerInput();
    bool                    ProcessTouchPointerInput();

    void                    Render();

    Size                    referenceResolution = Size(1280, 720);
    ScaleMode::Enum         scaleMode = ScaleMode::ScaleWithScreenSize;
    MatchMode::Enum         matchMode = MatchMode::MatchWidthOrHeight;
    float                   match = 0.0f;

    InputUtils::PointerState mousePointerState;
    HashTable<int32_t, InputUtils::PointerState> touchPointerStateTable;
};

BE_NAMESPACE_END
