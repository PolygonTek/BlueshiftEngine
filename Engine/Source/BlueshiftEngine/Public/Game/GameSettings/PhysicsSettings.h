#pragma once

#include "Game/GameSettings/GameSettings.h"

BE_NAMESPACE_BEGIN

class PhysicsSettings : public GameSettings {
public:
    OBJECT_PROTOTYPE(PhysicsSettings);

    PhysicsSettings() {}

    virtual void			Init() override;

protected:
    void					PropertyChanged(const char *classname, const char *propName);
};

BE_NAMESPACE_END