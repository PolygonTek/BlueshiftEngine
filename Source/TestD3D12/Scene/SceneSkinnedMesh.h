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

#include "Core/JointPose.h"
#include "Containers/Array.h"
#include "Math/AABB.h"
#include "Scene/Scene.h"
#include "Scene/GameObject.h"
#include "Scene/GameLight.h"

class Anim;
class Material;
class Mesh;
class MeshInstance;
class Skeleton;

class SceneSkinnedMesh : public Scene {
public:
    void                            Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) override;
    void                            Shutdown() override;

    void                            Update(uint64_t elapsedUsec) override;

#ifdef ENABLE_IMGUI
    void                            BuildImGui() override;
#endif

private:
    static constexpr int            MeshCount = 2;
    static constexpr int            AnimCount = 3;
    static constexpr int            MaxObjectCountPerAxis = 50;
    static constexpr float          ObjectSpacing = 2.5f;
    static constexpr float          GroundTextureTileSize = BE1::MeterToUnit(10.0f);

    struct SkinnedMeshObject {
        GameObject                  gameObject;
        SharedPtr<MeshInstance>     meshInstances[MeshCount];
        int                         renderObjectHandles[MeshCount] = { -1, -1 };
        int                         animIndex = 0;
        float                       animationSpeed = 1.0f;
        double                      animationTimeMsec = 0.0;
    };

    struct AnimState {
        BE1::Array<BE1::Mat3x4>     jointMatrices;
        BE1::Array<BE1::AABB>       frameAABBs[MeshCount];
        BE1::AABB                   aabbs[MeshCount] = { BE1::AABB::empty, BE1::AABB::empty };
    };

    bool                            LoadCharacter();
    bool                            InitGameObjects(int countX, int countY);
    void                            InitGround();
    void                            ClearGameObjects();
    void                            ChangeGameObjectCount(int countX, int countY);
    bool                            EvaluateAnimationPose(int animIndex, int animationTimeMsec);
    bool                            UpdateAnimationPoses(double deltaMsec);
    void                            DrawBounds() const;
    void                            UpdateDirectionalLightDirection();

    RenderWorld *                   renderWorld = nullptr;
    BE1::PhysicsWorld *             physicsWorld = nullptr;

    SharedPtr<Mesh>                 meshes[MeshCount];
    SharedPtr<Skeleton>             skeleton;
    SharedPtr<Anim>                 anims[AnimCount];
    AnimState                       animStates[AnimCount];

    BE1::Array<SkinnedMeshObject>   gameObjects;
    BE1::Vec3                       characterOffset = BE1::Vec3::zero;
    int                             objectCountX = 1;
    int                             objectCountY = 1;

    SharedPtr<Material>             characterMaterial;
    SharedPtr<Material>             groundMaterial;
    SharedPtr<MeshInstance>         groundMeshInstance;
    float                           groundHeight = 0.0f;

    GameLight                       directionalLight;
    float                           directionalLightAzimuth = 190.0f;
    float                           directionalLightElevation = 48.0f;
    bool                            directionalLightEnabled = false;

    BE1::Array<int>                 jointIndexes;
    BE1::Array<BE1::JointPose>      jointPoses;

    uint64_t                        previousUpdateUsec = 0;
    float                           playbackSpeed = 1.0f;
    bool                            playing = true;
    bool                            showBounds = false;
    bool                            envLightAdded = false;

#ifdef ENABLE_IMGUI
    float                           envLightRotation = 0.0f;
    uint64_t                        imguiTrianglesPerObject = 0;
    int                             imguiObjectCountX = 1;
    int                             imguiObjectCountY = 1;
    bool                            pendingObjectCountChange = false;
#endif
};
