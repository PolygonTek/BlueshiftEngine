#pragma once

#include "ComJoint.h"

BE_NAMESPACE_BEGIN

class LuaVM;

class ComSpringJoint : public ComJoint {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComSpringJoint);

    ComSpringJoint();
    virtual ~ComSpringJoint();

    virtual void            Init() override;

    virtual void            Start() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    const Vec3 &            GetAnchor() const;
    void                    SetAnchor(const Vec3 &anchor);

    const Angles            GetAngles() const;
    void                    SetAngles(const Angles &angles);

    const float             GetLowerLimit() const;
    void                    SetLowerLimit(const float limit);

    const float             GetUpperLimit() const;
    void                    SetUpperLimit(const float limit);

    const float             GetStiffness() const;
    void                    SetStiffness(const float stiffness);

    const float             GetDamping() const;
    void                    SetDamping(const float damping);

    void                    PropertyChanged(const char *classname, const char *propName);

    Vec3                    anchor;
    Mat3                    axis;
    float                   lowerLimit;
    float                   upperLimit;
    float                   stiffness;
    float                   damping;
};

BE_NAMESPACE_END
