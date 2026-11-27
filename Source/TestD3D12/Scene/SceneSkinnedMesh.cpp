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
#include "Profiler.h"
#include "App.h"
#include "Render/Animation/Anim.h"
#include "Render/Animation/AnimManager.h"
#include "Render/Animation/Skeleton.h"
#include "Render/Animation/SkeletonManager.h"
#include "Render/Animation/SkinningPose.h"
#include "Render/Frontend/EnvironmentLight.h"
#include "Render/Frontend/HeightFog.h"
#include "Render/Frontend/RenderWorld.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "Render/Mesh/MeshManager.h"
#include "Scene/SceneSkinnedMesh.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

void SceneSkinnedMesh::Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) {
    this->renderWorld = renderWorld;
    this->physicsWorld = physicsWorld;

    //renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);
    EnvironmentLight::Desc envLightDesc;
    renderWorld->SetSkyboxMaterial(materialManager.GetMaterial("Source/TestD3D12/Materials/skybox.material"));
    renderWorld->SetSkyboxLighting(envLightDesc.linearColor, envLightDesc.intensity);
    renderWorld->SetSkyboxRotation(envLightDesc.axis);

    app.SetCameraPitchYaw(30.0f, 0.0f);
    app.SetCameraDistance(104.0f);

    directionalLight.type = RenderLight::Type::Directional;
    directionalLight.flags = RenderLight::Flag::CastShadows | RenderLight::Flag::CastVolumetricShadows;
    directionalLight.linearColor = BE1::Color3::white;
    directionalLight.intensity = 5.0f;
    directionalLightEnabled = true;
    UpdateDirectionalLightDirection();
    directionalLightEnabled = directionalLight.SetEnabled(renderWorld, directionalLightEnabled);

    previousUpdateUsec = 0;
    animationUpdateFrameIndex = 0;
    animationUpdateMode = AnimationUpdateMode::DistanceBased;
    animationUpdateFixedInterval = 2;
    animationUpdateDistanceThresholds[0] = 20.0f;
    animationUpdateDistanceThresholds[1] = 50.0f;
    animationUpdateDistanceThresholds[2] = 100.0f;
    animationUpdateStats = {};
    pendingAnimationPoseUpdate = false;
    pendingAnimationUpdateRateChange = false;
    playbackSpeed = 1.0f;
    playing = true;
    showBounds = false;

    if (!LoadCharacter()) {
        BE_WARNLOG("SceneSkinnedMesh::Init: failed to load the YBot test character");
        return;
    }

    BE1::AABB characterAABB = animStates[0].aabbs[0];
    for (int meshIndex = 1; meshIndex < MeshCount; ++meshIndex) {
        characterAABB.AddAABB(animStates[0].aabbs[meshIndex]);
    }

    characterOffset = -characterAABB.Center();

    // 모든 애니메이션 클립에서 샘플링한 발 높이의 최솟값에 지면을 고정한다.
    float minCharacterZ = characterAABB[0].z;
    for (const AnimState &animState : animStates) {
        for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
            for (const BE1::AABB &frameAABB : animState.frameAABBs[meshIndex]) {
                minCharacterZ = BE1::Min(minCharacterZ, frameAABB[0].z);
            }
        }
    }
    groundHeight = characterOffset.z + minCharacterZ;

    groundMeshInstance = meshManager.defaultQuadMesh->GetStaticInstance();
    if (!groundMeshInstance) {
        BE_WARNLOG("SceneSkinnedMesh::Init: failed to instantiate the ground plane");
        return;
    }

    characterMaterial = materialManager.GetMaterial("Source/TestD3D12/Materials/character.material");
    if (!characterMaterial || characterMaterial == materialManager.defaultUnlitMaterial) {
        BE_WARNLOG("SceneSkinnedMesh::Init: failed to load the character material");
        characterMaterial = materialManager.whiteLitMaterial;
    }

    groundMaterial = materialManager.GetMaterial("Source/TestD3D12/Materials/ground.material");
    if (!groundMaterial || groundMaterial == materialManager.defaultUnlitMaterial) {
        BE_WARNLOG("SceneSkinnedMesh::Init: failed to load the ground material");
        groundMaterial = materialManager.whiteLitMaterial;
    }

    if (!InitGameObjects(32, 32)) {
        return;
    }

    envLightAdded = renderWorld->AddEnvironmentLight(envLightDesc);
    if (envLightAdded) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }

    HeightFog::Desc fogDesc;
    fogDesc.enabled = false;
    fogDesc.baseHeight = 0.0f;
    fogDesc.heightFalloff = 0.2f;
    fogDesc.medium.baseExtinction = 0.02f;
    fogDesc.medium.scatteringAlbedo = float3(1.0f, 1.0f, 1.0f);
    renderWorld->AddHeightFog(fogDesc);
}

