#pragma once

#include "Render/SceneEntity.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class Mesh;
class RenderContext;
class RenderWorld;
class ComTransform;
class ComRigidBody;
class PhysRigidBody;

class ComCamera : public Component {
    friend class GameWorld;

public:
    OBJECT_PROTOTYPE(ComCamera);

    ComCamera();
    virtual ~ComCamera();
    
    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Enable(bool enable) override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

    virtual void            Update() override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    virtual const AABB      GetAABB() override;

    const Point             WorldToScreen(const Vec3 &position) const;

    const Ray               ScreenToRay(const Point &screenPoint);

private:
    int                     GetOrder() const { return order; }

    void                    ProcessPointerInput(const Point &screenPoint);

    void                    Render();    

protected:
    void                    UpdateVisuals();

    const int               GetLayerMask() const;
    void                    SetLayerMask(const int layerMask);

    const int               GetProjectionMethod() const;
    void                    SetProjectionMethod(const int projectionMethod);

    const float             GetNear() const;
    void                    SetNear(const float zNear);

    const float             GetFar() const;
    void                    SetFar(const float zFar);

    const int               GetClearMethod() const;
    void                    SetClearMethod(const int clearMethod);

    const Color3 &          GetClearColor() const;
    void                    SetClearColor(const Color3 &clearColor);

    const float             GetClearAlpha() const;
    void                    SetClearAlpha(const float clearAlpha);

    void                    PropertyChanged(const char *classname, const char *propName);
    void                    TransformUpdated(const ComTransform *transform);
    void                    PhysicsUpdated(const PhysRigidBody *body);
        
    Mesh *                  spriteMesh;
    SceneEntity::Parms      sprite;
    int                     spriteHandle;

    SceneView *             view;
    SceneView::Parms        viewParms;
    float                   fov;
    float                   size;
    float                   nx, ny, nw, nh;     ///< normalized screen coordinates
    int                     order;              ///< rendering order

    RenderWorld *           renderWorld;
};

BE_NAMESPACE_END
