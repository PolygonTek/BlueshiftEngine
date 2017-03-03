#pragma once

#include "ComCollider.h"

BE_NAMESPACE_BEGIN

class LuaVM;

class ComBoxCollider : public ComCollider {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComBoxCollider);

    ComBoxCollider();
    virtual ~ComBoxCollider();

    virtual void            Init() override;

    virtual void            Enable(bool enable) override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    void                    PropertyChanged(const char *classname, const char *propName);

    Vec3                    center;
    Vec3                    extents;
};

BE_NAMESPACE_END
