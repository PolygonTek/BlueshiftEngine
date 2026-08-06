// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Scene/SceneMaterialBall.h"
#include "Scene/SceneMatrix.h"
#include "Scene/SceneDebugDraw.h"
#include "Scene/SceneRegistry.h"

using SceneFactory = Scene *(*)();

struct SceneEntry {
    const char *    name;
    SceneFactory    factory;
};

static Scene *CreateSceneMatrix() {
    return new SceneMatrix;
}

static Scene *CreateSceneMaterialBall() {
    return new SceneMaterialBall;
}

static Scene *CreateSceneDebugDraw() {
    return new SceneDebugDraw;
}

static constexpr SceneEntry SceneEntries[] = {
    { "Matrix", CreateSceneMatrix },
    { "Material Ball", CreateSceneMaterialBall },
    { "Debug Draw", CreateSceneDebugDraw },
};

int SceneRegistry::GetCount() {
    return COUNT_OF(SceneEntries);
}

const char *SceneRegistry::GetName(int sceneIndex) {
    assert(sceneIndex >= 0 && sceneIndex < GetCount());
    return SceneEntries[sceneIndex].name;
}

Scene *SceneRegistry::Create(int sceneIndex) {
    assert(sceneIndex >= 0 && sceneIndex < GetCount());
    return SceneEntries[sceneIndex].factory();
}