void SceneSkinnedMesh::Shutdown() {
    if (renderWorld) {
        renderWorld->ClearScene();
    }
    directionalLight.renderLightHandle = -1;
    envLightAdded = false;
    gameObjects.Clear();
    characterMaterial.reset();
    groundMeshInstance.reset();
    groundMaterial.reset();

    for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
        meshes[meshIndex].reset();
    }

    for (int animIndex = 0; animIndex < AnimCount; ++animIndex) {
        anims[animIndex].reset();
        AnimState &animState = animStates[animIndex];
        for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
            animState.frameAABBs[meshIndex].Clear();
            animState.aabbs[meshIndex] = BE1::AABB::empty;
        }
    }

    skeleton.reset();

    jointIndexes.Clear();
    jointPoses.Clear();
    jointMatrices.Clear();

    previousUpdateUsec = 0;
    animationUpdateFrameIndex = 0;
    animationUpdateStats = {};
    pendingAnimationPoseUpdate = false;
    pendingAnimationUpdateRateChange = false;
#ifdef ENABLE_IMGUI
    animationUpdateUsec = 0;
    imguiTrianglesPerObject = 0;
    pendingObjectCountChange = false;
#endif
    physicsWorld = nullptr;
    renderWorld = nullptr;
}

bool SceneSkinnedMesh::LoadCharacter() {
    static constexpr const char *meshPaths[MeshCount] = {
        "Data/Test/YBot/Alpha_Surface.bmesh",
        "Data/Test/YBot/Alpha_Joints.bmesh"
    };
    static constexpr const char *animPaths[AnimCount] = {
        "Data/Test/YBot/idle.banim",
        "Data/Test/YBot/idle2.banim",
        "Data/Test/YBot/idle3.banim"
    };

    for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
        meshes[meshIndex] = meshManager.GetMesh(meshPaths[meshIndex], false);
    }

    skeleton = skeletonManager.GetSkeleton("Data/Test/YBot/Y Bot.bskel", false);
    if (!skeleton || !skeleton->IsReady() || skeleton == skeletonManager.defaultSkeleton) {
        return false;
    }

    for (int animIndex = 0; animIndex < AnimCount; ++animIndex) {
        anims[animIndex] = animManager.GetAnim(animPaths[animIndex], false);
        const SharedPtr<Anim> &anim = anims[animIndex];
        if (!anim || !anim->IsReady() || anim->IsDefaultAnim() || !anim->CheckHierarchy(skeleton.get())) {
            BE_WARNLOG("SceneSkinnedMesh::LoadCharacter: failed to load a compatible YBot animation '%s'", animPaths[animIndex]);
            return false;
        }
    }

    for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
        if (!meshes[meshIndex] || !meshes[meshIndex]->IsReady() || meshes[meshIndex] == meshManager.missingMesh ||
            !meshes[meshIndex]->IsCompatibleSkeleton(skeleton.get())) {
            BE_WARNLOG("SceneSkinnedMesh::LoadCharacter: YBot mesh, skeleton, and animation hierarchies do not match");
            return false;
        }
    }

#ifdef ENABLE_IMGUI
    // 두 메시 파트의 정보를 한 번만 캐시하고, 객체 수가 바뀌면 표시할 합계만 갱신한다.
    imguiTrianglesPerObject = 0;
    for (const SharedPtr<Mesh> &mesh : meshes) {
        const int numSurfaces = mesh->NumSurfaces();
        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; ++surfaceIndex) {
            imguiTrianglesPerObject += mesh->GetSurface(surfaceIndex)->subMesh->NumIndexes() / 3;
        }
    }
