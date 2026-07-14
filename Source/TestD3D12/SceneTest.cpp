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
#include "Render/RenderWorld.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "SceneTest.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

void SceneTest::Init(RenderWorld *renderWorld) {
    assert(!this->renderWorld);
    assert(renderWorld);

    this->renderWorld = renderWorld;

    renderWorld->SetSkyboxMaterial(materialManager.GetMaterial("Source/TestD3D12/Materials/skybox.material"));

    InitGameObjects(32, 32, 10);
    InitGameLights(16, 16, 4);
}

void SceneTest::Shutdown() {
    if (!renderWorld) {
        return;
    }

    ClearGameObjects();
    ClearGameLights();

    renderWorld = nullptr;
}

void SceneTest::Update(uint64_t elapsedUsec) {
    PROFILER_CPU_SCOPED_EVENT("SceneTest::Update");

#ifdef ENABLE_IMGUI
    if (pendingObjectCountChange) {
        pendingObjectCountChange = false;
        ChangeGameObjectCount(static_cast<uint32_t>(imguiObjectCountX), static_cast<uint32_t>(imguiObjectCountY), static_cast<uint32_t>(imguiObjectCountZ));
    }

    if (pendingLightCountChange) {
        pendingLightCountChange = false;
        ChangeGameLightCount(static_cast<uint32_t>(imguiLightCountX), static_cast<uint32_t>(imguiLightCountY), static_cast<uint32_t>(imguiLightCountZ));
    }
#endif

    AnimateGameObjects(elapsedUsec);
    AnimateGameLights(elapsedUsec);
}

void SceneTest::InitGameObjects(uint32_t countX, uint32_t countY, uint32_t countZ) {
    objectCountX = countX;
    objectCountY = countY;
    objectCountZ = countZ;

#ifdef ENABLE_IMGUI
    imguiObjectCountX = static_cast<int>(countX);
    imguiObjectCountY = static_cast<int>(countY);
    imguiObjectCountZ = static_cast<int>(countZ);
#endif

    const uint32_t objectCount = objectCountX * objectCountY * objectCountZ;
    gameObjects.Reserve(objectCount);

    constexpr float spacing = 4.0f;
    const float startX = -spacing * (objectCountX - 1) * 0.5f;
    const float startY = -spacing * (objectCountY - 1) * 0.5f;
    const float startZ = -spacing * (objectCountZ - 1) * 0.5f;

    testMesh = meshManager.GetMesh("_defaultBoxMesh");

    static constexpr const char *TextureFilenames[] = {
        "Data/Test/Brick_Wall_Worn.dds",
        "Data/Test/Construction_Gravel.dds",
        "Data/Test/Dry_Cracked_Mud.dds",
        "Data/Test/Flagstone_Wall.dds",
        "Data/Test/Lawn_Grass.dds",
        "Data/Test/Roman_Stone_Floor.dds",
        "Data/Test/Stone_Floor.dds",
        "Data/Test/Stone_Tiles_Facade.dds",
        "Data/Test/Arid_Gravel_Ground.dds",
        "Data/Test/yd0kejgga.dds",
        "Data/Test/T_sc2nbisc.dds",
        "Data/Test/Gouged_Rock_Cliff.dds",
        "Data/Test/Scratched_Polypropylene_Plastic.dds",
        "Data/Test/Rippled_Sand.dds",
        "Data/Test/Dirty_Sidewalk_Tiles.dds",
        "Data/Test/Curated_Stone_Facade.dds",
    };

    for (uint32_t z = 0; z < objectCountZ; ++z) {
        for (uint32_t y = 0; y < objectCountY; ++y) {
            for (uint32_t x = 0; x < objectCountX; ++x) {
                GameObject *gameObject = new GameObject;
                gameObjects.Append(gameObject);

                RenderObject::Desc &desc = gameObject->renderObjectDesc;
                desc.worldMatrix.SetTranslation(BE1::Vec3(startX + spacing * x, startY + spacing * y, startZ + spacing * z));
                desc.meshInstance = testMesh->GetStaticInstance();
                desc.aabb = testMesh->GetAABB();
                desc.materials.SetCount(1);
                desc.materials[0] = materialManager.GetSingleTextureMaterial(TextureFilenames[BE1::Math::RandomRange(0, COUNT_OF(TextureFilenames) - 1)]);

                gameObject->renderObjectHandle = renderWorld->AddRenderObject(desc);
            }
        }
    }

    // Text 오브젝트
    /*GameObject *gameObject = new GameObject;
    gameObjects.Append(gameObject);

    RenderObject::Desc &desc = gameObject->renderObjectDesc;
    desc.worldMatrix.SetTRS(BE1::Vec3(0, 0, 10), BE1::Mat3::identity, BE1::Vec3::one);
    RenderObject::TextDesc &textDesc = desc.GetTextDesc();
    textDesc.font = mainFont;
    textDesc.text = "Hello World";
    textDesc.textParams.bits.textAnchor = (uint16_t)RenderObject::TextAnchor::MiddleCenter;

    gameObject->renderObjectHandle = renderWorld->AddRenderObject(gameObject->renderObjectDesc);*/

}

