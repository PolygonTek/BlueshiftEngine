#include "Precompiled.h"
#include "Game/GameSettings/GameSettings.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Game Settings", GameSettings, Object)
BEGIN_EVENTS(GameSettings)
END_EVENTS
BEGIN_PROPERTIES(GameSettings)
END_PROPERTIES

void GameSettings::Init() {
    SetInitialized(true);
}

void GameSettings::PropertyChanged(const char *classname, const char *propName) {
}

BE_NAMESPACE_END