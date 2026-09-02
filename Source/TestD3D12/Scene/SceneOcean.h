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
#include "Render/Frontend/OceanSurface.h"

class Material;
class Mesh;

namespace BE1 {
    class Collider;
    class PhysRigidBody;
}

class SceneOcean : public Scene {
public:
    void                        Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) override;
    void                        Shutdown() override;

    void                        Update(uint64_t elapsedUsec) override;

#ifdef ENABLE_IMGUI
    void                        BuildImGui() override;
#endif

private:
    static constexpr uint32_t   BoxCount = 6;

    struct Box {
        GameObject              gameObject;
        BE1::Vec3               size;
        BE1::Collider *         collider = nullptr;
        BE1::PhysRigidBody *    rigidBody = nullptr;
    };

    void                        DrawBuoyancyPontoons() const;
    void                        UpdateDirectionalLightDirection();

    RenderWorld *               renderWorld = nullptr;
    SharedPtr<Mesh>             boxMesh;
    SharedPtr<Material>         boxMaterial;
    Box                         boxes[BoxCount];
    OceanSurface::Desc          oceanDesc;
    GameLight                   directionalLight;

    float                       directionalLightAzimuth = 145.0f;
    float                       directionalLightElevation = 35.0f;
    bool                        showPontoons = false;
    bool                        oceanAdded = false;
    bool                        environmentLightAdded = false;
};
