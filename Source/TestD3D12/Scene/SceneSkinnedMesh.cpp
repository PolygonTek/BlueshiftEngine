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
#include "App.h"
#include "Render/Animation/AnimManager.h"
#include "Render/Animation/SkeletonManager.h"
#include "Render/Frontend/EnvironmentLight.h"
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

    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);

    app.SetCameraPitchYaw(30.0f, 0.0f);
    app.SetCameraDistance(104.0f);

    directionalLight.type = RenderLight::Type::Directional;
    directionalLight.flags = RenderLight::Flag::CastShadows;
    directionalLight.linearColor = BE1::Color3::white;
    directionalLight.intensity = 2.0f;
    directionalLightEnabled = true;
    UpdateDirectionalLightDirection();
    directionalLightEnabled = directionalLight.SetEnabled(renderWorld, directionalLightEnabled);

    previousUpdateUsec = 0;
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

    // Keep the ground fixed at the lowest sampled foot height across all animation clips.
    float minCharacterZ = characterAABB[0].z;
    for (const AnimState &animState : animStates) {
        for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
            for (const BE1::AABB &frameAABB : animState.frameAABBs[meshIndex]) {
                minCharacterZ = BE1::Min(minCharacterZ, frameAABB[0].z);
            }
        }
    }
    groundHeight = characterOffset.z + minCharacterZ;

    // Retain the shared two-triangle quad instance across object-count changes.
    groundMeshInstance = meshManager.defaultQuadMesh->GetStaticInstance();
    if (!groundMeshInstance) {
        BE_WARNLOG("SceneSkinnedMesh::Init: failed to instantiate the ground plane");
        return;
    }

    if (!InitGameObjects(32, 32)) {
        return;
    }

    EnvironmentLight::Desc envLightDesc;
    envLightAdded = renderWorld->AddEnvironmentLight(envLightDesc);
    if (envLightAdded) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }

}

void SceneSkinnedMesh::Shutdown() {
    if (renderWorld) {
        renderWorld->ClearScene();
    }
    directionalLight.renderLightHandle = -1;
    envLightAdded = false;
    gameObjects.Clear();
    groundMeshInstance.reset();

    for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
        meshes[meshIndex].reset();
    }

    for (int animIndex = 0; animIndex < AnimCount; ++animIndex) {
        anims[animIndex].reset();
        AnimState &animState = animStates[animIndex];
        animState.jointMatrices.Clear();
        for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
            animState.frameAABBs[meshIndex].Clear();
            animState.aabbs[meshIndex] = BE1::AABB::empty;
        }
    }

    skeleton.reset();

    jointIndexes.Clear();
    jointPoses.Clear();

    previousUpdateUsec = 0;
#ifdef ENABLE_IMGUI
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
        if (!meshes[meshIndex] || !meshes[meshIndex]->IsReady() || meshes[meshIndex] == meshManager.defaultMesh ||
            !meshes[meshIndex]->IsCompatibleSkeleton(skeleton.get())) {
            BE_WARNLOG("SceneSkinnedMesh::LoadCharacter: YBot mesh, skeleton, and animation hierarchies do not match");
            return false;
        }
    }

#ifdef ENABLE_IMGUI
    // Cache both mesh parts once; object-count changes only affect the displayed totals.
    imguiTrianglesPerObject = 0;
    for (const SharedPtr<Mesh> &mesh : meshes) {
        const int numSurfaces = mesh->NumSurfaces();
        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; ++surfaceIndex) {
            imguiTrianglesPerObject += mesh->GetSurface(surfaceIndex)->subMesh->NumIndexes() / 3;
        }
    }
#endif

    const int numJoints = skeleton->NumJoints();
    jointIndexes.SetCount(numJoints);
    jointPoses.SetCount(numJoints);

    for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex) {
        jointIndexes[jointIndex] = jointIndex;
    }

    // Animated bounds are computed once at scene initialization and sampled without per-frame allocation.
    for (int animIndex = 0; animIndex < AnimCount; ++animIndex) {
        AnimState &animState = animStates[animIndex];
        animState.jointMatrices.SetCount(numJoints);
        for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
            anims[animIndex]->ComputeFrameAABBs(skeleton.get(), meshes[meshIndex].get(), animState.frameAABBs[meshIndex]);
        }
    }

    return EvaluateAnimationPose(0, 0);
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

    const float startX = -ObjectSpacing * (objectCountX - 1) * 0.5f;
    const float startY = -ObjectSpacing * (objectCountY - 1) * 0.5f;
    const BE1::Mat3 characterRotation = BE1::Mat3::FromRotationZ(DEG2RAD(180.0f));

    RenderObject::Desc desc;
    desc.flags = RenderObject::Flag::CastShadows;
    desc.skeleton = skeleton;
    desc.materials.SetCount(1);
    desc.materials[0] = materialManager.whiteLitMaterial;

    bool evaluatedAnims[AnimCount] = {};
    for (int y = 0; y < objectCountY; ++y) {
        for (int x = 0; x < objectCountX; ++x) {
            SkinnedMeshObject &object = gameObjects[y * objectCountX + x];

            // Choose the clip and speed once per character so both mesh parts always share a pose.
            object.animIndex = BE1::RNG::GetRandom(0, AnimCount - 1);
            object.animationSpeed = BE1::RNG::GetRandom(0.5f, 1.5f);
            object.animationTimeMsec = 0.0;

            // All new characters start at time zero, so their initial poses can still be shared per clip.
            if (!evaluatedAnims[object.animIndex]) {
                if (!EvaluateAnimationPose(object.animIndex, 0)) {
                    ClearGameObjects();
                    return false;
                }
                evaluatedAnims[object.animIndex] = true;
            }
            const AnimState &animState = animStates[object.animIndex];

            const BE1::Vec3 position = characterOffset + BE1::Vec3(startX + ObjectSpacing * x, startY + ObjectSpacing * y, 0.0f);
            object.gameObject.worldMatrix.SetTranslationRotation(position, characterRotation, false);
            desc.worldMatrix = object.gameObject.worldMatrix;

            for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
                object.meshInstances[meshIndex] = meshes[meshIndex]->InstantiateSkinned();
                if (!object.meshInstances[meshIndex] ||
                    !object.meshInstances[meshIndex]->SetJointMats(animState.jointMatrices.Ptr(), animState.jointMatrices.Count())) {
                    BE_WARNLOG("SceneSkinnedMesh::InitGameObjects: failed to instantiate YBot mesh %i", meshIndex);
                    ClearGameObjects();
                    return false;
                }

                desc.aabb = animState.aabbs[meshIndex];
                desc.meshInstance = object.meshInstances[meshIndex];
                object.renderObjectHandles[meshIndex] = renderWorld->AddRenderObject(desc);
            }
        }
    }

    InitGround();
    return true;
}

