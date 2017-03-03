#pragma once

#include "ComJoint.h"

BE_NAMESPACE_BEGIN

class LuaVM;

class ComHingeJoint : public ComJoint {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComHingeJoint);

    ComHingeJoint();
    virtual ~ComHingeJoint();

    virtual void            Init() override;

    virtual void            Start() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    const Vec3 &            GetAnchor() const;
    void                    SetAnchor(const Vec3 &anchor);

    const Angles            GetAngles() const;
    void                    SetAngles(const Angles &angles);

    const float             GetMotorSpeed() const;
    void                    SetMotorSpeed(const float motorSpeed);

    const float             GetMaxMotorImpulse() const;
    void                    SetMaxMotorImpulse(const float maxMotorImpulse);

    void                    PropertyChanged(const char *classname, const char *propName);

    Vec3                    anchor;
    Mat3                    axis;
    float                   motorSpeed;
    float                   maxMotorImpulse;
};

BE_NAMESPACE_END
