#pragma once

#include "Core/Object.h"

BE_NAMESPACE_BEGIN

class GameWorld;

class GameSettings : public Object {
    friend class GameWorld;

public:
    ABSTRACT_PROTOTYPE(GameSettings);

    GameSettings() {}

    bool					IsInitalized() const { return initialized; }

    GameWorld *				GetGameWorld() const { return gameWorld; }

    virtual void			Init();

protected:
    void					SetInitialized(bool init) { initialized = init; }
    void					SetGameWorld(GameWorld *gameWorld) { this->gameWorld = gameWorld; }

    void					PropertyChanged(const char *classname, const char *propName);

    bool					initialized;
    GameWorld *				gameWorld;
};

BE_NAMESPACE_END