#endif

    const int numJoints = skeleton->NumJoints();
    if (numJoints <= 0) {
        return false;
    }

    jointIndexes.SetCount(numJoints);
    jointPoses.SetCount(numJoints);
    jointMatrices.SetCount(numJoints);

    for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex) {
        jointIndexes[jointIndex] = jointIndex;
    }

    // 애니메이션 바운드는 씬 초기화 시 한 번 계산하고, 매 프레임 추가 할당 없이 샘플링한다.
    for (int animIndex = 0; animIndex < AnimCount; ++animIndex) {
        AnimState &animState = animStates[animIndex];
        for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
            anims[animIndex]->ComputeFrameAABBs(skeleton.get(), meshes[meshIndex].get(), animState.frameAABBs[meshIndex]);
        }
    }

    // 씬 배치에는 초기 바운드만 필요하며, 조인트 행렬은 Update에서 계산한다.
    for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
        anims[0]->GetAABB(animStates[0].aabbs[meshIndex], animStates[0].frameAABBs[meshIndex], 0);
    }

    return true;
}

bool SceneSkinnedMesh::InitGameObjects(int countX, int countY) {
    assert(gameObjects.IsEmpty());

    objectCountX = BE1::Clamp(countX, 1, MaxObjectCountPerAxis);
    objectCountY = BE1::Clamp(countY, 1, MaxObjectCountPerAxis);

#ifdef ENABLE_IMGUI
    imguiObjectCountX = objectCountX;
    imguiObjectCountY = objectCountY;
    pendingObjectCountChange = false;
#endif

    gameObjects.SetCount(objectCountX * objectCountY);
    animationUpdateFrameIndex = 0;
    animationUpdateStats = {};
    animationUpdateStats.intervalCounts[0] = gameObjects.Count();

    const float startX = -ObjectSpacing * (objectCountX - 1) * 0.5f;
    const float startY = -ObjectSpacing * (objectCountY - 1) * 0.5f;
    const BE1::Mat3 characterRotation = BE1::Mat3::FromRotationZ(DEG2RAD(180.0f));

    RenderObject::Desc desc;
    desc.flags = RenderObject::Flag::CastShadows;
    desc.skeleton = skeleton;
    desc.materials.SetCount(1);
    desc.materials[0] = characterMaterial;

    for (int y = 0; y < objectCountY; ++y) {
        for (int x = 0; x < objectCountX; ++x) {
            SkinnedMeshObject &object = gameObjects[y * objectCountX + x];

            // 두 메시 파트가 항상 같은 포즈를 공유하도록 캐릭터별 클립과 재생 속도를 한 번만 선택한다.
            object.animIndex = BE1::RNG::GetRandom(0, AnimCount - 1);
            object.animationSpeed = BE1::RNG::GetRandom(0.5f, 1.5f);
            object.animationTimeMsec = 0.0;
            object.updateInterval = 1;
            object.poseUpdatePending = true;

            // 이 캐릭터의 메시 파트끼리 최종 포즈 하나를 공유하며, 다른 캐릭터는 별도의 포즈를 소유한다.
            object.skinningPose = MakeShared<SkinningPose>(skeleton);

            const BE1::Vec3 position = characterOffset + BE1::Vec3(startX + ObjectSpacing * x, startY + ObjectSpacing * y, 0.0f);
            object.gameObject.worldMatrix.SetTranslationRotation(position, characterRotation, false);
            desc.worldMatrix = object.gameObject.worldMatrix;

            for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
                object.meshInstances[meshIndex] = meshes[meshIndex]->InstantiateSkinned(object.skinningPose);
                if (!object.meshInstances[meshIndex]) {
                    BE_WARNLOG("SceneSkinnedMesh::InitGameObjects: failed to instantiate YBot mesh %i", meshIndex);
                    ClearGameObjects();
                    return false;
                }

                // 첫 Update에서 렌더링 전에 원본 바운드를 애니메이션 바운드로 교체한다.
                desc.aabb = meshes[meshIndex]->GetAABB();
                desc.meshInstance = object.meshInstances[meshIndex];
                object.renderObjectHandles[meshIndex] = renderWorld->AddRenderObject(desc);
            }
        }
    }

    InitGround();

    // 캐릭터를 한꺼번에 생성하므로 재생이 정지된 상태에서도 최초 포즈는 한 번 평가한다.
    pendingAnimationPoseUpdate = true;
    return true;
}

