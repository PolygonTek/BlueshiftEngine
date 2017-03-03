#pragma once

#include "ComJoint.h"

BE_NAMESPACE_BEGIN

class ComFixedJoint : public ComJoint {
public:
    OBJECT_PROTOTYPE(ComFixedJoint);

    ComFixedJoint();
    virtual ~ComFixedJoint();

    virtual void            Init() override;

    virtual void            Start() override;

protected:
    void                    PropertyChanged(const char *classname, const char *propName);
};

BE_NAMESPACE_END
