#pragma once

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComLogic : public Component {
public:
    ABSTRACT_PROTOTYPE(ComLogic);

    ComLogic();
    virtual ~ComLogic() = 0;

    virtual bool            AllowSameComponent() const override { return true; }

    virtual void            OnPointerEnter() {}
    virtual void            OnPointerExit() {}
    virtual void            OnPointerOver() {}
    virtual void            OnPointerDown() {}
    virtual void            OnPointerUp() {}
    virtual void            OnPointerDrag() {}

protected:
    void                    PropertyChanged(const char *classname, const char *propName);
};

BE_NAMESPACE_END
