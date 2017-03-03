#pragma once

#include "ComJoint.h"

BE_NAMESPACE_BEGIN

class LuaVM;

class ComSocketJoint : public ComJoint {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComSocketJoint);

    ComSocketJoint();
    virtual ~ComSocketJoint();

    virtual void            Init() override;

    virtual void            Start() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    const Vec3 &            GetAnchor() const;
    void                    SetAnchor(const Vec3 &anchor);

    void                    PropertyChanged(const char *classname, const char *propName);

    Vec3                    anchor;
};

BE_NAMESPACE_END