void SceneTest::InitGameLights(uint32_t countX, uint32_t countY, uint32_t countZ) {
    lightCountX = countX;
    lightCountY = countY;
    lightCountZ = countZ;

#ifdef ENABLE_IMGUI
    imguiLightCountX = static_cast<int>(countX);
    imguiLightCountY = static_cast<int>(countY);
    imguiLightCountZ = static_cast<int>(countZ);
#endif

    const uint32_t lightCount = lightCountX * lightCountY * lightCountZ;
    gameLights.Reserve(lightCount);

    constexpr float spacing = 16.0f;
    constexpr float radius = 12.0f;
    constexpr float jitterRange = spacing * 0.35f;
    constexpr float minColorTemperature = 2200.0f;
    constexpr float maxColorTemperature = 8000.0f;
    constexpr float miredScale = 1000000.0f;
    constexpr float minMired = miredScale / maxColorTemperature;
    constexpr float maxMired = miredScale / minColorTemperature;

    const float startX = -spacing * (lightCountX - 1) * 0.5f;
    const float startY = -spacing * (lightCountY - 1) * 0.5f;
    const float startZ = -spacing * (lightCountZ - 1) * 0.5f;

    BE1::RNG rng(1);

    for (uint32_t z = 0; z < lightCountZ; ++z) {
        for (uint32_t y = 0; y < lightCountY; ++y) {
            for (uint32_t x = 0; x < lightCountX; ++x) {
                GameLight &gameLight = gameLights.Alloc();
                RenderLight::Desc &desc = gameLight.renderLightDesc;
                desc.type = RenderLight::Type::Point;
                desc.flags = RenderLight::Flag::None;
                desc.origin.Set(
                    startX + spacing * x + rng.GetNextFloat(-jitterRange, jitterRange),
                    startY + spacing * y + rng.GetNextFloat(-jitterRange, jitterRange),
                    startZ + spacing * z + rng.GetNextFloat(-jitterRange, jitterRange));
                gameLight.baseOriginZ = desc.origin.z;
                desc.size.Set(radius, radius, radius);
                desc.projectionPolicy = RHI::renderer->GetProjectionPolicy();

                // Reciprocal color-temperature sampling distributes the visible warm/cool variation more evenly.
                const float mired = rng.GetNextFloat(minMired, maxMired);
                const float colorTemperature = miredScale / mired;
                const BE1::Color3 color = BE1::Color3::FromColorTemperature(colorTemperature);
                const float colorScale = BE1::Max3(color.r, color.g, color.b);
                const float baseIntensity = rng.GetNextFloat(1.0f, 5.0f);
                assert(colorScale > 0.0f);

                desc.linearColor = color / colorScale;
                desc.intensity = baseIntensity * colorScale;

                gameLight.renderLightHandle = renderWorld->AddRenderLight(desc);
            }
        }
    }
}

void SceneTest::ClearGameObjects() {
    renderWorld->ClearScene(RenderWorld::ClearFlag::RenderObjects);

    gameObjects.DeleteContents(true);
    testMesh.reset();
}

