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
#include "Render/Frontend/RenderWorld.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "Scene/SceneMatrix.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

static constexpr float lightSpacing = 6.0f;
static constexpr float lightOriginZOffset = 2.0f;

void SceneMatrix::Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) {
    this->renderWorld = renderWorld;
    this->physicsWorld = physicsWorld;

    renderWorld->SetSkyboxMaterial(materialManager.GetMaterial("Source/TestD3D12/Materials/skybox.material"));

    directionalLight.type = RenderLight::Type::Directional;
    directionalLight.flags = RenderLight::Flag::CastShadows;
    directionalLight.linearColor = BE1::Color3::white;
    directionalLight.intensity = 5.0f;
    directionalLightEnabled = true;
    UpdateDirectionalLightDirection();
    directionalLightEnabled = directionalLight.SetEnabled(renderWorld, directionalLightEnabled);

    InitGameObjects(50, 50, 4);
    InitGameLights(32, 32, 1);
}

void SceneMatrix::Shutdown() {
    if (!renderWorld) {
        physicsWorld = nullptr;
        return;
    }

    ClearGameObjects();
    ClearGameLights();
    directionalLightEnabled = directionalLight.SetEnabled(renderWorld, false);

    physicsWorld = nullptr;
    renderWorld = nullptr;
}

void SceneMatrix::Update(uint64_t elapsedUsec) {
    PROFILER_CPU_SCOPED_EVENT("SceneMatrix::Update");

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

    if (animateObjects) {
        AnimateGameObjects(elapsedUsec);
    }
    if (animateLights) {
        AnimateGameLights(elapsedUsec);
    }
}

void SceneMatrix::InitGameObjects(uint32_t countX, uint32_t countY, uint32_t countZ) {
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

    static constexpr const char *textureFilenames[] = {
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

                gameObject->worldMatrix.SetTranslation(BE1::Vec3(startX + spacing * x, startY + spacing * y, startZ + spacing * z));

                RenderObject::Desc desc;
                desc.flags = RenderObject::Flag::CastShadows;
                desc.worldMatrix = gameObject->worldMatrix;
                desc.meshInstance = testMesh->GetStaticInstance();
                desc.aabb = testMesh->GetAABB();
                desc.materials.SetCount(1);
                desc.materials[0] = materialManager.GetSingleTextureMaterial(textureFilenames[BE1::RNG::GetRandom(0, COUNT_OF(textureFilenames) - 1)]);

                gameObject->renderObjectHandle = renderWorld->AddRenderObject(desc);
            }
        }
    }

    // Text 오브젝트
    /*GameObject *gameObject = new GameObject;
    gameObjects.Append(gameObject);

    gameObject->worldMatrix.SetTRS(BE1::Vec3(0, 0, 10), BE1::Mat3::identity, BE1::Vec3::one);

    RenderObject::Desc desc;
    desc.worldMatrix = gameObject->worldMatrix;
    RenderObject::TextDesc &textDesc = desc.GetTextDesc();
    textDesc.font = mainFont;
    textDesc.text = "Hello World";
    textDesc.textParams.bits.textAnchor = (uint16_t)RenderObject::TextAnchor::MiddleCenter;

    gameObject->renderObjectHandle = renderWorld->AddRenderObject(desc);*/

}

void SceneMatrix::InitGameLights(uint32_t countX, uint32_t countY, uint32_t countZ) {
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

    constexpr float radius = 8.0f;
    constexpr float minLightIntensity = 1.0f;
    constexpr float maxLightIntensity = 5.0f;

    const float startX = -lightSpacing * (lightCountX - 1) * 0.5f;
    const float startY = -lightSpacing * (lightCountY - 1) * 0.5f;
    const float startZ = -lightSpacing * (lightCountZ - 1) * 0.5f;

    BE1::RNG rng(1);
    for (uint32_t z = 0; z < lightCountZ; ++z) {
        for (uint32_t y = 0; y < lightCountY; ++y) {
            for (uint32_t x = 0; x < lightCountX; ++x) {
                GameLight &gameLight = gameLights.Alloc();
                gameLight.type = RenderLight::Type::Point;
                gameLight.flags = pointLightShadows ? RenderLight::Flag::CastShadows : RenderLight::Flag::None;
                gameLight.origin.Set(
                    startX + lightSpacing * x,
                    startY + lightSpacing * y,
                    startZ + lightSpacing * z + lightOriginZOffset);
                gameLight.range = radius;

                // 밝기와 색상을 독립적으로 무작위 선택한다.
                gameLight.intensity = rng.GetNextFloat(minLightIntensity, maxLightIntensity);
                const float hue = rng.GetNextFloat();
                gameLight.linearColor = BE1::Color3::FromHSL(hue, 1.0f, 0.5f).SRGBToLinear();

                gameLight.SetEnabled(renderWorld, true);
            }
        }
    }
}

