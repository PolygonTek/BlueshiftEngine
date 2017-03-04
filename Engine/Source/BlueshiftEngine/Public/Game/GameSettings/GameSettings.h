// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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