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

#pragma once

#include "Scene/Scene.h"
#include "Scene/GameLight.h"

class Mesh;

class SceneStaticMesh : public Scene {
public:
    void                        Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) override;
    void                        Shutdown() override;

    void                        Update(uint64_t elapsedUsec) override;

#ifdef ENABLE_IMGUI
    void                        BuildImGui() override;
#endif

private:
    RenderWorld *               renderWorld = nullptr;
    SharedPtr<Mesh>             testMesh;
    GameLight                   pointLight;
    GameLight                   spotLight;
    float                       spotPitch = 90.0f;
    float                       spotYaw = 0.0f;

    bool                        pointLightEnabled = false;
    bool                        spotLightEnabled = false;
};