void SceneMatrix::ClearGameObjects() {
    renderWorld->ClearScene(RenderWorld::ClearFlag::RenderObjects);

    gameObjects.DeleteContents(true);
    testMesh.reset();
}

void SceneMatrix::ClearGameLights() {
    for (GameLight &gameLight : gameLights) {
        gameLight.SetEnabled(renderWorld, false);
    }
    gameLights.Clear();
}

void SceneMatrix::ChangeGameObjectCount(uint32_t countX, uint32_t countY, uint32_t countZ) {
    ClearGameObjects();

    InitGameObjects(countX, countY, countZ);
}

void SceneMatrix::ChangeGameLightCount(uint32_t countX, uint32_t countY, uint32_t countZ) {
    ClearGameLights();

    InitGameLights(countX, countY, countZ);
}

void SceneMatrix::AnimateGameObjects(uint64_t elapsedUsec) {
    const float elapsedSeconds = (float)MICRO2SEC((double)elapsedUsec);

    const float zRotationSpeed = 1.25f;

    for (uint32_t z = 0; z < objectCountZ; ++z) {
        for (uint32_t y = 0; y < objectCountY; ++y) {
            for (uint32_t x = 0; x < objectCountX; ++x) {
                const uint32_t index = objectCountX * objectCountY * z + objectCountX * y + x;
                const float t = elapsedSeconds + index * 0.1f;

                GameObject *gameObject = gameObjects[index];
                gameObject->worldMatrix.SetRotation(BE1::Mat3::FromRotationZ(t * zRotationSpeed), false);

                renderWorld->UpdateRenderObjectTransform(gameObject->renderObjectHandle, gameObject->worldMatrix);
            }
        }
    }
}

void SceneMatrix::AnimateGameLights(uint64_t elapsedUsec) {
    const float elapsedSeconds = (float)MICRO2SEC((double)elapsedUsec);

    const float movementAmplitude = 4.0f;
    constexpr float movementSpeed = 1.0f;
    constexpr float phaseStep = 0.37f;
    const uint32_t lightsPerLayer = lightCountX * lightCountY;
    const float startZ = -lightSpacing * (lightCountZ - 1) * 0.5f;

    int index = 0;
    for (uint32_t z = 0; z < lightCountZ; ++z) {
        const float baseOriginZ = startZ + lightSpacing * z + lightOriginZOffset;

        for (uint32_t i = 0; i < lightsPerLayer; ++i, ++index) {
            const float phase = index * phaseStep;

            GameLight &gameLight = gameLights[index];
            gameLight.origin.z = baseOriginZ + BE1::Math::SinFast(elapsedSeconds * movementSpeed + phase) * movementAmplitude;

            renderWorld->UpdateRenderLightTransform(gameLight.renderLightHandle, gameLight.origin, gameLight.axis);
        }
    }
}

void SceneMatrix::UpdateDirectionalLightDirection() {
    // The UI describes the direction toward the sun or moon, while axis[0] is the direction in which light rays travel.
    directionalLight.axis = BE1::Angles(0.0f, directionalLightElevation, directionalLightAzimuth + 180.0f).ToMat3();
}

#ifdef ENABLE_IMGUI
void SceneMatrix::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 17.0f;

    ImGui::Text("Objects (%i)", gameObjects.Count());
    ImGui::Checkbox("Animate Objects", &animateObjects);

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
    ImGui::Text("Point Lights (%i)", gameLights.Count());
    ImGui::Checkbox("Animate Lights", &animateLights);
    if (ImGui::Checkbox("Cast Shadows##PointLights", &pointLightShadows)) {
        for (GameLight &gameLight : gameLights) {
            if (pointLightShadows) {
                gameLight.flags |= RenderLight::Flag::CastShadows;
            } else {
                gameLight.flags &= ~RenderLight::Flag::CastShadows;
            }
            if (gameLight.renderLightHandle >= 0) {
                renderWorld->UpdateRenderLightFlags(gameLight.renderLightHandle, gameLight.flags);
            }
        }
    }

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

    ImGui::Spacing();
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
}
#endif