void SceneTest::ClearGameLights() {
    renderWorld->ClearScene(RenderWorld::ClearFlag::RenderLights);

    gameLights.Clear();
}

void SceneTest::ChangeGameObjectCount(uint32_t countX, uint32_t countY, uint32_t countZ) {
    ClearGameObjects();

    InitGameObjects(countX, countY, countZ);
}

void SceneTest::ChangeGameLightCount(uint32_t countX, uint32_t countY, uint32_t countZ) {
    ClearGameLights();

    InitGameLights(countX, countY, countZ);
}

void SceneTest::AnimateGameObjects(uint64_t elapsedUsec) {
    const float elapsedSeconds = (float)MICRO2SEC((double)elapsedUsec);

    const float zRotationSpeed = 1.25f;

    for (uint32_t z = 0; z < objectCountZ; ++z) {
        for (uint32_t y = 0; y < objectCountY; ++y) {
            for (uint32_t x = 0; x < objectCountX; ++x) {
                const uint32_t index = objectCountX * objectCountY * z + objectCountX * y + x;
                const float t = elapsedSeconds + index * 0.1f;

                GameObject *gameObject = gameObjects[index];
                gameObject->renderObjectDesc.worldMatrix.SetRotation(BE1::Mat3::FromRotationZ(t * zRotationSpeed), false);

                renderWorld->UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDesc, RenderObject::Desc::UpdateFlag::Transform);
            }
        }
    }
}

void SceneTest::AnimateGameLights(uint64_t elapsedUsec) {
    const float elapsedSeconds = (float)MICRO2SEC((double)elapsedUsec);

    constexpr float movementAmplitude = 4.0f;
    constexpr float movementSpeed = 3.0f;
    constexpr float phaseStep = 0.37f;

    for (int index = 0; index < gameLights.Count(); ++index) {
        const float phase = index * phaseStep;

        GameLight &gameLight = gameLights[index];
        RenderLight::Desc &desc = gameLight.renderLightDesc;

        desc.origin.z = gameLight.baseOriginZ + BE1::Math::Sin(elapsedSeconds * movementSpeed + phase) * movementAmplitude;

        renderWorld->UpdateRenderLight(gameLight.renderLightHandle, desc, RenderLight::Desc::UpdateFlag::Transform);
    }
}

#ifdef ENABLE_IMGUI
void SceneTest::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 10.0f;

    ImGui::Text("Objects (%i)", gameObjects.Count());

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderInt("Count X##Objects", &imguiObjectCountX, 1, 50);
    const bool applyObjectCountX = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderInt("Count Y##Objects", &imguiObjectCountY, 1, 50);
    const bool applyObjectCountY = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderInt("Count Z##Objects", &imguiObjectCountZ, 1, 50);
    const bool applyObjectCountZ = ImGui::IsItemDeactivatedAfterEdit();

    if ((applyObjectCountX || applyObjectCountY || applyObjectCountZ) &&
        (static_cast<uint32_t>(imguiObjectCountX) != objectCountX || static_cast<uint32_t>(imguiObjectCountY) != objectCountY || static_cast<uint32_t>(imguiObjectCountZ) != objectCountZ)) {
        pendingObjectCountChange = true;
    }

    ImGui::Spacing();
    ImGui::Text("Lights (%i)", gameLights.Count());

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderInt("Count X##Lights", &imguiLightCountX, 1, 50);
    const bool applyLightCountX = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderInt("Count Y##Lights", &imguiLightCountY, 1, 50);
    const bool applyLightCountY = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderInt("Count Z##Lights", &imguiLightCountZ, 1, 50);
    const bool applyLightCountZ = ImGui::IsItemDeactivatedAfterEdit();

    if ((applyLightCountX || applyLightCountY || applyLightCountZ) &&
        (static_cast<uint32_t>(imguiLightCountX) != lightCountX || static_cast<uint32_t>(imguiLightCountY) != lightCountY || static_cast<uint32_t>(imguiLightCountZ) != lightCountZ)) {
        pendingLightCountChange = true;
    }
}
#endif
