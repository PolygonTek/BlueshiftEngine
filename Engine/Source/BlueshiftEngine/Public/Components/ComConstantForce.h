#pragma once

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComConstantForce : public Component {
public:
    OBJECT_PROTOTYPE(ComConstantForce);

    ComConstantForce();
    virtual ~ComConstantForce();

    virtual bool            AllowSameComponent() const override { return true; }

    virtual void            Init() override;

    virtual void            Update() override;

protected:
    void                    PropertyChanged(const char *classname, const char *propName);

    Vec3                    force;
    Vec3                    torque;
};

BE_NAMESPACE_END
