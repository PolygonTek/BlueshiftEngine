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

    virtual void            Init() override;

    virtual void            Enable(bool enable) override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    virtual const AABB      GetAABB() override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

protected:
    void                    UpdateVisuals();

    SceneLight::Type       GetLightType() const;
    void                    SetLightType(SceneLight::Type type);

    bool                    IsMainLight() const;
    void                    SetMainLight(bool isMainLight);

    bool                    IsTurnOn() const;
    void                    SetTurnOn(bool turnOn);

    Vec3                    GetLightSize() const;
    void                    SetLightSize(const Vec3 &lightSize);

    float                   GetLightZNear() const;
    void                    SetLightZNear(float lightZNear);

    float                   GetMaxVisDist() const;
    void                    SetMaxVisDist(float maxVisDist);

    Guid                    GetMaterial() const;
    void                    SetMaterial(const Guid &material);

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

    void                    PropertyChanged(const char *classname, const char *propName);
    void                    LayerChanged(const Entity *entity);
    void                    TransformUpdated(const ComTransform *transform);
    void                    PhysicsUpdated(const PhysRigidBody *body);

    SceneLight::Parms       sceneLight;
    int                     sceneLightHandle;

    Mesh *                  spriteMesh;
    SceneEntity::Parms      sprite;
    int                     spriteHandle;

    RenderWorld *           renderWorld;
};

BE_NAMESPACE_END