void SceneSkinnedMesh::InitGround() {
    const float sizeX = ObjectSpacing * (objectCountX + 1);
    const float sizeY = ObjectSpacing * (objectCountY + 1);
    const BE1::AABB &quadAABB = groundMeshInstance->GetAABB();
    const BE1::Vec3 quadSize = quadAABB[1] - quadAABB[0];

    // 캐릭터 격자 크기에 따라 지면 크기가 바뀌어도 지면 텍스처 밀도는 일정하게 유지한다.
    if (groundMaterial != materialManager.whiteLitMaterial &&
        !groundMaterial->SetPropertyValue("tcScale", BE1::Vec2(sizeY / GroundTextureTileSize, sizeX / GroundTextureTileSize))) {
        BE_WARNLOG("SceneSkinnedMesh::InitGround: failed to update the ground texture coordinate scale");
    }

    RenderObject::Desc desc;
    desc.flags = RenderObject::Flag::ReceiveShadows;
    desc.worldMatrix.SetTRS(BE1::Vec3(characterOffset.x, characterOffset.y, groundHeight), BE1::Angles(0.0f, -90.0f, 0.0f).ToMat3(), BE1::Vec3(1.0f, sizeY / quadSize.y, sizeX / quadSize.z));
    desc.meshInstance = groundMeshInstance;
    desc.aabb = quadAABB;
    desc.materials.SetCount(1);
    desc.materials[0] = groundMaterial;
    renderWorld->AddRenderObject(desc);
}

void SceneSkinnedMesh::ClearGameObjects() {
    renderWorld->ClearScene(RenderWorld::ClearFlag::RenderObjects);
    gameObjects.Clear();
    pendingAnimationPoseUpdate = false;
    animationUpdateStats = {};
}

void SceneSkinnedMesh::ChangeGameObjectCount(int countX, int countY) {
    ClearGameObjects();
    InitGameObjects(countX, countY);
}

bool SceneSkinnedMesh::EvaluateAnimationPose(int animIndex, int animationTimeMsec) {
    const SharedPtr<Anim> &anim = anims[animIndex];
    if (!anim || !anim->IsReady() || !skeleton || !skeleton->IsReady()) {
        return false;
    }

    AnimState &animState = animStates[animIndex];
    const int numJoints = skeleton->NumJoints();
    if (numJoints <= 0 || jointIndexes.Count() != numJoints || jointPoses.Count() != numJoints || jointMatrices.Count() != numJoints) {
        return false;
    }

    Anim::FrameInterpolation frameInterpolation;
    anim->TimeToFrameInterpolation(animationTimeMsec, frameInterpolation);
    anim->GetInterpolatedFrame(frameInterpolation, numJoints, jointIndexes.Ptr(), jointPoses.Ptr());

    BE1::simdProcessor->ConvertJointPosesToJointMats(jointMatrices.Ptr(), jointPoses.Ptr(), numJoints);
    BE1::simdProcessor->TransformJoints(jointMatrices.Ptr(), skeleton->GetJointParentIndexes(), 0, numJoints - 1);

    for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
        anim->GetAABB(animState.aabbs[meshIndex], animState.frameAABBs[meshIndex], animationTimeMsec);
    }

    return true;
}