void SceneSkinnedMesh::InitGround() {
    // Extend one object spacing beyond the outermost character centers on each side.
    const float sizeX = ObjectSpacing * (objectCountX + 1);
    const float sizeY = ObjectSpacing * (objectCountY + 1);
    const BE1::AABB &quadAABB = groundMeshInstance->GetAABB();
    const BE1::Vec3 quadSize = quadAABB[1] - quadAABB[0];

    RenderObject::Desc desc;
    desc.flags = RenderObject::Flag::ReceiveShadows;
    // The engine quad lies in YZ; rotate it back onto XY with its normal pointing up.
    desc.worldMatrix.SetTRS(BE1::Vec3(characterOffset.x, characterOffset.y, groundHeight),
        BE1::Angles(0.0f, -90.0f, 0.0f).ToMat3(), BE1::Vec3(1.0f, sizeY / quadSize.y, sizeX / quadSize.z));
    desc.meshInstance = groundMeshInstance;
    desc.aabb = quadAABB;
    desc.materials.SetCount(1);
    desc.materials[0] = materialManager.whiteLitMaterial;
    renderWorld->AddRenderObject(desc);
}

void SceneSkinnedMesh::ClearGameObjects() {
    renderWorld->ClearScene(RenderWorld::ClearFlag::RenderObjects);
    gameObjects.Clear();
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
    if (numJoints <= 0 || jointIndexes.Count() != numJoints || jointPoses.Count() != numJoints || animState.jointMatrices.Count() != numJoints) {
        return false;
    }

    Anim::FrameInterpolation frameInterpolation;
    anim->TimeToFrameInterpolation(animationTimeMsec, frameInterpolation);
    anim->GetInterpolatedFrame(frameInterpolation, numJoints, jointIndexes.Ptr(), jointPoses.Ptr());

    BE1::simdProcessor->ConvertJointPosesToJointMats(animState.jointMatrices.Ptr(), jointPoses.Ptr(), numJoints);
    BE1::simdProcessor->TransformJoints(animState.jointMatrices.Ptr(), skeleton->GetJointParentIndexes(), 0, numJoints - 1);

    for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
        anim->GetAABB(animState.aabbs[meshIndex], animState.frameAABBs[meshIndex], animationTimeMsec);
    }

    return true;
}

bool SceneSkinnedMesh::UpdateAnimationPoses(double deltaMsec) {
    double animationLengths[AnimCount];
    for (int animIndex = 0; animIndex < AnimCount; ++animIndex) {
        const SharedPtr<Anim> &anim = anims[animIndex];
        if (!anim || !anim->IsReady()) {
            return false;
        }
        animationLengths[animIndex] = static_cast<double>(anim->Length());
    }

    // Each character has its own playback time. Reuse the clip's pose buffers and apply to both parts before overwriting them.
    for (SkinnedMeshObject &object : gameObjects) {
        const double animationLength = animationLengths[object.animIndex];
        object.animationTimeMsec = animationLength > 0.0 ?
            BE1::Mod(object.animationTimeMsec + deltaMsec * static_cast<double>(object.animationSpeed), animationLength) : 0.0;

        if (!EvaluateAnimationPose(object.animIndex, static_cast<int>(object.animationTimeMsec))) {
            return false;
        }

        const AnimState &animState = animStates[object.animIndex];
        for (int meshIndex = 0; meshIndex < MeshCount; ++meshIndex) {
            if (!object.meshInstances[meshIndex]->SetJointMats(animState.jointMatrices.Ptr(), animState.jointMatrices.Count())) {
                return false;
            }

            if (renderWorld && object.renderObjectHandles[meshIndex] >= 0) {
                renderWorld->UpdateRenderObjectBounds(object.renderObjectHandles[meshIndex], animState.aabbs[meshIndex]);
            }
        }
    }

    return true;
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

    if (gameObjects.IsEmpty()) {
        return;
    }

    if (playing && playbackSpeed != 0.0f && deltaUsec > 0) {
        const double deltaMsec = static_cast<double>(deltaUsec) * 0.001 * static_cast<double>(playbackSpeed);
        UpdateAnimationPoses(deltaMsec);
    }

    if (showBounds) {
        DrawBounds();
    }
}

void SceneSkinnedMesh::DrawBounds() const {
    renderWorld->SetDebugColor(BE1::Color4::lime, BE1::Color4::zero);

    // Reuse the current culling bounds so visualization also works while animation is paused.
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
    // The UI describes the direction toward the sun or moon, while axis[0] is the direction in which light rays travel.
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
    if (ImGui::SliderFloat("Intensity##DirectionalLight", &directionalLight.intensity, 0.0f, 10.0f, "%.3f")) {
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
}
#endif
