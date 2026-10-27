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
#include "Scene/GameObject.h"
#include "Scene/GameLight.h"

class Mesh;

class SceneMatrix : public Scene {
public:
    void                        Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) override;
    void                        Shutdown() override;

    void                        Update(uint64_t elapsedUsec) override;

#ifdef ENABLE_IMGUI
    void                        BuildImGui() override;
#endif

private:
    void                        InitGameObjects(uint32_t countX, uint32_t countY, uint32_t countZ);
    void                        InitGameLights(uint32_t countX, uint32_t countY, uint32_t countZ);
    void                        ClearGameObjects();
    void                        ClearGameLights();
    void                        ChangeGameObjectCount(uint32_t countX, uint32_t countY, uint32_t countZ);
    void                        ChangeGameLightCount(uint32_t countX, uint32_t countY, uint32_t countZ);
    void                        AnimateGameObjects(uint64_t elapsedUsec);
    void                        AnimateGameLights(uint64_t elapsedUsec);
    void                        UpdateDirectionalLightDirection();

    RenderWorld *               renderWorld = nullptr;
    BE1::PhysicsWorld *         physicsWorld = nullptr;
    SharedPtr<Mesh>             testMesh;
    BE1::Array<GameObject *>    gameObjects;
    GameLight                   directionalLight;
    BE1::Array<GameLight>       gameLights;

    float                       directionalLightAzimuth = 190.0f;
    float                       directionalLightElevation = 48.0f;
    bool                        directionalLightEnabled = true;

    uint32_t                    objectCountX = 1;
    uint32_t                    objectCountY = 1;
    uint32_t                    objectCountZ = 1;

    uint32_t                    lightCountX = 1;
    uint32_t                    lightCountY = 1;
    uint32_t                    lightCountZ = 1;

    bool                        animateObjects = true;
    bool                        animateLights = true;
    bool                        pointLightShadows = false;

#ifdef ENABLE_IMGUI
    float                       envLightRotation = 0.0f;
    int                         imguiObjectCountX = 1;
    int                         imguiObjectCountY = 1;
    int                         imguiObjectCountZ = 1;

    int                         imguiLightCountX = 1;
    int                         imguiLightCountY = 1;
    int                         imguiLightCountZ = 1;

    bool                        pendingObjectCountChange = false;
    bool                        pendingLightCountChange = false;
#endif
};
