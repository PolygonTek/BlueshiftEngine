#pragma once

#include "ComCollider.h"

BE_NAMESPACE_BEGIN

class ComMeshCollider : public ComCollider {
public:
    OBJECT_PROTOTYPE(ComMeshCollider);

    ComMeshCollider();
    virtual ~ComMeshCollider();

    virtual void            Init() override;

    virtual void            Enable(bool enable) override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    Guid                    GetMesh() const;
    void                    SetMesh(const Guid &meshGuid);

    void                    PropertyChanged(const char *classname, const char *propName);

    Guid                    meshGuid;
    bool                    convex;
};

BE_NAMESPACE_END
