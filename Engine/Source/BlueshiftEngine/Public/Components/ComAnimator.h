#pragma once

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComAnimator : public Component {
public:
    OBJECT_PROTOTYPE(ComAnimator);

    ComAnimator();
    virtual ~ComAnimator();

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            Enable(bool enable) override;

    virtual void            Update() override;

protected:
    void                    PropertyChanged(const char *classname, const char *propName);
};

BE_NAMESPACE_END
