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
#include "Render/Frontend/RenderWorld.h"
#include "Render/Frontend/EnvironmentLight.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "Scene/SceneMatrix.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

static float CalcLightMarkerEmissiveFactor(float intensity, float markerRadius) {
    // 균일하게 발광하는 구체는 I = L * PI * r^2 관계를 이용해 광도 (cd) 를 표면 휘도 (cd/m^2) 로 변환한다.
    // 광도가 한 번만 적용되도록 계산한 표면 휘도를 emissiveFactor에 저장한다.
    const float radiusMeters = BE1::UnitToMeter(markerRadius);
    const float invProjectedArea = BE1::Math::InvPi / (radiusMeters * radiusMeters);
    intensity = BE1::Math::IsFinite(intensity) ? BE1::Max(intensity, 0.0f) : 0.0f;
    return intensity * invProjectedArea;
}

static void CalcLightMarkerEmissive(const GameLight &gameLight, float markerRadius, BE1::Color3 &emissiveColor, float &emissiveFactor) {
    // RenderLight::UpdateLighting 과 동일하게, 매우 어두운 색상도 안정적으로 휘도 정규화한다.
    const BE1::Color3 &linearColor = gameLight.linearColor;
    const double luminance = 0.212655 * linearColor.r + 0.715158 * linearColor.g + 0.072187 * linearColor.b;
    emissiveColor = BE1::Color3::black;
    if (luminance > 0.0) {
        const double invLuminance = 1.0 / luminance;
        emissiveColor = BE1::Color3(static_cast<float>(linearColor.r * invLuminance), static_cast<float>(linearColor.g * invLuminance), static_cast<float>(linearColor.b * invLuminance));
    }

    emissiveFactor = CalcLightMarkerEmissiveFactor(gameLight.intensity, markerRadius);
}

static SharedPtr<Material> CreateLightMarkerMaterial(int lightIndex, const GameLight &gameLight, float markerRadius, float brightnessScale) {
    char name[64];
    BE1::Str::snPrintf(name, sizeof(name), "_sceneMatrixLightMarker_%i", lightIndex);

    BE1::Color3 emissiveColor;
    float emissiveFactor;
    CalcLightMarkerEmissive(gameLight, markerRadius, emissiveColor, emissiveFactor);
    // 실제 광도는 유지하고 마커 표시에만 밝기 배율을 적용한다.
    emissiveFactor *= brightnessScale;

    // 이전 프레임에서 이 재질을 보관하고 있을 수 있으므로 재사용할 때 발광 값을 갱신한다.
    SharedPtr<Material> material = materialManager.FindMaterial(name);
    if (material) {
        material->SetPropertyValue("emissiveColor", emissiveColor);
        material->SetPropertyValue("emissiveFactor", emissiveFactor);
        return material;
    }

    BE1::Str text = BE1::va("{\n"
        "shaderTemplate \"Source/TestD3D12/Shaders/Unlit/Unlit\"\n"
        "properties {\n"
        "   _EMISSIVE \"0\"\n"
        "   emissiveColor \"%.9g %.9g %.9g\"\n"
        "   emissiveFactor \"%.9g\"\n"
        "}\n"
        "}", emissiveColor.r, emissiveColor.g, emissiveColor.b, emissiveFactor);
    material = materialManager.AllocMaterial(name);
    if (!material->Create(text, nullptr)) {
        BE_WARNLOG("SceneMatrix::InitGameLights: failed to create material '%s'\n", name);
        return materialManager.defaultUnlitMaterial;
    }
    return material;
}

void SceneMatrix::Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) {
    this->renderWorld = renderWorld;
    this->physicsWorld = physicsWorld;

    EnvironmentLight::Desc envLightDesc;
    renderWorld->SetSkyboxMaterial(materialManager.GetMaterial("Source/TestD3D12/Materials/skybox.material"));
    renderWorld->SetSkyboxLighting(envLightDesc.linearColor, envLightDesc.intensity);
    renderWorld->SetSkyboxRotation(envLightDesc.axis);

    directionalLight.type = RenderLight::Type::Directional;
    directionalLight.flags = RenderLight::Flag::CastShadows | RenderLight::Flag::CastVolumetricShadows;
    directionalLight.linearColor = BE1::Color3::white;
    directionalLight.intensity = 5.0f;
    directionalLightEnabled = true;
    UpdateDirectionalLightDirection();
    directionalLightEnabled = directionalLight.SetEnabled(renderWorld, directionalLightEnabled);

    InitGameObjects(50, 50, 4);
    InitGameLights(32, 32, 1);

    if (renderWorld->AddEnvironmentLight(envLightDesc)) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }
}