bool SceneSkinnedMesh::UpdateAnimationPoses(double deltaMsec) {
    PROFILER_CPU_SCOPED_EVENT("SceneSkinnedMesh::UpdateAnimationPoses");

#ifdef ENABLE_IMGUI
    BE1::ScopedCpuTimer animationTimer(animationUpdateUsec);
#endif

    double animationLengths[AnimCount];
    for (int animIndex = 0; animIndex < AnimCount; ++animIndex) {
        const SharedPtr<Anim> &anim = anims[animIndex];
        animationLengths[animIndex] = anim && anim->IsReady() ? static_cast<double>(anim->Length()) : -1.0;
    }

    const bool distanceBased = animationUpdateMode == AnimationUpdateMode::DistanceBased;
    const bool updateDistanceRates = distanceBased && (deltaMsec != 0.0 || pendingAnimationUpdateRateChange);
    const uint8_t fixedInterval = animationUpdateMode == AnimationUpdateMode::Disabled ? 1 : animationUpdateFixedInterval;

    BE1::Vec3 cameraOrigin = BE1::Vec3::zero;
    float enterDistanceSqr[AnimationUpdateDistanceThresholdCount];
    float exitDistanceSqr[AnimationUpdateDistanceThresholdCount];

    if (updateDistanceRates) {
        cameraOrigin = app.GetCameraOrigin();

        // 경계 부근에서 갱신 주기가 반복 전환되지 않도록 진입·이탈 거리에 각각 10% 의 여유를 둔다.
        for (int thresholdIndex = 0; thresholdIndex < AnimationUpdateDistanceThresholdCount; ++thresholdIndex) {
            const float distance = BE1::MeterToUnit(animationUpdateDistanceThresholds[thresholdIndex]);
            const float distanceSqr = distance * distance;
            enterDistanceSqr[thresholdIndex] = distanceSqr * (0.9f * 0.9f);
            exitDistanceSqr[thresholdIndex] = distanceSqr * (1.1f * 1.1f);
        }
    }

    const bool forceAllPoses = pendingAnimationUpdateRateChange && animationUpdateMode == AnimationUpdateMode::Disabled;
    pendingAnimationUpdateRateChange = false;
    animationUpdateStats = {};
    bool allPosesUpdated = true;

    for (int objectIndex = 0; objectIndex < gameObjects.Count(); ++objectIndex) {
        SkinnedMeshObject &object = gameObjects[objectIndex];

        // 포즈 평가를 생략해도 캐릭터별 재생 시간은 계속 진행한다.
        const double animationLength = animationLengths[object.animIndex];
        object.animationTimeMsec += deltaMsec * static_cast<double>(object.animationSpeed);
        if (animationLength > 0.0) {
            object.animationTimeMsec = BE1::Mod(object.animationTimeMsec, animationLength);
        } else {
            object.animationTimeMsec = 0.0;
        }

        const uint8_t previousInterval = object.updateInterval;
        uint8_t interval = distanceBased ? previousInterval : fixedInterval;

        if (updateDistanceRates) {
            const float distanceSqr = (object.gameObject.worldMatrix.ToTranslationVec3() - cameraOrigin).LengthSqr();
            interval = 1;
            for (int thresholdIndex = 0; thresholdIndex < AnimationUpdateDistanceThresholdCount; ++thresholdIndex) {
                // 현재 간격이 경계의 가까운 쪽이면 이탈 거리, 먼 쪽이면 진입 거리를 사용한다.
                const float thresholdSqr = previousInterval <= interval ? exitDistanceSqr[thresholdIndex] : enterDistanceSqr[thresholdIndex];
                if (distanceSqr < thresholdSqr) {
                    break;
                }
                interval = static_cast<uint8_t>(interval << 1);
            }
        }
        object.updateInterval = interval;

        ++animationUpdateStats.intervalCounts[CountTrailingZeros(static_cast<uint32_t>(interval))];

        // 고정된 객체 인덱스로 평가 시점을 분산하므로 큐나 캐릭터별 평가 시점 저장 공간이 필요하지 않다.
        const bool scheduled = deltaMsec != 0.0 && ((animationUpdateFrameIndex + static_cast<uint64_t>(objectIndex)) & (interval - 1u)) == 0;
        const bool forced = object.poseUpdatePending || forceAllPoses || interval < previousInterval;
        if (!forced && !scheduled) {
            ++animationUpdateStats.skipped;
            continue;
        }

        // 씬의 임시 버퍼를 재사용하기 전에 두 파트가 공유할 평가 결과를 한 번 복사한다.
        // 실패한 캐릭터만 재시도 대상으로 남기고, 성공한 캐릭터의 재평가는 강제하지 않는다.
        if (animationLength < 0.0 || !EvaluateAnimationPose(object.animIndex, static_cast<int>(object.animationTimeMsec)) || !object.skinningPose->SetJointMats(jointMatrices.Ptr(), jointMatrices.Count())) {
            object.poseUpdatePending = true;
            allPosesUpdated = false;
            ++animationUpdateStats.failed;
            continue;
        }

        object.poseUpdatePending = false;
        ++animationUpdateStats.evaluated;
        animationUpdateStats.forced += forced ? 1 : 0;

        // 포즈를 유지하는 동안에는 해당 바운드도 유지하고, 새 포즈를 적용할 때만 두 메시 파트를 갱신한다.
        const AnimState &animState = animStates[object.animIndex];
        for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
            if (renderWorld && object.renderObjectHandles[meshIndex] >= 0) {
                renderWorld->UpdateRenderObjectBounds(object.renderObjectHandles[meshIndex], animState.aabbs[meshIndex]);
            }
        }
    }

    return allPosesUpdated;
}

