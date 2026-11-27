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
class SkinningPose;

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
    static constexpr int            MaxObjectCountPerAxis = 64;
    static constexpr int            AnimationUpdateDistanceThresholdCount = 3;
    static constexpr float          ObjectSpacing = 2.5f;
    static constexpr float          GroundTextureTileSize = BE1::MeterToUnit(10.0f);

    enum class AnimationUpdateMode : uint8_t {
        Disabled,
        FixedInterval,
        DistanceBased
    };

    struct AnimationUpdateStats {
        int                         evaluated = 0;              // 이번 프레임에 포즈 평가와 적용에 성공한 캐릭터 수.
        int                         skipped = 0;                // 이번 프레임에 포즈 평가를 생략한 캐릭터 수.
        int                         forced = 0;                 // evaluated 중 갱신 주기와 무관하게 강제 평가한 캐릭터 수.
        int                         failed = 0;                 // 이번 프레임에 포즈 평가 또는 적용에 실패한 캐릭터 수.
        int                         intervalCounts[AnimationUpdateDistanceThresholdCount + 1] = {}; // 평가 간격별 캐릭터 수. 인덱스 0·1·2·3은 각각 1·2·4·8프레임에 대응한다.
    };

    struct SkinnedMeshObject {
        GameObject                  gameObject;
        SharedPtr<SkinningPose>     skinningPose;
        SharedPtr<MeshInstance>     meshInstances[MeshCount];
        int                         renderObjectHandles[MeshCount] = { -1, -1 };
        int                         animIndex = 0;
        float                       animationSpeed = 1.0f;
        double                      animationTimeMsec = 0.0;
        uint8_t                     updateInterval = 1;
        bool                        poseUpdatePending = true;
    };

    struct AnimState {
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
    BE1::Array<BE1::Mat3x4>         jointMatrices;

    uint64_t                        previousUpdateUsec = 0;
    uint64_t                        animationUpdateFrameIndex = 0;
    AnimationUpdateMode             animationUpdateMode = AnimationUpdateMode::DistanceBased;
    float                           animationUpdateDistanceThresholds[AnimationUpdateDistanceThresholdCount] = { 20.0f, 50.0f, 100.0f }; // 1→2, 2→4, 4→8프레임 전환 기준 거리. 미터 단위.
    uint8_t                         animationUpdateFixedInterval = 2;
    AnimationUpdateStats            animationUpdateStats;
    float                           playbackSpeed = 1.0f;
    bool                            playing = true;
    bool                            pendingAnimationPoseUpdate = false; // 포즈 갱신을 기다리는 캐릭터가 있는지 나타낸다.
    bool                            pendingAnimationUpdateRateChange = false; // 변경된 갱신 주기 설정을 다음 업데이트에 적용한다.
    bool                            showBounds = false;
    bool                            envLightAdded = false;

#ifdef ENABLE_IMGUI
    uint64_t                        animationUpdateUsec = 0;
    float                           envLightRotation = 0.0f;
    uint64_t                        imguiTrianglesPerObject = 0;
    int                             imguiObjectCountX = 1;
    int                             imguiObjectCountY = 1;
    bool                            pendingObjectCountChange = false;
#endif
};
