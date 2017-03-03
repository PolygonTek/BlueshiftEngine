#include "Precompiled.h"
#include "Components/ComLogic.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Logic", ComLogic, Component)
BEGIN_EVENTS(ComLogic)
END_EVENTS
BEGIN_PROPERTIES(ComLogic)
END_PROPERTIES

ComLogic::ComLogic() {
}

ComLogic::~ComLogic() {
}

void ComLogic::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