void SceneSkinnedMesh::Update(uint64_t elapsedUsec) {
    PROFILER_CPU_SCOPED_EVENT("SceneSkinnedMesh::Update");

#ifdef ENABLE_IMGUI
    if (pendingObjectCountChange) {
        pendingObjectCountChange = false;
        ChangeGameObjectCount(imguiObjectCountX, imguiObjectCountY);
    }
#endif

    const uint64_t deltaUsec = previousUpdateUsec > 0 && elapsedUsec >= previousUpdateUsec ? elapsedUsec - previousUpdateUsec : 0;
    previousUpdateUsec = elapsedUsec;

    animationUpdateStats.evaluated = 0;
    animationUpdateStats.skipped = gameObjects.Count();
    animationUpdateStats.forced = 0;
    animationUpdateStats.failed = 0;
#ifdef ENABLE_IMGUI
    animationUpdateUsec = 0;
#endif

    if (gameObjects.IsEmpty()) {
        return;
    }

    const double deltaMsec = playing ? static_cast<double>(deltaUsec) * 0.001 * static_cast<double>(playbackSpeed) : 0.0;
    if (pendingAnimationPoseUpdate || pendingAnimationUpdateRateChange || deltaMsec != 0.0) {
        pendingAnimationPoseUpdate = !UpdateAnimationPoses(deltaMsec);
    }
    ++animationUpdateFrameIndex;

    if (showBounds) {
        DrawBounds();
    }
}

void SceneSkinnedMesh::DrawBounds() const {
    renderWorld->SetDebugColor(BE1::Color4::lime, BE1::Color4::zero);

    // 애니메이션이 정지된 상태에서도 표시할 수 있도록 현재 컬링 바운드를 재사용한다.
    for (const SkinnedMeshObject &object : gameObjects) {
        for (int renderObjectHandle : object.renderObjectHandles) {
            if (renderObjectHandle < 0) {
                continue;
            }

            const RenderObject *renderObject = renderWorld->GetRenderObject(renderObjectHandle);
            if (renderObject) {
                renderWorld->DrawDebugAABB(renderObject->GetWorldAABB());
            }
        }
    }

    renderWorld->SetDebugColor(BE1::Color4::zero, BE1::Color4::zero);
}

void SceneSkinnedMesh::UpdateDirectionalLightDirection() {
    // UI는 태양이나 달을 향하는 방향을 표시하고, axis[0]은 빛이 진행하는 방향을 나타낸다.
    directionalLight.axis = BE1::Angles(0.0f, directionalLightElevation, directionalLightAzimuth + 180.0f).ToMat3();
}

