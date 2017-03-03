#pragma once

#include "Render/SceneEntity.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class PhysRigidBody;
class RenderWorld;
class ComTransform;

class ComRenderable : public Component {
public:
    ABSTRACT_PROTOTYPE(ComRenderable);

    ComRenderable();
    virtual ~ComRenderable() = 0;

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Enable(bool enable) override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

    virtual const AABB      GetAABB() override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

    void                    SetWireframeColor(const Color4 &color);
    void                    ShowWireframe(SceneEntity::WireframeMode wireframeMode);

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

protected:
    void                    UpdateVisuals();

    void                    PropertyChanged(const char *classname, const char *propName);
    void                    LayerChanged(const Entity *entity);
    void                    TransformUpdated(const ComTransform *transform);
    void                    PhysicsUpdated(const PhysRigidBody *body);

    SceneEntity::Parms      sceneEntity;
    int                     sceneEntityHandle;
    RenderWorld *           renderWorld;
};

BE_NAMESPACE_END
