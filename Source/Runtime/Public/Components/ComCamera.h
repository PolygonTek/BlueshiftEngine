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
#include "Render/RenderObject.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class Mesh;
class RenderWorld;
class ComTransform;

class ComCamera : public Component {
    friend class GameWorld;

public:
    OBJECT_PROTOTYPE(ComCamera);

    ComCamera();
    virtual ~ComCamera();
    
    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderView::State &viewState, bool selected) override;

    virtual const AABB      GetAABB() override;

    float                   GetSize() const { return size; }

    float                   GetAspectRatio() const;

    const Point             WorldToScreen(const Vec3 &position) const;

    const Ray               ScreenToRay(const Point &screenPoint);

    int                     GetLayerMask() const;
    void                    SetLayerMask(int layerMask);

    int                     GetProjectionMethod() const;
    void                    SetProjectionMethod(int projectionMethod);

    float                   GetNear() const;
    void                    SetNear(const float zNear);

    float                   GetFar() const;
    void                    SetFar(const float zFar);

    RenderView::ClearMethod GetClearMethod() const;
    void                    SetClearMethod(RenderView::ClearMethod clearMethod);

    const Color3 &          GetClearColor() const;
    void                    SetClearColor(const Color3 &clearColor);

    float                   GetClearAlpha() const;
    void                    SetClearAlpha(float clearAlpha);

private:
    int                     GetOrder() const { return order; }

    bool                    ProcessMousePointerInput(const Point &screenPoint);
    bool                    ProcessTouchPointerInput();

    void                    RenderScene();

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    UpdateVisuals();

    void                    TransformUpdated(const ComTransform *transform);

    RenderView *            renderView;
    RenderView::State       renderViewDef;
    float                   fov;
    float                   size;
    float                   nx, ny, nw, nh;     ///< normalized screen coordinates
    int                     order;              ///< rendering order

    RenderWorld *           renderWorld;

    Mesh *                  spriteMesh;
    RenderObject::State     spriteDef;
    int                     spriteHandle;

    struct PointerState {
        Guid                oldHitEntityGuid;
        Guid                captureEntityGuid;
    };

    PointerState            mousePointerState;
    HashTable<int32_t, PointerState> touchPointerStateTable;
};

BE_NAMESPACE_END