void SceneMatrix::Shutdown() {
    if (!renderWorld) {
        physicsWorld = nullptr;
        return;
    }

    ClearGameObjects();
    ClearGameLights();
    lightMarkerMesh.reset();
    directionalLightEnabled = directionalLight.SetEnabled(renderWorld, false);
    renderWorld->ClearScene(RenderWorld::ClearFlag::EnvironmentLight);

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

    // 텍스트 오브젝트
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
    lightMarkerHandles.Reserve(lightCount);
    lightMarkerMaterials.Reserve(lightCount);

    if (!lightMarkerMesh) {
        lightMarkerMesh = meshManager.AllocTransientMesh("SceneMatrixLightMarker");
        lightMarkerMesh->CreateGeosphere(BE1::Vec3::origin, pointLightMarkerRadius, 2);
    }

    RenderObject::Desc markerDesc;
    markerDesc.flags = RenderObject::Flag::None;
    markerDesc.meshInstance = lightMarkerMesh->GetStaticInstance();
    markerDesc.aabb = lightMarkerMesh->GetAABB();
    markerDesc.materials.SetCount(1);

    const float startX = -pointLightSpacing * (lightCountX - 1) * 0.5f;
    const float startY = -pointLightSpacing * (lightCountY - 1) * 0.5f;
    const float startZ = -pointLightSpacing * (lightCountZ - 1) * 0.5f;

    BE1::RNG rng(1);
    for (uint32_t z = 0; z < lightCountZ; ++z) {
        for (uint32_t y = 0; y < lightCountY; ++y) {
            for (uint32_t x = 0; x < lightCountX; ++x) {
                GameLight &gameLight = gameLights.Alloc();
                gameLight.type = RenderLight::Type::Point;
                gameLight.flags = pointLightShadows ? (RenderLight::Flag::CastShadows | RenderLight::Flag::CastVolumetricShadows) : RenderLight::Flag::None;
                gameLight.origin.Set(
                    startX + pointLightSpacing * x,
                    startY + pointLightSpacing * y,
                    startZ + pointLightSpacing * z + pointLightOriginZOffset);
                gameLight.range = pointLightRadius;

                gameLight.intensity = pointLightIntensity;
                const float hue = rng.GetNextFloat();
                const float saturation = rng.GetNextFloat();
                gameLight.linearColor = BE1::Color3::FromHSL(hue, saturation, 0.5f).SRGBToLinear();

                gameLight.SetEnabled(renderWorld, true);

                markerDesc.worldMatrix.SetTranslation(gameLight.origin);
                markerDesc.materials[0] = CreateLightMarkerMaterial(gameLights.Count() - 1, gameLight, pointLightMarkerRadius, pointLightMarkerBrightnessScale);
                lightMarkerHandles.Append(renderWorld->AddRenderObject(markerDesc));
                lightMarkerMaterials.Append(markerDesc.materials[0]);
            }
        }
    }
}

void SceneMatrix::ClearGameObjects() {
    // 라이트 마커는 렌더 월드를 공유하지만 수명은 별도로 관리한다.
    for (const GameObject *gameObject : gameObjects) {
        renderWorld->RemoveRenderObject(gameObject->renderObjectHandle);
    }

    gameObjects.DeleteContents(true);
    testMesh.reset();
}

void SceneMatrix::ClearGameLights() {
    for (int handle : lightMarkerHandles) {
        renderWorld->RemoveRenderObject(handle);
    }
    lightMarkerHandles.Clear();
    lightMarkerMaterials.Clear();

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

    const float movementAmplitude = objectCountZ * 2.0f;
    constexpr float movementSpeed = 0.5f;
    constexpr float phaseStep = 0.37f;
    const uint32_t lightsPerLayer = lightCountX * lightCountY;
    const float startZ = -pointLightSpacing * (lightCountZ - 1) * 0.5f;

    BE1::Mat3x4 markerWorldMatrix = BE1::Mat3x4::identity;
    int index = 0;
    for (uint32_t z = 0; z < lightCountZ; ++z) {
        const float baseOriginZ = startZ + pointLightSpacing * z + pointLightOriginZOffset;

        for (uint32_t i = 0; i < lightsPerLayer; ++i, ++index) {
            const float phase = index * phaseStep;

            GameLight &gameLight = gameLights[index];
            gameLight.origin.z = baseOriginZ + BE1::Math::SinFast(elapsedSeconds * movementSpeed + phase) * movementAmplitude;

            renderWorld->UpdateRenderLightTransform(gameLight.renderLightHandle, gameLight.origin, gameLight.axis);
            markerWorldMatrix.SetTranslation(gameLight.origin);
            renderWorld->UpdateRenderObjectTransform(lightMarkerHandles[index], markerWorldMatrix);
        }
    }
}

void SceneMatrix::UpdatePointLightIntensity() {
    for (GameLight &gameLight : gameLights) {
        gameLight.intensity = pointLightIntensity;
        if (gameLight.renderLightHandle >= 0) {
            renderWorld->UpdateRenderLightLighting(gameLight.renderLightHandle, gameLight.linearColor, gameLight.intensity);
        }
    }

    UpdateLightMarkerBrightness();
}

void SceneMatrix::UpdateLightMarkerBrightness() {
    // 반지름과 광도가 모두 같으므로 표면 휘도는 한 번만 계산하고 기존 재질의 밝기만 갱신한다.
    const float emissiveFactor = CalcLightMarkerEmissiveFactor(pointLightIntensity, pointLightMarkerRadius) * pointLightMarkerBrightnessScale;
    for (const SharedPtr<Material> &material : lightMarkerMaterials) {
        if (material && material != materialManager.defaultUnlitMaterial) {
            const int propertyIndex = material->FindPropertyIndex(ShaderTemplate::StandardProperty::EmissiveFactor);
            material->SetPropertyValue(propertyIndex, emissiveFactor);
        }
    }
}

void SceneMatrix::UpdateDirectionalLightDirection() {
    // 사용자 인터페이스는 태양이나 달을 향하는 방향을 표시하고, axis[0]은 빛이 진행하는 방향을 나타낸다.
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

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("Intensity##PointLights", &pointLightIntensity, BE1::Max(pointLightIntensity * 0.01f, 0.01f), 0.0f, 100000.0f, "%.3f cd", ImGuiSliderFlags_AlwaysClamp)) {
        UpdatePointLightIntensity();
    }

    if (ImGui::Checkbox("Cast Shadows##PointLights", &pointLightShadows)) {
        for (GameLight &gameLight : gameLights) {
            if (pointLightShadows) {
                gameLight.flags |= RenderLight::Flag::CastShadows | RenderLight::Flag::CastVolumetricShadows;
            } else {
                gameLight.flags &= ~(RenderLight::Flag::CastShadows | RenderLight::Flag::CastVolumetricShadows);
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
}
#endif