#ifdef ENABLE_IMGUI
void SceneSkinnedMesh::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 17.0f;

    ImGui::Text("Skinned Mesh");

    if (gameObjects.IsEmpty()) {
        ImGui::TextDisabled("animation is unavailable.");
        return;
    }

    for (const SharedPtr<Anim> &anim : anims) {
        if (!anim || !anim->IsReady()) {
            ImGui::TextDisabled("animation is unavailable.");
            return;
        }
    }

    const int numObjects = gameObjects.Count();
    const int numJoints = jointPoses.Count();

    ImGui::Text("Joints: %i, Triangles: %i", numJoints, static_cast<unsigned long long>(imguiTrianglesPerObject));
    ImGui::Text("Objects (%i)", numObjects);

    ImGui::Spacing();

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderInt("Count X##Objects", &imguiObjectCountX, 1, MaxObjectCountPerAxis, "%d", ImGuiSliderFlags_AlwaysClamp);
    const bool applyObjectCountX = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderInt("Count Y##Objects", &imguiObjectCountY, 1, MaxObjectCountPerAxis, "%d", ImGuiSliderFlags_AlwaysClamp);
    const bool applyObjectCountY = ImGui::IsItemDeactivatedAfterEdit();

    if ((applyObjectCountX || applyObjectCountY) &&
        (imguiObjectCountX != objectCountX || imguiObjectCountY != objectCountY)) {
        pendingObjectCountChange = true;
    }

    ImGui::Spacing();

    if (ImGui::Button(playing ? "Stop Anim###AnimationPlayback" : "Play Anim###AnimationPlayback")) {
        playing = !playing;
    }

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderFloat("Playback Speed", &playbackSpeed, 0.0f, 2.0f, "%.2fx");

    ImGui::Checkbox("Show Bounds", &showBounds);

    ImGui::Separator();
    ImGui::TextUnformatted("Animation Update Rate");

    int updateMode = static_cast<int>(animationUpdateMode);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::Combo("Mode##AnimationUpdate", &updateMode, "Disabled\0Fixed interval\0Distance based\0")) {
        animationUpdateMode = static_cast<AnimationUpdateMode>(updateMode);
        pendingAnimationUpdateRateChange = true;
    }

    if (animationUpdateMode == AnimationUpdateMode::FixedInterval) {
        int intervalIndex = CountTrailingZeros(static_cast<uint32_t>(animationUpdateFixedInterval));
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::Combo("Interval##AnimationUpdate", &intervalIndex, "Every frame\0Every 2 frames\0Every 4 frames\0Every 8 frames\0")) {
            animationUpdateFixedInterval = static_cast<uint8_t>(1u << intervalIndex);
            pendingAnimationUpdateRateChange = true;
        }
    } else if (animationUpdateMode == AnimationUpdateMode::DistanceBased) {
        static constexpr const char *distanceLabels[AnimationUpdateDistanceThresholdCount] = {
            "1 -> 2 distance##AnimationUpdate",
            "2 -> 4 distance##AnimationUpdate",
            "4 -> 8 distance##AnimationUpdate"
        };
        for (int thresholdIndex = 0; thresholdIndex < AnimationUpdateDistanceThresholdCount; ++thresholdIndex) {
            // 인접한 경계와 최소 0.1m 간격을 두어 거리 설정을 오름차순으로 유지한다.
            const float minDistance = thresholdIndex > 0 ? animationUpdateDistanceThresholds[thresholdIndex - 1] + 0.1f : 0.1f;
            const float maxDistance = thresholdIndex + 1 < AnimationUpdateDistanceThresholdCount ? animationUpdateDistanceThresholds[thresholdIndex + 1] - 0.1f : 2000.0f;
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::DragFloat(distanceLabels[thresholdIndex], &animationUpdateDistanceThresholds[thresholdIndex], 0.5f, minDistance, maxDistance, "%.1f m", ImGuiSliderFlags_AlwaysClamp)) {
                pendingAnimationUpdateRateChange = true;
            }
        }
        ImGui::TextDisabled("Distance tiers: every 1 / 2 / 4 / 8 frames");
        ImGui::TextDisabled("Distance hysteresis: +/-10%%");
    }

    ImGui::Text("Evaluated: %i  Skipped: %i", animationUpdateStats.evaluated, animationUpdateStats.skipped);
    ImGui::Text("Forced: %i  Failed: %i", animationUpdateStats.forced, animationUpdateStats.failed);
    ImGui::Text("Every 1 / 2 / 4 / 8: %i / %i / %i / %i", animationUpdateStats.intervalCounts[0], animationUpdateStats.intervalCounts[1], animationUpdateStats.intervalCounts[2], animationUpdateStats.intervalCounts[3]);
    ImGui::Text("Animation update CPU: %.3f ms", static_cast<double>(animationUpdateUsec) * 0.001);

    ImGui::Separator();
    ImGui::Text("Directional Light");

    if (ImGui::Checkbox("Enabled##DirectionalLight", &directionalLightEnabled)) {
        directionalLightEnabled = directionalLight.SetEnabled(renderWorld, directionalLightEnabled);
    }

    ImGui::BeginDisabled(!directionalLightEnabled);
    ImGui::TextDisabled("Azimuth: 0 = +X, 90 = +Y; Elevation: 0 = horizon");

    bool directionalLightLightingChanged = false;
    bool directionalLightDirectionChanged = false;

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Azimuth##DirectionalLight", &directionalLightAzimuth, 0.0f, 360.0f, "%.1f deg")) {
        directionalLightDirectionChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Elevation##DirectionalLight", &directionalLightElevation, -90.0f, 90.0f, "%.1f deg")) {
        directionalLightDirectionChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::ColorEdit3("Color##DirectionalLight", directionalLight.linearColor.Ptr(), ImGuiColorEditFlags_Float)) {
        directionalLightLightingChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("Illuminance##DirectionalLight", &directionalLight.intensity, BE1::Max(directionalLight.intensity * 0.01f, 0.01f), 0.0f, 200000.0f, "%.3f lux", ImGuiSliderFlags_AlwaysClamp)) {
        directionalLightLightingChanged = true;
    }

    if (directionalLightDirectionChanged) {
        UpdateDirectionalLightDirection();
        renderWorld->UpdateRenderLightTransform(directionalLight.renderLightHandle, directionalLight.origin, directionalLight.axis);
    }

    if (directionalLightLightingChanged) {
        renderWorld->UpdateRenderLightLighting(directionalLight.renderLightHandle, directionalLight.linearColor, directionalLight.intensity);
    }

    ImGui::EndDisabled();

    if (const EnvironmentLight *envLight = renderWorld ? renderWorld->GetEnvironmentLight() : nullptr) {
        const EnvironmentLight::Desc &currentDesc = envLight->GetDesc();
        bool enabled = currentDesc.enabled;
        float intensity = currentDesc.intensity;
        BE1::Color3 linearColor = currentDesc.linearColor;
        const float envLightItemWidth = ImGui::GetFontSize() * 17.0f;

        ImGui::Separator();
        ImGui::TextUnformatted("Environment Light");
        ImGui::PushID("EnvironmentLight");

        const bool enabledChanged = ImGui::Checkbox("Enabled", &enabled);
        ImGui::BeginDisabled(!enabled);

        ImGui::SetNextItemWidth(envLightItemWidth);
        bool lightingChanged = ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::SetNextItemWidth(envLightItemWidth);
        lightingChanged |= ImGui::ColorEdit3("Color", linearColor.Ptr(), ImGuiColorEditFlags_Float);

        ImGui::SetNextItemWidth(envLightItemWidth);
        const bool rotationChanged = ImGui::SliderFloat("Rotation", &envLightRotation, 0.0f, 360.0f, "%.1f deg", ImGuiSliderFlags_AlwaysClamp);

        ImGui::EndDisabled();
        ImGui::PopID();

        if (enabledChanged || lightingChanged || rotationChanged) {
            EnvironmentLight::Desc desc = currentDesc;
            desc.enabled = enabled;
            desc.intensity = intensity;
            desc.linearColor = linearColor;
            if (rotationChanged) {
                desc.axis = BE1::Mat3::FromRotationZ(DEG2RAD(envLightRotation));
                renderWorld->SetSkyboxRotation(desc.axis);
            }
            renderWorld->UpdateEnvironmentLight(desc);
            if (lightingChanged) {
                renderWorld->SetSkyboxLighting(desc.linearColor, desc.intensity);
            }
        }
    }

    if (const HeightFog *fog = renderWorld ? renderWorld->GetHeightFog() : nullptr) {
        HeightFog::Desc desc = fog->GetDesc();
        ImGui::Separator();
        ImGui::TextUnformatted("Height Fog");
        ImGui::PushID("HeightFog");
        ImGui::PushItemWidth(ImGui::GetFontSize() * 17.0f);

        bool changed = ImGui::Checkbox("Enabled", &desc.enabled);
        ImGui::BeginDisabled(!desc.enabled);
        changed |= ImGui::DragFloat("Base height", &desc.baseHeight, 0.1f, 0.0f, 0.0f, "%.2f m");
        ImGui::SetItemTooltip("World Z height at which base extinction and base emission are defined.");
        changed |= ImGui::SliderFloat("Height falloff", &desc.heightFalloff, 0.0f, 10.0f, "%.3f /m", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Density halves for every 1 / falloff meters above the base height and increases below it. Zero gives a uniform medium.");
        changed |= ImGui::SliderFloat("Base extinction", &desc.medium.baseExtinction, 0.0f, 1000.0f, "%.4f /m", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Extinction at the base height, before height falloff.");
        changed |= ImGui::DragFloat("Max distance", &desc.maxDistance, 10.0f, 0.01f, 1000000.0f, "%.1f m", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Maximum fog ray distance for surfaces and sky. Independent of the froxel far depth.");
        changed |= ImGui::ColorEdit3("Scattering albedo", &desc.medium.scatteringAlbedo.x, ImGuiColorEditFlags_Float);
        if (ImGui::ColorEdit3("Base emission", &desc.medium.baseEmission.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)) {
            desc.medium.baseEmission.x = BE1::Max(desc.medium.baseEmission.x, 0.0f);
            desc.medium.baseEmission.y = BE1::Max(desc.medium.baseEmission.y, 0.0f);
            desc.medium.baseEmission.z = BE1::Max(desc.medium.baseEmission.z, 0.0f);
            changed = true;
        }
        ImGui::SetItemTooltip("RGB radiance emitted per meter at the base height. Scales with height falloff and is independent of scattering albedo.");
        changed |= ImGui::ColorEdit3("Ambient color", desc.ambientColor.Ptr(), ImGuiColorEditFlags_Float);
        changed |= ImGui::DragFloat("Ambient intensity", &desc.ambientIntensity, 0.05f, 0.0f, 32.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Uniform ambient incident radiance for height fog. Scattering follows extinction, height falloff, and albedo.");
        ImGui::EndDisabled();

        if (changed) {
            renderWorld->UpdateHeightFog(desc);
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
    }
}
#endif
