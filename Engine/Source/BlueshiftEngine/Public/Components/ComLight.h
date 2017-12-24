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

#include "Render/Mesh.h"
#include "Render/SceneLight.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class PhysRigidBody;
class RenderWorld;
class ComTransform;

class ComLight : public Component {
public:
    OBJECT_PROTOTYPE(ComLight);

    ComLight();
    virtual ~ComLight();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    virtual const AABB      GetAABB() override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

    SceneLight::Type        GetLightType() const;
    void                    SetLightType(SceneLight::Type type);

    bool                    IsPrimaryLight() const;
    void                    SetPrimaryLight(bool isPrimaryLight);

    bool                    IsTurnOn() const;
    void                    SetTurnOn(bool turnOn);

    Vec3                    GetLightSize() const;
    void                    SetLightSize(const Vec3 &lightSize);

    float                   GetLightZNear() const;
    void                    SetLightZNear(float lightZNear);

    float                   GetMaxVisDist() const;
    void                    SetMaxVisDist(float maxVisDist);

    Guid                    GetMaterialGuid() const;
    void                    SetMaterialGuid(const Guid &material);

    Color3                  GetColor() const;
    void                    SetColor(const Color3 &color);

    float                   GetTimeOffset() const;
    void                    SetTimeOffset(float timeOffset);

    float                   GetTimeScale() const;
    void                    SetTimeScale(float timeScale);

    bool                    IsCastShadows() const;
    void                    SetCastShadows(bool castShadows);

    float                   GetShadowOffsetFactor() const;
    void                    SetShadowOffsetFactor(float factor);

    float                   GetShadowOffsetUnits() const;
    void                    SetShadowOffsetUnits(float units);

    float                   GetFallOffExponent() const;
    void                    SetFallOffExponent(float fallOff);

    float                   GetIntensity() const;
    void                    SetIntensity(float intensity);

    const Vec3 &            GetRadius() const;
    void                    SetRadius(const Vec3 &radius);

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    UpdateVisuals();

    void                    LayerChanged(const Entity *entity);
    void                    TransformUpdated(const ComTransform *transform);

    SceneLight::Parms       sceneLight;
    int                     sceneLightHandle;

    Mesh *                  spriteMesh;
    SceneEntity::Parms      sprite;
    int                     spriteHandle;

    RenderWorld *           renderWorld;
};

BE_NAMESPACE_END
