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
class Material;

class SceneReflection : public Scene {
public:
    void                        Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) override;
    void                        Shutdown() override;
    void                        Update(uint64_t elapsedUsec) override;

#ifdef ENABLE_IMGUI
    void                        BuildImGui() override;
#endif

private:
    static constexpr int        PrimitiveCount = 4;
    static constexpr int        ObjectCount = PrimitiveCount;

    void                        UpdateDirectionalLightDirection();

    RenderWorld *               renderWorld = nullptr;
    SharedPtr<Mesh>             planeMesh;
    SharedPtr<Mesh>             primitiveMeshes[PrimitiveCount];
    SharedPtr<Material>         planeMaterial;
    SharedPtr<Material>         objectMaterials[ObjectCount];
    GameObject                  objects[ObjectCount];

    uint64_t                    previousElapsedUsec = UINT64_MAX;
    uint64_t                    rotationUsec = 0;
    bool                        rotateObjects = true;

    GameLight                   directionalLight;
    float                       directionalLightAzimuth = 30.0f;
    float                       directionalLightElevation = 45.0f;
    bool                        directionalLightEnabled = false;

#ifdef ENABLE_IMGUI
    float                       envLightRotation = 0.0f;
#endif
};
