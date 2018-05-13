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

#include "Render/RenderObject.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class RenderWorld;
class StaticBatch;
class MeshCombiner;
class ComTransform;

class ComRenderable : public Component {
    friend class StaticBatch;
    friend class MeshCombiner;

public:
    ABSTRACT_PROTOTYPE(ComRenderable);

    ComRenderable();
    virtual ~ComRenderable() = 0;

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

    virtual const AABB      GetAABB() override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

    void                    SetWireframeColor(const Color4 &color);

    RenderObject::WireframeMode GetWireframeMode() const { return renderObjectDef.wireframeMode; }
    void                    SetWireframeMode(RenderObject::WireframeMode wireframeMode);

    float                   GetMaxVisDist() const;
    void                    SetMaxVisDist(float maxVisDist);

    Color3                  GetColor() const;
    void                    SetColor(const Color3 &color);

    float                   GetAlpha() const;
    void                    SetAlpha(float alpha);

    float                   GetTimeOffset() const;
    void                    SetTimeOffset(float timeOffset);

    float                   GetTimeScale() const;
    void                    SetTimeScale(float timeScale);

    bool                    IsBillboard() const;
    void                    SetBillboard(bool billboard);

    bool                    IsSkipSelection() const;
    void                    SetSkipSelection(bool skip);

    bool                    IsVisibleInPreviousFrame() const;

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    virtual void            UpdateVisuals();

    void                    LayerChanged(const Entity *entity);
    void                    TransformUpdated(const ComTransform *transform);

    RenderObject::State     renderObjectDef;
    int                     renderObjectHandle;
    int                     staticBatchIndex;
    RenderWorld *           renderWorld;
};

BE_NAMESPACE_END
